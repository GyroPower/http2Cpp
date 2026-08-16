#ifndef SESSION_HPP
#define SESSION_HPP
#include "http2_session.hpp"
#include "net.hpp"
#include "service.hpp"
#include <cstdint>
#include <map>
#include <memory>
#include <nghttp2/nghttp2.h>
#include <string>

namespace http2Cpp {
#define BUFFER_SIZE 100857
#define MAKE_NV(NAME, VALUE)                                                   \
  {                                                                            \
      (uint8_t *)NAME,   (uint8_t *)VALUE,     sizeof(NAME) - 1,               \
      sizeof(VALUE) - 1, NGHTTP2_NV_FLAG_NONE,                                 \
  }

const static std::string ERROR_HTML_ =
    "<html><head><title>404</title></head>"
    "<body><h1>404 Not Found</h1></body></html>";

class session : public std::enable_shared_from_this<session> {

  ssl::context &m_ssl_ctx;

  std::vector<std::vector<std::uint8_t>> m_queue_write;
  std::vector<std::vector<std::uint8_t>> m_queue_stage;
  std::vector<asio::const_buffer> m_send_buffers;
  bool writing = false;
  std::unique_ptr<ssl::stream<tcp::socket>> m_secSocket = nullptr;
  std::unique_ptr<http2_session> m_https2_session_ = nullptr;
  tcp::socket m_socket;
  short local_port;

  std::map<std::string, service> &m_services;

  unsigned char m_data[BUFFER_SIZE];
  std::array<char, BUFFER_SIZE> m_buffer;

public:
  explicit session(tcp::socket socket, short port, asio::ssl::context &ssl_ctx,
                   std::map<std::string, service> &services);

  ~session();

  response invoke_url_cb(const std::string &service, const std::string &method,
                         const std::string &path); /*{
// return this->m_test.invoke_cb(path);
}*/

  void add_to_queue(const std::uint8_t *data, std::size_t bytes_len);

  void start();

private:
  void write();

  void do_handshake();

  void read();

  void run();
};
} // namespace http2Cpp
#endif
