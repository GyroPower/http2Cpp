#include "url.hpp"
#include "response.hpp"
namespace http2Cpp {
url::url(std::string url_prefix_) : url_prefix(url_prefix_) {}

void url::insert_callback(method::type method_,
                          std::function<response(const std::string &)> cb) {
  this->url_callbacks.insert({method_, cb});
}

response url::invoke_callback(method::type method_,
                              const std::string &some_text) {
  return this->url_callbacks.at(method_)(some_text);
}

const std::string url::get_url_prefix() { return this->url_prefix; }
} // namespace http2Cpp
