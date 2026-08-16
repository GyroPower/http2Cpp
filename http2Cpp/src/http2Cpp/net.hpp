#include <asio.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ssl.hpp>
#include <nghttp2/nghttp2.h>

namespace http2Cpp {

using tcp = asio::ip::tcp;
namespace ssl = asio::ssl;
} // namespace http2Cpp
