#include "session.hpp"
#include "http2_session.hpp"
#include "logger.hpp"
#include <asio/ssl/context.hpp>
#include <cstddef>
#include <cstdint>
#include <nghttp2/nghttp2.h>

namespace http2Cpp {

session::session(tcp::socket socket, short port, ssl::context &ssl_ctx,
                 std::map<std::string, service> &services)
    : m_socket(std::move(socket)), m_ssl_ctx(ssl_ctx), m_services(services),
      local_port(port) {
  this->m_queue_stage.reserve(100);
}

session::~session() {}

void session::start() { this->run(); }

void session::run() {

  auto self = shared_from_this();
  auto buffer = std::make_shared<std::array<std::uint8_t, 100>>();

  //    Read partially the the tcp request without consuming the buffer
  this->m_socket.async_receive(
      asio::buffer(*buffer), asio::socket_base::message_peek,
      [self, buffer, this](std::error_code error, std::size_t) {
        if (error) {
          DebugLog::logClassStatus(DebugLog::LOG_ERROR, error.message(),
                                   "[session]");
        }
        // this hexadecimal code represents if it is a tsl handshake
        // record
        if ((*buffer)[0] == 0x16) {

          DebugLog::logClassStatus(DebugLog::LOG_INFO, "HTTPS REQUEST",
                                   "[session]");
          this->m_secSocket = std::make_unique<ssl::stream<tcp::socket>>(
              std::move(this->m_socket), this->m_ssl_ctx);
          this->do_handshake();

        } else if (static_cast<char>((*buffer)[0]) == 'G') {

          /*  TODO
           * have a http1 class for better structure and to add support for the
           * server to fallback to the protocol when http2 not supported (Maybe
           * http3 nghttp implementation will be added)
           */
          std::string body(reinterpret_cast<char *>(buffer->data()),
                           buffer->size());

          DebugLog::logClassStatus(DebugLog::LOG_INFO, "HTTP request",
                                   "[session]");
          std::size_t begin = body.find("/");
          std::size_t end = body.find("H", begin) - 1;
          std::string target(body.begin() + begin, body.begin() + end);
          if (target == "/")
            target = "";

          std::string responseBody = "HTTP/1.1 302 Found\r\n";
          responseBody += "Location: https://localhost:" +
                          std::to_string(this->local_port) + target + "\r\n";
          responseBody += "Content-Length: 0\r\n";
          responseBody += "Connection: close\r\n";
          responseBody += "\r\n";

          asio::async_write(
              this->m_socket,
              asio::buffer(responseBody.data(), responseBody.size()),
              [self, this](std::error_code ec, size_t bytes) {
                DebugLog::logClassStatus(DebugLog::LOG_INFO, "Redirecting",
                                         "[session]");
                this->m_socket.shutdown(tcp::socket::shutdown_both);
                this->m_socket.close();
              });
        }
      });
}

void session::do_handshake() {

  auto self = shared_from_this();

  // The secure socket is build when is confirm is a tsl request connection

  this->m_secSocket->async_handshake(
      ssl::stream_base::server, [this, self](const std::error_code &erro) {
        if (!erro) {
          DebugLog::logClassStatus(DebugLog::LOG_INFO, "handshake made",
                                   "[session]");
          SSL *ssl = this->m_secSocket->native_handle();

          // std::vector<unsigned char> alpn_info;
          const unsigned char *alpn = nullptr;
          unsigned int alpnlen = 0;
          SSL_get0_alpn_selected(ssl, &alpn, &alpnlen);

          /*  TODO
           *  Add http1 support for the session connection
           */
          if (alpn == nullptr || alpnlen != 2 || memcmp("h2", alpn, 2) != 0) {
            DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                                     "h2 is not  negotiaded", "[session]");
            return;
          } else {

            DebugLog::logClassStatus(DebugLog::LOG_INFO,
                                     "Looks like is a h2 connection",
                                     "[session]");

            this->m_https2_session_ =
                std::make_unique<class http2_session>(this);

            if (!this->m_https2_session_->set_config())
              return;
          }

          this->read();
        } else {
          DebugLog::logClassStatus(DebugLog::LOG_INFO, erro.message(),
                                   "[session]");
        }
      });
}

void session::read() {

  auto self = shared_from_this();
  this->m_data[0] = '\0';

  DebugLog::logClassStatus(DebugLog::LOG_INFO, "expecting request",
                           "[session]");

  this->m_secSocket->async_read_some(
      asio::buffer(m_data, BUFFER_SIZE),
      [self, this](const std::error_code &ec, std::size_t bytes) {
        {
          // std::lock_guard<std::mutex>
          // session_mutex(session::getMutex());

          if (ec) {
            DebugLog::logClassStatus(DebugLog::LOG_INFO, "EC on m_read handler",
                                     "[session]");
            DebugLog::logClassStatus(DebugLog::LOG_ERROR, ec.message(),
                                     "[session]");
            return;
          }

          /*  TODO
           *  add http1 handle request implementation
           */

          if (this->m_https2_session_) {
            if (!this->m_https2_session_->read_request_bytes(
                    reinterpret_cast<std::uint8_t *>(this->m_data), bytes))
              return;
          }
          this->read();
        }
      });
}

void session::write() {

  if (!this->m_queue_write.empty() || this->m_queue_stage.empty())
    return;

  DebugLog::logClassStatus(DebugLog::LOG_INFO, "M_WRITE", "[session]");

  std::swap(this->m_queue_write, this->m_queue_stage);

  this->m_send_buffers.reserve(this->m_queue_write.size());
  for (const auto &frame : this->m_queue_write) {
    this->m_send_buffers.emplace_back(asio::buffer(frame));
  }

  DebugLog::logClassStatus(DebugLog::LOG_INFO,
                           "Size of queue: " +
                               std::to_string(this->m_queue_write.size()),
                           "[session]");

  asio::async_write(*this->m_secSocket, this->m_send_buffers,
                    [self = shared_from_this(), this](
                        const std::error_code &ec, std::size_t bytes_sended) {
                      if (!ec) {

                        this->m_queue_write.clear();
                        this->m_send_buffers.clear();
                        DebugLog::logClassStatus(DebugLog::LOG_INFO,
                                                 "After writing", "[session]");

                        if (!this->m_queue_stage.empty()) {

                          this->write();
                        }

                      } else {
                        DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                                                 "ERROR: " + ec.message(),
                                                 "[session]");
                      }
                    });
}

void session::add_to_queue(const std::uint8_t *data, std::size_t bytes_len) {

  this->m_queue_stage.emplace_back(data, data + bytes_len);
  this->write();
}

response session::invoke_url_cb(const std::string &service,
                                const std::string &method,
                                const std::string &path) {
  if (this->m_services.find(service) != this->m_services.end()) {

    return this->m_services.at(service).invoke_cb(method, path);
  }

  return {.error_response = true};
}

} // namespace http2Cpp
