#ifndef URL_HHP
#define URL_HHP
#include <functional>
#include <map>
#include <string>

namespace http2Cpp {

struct response;

class method {
public:
  enum class type { Get, Post, Put, Delete };
};

// The core idea is to have a wrapper for a common url to add different methods
// after defining all the callbacks for every method it's being added to the
// service to keep alive all the callbacks
// TODO
// add feature for url variables parser and sanatize the variables
class url {
  std::map<method::type, std::function<response(const std::string &)>>
      url_callbacks;
  std::string url_prefix;

public:
  url(std::string url_prefix_);

  void insert_callback(method::type method_,
                       std::function<response(const std::string &)> cb);

  response invoke_callback(method::type method_, const std::string &doc_root);

  const std::string get_url_prefix();
};
} // namespace http2Cpp
#endif
