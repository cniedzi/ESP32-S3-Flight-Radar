#pragma once

#include <HTTPClient.h>
#include <vector>
#include <ArduinoJson.h>

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

    // Bezpieczna metoda GET parsująca strumień prosto do JSON
    [[nodiscard]] HttpResult GetJson(const String& url, JsonDocument& jsonDoc, const std::vector<std::pair<String, String>>& headers = {});
    [[nodiscard]] HttpResult Post(const String& url, const String& body = "", const std::vector<std::pair<String, String>>& headers = {});
    
};