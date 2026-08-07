#include "HttpRequestManager.h"
#include <WiFiClient.h>
#include <esp_heap_caps.h>





HttpResult HttpRequestManager::GetJson(const String& url, JsonDocument& jsonDoc, const std::vector<std::pair<String, String>>& headers) {
    HttpResult result{ false, 0, "", "" };

    http.begin(url);

    // Dodawanie nagłówków HTTP (tokena uwierzytelniającego dla OpenSky)
    for (const auto& header : headers) {
        http.addHeader(header.first, header.second);
    }

    http.useHTTP10(true);
    http.setTimeout(10000);

    int responseCode = http.GET();
    result.statusCode = responseCode;
    
    if (responseCode > 0) {
        int len = http.getSize();
        if (len < 0) len = 100000;
        
        char* buffer = (char*)heap_caps_malloc(len + 1, MALLOC_CAP_SPIRAM);

        if (buffer != nullptr) {
            WiFiClient* client = http.getStreamPtr();
            int bytesRead = 0;
            
            while (http.connected() && (len > 0 || len == -1)) {
                size_t size = client->available();
                if (size > 0) {
                    size_t toRead = std::min(size, (size_t)1024);
                    int c = client->readBytes(buffer + bytesRead, toRead);
                    if (c > 0) {
                        bytesRead += c;
                        if (len > 0) len -= c;
                    }
                }
                yield();
            }
            buffer[bytesRead] = '\0';
            
            // Zabezpieczenie przed zablokowaniem procesora przy parsowaniu JSON-a
            yield();
            DeserializationError error = deserializeJson(jsonDoc, (const char*)buffer);
            yield();

            heap_caps_free(buffer);

            if (error) {
                result.success = false;
                result.errorMessage = String("JSON Parse Error: ") + error.c_str();
            } else {
                result.success = true;
            }
        } else {
            result.success = false;
            result.errorMessage = "Out of PSRAM memory during fetch";
        }
    }
    else {
        result.success = false;
        result.errorMessage = http.errorToString(responseCode);
    }

    http.end();
    return result;
}







HttpResult HttpRequestManager::Post(const String& url, const String& body, const std::vector<std::pair<String, String>>& headers)
{
    HttpResult result{ false, 0, "", "" };

    http.begin(url);

    // add headers to request
    for (const auto& header : headers) {
        http.addHeader(header.first, header.second);
    }

    // send request and handle response
    int responseCode = http.POST(body);
    result.statusCode = responseCode;

    if (responseCode > 0) {
        result.response = http.getString();
        
        // Sukces to tylko kody z zakresu 2xx (np. 200 OK)
        if (responseCode >= 200 && responseCode < 300) {
            result.success = true;
        } else {
            result.success = false;
            result.errorMessage = "HTTP Error status: " + String(responseCode);
            Serial.print("[POST] HTTP error status (");
            Serial.print(responseCode);
            Serial.print("): ");
            Serial.println(result.response); // Pokaże np. komunikat o błędzie z OpenSky
        }
    }
    else {
        result.success = false;
        result.errorMessage = http.errorToString(responseCode);
        Serial.print("[POST] Network Error (");
        Serial.print(responseCode);
        Serial.print("): ");
        Serial.println(result.errorMessage);
    }

    http.end();
    return result;
}