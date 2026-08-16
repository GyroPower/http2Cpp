#include "http2_session.hpp"
#include "logger.hpp"
#include "nghttp2_callbacks.hpp"
#include "session.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <nghttp2/nghttp2.h>
#include <string>

// #define MAKE_NV(NAME, VALUE) \
//   { \
//       (uint8_t *)NAME,   (uint8_t *)VALUE,     sizeof(NAME) - 1, \
//       sizeof(VALUE) - 1, NGHTTP2_NV_FLAG_NONE, \
//   }

#define MAKE_NV(NAME, VALUE)                                                   \
  {                                                                            \
      (uint8_t *)NAME,   (uint8_t *)VALUE,     sizeof(NAME) - 1,               \
      sizeof(VALUE) - 1, NGHTTP2_NV_FLAG_NONE,                                 \
  }

namespace http2Cpp {

http2_session::http2_session(session *connection_session)
    : connection_session(connection_session) {
  this->initialize_nghttp2_session();
}

http2_session::~http2_session() { nghttp2_session_del(this->m_https2_session); }

bool http2_session::set_config() {
  if (!this->m_set_config() || !this->send_session()) {
    DebugLog::logClassStatus(
        DebugLog::LOG_ERROR,
        "[SET CONFIG][SEND SESSION] Could not set one of those",
        "[http2_session]");
    return false;
  }

  return true;
}

bool http2_session::m_set_config() {
  nghttp2_settings_entry settings_entry[1] = {
      {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};

  int result;

  result = nghttp2_submit_settings(
      this->m_https2_session, NGHTTP2_FLAG_NONE, settings_entry,
      (sizeof(settings_entry) / sizeof(settings_entry[0])));

  if (result != 0) {
    DebugLog::logClassStatus(
        DebugLog::LOG_ERROR,
        "settings " + std::string(nghttp2_strerror(result)), "[http2_session]");
    return false;
  }

  return true;
}

void http2_session::initialize_nghttp2_session() {

  nghttp2_session_callbacks *callbacks;
  nghttp2_session_callbacks_new(&callbacks);

  nghttp2_session_callbacks_set_send_callback2(
      callbacks, http2_static_callbacks::send_callback);

  nghttp2_session_callbacks_set_on_frame_recv_callback(
      callbacks, http2_static_callbacks::on_frame_recv_callback);

  nghttp2_session_callbacks_set_on_stream_close_callback(
      callbacks, http2_static_callbacks::on_stream_close_callback);

  nghttp2_session_callbacks_set_on_header_callback(
      callbacks, http2_static_callbacks::on_header_callback);

  nghttp2_session_callbacks_set_on_begin_headers_callback(
      callbacks, http2_static_callbacks::on_begin_headers_callback);

  nghttp2_session_server_new(&this->m_https2_session, callbacks, this);

  nghttp2_session_callbacks_del(callbacks);
}

stream_data *http2_session::get_stream(std::uint32_t stream_id) {
  if (this->m_streams.find(stream_id) != this->m_streams.end()) {
    return &this->m_streams.at(stream_id);
  }
  return nullptr;
}
bool http2_session::send_session() {
  int result = nghttp2_session_send(this->m_https2_session);

  if (result != 0) {
    DebugLog::logClassStatus(DebugLog::LOG_ERROR, nghttp2_strerror(result),
                             "[http2_session]");

    return false;
  }

  return true;
}

bool http2_session::read_request_bytes(std::uint8_t *data,
                                       std::size_t bytes_size) {
  nghttp2_ssize readlen;

  readlen = nghttp2_session_mem_recv2(this->m_https2_session, data, bytes_size);

  DebugLog::logClassStatus(DebugLog::LOG_INFO, "Reading request bytes http2",
                           "[http2_session]");

  if (readlen < 0) {
    DebugLog::logClassStatus(DebugLog::LOG_ERROR,
                             "[READING]" +
                                 std::string(nghttp2_strerror((int)readlen)),
                             "[http2_session]");
    return false;
  }

  return this->send_session();
}

nghttp2_session *http2_session::get_session_ptr() {
  return this->m_https2_session;
}

void http2_session::remove_stream_data(std::uint32_t stream_id) {
  if (this->m_streams.find(stream_id) != this->m_streams.end()) {
    this->m_streams.erase(stream_id);
  } else {
    DebugLog::logClassStatus(DebugLog::LOG_WARNING,
                             "[REMOVE STREAM] Stream not found | id: " +
                                 std::to_string(stream_id),
                             "[http2_session]");
  }
}

stream_data *http2_session::add_stream(stream_data data) {
  this->m_streams.insert({data.stream_id, data});
  return &this->m_streams.at(data.stream_id);
}

bool http2_session::set_data_response_to_stream(std::uint32_t stream_id,
                                                response cb_response) {

  if (this->m_streams.find(stream_id) != this->m_streams.end()) {
    auto &stream = this->m_streams.at(stream_id);
    stream.res = cb_response;
    return true;
  } else {
    DebugLog::logClassStatus(DebugLog::LOG_WARNING,
                             "[SET DATA TO STREAM] stream not found | id: " +
                                 std::to_string(stream_id),
                             "[http2_session]");
    return false;
  }
}

bool http2_session::set_file_path_to_stream(std::uint32_t stream_id,
                                            std::string file_path) {
  if (this->m_streams.find(stream_id) != this->m_streams.end()) {
    this->m_streams.at(stream_id).res.file_path = file_path;
    return true;
  } else {
    DebugLog::logClassStatus(DebugLog::LOG_WARNING,
                             "[SET FILE PATH STREAM] no stream found | id: " +
                                 std::to_string(stream_id),
                             "[http2_session]");
    return false;
  }
}

response http2_session::invoke_session_url_callback(const std::string &service,
                                                    const std::string &method,
                                                    const std::string &url) {

  return this->connection_session->invoke_url_cb(service, method, url);
}

void http2_session::add_to_queue(const std::uint8_t *data,
                                 std::size_t bytes_len) {
  this->connection_session->add_to_queue(data, bytes_len);
}

} // namespace http2Cpp
