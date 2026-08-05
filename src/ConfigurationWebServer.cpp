#include "ConfigurationWebServer.h"
#include <ESPmDNS.h>

// HTML stored in flash
static const char CONFIG_HTML[] = R"rawliteral(
<html>
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>ESP32 Flightradar</title>
        <script src="https://cdn.jsdelivr.net/npm/@tailwindcss/browser@4"></script>
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
                        min="10"
                        step="10"
                        max="250"
                        value='%RADIUS%'
                        class="flex-1 border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                </label>

                <!-- UI Options Checkboxes -->
                <div class="grid grid-cols-1 sm:grid-cols-2 gap-3 pt-4 border-t border-green-800">
                    <label class="flex items-center gap-2 cursor-pointer">
                        <input name="infotext" type="checkbox" %INFOTEXT% class="w-4 h-4 accent-green-500">
                        <span>Show aircrafts information</span>
                    </label>

                    <label class="flex items-center gap-2 cursor-pointer">
                        <input name="meminfo" type="checkbox" %MEMINFO% class="w-4 h-4 accent-green-500">
                        <span>Show memory information</span>
                    </label>

                    <label class="flex items-center gap-2 cursor-pointer">
                        <input name="rssiinfo" type="checkbox" %RSSIINFO% class="w-4 h-4 accent-green-500">
                        <span>Show Wifi RSSI</span>
                    </label>

                    <label class="flex items-center gap-2 cursor-pointer">
                        <input name="rangeinfo" type="checkbox" %RANGEINFO% class="w-4 h-4 accent-green-500">
                        <span>Show range</span>
                    </label>

                    <label class="flex items-center gap-2 cursor-pointer">
                        <input name="updateinfo" type="checkbox" %UPDATEINFO% class="w-4 h-4 accent-green-500">
                        <span>Show aircrafts update indicator</span>
                    </label>
                </div>

                <div class="flex flex-col sm:flex-row gap-4 sm:gap-5 pt-2">
                    <input
                        type="submit"
                        value="Save"
                        class="bg-green-500 text-black mt-4 px-4 py-3 text-lg sm:text-base sm:px-2 sm:py-0 self-start cursor-pointer font-bold hover:bg-green-400">

                    <div id="result" class="mt-4 px-1 sm:px-10 font-bold text-yellow-400"></div>
                </div>
            </form>
        </fieldset>

        <script>
            document.getElementById('cfg').addEventListener('submit', function(e) {
                e.preventDefault();
                const encodedData = new URLSearchParams(new FormData(this));
                
                fetch(this.action, { 
                    method: 'POST', 
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded',
                    },
                    body: encodedData 
                })
                .then(r => {
                    const resDiv = document.getElementById('result');
                    resDiv.innerHTML = "Saved...";
                    setTimeout(() => {
                        resDiv.innerHTML = "";
                    }, 1000);
                });
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

    // Handle visit to config web server
    server.on("/", HTTP_GET, [&](AsyncWebServerRequest* request) {
        Serial.println("[GET] Handling request to config web server (PSRAM Chunked)...");

        // Alokacja pamięci PSRAM
        // Dodajemy mały zapas na ewentualne wydłużenie stringa po podmianach
        size_t maxSize = sizeof(CONFIG_HTML) + 1024; 
        char* localPsramBuf = (char*)heap_caps_malloc(maxSize, MALLOC_CAP_SPIRAM);
        
        // Zabezpieczenie przed brakiem pamięci PSRAM
        if (localPsramBuf == nullptr) {
            request->send(500, "text/plain", "Blad krytyczny: Brak pamieci PSRAM!");
            return;
        }
        
        // Kopiujemy całą zawartość Flasha (PROGMEM) do bufora w PSRAM
        strcpy_P(localPsramBuf, CONFIG_HTML);

        // Procesor - podmiana zmiennych
        char _buf[16] = {0};
        snprintf(_buf, sizeof(_buf), "%.6f", settings.GetLatitude()); psram_replace(localPsramBuf, maxSize, "%LATITUDE%", _buf);
        snprintf(_buf, sizeof(_buf), "%.6f", settings.GetLongitude()); psram_replace(localPsramBuf, maxSize, "%LONGITUDE%", _buf);
        snprintf(_buf, sizeof(_buf), "%d", settings.GetRadius()); psram_replace(localPsramBuf, maxSize, "%RADIUS%", _buf);
        psram_replace(localPsramBuf, maxSize, "%INFOTEXT%",   settings.GetInfoTextVisible() ? "checked" : "");
        psram_replace(localPsramBuf, maxSize, "%MEMINFO%",    settings.GetDisplayMemoryInfo() ? "checked" : "");
        psram_replace(localPsramBuf, maxSize, "%RSSIINFO%",   settings.GetDisplayRSSI() ? "checked" : "");
        psram_replace(localPsramBuf, maxSize, "%RANGEINFO%",  settings.GetDisplayRange() ? "checked" : "");
        psram_replace(localPsramBuf, maxSize, "%UPDATEINFO%", settings.GetDisplayAircraftsUpdateIndicator() ? "checked" : "");

        // Sprawdzamy finalną długość i wysyłamy asynchronicznie (Chunked)
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

        if (request->hasParam("latitude", true)) { settings.SetLatitude(request->getParam("latitude", true)->value().toDouble()); aircraftmanager.ForceUpdate(); }
        if (request->hasParam("longitude", true)) { settings.SetLongitude(request->getParam("longitude", true)->value().toDouble()); aircraftmanager.ForceUpdate(); }
        if (request->hasParam("radius", true)) { settings.SetRadius(request->getParam("radius", true)->value().toInt()); aircraftmanager.ForceUpdate(); }
        settings.SetInfoTextVisible(request->hasParam("infotext", true));
        settings.SetDisplayMemoryInfo(request->hasParam("meminfo", true));
        settings.SetDisplayRSSI(request->hasParam("rssiinfo", true));
        settings.SetDisplayRange(request->hasParam("rangeinfo", true));
        settings.SetDisplayAircraftsUpdateIndicator(request->hasParam("updateinfo", true));

        request->send(200, "text/html", "");
    });

    server.begin();
}
