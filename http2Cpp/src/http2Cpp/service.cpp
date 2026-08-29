#include "service.hpp"
#include "http2_session.hpp"
#include "logger.hpp"
#include "mime_types.hpp"
#include "url.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <vector>

namespace http2Cpp {
namespace fs = std::filesystem;

// Default base url callback for the home url
// you can overwrite your own one of course
static response index_cb(const std::string &doc_root) {
  fs::path file_path = doc_root + "home.html";

  DebugLog::logClassStatus(DebugLog::LOG_INFO, "Index cb", "[url callback]");
  response cb_response;
  if (fs::exists(file_path)) {
    DebugLog::logClassStatus(DebugLog::LOG_INFO,
                             "file_path exists: " + file_path.string(),
                             "[url callback]");

    cb_response.file_path = file_path.string();
    cb_response.mime_type = extension_to_type("html");
    return cb_response;
  }

  return {.error_response = true};
}

service::service(std::string service_name, std::string doc_root) {

  if (service_name == "")
    this->m_service_name = "/";
  if (doc_root == "")
    this->m_doc_root = "views/";

  url root{"/"};
  root.insert_callback(method::type::Get, index_cb);
  this->m_urls.insert({root.get_url_prefix(), root});

  // this->m_urls.insert({"/", index_cb});
}

response service::m_get_static(const std::string &doc_req) {

  std::string target;
  std::string extension;
  response r{};

  DebugLog::logClassStatus(DebugLog::LOG_INFO, "Doc request: " + doc_req,
                           "[service]");

  target.resize(doc_req.size());
  std::memcpy(target.data(), doc_req.data(), doc_req.size());

  fs::path file_path = this->m_doc_root + target;

  DebugLog::logClassStatus(DebugLog::LOG_INFO,
                           "File target: " + file_path.string(), "[service]");

  if (fs::exists(file_path)) {

    DebugLog::logClassStatus(DebugLog::LOG_INFO,
                             "File exists: " + file_path.string(), "[service]");
    extension.resize(sizeof(file_path.extension().c_str()) - 1);

    std::memcpy(extension.data(), file_path.extension().c_str() + 1,
                sizeof(file_path.extension().c_str()) - 1);

    r.mime_type = extension_to_type(extension);
    r.file_path = file_path.string();
  }

  return r;
}

void service::insert_url(std::string url_name, url url_object) {}

response service::invoke_cb(const std::string &method,
                            const std::string &url_path) {
  // TODO
  // sanatize correctly the url path for last emptu spaces
  // this fix is dum but is for last empty space
  // std::string path_(url_path.begin(), url_path.end() - 1);

  DebugLog::logClassStatus(DebugLog::LOG_INFO, "path url: " + url_path,
                           "[service]");

  method::type m;

  if (method == "GET")
    m = method::type::Get;
  else if (method == "POST")
    m = method::type::Post;
  else if (method == "PUT")
    m = method::type::Put;
  else
    m = method::type::Delete;

  response url_response{};

  if (this->m_urls.find(url_path) != this->m_urls.end()) {
    url_response =
        this->m_urls.at(url_path).invoke_callback(m, this->m_doc_root);
  } else {
    url_response = this->m_get_static(url_path);
  }

  if (url_response.body == "" && url_response.file_path == "")
    url_response.error_response = true;
  return url_response;
}

} // namespace http2Cpp
