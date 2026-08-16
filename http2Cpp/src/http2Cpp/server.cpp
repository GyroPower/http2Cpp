#include "server.hpp"
#include "io_context_pool.hpp"
#include "logger.hpp"
#include "service.hpp"
#include "session.hpp"
#include <asio/ssl/context.hpp>
#include <asio/ssl/context_base.hpp>
#include <exception>
#include <string>

namespace http2Cpp {

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

server::server(short port_, std::size_t num_threads, bool https,
               const std::string &key_path, const std::string &key_secret,
               ssl::context_base::method context_method)
    : m_io_ctx_pool(std::make_unique<io_context_pool>(num_threads)),
      m_acceptor(m_io_ctx_pool->get_io_context(),
                 tcp::endpoint(tcp::v4(), port_)),
      num_threads(num_threads), m_ssl_ctx(context_method),
      m_signals(this->m_io_ctx_pool->get_io_context()), port(port_) {

  if (https && key_path == "" || key_secret == "") {
    DebugLog::logClassStatus(
        DebugLog::LOG_ERROR,
        "https is set but not certifications files where given", "[SERVER]");
  }

  try {
    this->m_ssl_ctx.use_certificate_chain_file(key_path);
    this->m_ssl_ctx.use_private_key_file(key_secret,
                                         ssl::context::file_format::pem);

    SSL_CTX_set_alpn_select_cb(this->m_ssl_ctx.native_handle(),
                               alpn_select_proto_cb, NULL);

    this->m_signals.add(SIGINT);
    this->m_signals.add(SIGTERM);

#if defined(SIGQUIT)
    this->m_signals.add(SIGQUIT);
#endif

    this->await_stop();

    this->m_acceptor.set_option(tcp::acceptor::reuse_address(true));
    this->m_acceptor.set_option(tcp::acceptor::keep_alive(true));
    this->m_acceptor.listen(asio::socket_base::max_listen_connections);

    this->do_accept();

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
  }
}

// If service with the name is already registered it will update it to have
// the new service you pass to the method
void server::add_service(service service_, std::string name) {

  if (name == "")
    name = "/";

  if (this->m_services.find(name) != this->m_services.end()) {
    this->m_services[name] = service_;
    return;
  }

  this->m_services.insert({name, service_});
}

void server::run() { this->m_io_ctx_pool->run(); }

void server::await_stop() {
  this->m_signals.async_wait(
      [this](std::error_code ec, int sign) { this->m_io_ctx_pool->stop(); });
}
void server::do_accept() {
  this->m_acceptor.async_accept(this->m_io_ctx_pool->get_io_context(),
                                [this](std::error_code ec, tcp::socket socket) {
                                  if (!ec) {

                                    socket.set_option(tcp::no_delay(true));

                                    std::make_shared<session>(
                                        std::move(socket), this->port,
                                        this->m_ssl_ctx, this->m_services)
                                        ->start();
                                  }
                                  this->do_accept();
                                });
}
} // namespace http2Cpp
