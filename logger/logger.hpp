#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <mutex>
#include <string>

extern std::mutex log_mutex;

class DebugLog {
public:
  enum debugStatus { LOG_ERROR, LOG_INFO, LOG_WARNING };

  static void logInfo(const std::string &message);

  template <typename T>
  static void logVar(const std::string &label, const T &value) {
#ifdef DEBUG
    std::lock_guard<std::mutex> lock(DebugLog::GetMutex());
    std::cout << "[VAR] " << label << " = " << value << "\n";
#endif
  }

  static void logTitle(const std::string &title);

  // Log a class or struct status info
  static void logClassStatus(debugStatus status, const std::string &info = "",
                             const std::string &className = "");

private:
  static std::mutex &GetMutex();
};

#endif // LOGGER_HPP
