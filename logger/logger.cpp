#include "logger.hpp"
#include <cstring>

std::mutex log_mutex;

void DebugLog::logInfo(const std::string &message) {
#ifdef DEBUG
  std::lock_guard<std::mutex> lock(DebugLog::GetMutex());
  std::cout << "[INFO] " << message << "\n";
#endif
}

void DebugLog::logTitle(const std::string &title) {
#ifdef DEBUG
  std::lock_guard<std::mutex> lock(DebugLog::GetMutex());
  std::cout << "-------------------------" << "\n";
  std::cout << title << "\n";
  std::cout << "-------------------------" << "\n";
#endif
}

void DebugLog::logClassStatus(debugStatus status, const std::string &info,
                              const std::string &className) {
// Use the enum in the class for the kind of log you want
#ifdef DEBUG
  char status_type[10];
  if (status == debugStatus::LOG_ERROR)
    strncpy(status_type, "[ERROR]", sizeof(status_type));
  else if (status == debugStatus::LOG_INFO)
    strncpy(status_type, "[INFO]", sizeof(status_type));
  else if (status == debugStatus::LOG_WARNING)
    strncpy(status_type, "[WARNING]", sizeof(status_type));

  std::lock_guard<std::mutex> lock(DebugLog::GetMutex());
  std::cout << className << status_type << ": " << info << "\n";
#endif
}

std::mutex &DebugLog::GetMutex() {
#ifdef DEBUG
  static std::mutex m;
  return m;
#endif // DEBUG
}
