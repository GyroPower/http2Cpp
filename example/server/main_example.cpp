/* Old initial implementation of the idea of the library for a http2 server
 * using asio and nghttp2.
 *
 * It's a very messy code and not well documented, and the main http2Cpp api it
 * still needs documentation
 *
 * */

#include "http2Cpp/io_context_pool.hpp"
#include "logger.hpp"
#include <asio.hpp>
#include <asio/any_io_executor.hpp>
#include <asio/buffer.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/placeholders.hpp>
#include <asio/socket_base.hpp>
#include <asio/ssl.hpp>
#include <asio/ssl/context.hpp>
#include <asio/ssl/verify_mode.hpp>
#include <asio/strand.hpp>
#include <asio/stream_file.hpp>
#include <asio/use_awaitable.hpp>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <system_error>
#include <unistd.h>

#define NGHTTP2_NO_SSIZE_T
#include <nghttp2/nghttp2.h>

#include <string>

#define BUFFER_SIZE 100857
#define MAKE_NV(NAME, VALUE)                                                   \
  {                                                                            \
      (uint8_t *)NAME,   (uint8_t *)VALUE,     sizeof(NAME) - 1,               \
      sizeof(VALUE) - 1, NGHTTP2_NV_FLAG_NONE,                                 \
  }

namespace ssl = asio::ssl;
using asio::ip::tcp;

std::array<char, 4096> bufferAsync;

const static std::string ERROR_HTML =
    "<html><head><title>404</title></head>"
    "<body><h1>404 Not Found</h1></body></html>";

struct stream_data {
  std::string request_path = "";
  std::uint32_t stream_id;
  std::string data = "";
  std::size_t lastlenght = 0;
  std::size_t left = 0;
  std::string file_path = "";
  std::string method = "";
};

typedef struct http2_stream_data {
  struct http2_stream_data *prev, *next;
  char *request_path;
  int32_t stream_id;
  int fd;
} http2_stream_data;

class session : public std::enable_shared_from_this<session> {

  std::map<std::uint32_t, stream_data> m_streams;
  // test_web_app m_test;
  nghttp2_session *m_https2_session = nullptr;
  ssl::context &m_ssl_ctx;

  std::vector<std::vector<std::uint8_t>> m_queue_write;
  std::vector<std::vector<std::uint8_t>> m_queue_stage;
  std::vector<asio::const_buffer> m_send_buffers;
  bool writing = false;
  std::unique_ptr<ssl::stream<tcp::socket>> m_secSocket = nullptr;
  tcp::socket m_socket;

  unsigned char m_data[BUFFER_SIZE];
  std::array<char, BUFFER_SIZE> m_buffer;

public:
  bool m_error_reply = false;

  session(tcp::socket socket, asio::ssl::context &ssl_ctx)
      : m_socket(std::move(socket)), m_ssl_ctx(ssl_ctx) {
    this->m_queue_stage.reserve(100);
  }

  ~session() { nghttp2_session_del(this->m_https2_session); }

  std::string invoke_url_cb(std::string path) {
    // return this->m_test.invoke_cb(path);
    return "";
  }

  void m_add_to_queue(const std::uint8_t *data, std::size_t bytes_len) {

    this->m_queue_stage.emplace_back(data, data + bytes_len);
    this->m_write();
  }

  void start() { this->m_run(); }

  static std::mutex &getMutex() {
    static std::mutex m;
    return m;
  }

private:
  void m_write() {

    if (!this->m_queue_write.empty() || this->m_queue_stage.empty())
      return;

    DebugLog::logClassStatus(DebugLog::LOG_INFO, "M_WRITE");

    std::swap(this->m_queue_write, this->m_queue_stage);

    this->m_send_buffers.reserve(this->m_queue_write.size());
    for (const auto &frame : this->m_queue_write) {
      this->m_send_buffers.emplace_back(asio::buffer(frame));
    }

    DebugLog::logClassStatus(DebugLog::LOG_INFO,
                             "Size of queue: " +
                                 std::to_string(this->m_queue_write.size()));

    asio::async_write(*this->m_secSocket, this->m_send_buffers,
                      [self = shared_from_this(), this](
                          const std::error_code &ec, std::size_t bytes_sended) {
                        if (!ec) {

                          this->m_queue_write.clear();
                          this->m_send_buffers.clear();

                          if (!this->m_queue_stage.empty()) {

                            this->m_write();
                          }

                        } else {
                          DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                                                   "ERROR: " + ec.message());
                        }
                      });
  }

  static bool check_path(const std::string &path) {
    return path[0] == '/' && !path.contains('\\') && !path.contains("/../") &&
           !path.contains("/./") && !path.contains("/..") &&
           ((path.size() > 1 && path[path.size() - 2] != '/' &&
             path[path.size() - 1] != '.') ||
            path.size() == 1);
  }

  void remove_stream_data(std::uint32_t stream_id) {
    this->m_streams.erase(stream_id);
  }

  stream_data *add_stream(stream_data data) {

    this->m_streams.insert({data.stream_id, data});
    return &this->m_streams.at(data.stream_id);
  }

  static std::uint8_t hex_to_uint(std::uint8_t c) {
    if ('0' <= c && c <= '9')
      return static_cast<std::uint8_t>(c - '0');
    if ('A' <= c && c <= 'F')
      return static_cast<std::uint8_t>(c - 'A' + 10);
    if ('a' <= c && c <= 'f')
      return static_cast<std::uint8_t>(c - 'a' + 10);

    return 0;
  }

  static std::string percent_decode(const std::uint8_t *value,
                                    std::size_t valuelen) {

    std::string res;
    res.resize(valuelen + 1);
    if (valuelen > 3) {
      std::size_t i, j;

      for (i = 0, j = 0; i < valuelen - 2;) {
        if (value[i] != '%' || !std::isxdigit(value[i + 1]) ||
            !std::isxdigit(value[i + 2])) {
          res[j++] = static_cast<char>(value[i++]);
          continue;
        }

        res[j++] = static_cast<char>((hex_to_uint(value[i + 1]) << 4) +
                                     hex_to_uint(value[i + 2]));
        i += 3;
      }

      std::memcpy(&res[j], &value[i], 2);
      res[j + 2] = '\0';
    } else {
      memcpy(res.data(), value, valuelen);
      res[valuelen] = '\0';
    }

    return res;
  }

  static nghttp2_ssize send_callback(nghttp2_session *http2_session,
                                     const uint8_t *data, size_t length,
                                     int flags, void *user_data) {
    auto session_ptr =
        reinterpret_cast<session *>(user_data)->shared_from_this();

    if (length > BUFFER_SIZE)
      return NGHTTP2_ERR_WOULDBLOCK;

    DebugLog::logClassStatus(DebugLog::LOG_INFO, "[SEND_CALLBACK]");
    session_ptr->m_add_to_queue(data, length);

    return (nghttp2_ssize)length;
  }

  stream_data *m_getStream(std::uint32_t stream_id) {
    stream_data *data = &this->m_streams.at(stream_id);

    if (data)
      return data;

    return nullptr;
  }

  static nghttp2_ssize my_read_callback(nghttp2_session *ngttp2_session,
                                        int32_t stream_id, uint8_t *buf,
                                        size_t length, uint32_t *data_flags,
                                        nghttp2_data_source *source,
                                        void *user_data) {
    auto session_ptr =
        reinterpret_cast<session *>(user_data)->shared_from_this();

    stream_data *data = session_ptr->m_getStream(stream_id);
    DebugLog::logClassStatus(DebugLog::LOG_INFO, "[READ CALLBACK]");

    if (!data)
      return -1;

    std::size_t len = 0;

    if (data->file_path != "") {
      std::ifstream file;

      file.open(data->file_path, std::ios::in | std::ios::binary);

      if (!file.is_open()) {
        DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                                 "Could not open file: " + data->file_path);
        return -1;
      }

      if (data->lastlenght == 0 && data->left == 0) {
        file.seekg(0, std::ifstream::end);

        data->left = file.tellg();
        file.seekg(0, std::ifstream::beg);
      }

      if (data->left > 0) {
        DebugLog::logClassStatus(DebugLog::LOG_INFO, "READING");
        std::size_t to_read = data->left > length ? length : data->left;
        std::vector<char> bytes(to_read);

        file.seekg(data->lastlenght);

        file.read(bytes.data(), to_read);
        memcpy(buf, bytes.data(), bytes.size());
        data->lastlenght += to_read;
        data->left -= to_read;
        len = to_read;
      }

    } else if (data->data != "") {

      if (data->left == 0 && data->lastlenght == 0) {
        data->left = data->data.size();
      }

      if (data->left > 0) {
        std::size_t to_read = data->left > length ? length : data->left;
        memcpy(buf, data->data.data() + data->lastlenght, to_read);
        data->lastlenght += to_read;
        data->left -= to_read;
        len = to_read;
      }
    }

    if (data->left == 0) {
      DebugLog::logClassStatus(DebugLog::LOG_INFO, "NGHTTP2_DATA_FLAG_EOF");
      *data_flags |= NGHTTP2_DATA_FLAG_EOF;
    }

    return (nghttp2_ssize)len;
  }

  bool m_setDataToStream(std::string data, std::uint32_t stream_id) {

    this->m_streams.at(stream_id).data = data;

    if (this->m_streams.at(stream_id).data == data)
      return true;

    return false;
  }

  bool m_setFilePathToStream(std::uint32_t stream_id, std::string path) {

    if (this->m_streams.find(stream_id) != this->m_streams.end()) {
      this->m_streams.at(stream_id).file_path = path;
      return true;
    }

    return false;
  }

  static void see_ptr(void *data) {
    std::string *string = reinterpret_cast<std::string *>(data);
  }

  static int send_response(std::shared_ptr<session> session,
                           nghttp2_session *http2_session,
                           std::uint32_t stream_id, nghttp2_nv *nva,
                           std::size_t nvlen, std::string bytes_to_send = "",
                           std::string file_path = "") {
    int result;
    nghttp2_data_provider2 data_prod;

    if (bytes_to_send != "") {
      if (!session->m_setDataToStream(bytes_to_send, stream_id)) {
        DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                                 "ERROR_SETTING DATA TO STREAM_" +
                                     std::to_string(stream_id));
        return -1;
      }

    } else if (file_path != "") {
      if (!session->m_setFilePathToStream(stream_id, file_path)) {
        return -1;
      }
    }

    data_prod.read_callback = my_read_callback;

    result = nghttp2_submit_response2(http2_session, stream_id, nva, nvlen,
                                      &data_prod);

    if (result != 0) {
      DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                               "[SUBMIT_RESPONSE]: " +
                                   std::string(nghttp2_strerror(result)));
      return -1;
    }

    return 0;
  }

  static int error_reply(std::shared_ptr<session> connect_session,
                         nghttp2_session *http2_session, stream_data *stream) {

    // nghttp2_nv hdrs[] = {MAKE_NV(":status", "404")};
    connect_session->m_error_reply = true;
    nghttp2_nv hdrs[] = {MAKE_NV(":status", "404"),
                         MAKE_NV("content-type", "text/html")};

    if (send_response(connect_session, http2_session, stream->stream_id, hdrs,
                      sizeof(hdrs) / sizeof(hdrs[0]), ERROR_HTML) != 0) {
      return NGHTTP2_ERR_CALLBACK_FAILURE;
    }

    return 0;
  }

  static int on_request_recv(std::shared_ptr<session> connect_session,
                             nghttp2_session *http2_session,
                             stream_data *stream) {

    std::string rel_path;

    if (stream->request_path == "") {
      if (error_reply(connect_session, http2_session, stream) != 0) {
        DebugLog::logClassStatus(DebugLog::LOG_ERROR, "ERROR_REPLY NOT SEND");
        return NGHTTP2_ERR_CALLBACK_FAILURE;
      }
      return 0;
    }

    if (!check_path(stream->request_path)) {
      if (error_reply(connect_session, http2_session, stream) != 0)
        return NGHTTP2_ERR_CALLBACK_FAILURE;

      return 0;
    }

    // std::string path =
    // connect_session->m_test.invoke_cb(stream->request_path);
    std::string path = "";

    if (path == "") {
      if (error_reply(connect_session, http2_session, stream) != 0)
        return NGHTTP2_ERR_CALLBACK_FAILURE;

      return 0;
    }

    nghttp2_nv hdrs[] = {MAKE_NV(":status", "200"),
                         MAKE_NV("content-type", "text/html")};

    DebugLog::logClassStatus(DebugLog::LOG_INFO, "on_request_recv: " + path);
    if (send_response(
            connect_session, http2_session, stream->stream_id, hdrs,
            sizeof(hdrs) / sizeof(hdrs[0]),
            /*R"({ " name ": " John ", " age ": 30, " city ": " New York " })"*/
            "", path) != 0)
      return NGHTTP2_ERR_CALLBACK_FAILURE;

    return 0;
  }

  static int on_frame_recv_callback(nghttp2_session *http2_session,
                                    const nghttp2_frame *frame,
                                    void *user_data) {
    stream_data *data = nullptr;
    auto session_ptr =
        reinterpret_cast<session *>(user_data)->shared_from_this();

    switch (frame->hd.type) {
    case NGHTTP2_DATA:
    case NGHTTP2_HEADERS:
      if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {

        data = (stream_data *)nghttp2_session_get_stream_user_data(
            http2_session, frame->hd.stream_id);
        if (!data) {
          DebugLog::logClassStatus(DebugLog::LOG_ERROR, "[NO STREAM_DATA]");
          return 0;
        }

        return on_request_recv(session_ptr, http2_session, data);
      }
      break;
    default:
      break;
    }

    return 0;
  }

  static int on_stream_close_callback(nghttp2_session *http2_session,
                                      int32_t stream_id,
                                      std::uint32_t error_code,
                                      void *user_data) {
    auto session_ptr =
        reinterpret_cast<session *>(user_data)->shared_from_this();

    stream_data *data = (stream_data *)nghttp2_session_get_stream_user_data(
        session_ptr->m_https2_session, stream_id);

    if (!data)
      return 0;

    session_ptr->remove_stream_data(stream_id);
    return 0;
  }

  static stream_data create_http2_stream_data(std::uint32_t stream_id) {
    stream_data data{.stream_id = stream_id};

    return data;
  }

  static int on_begin_headers_callback(nghttp2_session *http2_session,
                                       const nghttp2_frame *frame,
                                       void *user_data) {
    auto session_ptr =
        reinterpret_cast<session *>(user_data)->shared_from_this();

    if (frame->hd.type != NGHTTP2_HEADERS ||
        frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
      DebugLog::logClassStatus(
          DebugLog::LOG_WARNING,
          "No NGHTTP2_HEADERS and No NGHTTP2_HCAT_REQUEST");
      return 0;
    }

    stream_data data = create_http2_stream_data(frame->hd.stream_id);

    auto data_ptr = session_ptr->add_stream(data);

    nghttp2_session_set_stream_user_data(http2_session, frame->hd.stream_id,
                                         data_ptr);

    return 0;
  }

  static int on_header_callback(nghttp2_session *http2_session,
                                const nghttp2_frame *frame,
                                const std::uint8_t *name, std::size_t namelen,
                                const std::uint8_t *value, std::size_t valuelen,
                                std::uint8_t flags, void *user_data) {

    const char PATH[] = ":path";

    switch (frame->hd.type) {
    case NGHTTP2_HEADERS:
      if (frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
        break;
      }

      if (std::strcmp(reinterpret_cast<const char *>(name), ":method") == 0) {
        DebugLog::logClassStatus(
            DebugLog::LOG_INFO,
            "[ON HEADER_CALLBACK] method: " +
                std::string(reinterpret_cast<const char *>(value), valuelen),
            "[NGHHTP2 CALLBACKS]");
      }
      stream_data *data = (stream_data *)nghttp2_session_get_stream_user_data(
          http2_session, frame->hd.stream_id);

      if (!data)
        break;

      if (std::strcmp(reinterpret_cast<const char *>(name), ":method") == 0) {
        data->method =
            std::string(reinterpret_cast<const char *>(value), valuelen);
      }
      if (namelen == sizeof(PATH) - 1 && memcmp(PATH, name, namelen) == 0) {
        std::size_t j;
        for (j = 0; j < valuelen && value[j] != '?'; j++)
          ;
        data->request_path = percent_decode(value, j);
      }

      break;
    }

    return 0;
  }

  void initialize_nghttp2_session() {
    nghttp2_session_callbacks *callbacks;
    nghttp2_session_callbacks_new(&callbacks);

    nghttp2_session_callbacks_set_send_callback2(callbacks, send_callback);

    nghttp2_session_callbacks_set_on_frame_recv_callback(
        callbacks, on_frame_recv_callback);

    nghttp2_session_callbacks_set_on_stream_close_callback(
        callbacks, on_stream_close_callback);

    nghttp2_session_callbacks_set_on_header_callback(callbacks,
                                                     on_header_callback);

    nghttp2_session_callbacks_set_on_begin_headers_callback(
        callbacks, on_begin_headers_callback);

    nghttp2_session_server_new(&this->m_https2_session, callbacks, this);

    nghttp2_session_callbacks_del(callbacks);
  }

  void do_handshake() {
    auto self = shared_from_this();

    // The secure socket is build when is confirm is a tsl request connection

    this->m_secSocket->async_handshake(
        ssl::stream_base::server, [this, self](const std::error_code &erro) {
          if (!erro) {
            DebugLog::logClassStatus(DebugLog::LOG_INFO, "handshake made");
            SSL *ssl = this->m_secSocket->native_handle();

            // std::vector<unsigned char> alpn_info;
            const unsigned char *alpn = nullptr;
            unsigned int alpnlen = 0;
            SSL_get0_alpn_selected(ssl, &alpn, &alpnlen);

            if (alpn == nullptr || alpnlen != 2 || memcmp("h2", alpn, 2) != 0) {
              DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                                       "h2 is not  negotiaded");
              return;
            }
            DebugLog::logClassStatus(DebugLog::LOG_INFO,
                                     "Looks like is a h2 connection");

            this->initialize_nghttp2_session();

            if (!this->m_set_config() || !this->send_session()) {
              DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                                       "[SETTINGS][SEND_SESSION]");
              return;
            }

            this->m_read();
          } else {
            DebugLog::logClassStatus(DebugLog::LOG_INFO, erro.message());
          }
        });
  }

  bool session_send() {
    int result;

    result = nghttp2_session_send(this->m_https2_session);
    if (result != 0) {
      DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                               "[SESSION_SEND]: " +
                                   std::string(nghttp2_strerror(result)));
      return false;
    }

    return true;
  }
  bool m_set_config() {
    // setting the http2_session

    nghttp2_settings_entry settings_entry[1] = {
        {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};

    int result;

    result = nghttp2_submit_settings(
        this->m_https2_session, NGHTTP2_FLAG_NONE, settings_entry,
        (sizeof(settings_entry) / sizeof(settings_entry[0])));

    if (result != 0) {
      DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                               "[SETTINGS]: " +
                                   std::string(nghttp2_strerror(result)));
      return false;
    }

    return true;
  }

  void m_read() {
    auto self = shared_from_this();
    this->m_data[0] = '\0';

    DebugLog::logClassStatus(DebugLog::LOG_INFO, "[EXPECTING REQUEST]");

    this->m_secSocket->async_read_some(
        asio::buffer(m_data, BUFFER_SIZE),
        [self, this](const std::error_code &ec, std::size_t bytes) {
          {
            // std::lock_guard<std::mutex>
            // session_mutex(session::getMutex());

            if (ec) {
              DebugLog::logClassStatus(DebugLog::LOG_INFO,
                                       "EC on m_read handler");
              DebugLog::logClassStatus(DebugLog::LOG_ERROR, ec.message());
              return;
            }

            nghttp2_ssize readlen;

            readlen = nghttp2_session_mem_recv2(this->m_https2_session,
                                                this->m_data, bytes);

            // std::string request;
            //
            // std::memcpy(request.data(), this->m_data, size_bytes);
            //
            //
            if (readlen < 0) {
              DebugLog::logClassStatus(
                  DebugLog::LOG_ERROR,
                  "[READING]: " + std::string(nghttp2_strerror((int)readlen)));

              return;
            }

            // this->m_data[0] = '\0';

            if (!this->send_session()) {
              DebugLog::logClassStatus(DebugLog::LOG_ERROR, "[SEND_SESSION]");
              return;
            }
            this->m_read();
          }
        });
  }

  bool send_session() {
    int result = nghttp2_session_send(this->m_https2_session);

    if (result != 0) {
      DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                               "[SESSION]: " +
                                   std::string(nghttp2_strerror(result)));
      return false;
    }

    return true;
  }

  void m_run() {
    auto self = shared_from_this();
    auto buffer = std::make_shared<std::array<std::uint8_t, 100>>();

    //    Read partially the the tcp request without consuming the buffer
    this->m_socket.async_receive(
        asio::buffer(*buffer), asio::socket_base::message_peek,
        [self, buffer, this](std::error_code error, std::size_t) {
          if (error) {
            DebugLog::logClassStatus(DebugLog::LOG_ERROR, error.message());
          }
          // this hexadecimal code represents if it is a tsl handshake
          // record
          if ((*buffer)[0] == 0x16) {

            DebugLog::logClassStatus(DebugLog::LOG_INFO, "HTTPS REQUEST");
            this->m_secSocket = std::make_unique<ssl::stream<tcp::socket>>(
                std::move(this->m_socket), this->m_ssl_ctx);
            this->do_handshake();

          } else if (static_cast<char>((*buffer)[0]) == 'G') {

            std::string body(reinterpret_cast<char *>(buffer->data()),
                             buffer->size());

            DebugLog::logClassStatus(DebugLog::LOG_INFO, "HTTP request");
            std::size_t begin = body.find("/");
            std::size_t end = body.find("H", begin) - 1;
            std::string target(body.begin() + begin, body.begin() + end);
            if (target == "/")
              target = "";

            std::string responseBody = "HTTP/1.1 302 Found\r\n";
            responseBody +=
                "Location: https://localhost:8082" + target + "\r\n";
            responseBody += "Content-Length: 0\r\n";
            responseBody += "Connection: close\r\n";
            responseBody += "\r\n";

            asio::async_write(
                this->m_socket,
                asio::buffer(responseBody.data(), responseBody.size()),
                [self, this](std::error_code ec, size_t bytes) {
                  DebugLog::logClassStatus(DebugLog::LOG_INFO, "Redirecting");
                  this->m_socket.shutdown(tcp::socket::shutdown_both);
                  this->m_socket.close();
                });
          }
        });
  }
};

class server {
  http2Cpp::io_context_pool m_io_ctx_pool;
  tcp::acceptor m_acceptor;
  asio::ssl::context &m_ssl_ctx;
  std::size_t num_threads = 1;
  asio::signal_set m_signals;

public:
  server(ssl::context &ssl_ctx, short port, std::size_t num_threads = 1)
      : m_io_ctx_pool(num_threads), m_acceptor(m_io_ctx_pool.get_io_context(),
                                               tcp::endpoint(tcp::v4(), port)),
        m_ssl_ctx(ssl_ctx), num_threads(num_threads),
        m_signals(this->m_io_ctx_pool.get_io_context()) {
    this->m_signals.add(SIGINT);
    this->m_signals.add(SIGTERM);

#if defined(SIGQUIT)
    this->m_signals.add(SIGQUIT);
#endif

    this->await_stop();

    this->m_acceptor.set_option(tcp::acceptor::reuse_address(true));
    this->m_acceptor.listen(asio::socket_base::max_listen_connections);

    this->do_accept();
  }

  void m_run() { this->m_io_ctx_pool.run(); }

private:
  void await_stop() {
    this->m_signals.async_wait(
        [this](std::error_code ec, int sign) { this->m_io_ctx_pool.stop(); });
  }
  void do_accept() {

    this->m_acceptor.async_accept(
        this->m_io_ctx_pool.get_io_context(),
        [this](std::error_code ec, tcp::socket socket) {
          if (!ec) {

            socket.set_option(tcp::no_delay(true));

            std::make_shared<session>(std::move(socket), this->m_ssl_ctx)
                ->start();
          }
          this->do_accept();
        });
  }
};

static int alpn_select_proto_cb(SSL *ssl, const unsigned char **out,
                                unsigned char *outlen, const unsigned char *in,
                                unsigned int inlen, void *arg) {
  int rv;

  rv = nghttp2_select_alpn(out, outlen, in, inlen);

  if (rv != 1) {
    return SSL_TLSEXT_ERR_NOACK;
  }

  return SSL_TLSEXT_ERR_OK;
}

int main(int argc, char *argv[]) {
  try {

    asio::ssl::context ctx{asio::ssl::context_base::tlsv13_server};

    // you have to serve your own .pem files for the tsl use of the tcp
    // sockets
    ctx.use_certificate_chain_file("localhost+2.pem");
    ctx.use_private_key_file("localhost+2-key.pem", asio::ssl::context::pem);

    SSL_CTX *CTX = ctx.native_handle();

    SSL_CTX_set_alpn_select_cb(CTX, alpn_select_proto_cb, NULL);

    std::int32_t num_threads = 1;
    short port = 8082;

    server s(ctx, port, num_threads);
    s.m_run();

    return EXIT_SUCCESS;
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
}
