#ifndef SERVER_HPP
#define SERVER_HPP
#include "io_context_pool.hpp"
#include "net.hpp"
#include <asio/signal_set.hpp>
#include <asio/ssl/context.hpp>
#include <asio/ssl/context_base.hpp>
#include <map>
#include <string>

namespace http2Cpp {
class io_context_pool;
class service;

class server {
  io_context_pool m_io_ctx_pool;
  tcp::acceptor m_acceptor;
  ssl::context m_ssl_ctx;
  std::size_t num_threads = 1;
  asio::signal_set m_signals;
  short port;

  std::map<std::string, service> m_services;

public:
  server(short port_, std::size_t num_threads = 1, bool https = true,
         const std::string &key_path = "", const std::string &key_secret = "",
         ssl::context_base::method context_method =
             ssl::context_base::tlsv13_server);

  void add_service(service service_, std::string name = "");
  void run();

private:
  void await_stop();
  void do_accept();
};

} // namespace http2Cpp

#endif
