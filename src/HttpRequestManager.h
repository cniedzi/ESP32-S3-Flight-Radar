#pragma once

#include <HTTPClient.h>
#include <vector>
#include <ArduinoJson.h> // Dodane: Konieczne do obsługi JsonDocument

struct HttpResult {
    bool success;           // Whether the request succeeded
    int statusCode;         // HTTP status code (0 if network error)
    String response;        // Response body (empty on error or when using GetJson)
    String errorMessage;    // Error description if success == false
};

class HttpRequestManager
{
private:
    HTTPClient http;
public:
    HttpRequestManager() = default;
    ~HttpRequestManager() = default;

    // Nowa, bezpieczna metoda GET parsująca strumień prosto do JSON
    [[nodiscard]] HttpResult GetJson(const String& url, JsonDocument& jsonDoc);
};