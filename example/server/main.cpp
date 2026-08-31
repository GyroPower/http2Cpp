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
#include <CLI/CLI.hpp>
#include <exception>
#include <http2Cpp/server.hpp>
#include <http2Cpp/service.hpp>
#include <http2Cpp/session.hpp>
#include <iostream>
#include <logger.hpp>
#include <string>

int main(int argc, char **argv) {

  try {

    bool https = false;
    short port = 8082;
    short num_threads = 1;
    std::string public_key = "localhost+2.pem";
    std::string key = "localhost+2-key.pem";

    CLI::App app{"Http2 Server example from http2Cpp library"};

    // argv = app.ensure_utf8(argv);

    app.add_option("-p,--port", port, "Port to use for listening");
    app.add_option("--https", https,
                   "Set to use https, only for http1 (not yet implementation)");
    app.add_option("-t,--threads-num", num_threads, "Number of threads touse ");
    app.add_option("--pk,--public-key ", public_key,
                   "The public key for https encryption");
    app.add_option("-k,--key-private", key, "Private key for https encryption");

    CLI11_PARSE(app, argc, argv);

    DebugLog::logClassStatus(DebugLog::LOG_INFO,
                             "port: " + std::to_string(port));
    DebugLog::logClassStatus(DebugLog::LOG_INFO,
                             std::string("https: ") +
                                 (https ? "true" : "false"));
    DebugLog::logClassStatus(DebugLog::LOG_INFO, "public_key: " + public_key);
    DebugLog::logClassStatus(DebugLog::LOG_INFO, "key_private: " + key);

    if (public_key == "" || key == "") {
      DebugLog::logClassStatus(DebugLog::LOG_ERROR, "No keys set");
      return -1;
    }

    http2Cpp::service service{};

    http2Cpp::server server(port, num_threads, https, public_key, key);

    server.add_service(service);

    server.run();

    DebugLog::logClassStatus(DebugLog::LOG_INFO, "Ended");

    return 0;

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
  }
}
