#pragma once

#include <ESPAsyncWebServer.h>
#include "SettingsManager.h"
#include "AircraftManager.h"


class ConfigurationWebServer {
private:
    AsyncWebServer server;
    AsyncWebSocket ws{"/ws"};
    SettingsManager& settings;
    AircraftManager& aircraftmanager;
    
    void psram_replace(char *buffer, size_t max_len, const char *old_str, const char *new_str);

public:
    ConfigurationWebServer(SettingsManager& settingsManager, AircraftManager& aircraftManager) : server(80), settings(settingsManager), aircraftmanager(aircraftManager) {};
    ConfigurationWebServer(int port, SettingsManager& settingsManager, AircraftManager& aircraftManager) : server(port), settings(settingsManager), aircraftmanager(aircraftManager) {};

    void Initialise();
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