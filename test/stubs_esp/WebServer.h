#pragma once
#include "Arduino.h"
#include <functional>
enum HTTPMethod { HTTP_GET, HTTP_POST };
enum HTTPUploadStatus { UPLOAD_FILE_START, UPLOAD_FILE_WRITE, UPLOAD_FILE_END, UPLOAD_FILE_ABORTED };
struct HTTPUpload { HTTPUploadStatus status; uint8_t buf[64]; size_t currentSize; };
class WebServer {
public:
  typedef std::function<void(void)> THandlerFunction;
  WebServer(int) {}
  void on(const char *, HTTPMethod, THandlerFunction) {}
  void on(const char *, HTTPMethod, THandlerFunction, THandlerFunction) {}
  void onNotFound(THandlerFunction) {}
  void begin() {} void stop() {} void handleClient() {}
  bool hasArg(const char *) { return false; }
  String arg(const char *) { return String(); }
  String lastBody; void send(int, const char *, const String &b) { lastBody = b; }
  void send(int, const char *, const char *) {}
  void sendHeader(const char *, const String &, bool = false) {}
  HTTPUpload &upload() { static HTTPUpload u; return u; }
};
