/*
 * Very Simple example of how the library should work
 *
 * TODO
 * Change service to name it module
 *
 * TODO
 * show how to implement a custom service (module) for a fixed base url and
 * wrapp callbacks to it
 *
 * TODO
 * Add json to the library
 * */
#include <CLI/App.hpp>
#include <http2Cpp/server.hpp>
#include <http2Cpp/service.hpp>
#include <http2Cpp/session.hpp>
#include <string>

int main(int argc, char **argv) {

  bool https;
  short port;
  short num_threads = 1;
  std::string public_key;
  std::string key;

  CLI::App app{"Http2 Server example from http2Cpp library"};
  argv = app.ensure_utf8(argv);

  app.add_option("-p,--port", port, "Port to use for listening");
  app.add_option("-htpps,--user-https", https,
                 "Set to use https, only for http1 (not yet implementation)");
  app.add_option("-t,--threads-num", num_threads, "Number of threads to use");
  app.add_option("-pk,--public-key", public_key,
                 "The public key for https encryption");
  app.add_option("-k,--key-private", key, "Private key for https encryption");

  http2Cpp::service service{};

  http2Cpp::server server(port, num_threads, https, public_key, key);

  server.add_service(service);

  server.run();

  return 0;
}
