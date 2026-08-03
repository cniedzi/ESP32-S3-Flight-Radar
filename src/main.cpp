#include <Arduino.h>
#include <WiFiManager.h>

#include "LGFX.h"
#include "WiFiManagerHelpers.h"
#include "ConfigurationWebServer.h"
#include "HttpRequestManager.h"
#include "AircraftManager.h"
#include "models/Aircraft.h"
#include "models/TrackedAircraft.h"
#include "Airports.h"

#define AIRPORT_COLOR 0x2bf8
#define HOME_LAT 52.010457
#define HOME_LON 20.537429

void drawHome(LGFX_Sprite& backbuffer);
void drawAirports(LGFX_Sprite& backbuffer);

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

  drawAirports(backbuffer);
  drawHome(backbuffer);

  backbuffer.setCursor(0, 80); backbuffer.setTextColor(TFT_RED); backbuffer.printf("Free heap: %d", ESP.getFreeHeap());
  backbuffer.setCursor(0, 90); backbuffer.setTextColor(TFT_RED); backbuffer.printf("Free PSRAM: %d", ESP.getFreePsram());
  
  backbuffer.pushSprite(0, -80);
  delay(10);
}


void drawHome(LGFX_Sprite& backbuffer)
{
    float home_lat = HOME_LAT;
    float home_lon = HOME_LON;

    auto [x, y] = aircraftManager.ProjectCoordinateToScreen(home_lat, home_lon);

    backbuffer.drawCircle(x, y, 3, 0xdae2);
    backbuffer.setTextColor(TFT_WHITE, 0xdae2);
    backbuffer.drawString(" Home ", x + 10, y - 10);
}


void drawAirports(LGFX_Sprite& backbuffer) {
    // Iterujemy po każdym lotnisku w tablicy
    for (const auto& airport : airportList) {
        
        // Przeliczenie współrzędnych z danego lotniska na pozycję (X, Y) na ekranie
        auto [x, y] = aircraftManager.ProjectCoordinateToScreen(airport.latitude, airport.longitude);

        // Rysowanie kropki (znacznika) lotniska
        backbuffer.drawCircle(x, y, 3, AIRPORT_COLOR);

        // Ustawienie koloru tekstu (biały) i tła (0x2bf8, żeby ładnie współgrało z kropką)
        backbuffer.setTextColor(TFT_WHITE, AIRPORT_COLOR);

        // Przygotowanie etykiety ze spacjami na obrzeżach (np. " EPWA ") 
        // Używamy bufora char zamiast obiektu String dla maksymalnej wydajności RAM!
        char label[9];
        snprintf(label, sizeof(label), " %s ", airport.icao);

        // Rysowanie nazwy obok kropki (przesunięcie w prawo i do góry)
        backbuffer.drawString(label, x + 10, y - 10);
    }
}