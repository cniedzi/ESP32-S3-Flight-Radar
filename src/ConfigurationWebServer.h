#pragma once

#include <ESPAsyncWebServer.h>
#include <Preferences.h>

extern bool g_restartNeeded;

class ConfigurationWebServer {
private:
    AsyncWebServer server;
    Preferences prefs;
    void psram_replace(char *buffer, size_t max_len, const char *old_str, const char *new_str);

public:
    ConfigurationWebServer() : server(80) {};
    ConfigurationWebServer(int port) : server(port) {};

    void Initialise();
    double GetStoredDouble(const char* key, double defaultValue);
    int GetStoredInt(const char* key, int defaultValue);
    bool GetStoredBool(const char* key, bool defaultValue);
    void SaveDouble(const char* key, double value);
    void SaveInt(const char* key, int value);
    void SaveBool(const char* key, bool value);
};



class PSRAMChunkedResponse : public AsyncChunkedResponse {
private:
  char* _buffer;
public:
  PSRAMChunkedResponse(const char* contentType, std::function<size_t(uint8_t*, size_t, size_t)> callback, char* buffer)
      : AsyncChunkedResponse(contentType, callback), _buffer(buffer) {}
  virtual ~PSRAMChunkedResponse() {
    if (_buffer != nullptr) {
      heap_caps_free(_buffer);
    }
  }
};