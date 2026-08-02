#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

#include "LGFX.h"
#include "WiFiManagerHelpers.h"
#include "ConfigurationWebServer.h"
#include "HttpRequestManager.h"
#include "AircraftManager.h"
#include "models/Aircraft.h"
#include "models/TrackedAircraft.h"

#define EPWA_LAT 52.165970
#define EPWA_LON 20.966856
#define EPMO_LAT 52.451175
#define EPMO_LON 20.651959


#define HOME_LAT 52.010457
#define HOME_LON 20.537429

void drawEPWA(LGFX_Sprite& backbuffer);
void drawEPMO(LGFX_Sprite& backbuffer);
void drawHome(LGFX_Sprite& backbuffer);

bool g_restartNeeded = false;

constexpr int SCREEN_SIZE = 480;
constexpr int SCREEN_SIZE_DIV_2 = (SCREEN_SIZE / 2);

LGFX tft;
LGFX_Sprite backbuffer(&tft);

WiFiManager wm;
ConfigurationWebServer configServer;
HttpRequestManager http;

AircraftManager aircraftManager(configServer, http, tft);

void setup()
{
  Serial.begin(115200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setSwapBytes(true);
  tft.setRotation(1);

  backbuffer.setPsram(true);
  backbuffer.setColorDepth(8);
  backbuffer.createSprite(SCREEN_SIZE, SCREEN_SIZE);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("Connecting to WiFi...", tft.width() / 2, tft.height() / 2);

  WiFiManagerHelpers::ConfigureWiFiManager(wm, tft);
  wm.autoConnect(WiFiManagerHelpers::WiFiManagerName);

  configServer.Initialise();
  aircraftManager.Initialise();
}

void loop()
{
  if (g_restartNeeded) {
    delay(500);
    ESP.restart();
  }
  
  aircraftManager.Update();
  backbuffer.fillScreen(TFT_BLACK);
  aircraftManager.Draw(backbuffer);

  drawHome(backbuffer);
  drawEPWA(backbuffer);
  drawEPMO(backbuffer);

  backbuffer.setCursor(0, 80); backbuffer.setTextColor(TFT_RED); backbuffer.printf("Free heap: %d", ESP.getFreeHeap());
  backbuffer.setCursor(0, 90); backbuffer.setTextColor(TFT_RED); backbuffer.printf("Free PSRAM: %d", ESP.getFreePsram());
  
  backbuffer.pushSprite(0, -80);
  delay(10);
}

void drawEPWA(LGFX_Sprite& backbuffer)
{
    float epwa_lat = EPWA_LAT;
    float epwa_lon = EPWA_LON;

    auto [x, y] = aircraftManager.ProjectCoordinateToScreen(epwa_lat, epwa_lon);

    backbuffer.fillCircle(x, y, 3, 0x2bf8);
    backbuffer.setTextColor(TFT_WHITE, 0x2bf8);
    backbuffer.drawString(" EPWA ", x + 10, y - 10);
}

void drawEPMO(LGFX_Sprite& backbuffer)
{
    float epwa_lat = EPMO_LAT;
    float epwa_lon = EPMO_LON;

    auto [x, y] = aircraftManager.ProjectCoordinateToScreen(epwa_lat, epwa_lon);

    backbuffer.fillCircle(x, y, 3, 0x2bf8);
    backbuffer.setTextColor(TFT_WHITE, 0x2bf8);
    backbuffer.drawString(" EPMO ", x + 10, y - 10);
}

void drawHome(LGFX_Sprite& backbuffer)
{
    float home_lat = HOME_LAT;
    float home_lon = HOME_LON;

    auto [x, y] = aircraftManager.ProjectCoordinateToScreen(home_lat, home_lon);

    backbuffer.fillCircle(x, y, 3, 0x2bf8);
    backbuffer.setTextColor(TFT_WHITE, 0x2bf8);
    backbuffer.drawString(" Home ", x + 10, y - 10);
}