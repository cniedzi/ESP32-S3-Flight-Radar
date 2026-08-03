#include "ConfigurationWebServer.h"
#include <ESPmDNS.h>

// HTML stored in flash
static const char CONFIG_HTML[] = R"rawliteral(
<html>
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>ESP32 Flightradar</title>
        <script src="https://cdn.jsdelivr.net/npm/@tailwindcss/browser@4.3.0"></script>
    </head>
    <body class="font-mono bg-gray-900 text-green-500 min-h-screen p-4 sm:p-0 text-md sm:text-sm">
        <fieldset class="border border-green-500 p-5 w-full max-w-2xl mx-auto sm:m-10">
            <legend class="px-2">Configure ESP32 Flightradar</legend>

            <form id="cfg" action="/save" method="POST" class="flex flex-col gap-4 sm:gap-2">

                <div class="flex flex-col sm:flex-row gap-4 sm:gap-5">
                    <label class="flex flex-col sm:flex-row gap-2 flex-1">
                        <span>Latitude:</span>
                        <input
                            name="latitude"
                            type="number"
                            min="-90"
                            step="0.000001"
                            max="90"
                            value='%LATITUDE%'
                            class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                    </label>

                    <label class="flex flex-col sm:flex-row gap-2 flex-1">
                        <span>Longitude:</span>
                        <input
                            name="longitude"
                            type="number"
                            min="-180"
                            step="0.000001"
                            max="180"
                            value='%LONGITUDE%'
                            class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                    </label>
                </div>

                <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                    <span>Radius (in nm):</span>
                    <input
                        name="radius"
                        type="number"
                        min="1"
                        step="1"
                        max="250"
                        value='%RADIUS%'
                        class="flex-1 border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                </label>

                <div class="flex flex-col sm:flex-row gap-4 pt-4">
                    <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                        <span>Aircraft Info:</span>
                        <input
                            name="infotext"
                            type="checkbox"
                            %INFOTEXT%
                            class="px-3 sm:px-1 accent-green-500">
                    </label>
                </div>

                <div class="flex flex-col sm:flex-row gap-4 sm:gap-5">
                    <input
                        type="submit"
                        value="Save"
                        class="bg-green-500 text-black mt-4 px-4 py-3 text-lg sm:text-base sm:px-2 sm:py-0 self-start cursor-pointer">

                    <div id="result" class="mt-4 px-1 sm:px-10"></div>
                </div>
            </form>
        </fieldset>

        <script>
            document.getElementById('cfg').addEventListener('submit', function(e) {
                e.preventDefault();
                fetch(this.action, { method: 'POST', body: new FormData(this) })
                    .then(r => r.text())
                    .then(html => document.getElementById('result').innerHTML = html);
            });
        </script>
    </body>
</html>
)rawliteral";



// Funkcja pomocnicza zastępująca processor w ESPAsyncWebServer
void ConfigurationWebServer::psram_replace(char *buffer, size_t max_len, const char *old_str, const char *new_str) {
    if (buffer == nullptr || old_str == nullptr || new_str == nullptr) return;

    size_t old_len = strlen(old_str);
    size_t new_len = strlen(new_str);

    if (old_len == 0) return; 

    char *pos = strstr(buffer, old_str);
    
    while (pos != nullptr) {
        size_t current_len = strlen(buffer);
        size_t tail_len = strlen(pos + old_len);

        // Zabezpieczenie przed spuchnięciem poza bufor
        if (current_len - old_len + new_len >= max_len - 1) {
            Serial.println("BLAD: Brak miejsca w PSRAM na rozszerzenie tekstu!");
            return; 
        }

        // Sprzętowe przesunięcie reszty tekstu (w lewo lub w prawo)
        memmove(pos + new_len, pos + old_len, tail_len + 1);

        // Wklejenie nowej wartości
        memcpy(pos, new_str, new_len);

        // Szukaj dalej, jeśli ten sam tag występuje kilka razy
        pos = strstr(pos + new_len, old_str);
    }
}


void ConfigurationWebServer::Initialise() {
    // start mDNS and check result
    if (!MDNS.begin("flightradar")) {
        Serial.println("[WARN] Failed to start mDNS. Continuing without mDNS...");
    }

    prefs.begin("config", false); // Otwieramy raz na starcie w trybie odczyt/zapis

    // Handle visit to config web server
    server.on("/", HTTP_GET, [&](AsyncWebServerRequest* request) {
        Serial.println("[GET] Handling request to config web server (PSRAM Chunked)...");

        // 1. Odczyt danych z Preferences
        double _lat = GetStoredDouble("latitude", 0.0);
        double _lon = GetStoredDouble("longitude", 0.0);
        int _radius = GetStoredInt("radius", 60);
        bool infochecked = GetStoredBool("infotext", true);

        // 2. Alokacja pamięci PSRAM
        // Dodajemy mały zapas na ewentualne wydłużenie stringa po podmianach
        size_t maxSize = sizeof(CONFIG_HTML) + 512; 
        char* localPsramBuf = (char*)heap_caps_malloc(maxSize, MALLOC_CAP_SPIRAM);
        
        // Zabezpieczenie przed brakiem pamięci PSRAM
        if (localPsramBuf == nullptr) {
            request->send(500, "text/plain", "Blad krytyczny: Brak pamieci PSRAM!");
            return;
        }
        
        // 3. Kopiujemy całą zawartość Flasha (PROGMEM) do bufora w PSRAM
        strcpy_P(localPsramBuf, CONFIG_HTML);

        // 4. Procesor - podmiana zmiennych
        char _buf[16] = {0};
        snprintf(_buf, sizeof(_buf), "%.6f", _lat); psram_replace(localPsramBuf, maxSize, "%LATITUDE%", _buf);
        snprintf(_buf, sizeof(_buf), "%.6f", _lon); psram_replace(localPsramBuf, maxSize, "%LONGITUDE%", _buf);
        snprintf(_buf, sizeof(_buf), "%d", _radius); psram_replace(localPsramBuf, maxSize, "%RADIUS%", _buf);
        psram_replace(localPsramBuf, maxSize, "%INFOTEXT%", infochecked ? "checked" : "");

        // 5. Sprawdzamy finalną długość i wysyłamy asynchronicznie (Chunked)
        size_t finalLen = strlen(localPsramBuf);
        
        PSRAMChunkedResponse *response = new PSRAMChunkedResponse(
            "text/html",
            [localPsramBuf, finalLen](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
                if (index >= finalLen) return 0;
                
                size_t toSend = (finalLen - index > maxLen) ? maxLen : finalLen - index;
                memcpy(buffer, localPsramBuf + index, toSend);
                
                return toSend;
            },
            localPsramBuf // Przekazujemy wskaźnik do zniszczenia przez destruktor
        );
        
        response->addHeader("Connection", "close");
        request->send(response);
    });

    // Handle save submission to web server
    server.on("/save", HTTP_POST, [&](AsyncWebServerRequest* request) {
        Serial.println("[POST] Handling form submission to config web server...");

        if (request->hasParam("latitude", true)) { double _lat = request->getParam("latitude", true)->value().toDouble(); SaveDouble("latitude", _lat); }
        if (request->hasParam("longitude", true)) { double _lon = request->getParam("longitude", true)->value().toDouble(); SaveDouble("longitude", _lon); }
        if (request->hasParam("radius", true)) { int _radius = request->getParam("radius", true)->value().toInt(); SaveInt("radius", _radius); }
        SaveBool("infotext", request->hasParam("infotext", true));

        request->send(200, "text/html", "Saved - restarting device...");
        g_restartNeeded = true;
        }
    );

    server.begin();
}




double ConfigurationWebServer::GetStoredDouble(const char* key, double defaultValue) {
    if (key == nullptr) return defaultValue;
    return prefs.getDouble(key, defaultValue);
}




int ConfigurationWebServer::GetStoredInt(const char* key, int defaultValue) {
    if (key == nullptr) return defaultValue;
    return prefs.getInt(key, defaultValue);
}



bool ConfigurationWebServer::GetStoredBool(const char* key, bool defaultValue) {
    bool result;
    result = prefs.getBool(key, defaultValue); 
    return result;
}




void ConfigurationWebServer::SaveDouble(const char* key, double value) {
    if (key == nullptr) return;
    prefs.putDouble(key, value);
}




void ConfigurationWebServer::SaveInt(const char* key, int value) {
    if (key == nullptr) return;
    prefs.putInt(key, value);
}



void ConfigurationWebServer::SaveBool(const char* key, bool value) {
    if (key == nullptr) return;
    prefs.putBool(key, value);
}
