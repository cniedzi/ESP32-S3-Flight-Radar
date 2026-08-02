#include "HttpRequestManager.h"
#include <WiFiClient.h>
#include <esp_heap_caps.h>




HttpResult HttpRequestManager::GetJson(const String& url, JsonDocument& jsonDoc) {
    HttpResult result{ false, 0, "", "" };

    http.begin(url);
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
                if (size) {
                    int c = client->readBytes(buffer + bytesRead, size);
                    bytesRead += c;
                    if (len > 0) len -= c;
                }
                delay(1);
            }
            buffer[bytesRead] = '\0';

            DeserializationError error = deserializeJson(jsonDoc, (const char*)buffer);
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