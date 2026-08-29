#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>

namespace http2Cpp {

struct response {
  std::string mime_type;
  std::string body;
  std::string file_path;
  bool error_response = false;
};
} // namespace http2Cpp
#endif
