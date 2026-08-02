#pragma once

#include <WiFiManager.h>

#define CONNECTION_TIMEOUT 5

namespace WiFiManagerHelpers
{
    constexpr const char* WiFiManagerName = "Flightradar-Setup";

    static void ConfigureWiFiManager(WiFiManager& wm, LGFX& tft)
    {
        wm.setTitle("ESP32 Flightradar - Setup WiFi");
        wm.setCustomHeadElement("<style>body{background:#111;color:#00ff00;font-family:monospace;} div:has(> a){background:#00ff00;} a:hover{color:#111;}</style>");
        wm.setConnectTimeout(CONNECTION_TIMEOUT);
        wm.setCleanConnect(true);

        wm.setAPCallback([&tft](WiFiManager* wifiManager) {
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_WHITE);

            const int lineHeight = tft.fontHeight() + 10;
            tft.drawCenterString("- SETUP -", tft.width() / 2, tft.height() / 2 - lineHeight);
            tft.drawCenterString("Connect to this WiFi hotspot:", tft.width() / 2, tft.height() / 2);
            tft.drawCenterString(WiFiManagerName, tft.width() / 2, tft.height() / 2 + lineHeight);
            }
        );


    }
}