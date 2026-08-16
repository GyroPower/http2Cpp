#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "url.hpp"
#include <map>
#include <string>

namespace http2Cpp {

struct response;

class method;

// The core idea is to have a wrapper for a common url to add different methods
// after defining all the callbacks for every method it's being added to the
// service to keep alive all the callbacks
// TODO
// add feature for url variables parser and sanatize the variables
class url;

// A service is a module to call for the server
// you can write your own class implementation
// The base is to name a service created and insert
// url objects which can hold diffetent callbacks for differents
// http request methods
class service {
private:
  std::map<std::string, url> m_urls;
  std::string m_doc_root = "";
  std::string m_service_name = "";

  virtual response m_get_static(const std::string &doc_req);

public:
  // if doc_root not defined it will set default
  // views/ from where is invoked the server to run.
  // Example set doc_root to service_name/views to make it
  // ok to grab the static files that needs the client request
  service(std::string service_name = "", std::string doc_root = "");
  // virtual std::string get_doc_root();

  /*
   * The core idea is a client request /user/settings
   * the service is user and the url requested is settings
   * settings has assign different callbacks, one for GET and POST
   * */
  virtual response invoke_cb(const std::string &method,
                             const std::string &path_url);
  virtual void insert_url(std::string url_name, url url_object);
};
} // namespace http2Cpp
#endif
