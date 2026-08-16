#ifndef HTTP2_CALLBAKS_NG
#define HTTP2_CALLBAKS_NG

#include "http2_session.hpp"
#include "logger.hpp"
#include "mime_types.hpp"
#include "session.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <nghttp2/nghttp2.h>
#include <string>

namespace http2Cpp {

class http2_static_callbacks {
public:
  static std::uint8_t hex_to_uint(std::uint8_t c) {

    if ('0' <= c && c <= '9')
      return static_cast<std::uint8_t>(c - '0');
    if ('A' <= c && c <= 'F')
      return static_cast<std::uint8_t>(c - 'A' + 10);
    if ('a' <= c && c <= 'f')
      return static_cast<std::uint8_t>(c - 'a' + 10);

    return 0;
  }

  static bool check_path(const std::string &path) {

    return path[0] == '/' && !path.contains('\\') && !path.contains("/../") &&
           !path.contains("/./") && !path.contains("/..") &&
           !path.contains("/.");
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

  static stream_data create_http2_stream_data(std::uint32_t stream_id) {
    return stream_data{.stream_id = stream_id};
  }

  static int send_response(http2_session *session,
                           nghttp2_session *http2_session,
                           std::uint32_t stream_id, nghttp2_nv *nva,
                           std::size_t nvlen, response cb_response) {
    int result;
    nghttp2_data_provider2 data_prod;

    if (!session->set_data_response_to_stream(stream_id, cb_response))
      return -1;

    data_prod.read_callback = my_read_callback;

    result = nghttp2_submit_response2(http2_session, stream_id, nva, nvlen,
                                      &data_prod);

    if (result != 0) {
      DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                               "[SEND RESPONSE] Error submit response: " +
                                   std::string(nghttp2_strerror(result)),
                               "[NGHTTP2]");
      return -1;
    }

    return 0;
  }

  static nghttp2_ssize my_read_callback(nghttp2_session *ngttp2_session,
                                        int32_t stream_id, uint8_t *buf,
                                        size_t length, uint32_t *data_flags,
                                        nghttp2_data_source *source,
                                        void *user_data) {
    auto session_ptr = reinterpret_cast<class http2_session *>(user_data);

    stream_data *data = session_ptr->get_stream(stream_id);
    DebugLog::logClassStatus(DebugLog::LOG_INFO, "[READ CALLBACK]",
                             "[NGHTTP2]");

    if (!data)
      return -1;

    std::size_t len = 0;

    if (data->res.file_path != "") {
      std::ifstream file;

      file.open(data->res.file_path, std::ios::in | std::ios::binary);
      DebugLog::logClassStatus(DebugLog::LOG_INFO,
                               "file_name: " + data->res.file_path,
                               "[HTTP2_CALLBAKS_NG][MY READ CALLBACK]");

      if (!file.is_open()) {
        DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                                 "[MY READ CALLBACK] Could not open file: " +
                                     data->res.file_path,
                                 "[NGHTTP2]");
        return -1;
      }

      if (data->lastlenght == 0 && data->left == 0) {
        file.seekg(0, std::ifstream::end);

        data->left = file.tellg();
        DebugLog::logClassStatus(DebugLog::LOG_INFO,
                                 "File size: " + std::to_string(data->left),
                                 "[HTTP2_CALLBAKS_NG][my_read_callback]");

        file.seekg(0, std::ifstream::beg);
      }

      if (data->left > 0) {
        DebugLog::logClassStatus(DebugLog::LOG_INFO,
                                 "[MY READ CALLBACK] Reading", "[NGHTTP2]");
        std::size_t to_read = data->left > length ? length : data->left;
        std::vector<char> bytes(to_read);

        file.seekg(data->lastlenght);

        file.read(bytes.data(), to_read);
        memcpy(buf, bytes.data(), bytes.size());
        data->lastlenght += to_read;
        data->left -= to_read;
        len = to_read;
      }

    } else if (data->res.body != "") {

      if (data->left == 0 && data->lastlenght == 0) {
        data->left = data->res.body.size();
      }

      if (data->left > 0) {
        std::size_t to_read = data->left > length ? length : data->left;
        memcpy(buf, data->res.body.data() + data->lastlenght, to_read);
        data->lastlenght += to_read;
        data->left -= to_read;
        len = to_read;
      }
    }

    if (data->left == 0) {
      DebugLog::logClassStatus(DebugLog::LOG_INFO, "NGHTTP2_DATA_FLAG_EOF",
                               "[NGHTTP2]");
      *data_flags |= NGHTTP2_DATA_FLAG_EOF;
    }

    return (nghttp2_ssize)len;
  }

  static void get_service_and_url_path(const std::string &url,
                                       std::string &service_name,
                                       std::string &url_path) {

    std::string url_ = url.substr(0, url.size() - 1);

    DebugLog::logClassStatus(DebugLog::LOG_INFO,
                             "url size: " + std::to_string(url.size()),
                             "[HTTP2_CALLBAKS_NG][get_service_and_url_path]");

    DebugLog::logClassStatus(DebugLog::LOG_INFO, "url[0]: " + url.substr(0, 1),
                             "[HTTP2_CALLBAKS_NG][get_service_and_url_path]");

    if (url_.size() == 1 && url_[0] == '/') {
      DebugLog::logClassStatus(DebugLog::LOG_INFO, "path is /",
                               "[HTTP2_CALLBAKS_NG][get_service_and_url_path]");
      service_name = "/";
      url_path = "/";
      return;
    } else if (url.size() > 1) {
      std::size_t service_pos = url_.find("/", 1);
      std::size_t target_pos = url_.size();

      if (service_pos == std::string::npos) {
        service_name = "/";
        std::size_t offset = 1;

        url_path.resize(url_.size() - offset);
        std::size_t size_copy = url_.size() - offset;
        std::memcpy(url_path.data(), url_.data() + offset, size_copy);
        return;
      }

      std::size_t offset = 1;

      service_name.resize(service_pos - offset);
      url_path.resize(target_pos - service_pos);

      std::memcpy(service_name.data(), url_.data() + offset, service_pos);
      std::memcpy(url_path.data(), url_.data() + service_pos, target_pos);
    }
  }

  static int on_request_recv(http2_session *connect_session,
                             nghttp2_session *http2_session,
                             stream_data *stream) {
    response cb_response;
    if (stream->request_path == "") {
      if (error_reply(connect_session, http2_session, stream, cb_response) !=
          0) {
        DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                                 "[ERROR REPLY] Reply not send", "[NGHTTP2]");
        return NGHTTP2_ERR_CALLBACK_FAILURE;
      }
      return 0;
    }

    if (!check_path(stream->request_path)) {

      DebugLog::logClassStatus(DebugLog::LOG_WARNING,
                               "Path is not acceptable: " +
                                   stream->request_path,
                               "[HTTP2_CALLBAKS_NG");

      if (error_reply(connect_session, http2_session, stream, cb_response) !=
          0) {
        DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                                 "[ERROR REPLY] Reply not send", "[NGHTTP2]");
        return NGHTTP2_ERR_CALLBACK_FAILURE;
      }
      return 0;
    }

    std::string service_name;
    std::string url_service_request;

    get_service_and_url_path(stream->request_path, service_name,
                             url_service_request);

    DebugLog::logClassStatus(DebugLog::LOG_INFO,
                             "request_path: " + stream->request_path,
                             "[HTTP2_CALLBAKS_NG]");
    DebugLog::logClassStatus(DebugLog::LOG_INFO,
                             "service_name: " + service_name,
                             "[HTTP2_CALLBAKS_NG]");
    DebugLog::logClassStatus(DebugLog::LOG_INFO,
                             "url_service_request: " + url_service_request,
                             "[HTTP2_CALLBAKS_NG]");

    cb_response = connect_session->invoke_session_url_callback(
        service_name, stream->method, url_service_request);

    if (cb_response.error_response) {
      if (error_reply(connect_session, http2_session, stream, cb_response) != 0)
        return NGHTTP2_ERR_CALLBACK_FAILURE;

      return 0;
    }
    DebugLog::logClassStatus(DebugLog::LOG_INFO,
                             "on_request_recv: " + cb_response.file_path,
                             "[HTTP2_CALLBAKS_NG]");

    nghttp2_nv hdrs[] = {
        MAKE_NV(":status", "200"),
        MAKE_NV("content-type", cb_response.mime_type.c_str())};

    hdrs[1].valuelen = cb_response.mime_type.size();

    if (send_response(connect_session, http2_session, stream->stream_id, hdrs,
                      sizeof(hdrs) / sizeof(hdrs[0]), cb_response) != 0)
      return NGHTTP2_ERR_CALLBACK_FAILURE;

    return 0;
  }

  static int error_reply(http2_session *connect_session,
                         nghttp2_session *http2_session, stream_data *stream,
                         response cb_response) {

    cb_response.body = ERROR_HTML;
    cb_response.mime_type = extension_to_type("html");

    DebugLog::logClassStatus(DebugLog::LOG_INFO,
                             "mime-type: " + cb_response.mime_type,
                             "[HTTP2_CALLBAKS_NG]");

    nghttp2_nv hdrs[] = {
        MAKE_NV(":status", "404"),
        MAKE_NV("content-type", cb_response.mime_type.c_str())};

    hdrs[1].valuelen = cb_response.mime_type.size();
    DebugLog::logClassStatus(
        DebugLog::LOG_INFO,
        std::string(reinterpret_cast<char *>(hdrs[1].value), hdrs[1].valuelen),
        "[HTTP2_CALLBAKS_NG]");

    send_response(connect_session, http2_session, stream->stream_id, hdrs,
                  sizeof(hdrs) / sizeof(hdrs[0]), cb_response);

    return 0;
  }

  static int on_begin_headers_callback(nghttp2_session *http2_session,
                                       const nghttp2_frame *frame,
                                       void *user_data) {

    auto session_ptr = reinterpret_cast<class http2_session *>(user_data);

    if (frame->hd.type != NGHTTP2_HEADERS ||
        frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
      DebugLog::logClassStatus(DebugLog::LOG_WARNING,
                               "No NGHTTP2_HEADERS and No NGHTTP2_HCAT_REQUEST",
                               "[NGHTTP2]");
      return 0;
    }

    stream_data data = create_http2_stream_data(frame->hd.stream_id);

    auto data_ptr = session_ptr->add_stream(data);

    nghttp2_session_set_stream_user_data(http2_session, frame->hd.stream_id,
                                         data_ptr);

    return 0;
  }

  static nghttp2_ssize send_callback(nghttp2_session *http2_session,
                                     const uint8_t *data, size_t length,
                                     int flags, void *user_data) {
    auto session_ptr = reinterpret_cast<class http2_session *>(user_data);

    if (length > BUFFER_SIZE)
      return NGHTTP2_ERR_WOULDBLOCK;

    DebugLog::logClassStatus(DebugLog::LOG_INFO, "[SEND_CALLBACK]",
                             "[nghttp2_callbacks]");
    session_ptr->add_to_queue(data, length);

    return (nghttp2_ssize)length;
  }

  static int on_frame_recv_callback(nghttp2_session *http2_session,
                                    const nghttp2_frame *frame,
                                    void *user_data) {
    stream_data *data = nullptr;

    auto session_ptr = reinterpret_cast<class http2_session *>(user_data);

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

    auto session_ptr = reinterpret_cast<class http2_session *>(user_data);

    stream_data *data = (stream_data *)nghttp2_session_get_stream_user_data(
        session_ptr->get_session_ptr(), stream_id);

    if (!data)
      return 0;

    session_ptr->remove_stream_data(stream_id);
    DebugLog::logClassStatus(DebugLog::LOG_INFO, "Stream data removed",
                             "[HTTP2_CALLBAKS_NG]");
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
      stream_data *data = (stream_data *)nghttp2_session_get_stream_user_data(
          http2_session, frame->hd.stream_id);

      if (!data)
        break;

      if (std::strcmp(reinterpret_cast<const char *>(name), ":method") == 0) {
        data->method =
            std::string(reinterpret_cast<const char *>(value), valuelen);

      } else if (namelen == sizeof(PATH) - 1 &&
                 memcmp(PATH, name, namelen) == 0) {
        std::size_t j;
        for (j = 0; j < valuelen && value[j] != '?'; j++)
          ;
        data->request_path = percent_decode(value, j);
      }

      break;
    }

    return 0;
  }
};
} // namespace http2Cpp

#endif
