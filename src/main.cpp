#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

#include "LGFX.h"
#include "WiFiManagerHelpers.h"
#include "ConfigurationWebServer.h"
#include "HttpRequestManager.h"
#include "OpenSkyAuthTokenHandler.h"
#include "AircraftManager.h"
#include "DrawHelpers.h"
#include "models/Aircraft.h"
#include "models/TrackedAircraft.h"



#define EPWA_LAT 52.165970
#define EPWA_LON 20.966856

#define HOME_LAT 52.010457
#define HOME_LON 20.537429

void drawEPWA(LGFX_Sprite& backbuffer);
void drawHome(LGFX_Sprite& backbuffer);

constexpr int SCREEN_SIZE = 480;
constexpr int SCREEN_SIZE_DIV_2 = (SCREEN_SIZE / 2);

LGFX tft;
LGFX_Sprite backbuffer(&tft);

WiFiManager wm;
ConfigurationWebServer configServer;
HttpRequestManager http;
OpenSkyAuthTokenHandler authHandler(http);

AircraftManager aircraftManager(configServer, authHandler, http, tft);

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

  // establish WiFi connection
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("Connecting to WiFi...", SCREEN_SIZE / 2, SCREEN_SIZE / 2);

  WiFiManagerHelpers::ConfigureWiFiManager(wm, tft);

  wm.autoConnect(WiFiManagerHelpers::WiFiManagerName);

  // begin background server for configuration
  configServer.Initialise();

  // initialise aircraft manager
  aircraftManager.Initialise();
}

void loop()
{
  aircraftManager.Update();

  backbuffer.fillScreen(TFT_BLACK);

  aircraftManager.Draw(backbuffer);

  drawHome(backbuffer);
  drawEPWA(backbuffer);

  backbuffer.pushSprite(0, -80);
  delay(10);
}








void drawEPWA(LGFX_Sprite& backbuffer)
{
    
    float epwa_lat = EPWA_LAT;
    float epwa_lon = EPWA_LON;

    auto [x, y] = aircraftManager.ProjectCoordinateToScreen(epwa_lat, epwa_lon);

    backbuffer.fillCircle(x, y, 5, 0x141f);
    backbuffer.setTextColor(0x141f);
    backbuffer.drawString("EPWA", x + 10, y - 10);
}


void drawHome(LGFX_Sprite& backbuffer)
{
    
    float home_lat = HOME_LAT;
    float home_lon = HOME_LON;

    auto [x, y] = aircraftManager.ProjectCoordinateToScreen(home_lat, home_lon);

    backbuffer.fillCircle(x, y, 5, 0x141f);
    backbuffer.setTextColor(0x141f);
    backbuffer.drawString("Home", x + 10, y - 10);
}

