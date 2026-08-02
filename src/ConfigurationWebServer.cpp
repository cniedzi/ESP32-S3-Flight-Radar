#include "ConfigurationWebServer.h"
#include <ESPmDNS.h>

// HTML stored in flash
static const char CONFIG_HTML[] PROGMEM = R"(
<html>
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Configure Micro Radar</title>
        <script src="https://cdn.tailwindcss.com"></script>
    </head>
    <body class="font-mono bg-gray-900 text-green-500 min-h-screen p-4 sm:p-0 text-md sm:text-sm">
        <fieldset class="border border-green-500 p-5 w-full max-w-2xl mx-auto sm:m-10">
            <legend class="px-2">Configure Micro Radar</legend>

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
                        max="200"
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
                        class="bg-green-500 text-black mt-4 px-4 py-3 text-lg sm:text-base sm:px-2 sm:py-0 self-start cursor-pointer hover:bg-green-400">

                    <div id="result" class="mt-4 px-1 sm:px-10 text-yellow-400"></div>
                </div>
            </form>
        </fieldset>

        <script>
            document.getElementById('cfg').addEventListener('submit', function(e) {
                e.preventDefault();
                fetch(this.action, { method: 'POST', body: new FormData(this) })
                    .then(r => r.text())
                    .then(html => {
                        const res = document.getElementById('result');
                        res.innerHTML = html;
                        setTimeout(() => res.innerHTML = '', 3000);
                    });
            });
        </script>
    </body>
</html>
)";




void ConfigurationWebServer::Initialise() {
    // start mDNS and check result
    if (!MDNS.begin("microradar")) {
        Serial.println("[WARN] Failed to start mDNS. Continuing without mDNS...");
    }

    // Handle visit to config web server
    server.on("/", HTTP_GET, [&](AsyncWebServerRequest* request) {
        Serial.println("[GET] Handling request to config web server...");

        // read all values up front so the processor lambda can capture by value
        prefs.begin("config", true);
        const String latitude = prefs.getString("latitude", "");
        const String longitude = prefs.getString("longitude", "");
        const String radius = prefs.getString("radius", "60");
        const String infoTextEnabled = prefs.getString("infotext", "true");
        prefs.end();

        AsyncWebServerResponse* response = request->beginResponse(
            200, "text/html",
            (const uint8_t*)CONFIG_HTML, sizeof(CONFIG_HTML) - 1,
            [latitude, longitude, radius, infoTextEnabled]
            (const String& var) -> String {
                if (var == "LATITUDE")       return latitude;
                if (var == "LONGITUDE")      return longitude;
                if (var == "RADIUS")         return radius;
                if (var == "INFOTEXT")       return infoTextEnabled == "true" ? "checked" : "";
                return "";
            }
        );
        request->send(response);
        }
    );

    // Handle save submission to web server
    server.on("/save", HTTP_POST, [&](AsyncWebServerRequest* request) {
        Serial.println("[POST] Handling form submission to config web server...");

        // safe parameter retrieval helper lambda
        auto TrySaveParam = [request, this](const char* paramName) {
            const auto* param = request->getParam(paramName, true);
            if (param == nullptr)
                return false;

            prefs.putString(paramName, param->value());
            return true;
            };

        prefs.begin("config", false);

        TrySaveParam("latitude");
        TrySaveParam("longitude");
        TrySaveParam("radius");

        prefs.putString("infotext", request->hasParam("infotext", true) ? "true" : "false");
        prefs.end();

        request->send(200, "text/html", "Saved - restarting device...");
        ESP.restart();
        }
    );

    server.begin();
}

const String ConfigurationWebServer::GetStoredString(const char* key)
{
    prefs.begin("config", true);
    const String value = prefs.getString(key, "");
    prefs.end();
    return value;
}