#ifndef HTTP2_SESSION
#define HTTP2_SESSION
#include "response.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <nghttp2/nghttp2.h>
#include <string>

namespace http2Cpp {

class session;

const static std::string ERROR_HTML =
    "<html><head><title>404</title></head>"
    "<body><h1>404 Not Found</h1></body></html>";

struct stream_data {
  std::string request_path;
  std::uint32_t stream_id;
  std::size_t lastlenght = 0;
  std::size_t left = 0;
  std::string method;

  response res;
};

class http2_session {
private:
  nghttp2_session *m_https2_session = nullptr;
  std::map<std::uint32_t, stream_data> m_streams;
  session *connection_session = nullptr;

public:
  http2_session(session *connection_session);
  ~http2_session();

  stream_data *add_stream(stream_data data);
  bool send_session();
  bool set_config();
  bool read_request_bytes(std::uint8_t *data, std::size_t bytes_size);
  void add_to_queue(const std::uint8_t *data, std::size_t bytes_len);

  nghttp2_session *get_session_ptr();
  void remove_stream_data(std::uint32_t stream_id);
  stream_data *get_stream(std::uint32_t stream_id);

  bool set_file_path_to_stream(std::uint32_t stream_id, std::string file_path);
  bool set_data_response_to_stream(std::uint32_t stream_id,
                                   response cb_response);

  response invoke_session_url_callback(const std::string &service,
                                       const std::string &method,
                                       const std::string &url);

private:
  bool m_set_config();
  void initialize_nghttp2_session();

public:
};
} // namespace http2Cpp
#endif
