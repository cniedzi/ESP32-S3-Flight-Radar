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
#include "PolandMap.h"
#include "Home.h"


#define AIRPORT_COLOR 0x2bf8
#define HOME_LAT 52.010457
#define HOME_LON 20.537429

void drawHome(LGFX_Sprite& backbuffer);
void drawAirports(LGFX_Sprite& backbuffer);
void drawPolandMap(LGFX_Sprite& backbuffer, uint16_t color = TFT_DARKGREY);
void readSerialCommands();

bool g_restartNeeded = false;
bool g_zoomChangeActive = false;
unsigned long g_lastZoomChange = 0;


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
  backbuffer.setSwapBytes(tft.getSwapBytes());
  backbuffer.createSprite(SCREEN_SIZE, SCREEN_SIZE);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("Connecting to WiFi...", tft.width() / 2, tft.height() / 2);

  WiFiManagerHelpers::ConfigureWiFiManager(wm, tft);
  wm.autoConnect(WiFiManagerHelpers::WiFiManagerName);

  tft.fillScreen(TFT_BLACK);
  tft.drawCentreString("Initializing...", tft.width() / 2, tft.height() / 2);

  configServer.Initialise();
  aircraftManager.Initialise();
}

void loop()
{
  if (g_restartNeeded) {
    delay(500);
    ESP.restart();
  }
  
  readSerialCommands();

  if (g_zoomChangeActive && (millis() - g_lastZoomChange >= 1000)) g_zoomChangeActive = false;
  
  if (!g_zoomChangeActive) aircraftManager.Update();
  
  backbuffer.fillScreen(TFT_BLACK);
  aircraftManager.Draw(backbuffer);

  drawPolandMap(backbuffer);
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
    const uint8_t HOME_WIDTH = 18;
    const uint8_t HOME_HEIGHT = 15;

    auto [x, y] = aircraftManager.ProjectCoordinateToScreen(home_lat, home_lon);

   

    backbuffer.pushImage(x - HOME_WIDTH / 2, y - HOME_HEIGHT / 2 - 1 , HOME_WIDTH, HOME_HEIGHT, HOME, TFT_BLACK);

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






// Funkcja rysująca obrys na buforze (sprite) LovyanGFX
void drawPolandMap(LGFX_Sprite& backbuffer, uint16_t color) {
    int prevX = -1;
    int prevY = -1;
    const int polandPointsCount = sizeof(polandBorder) / sizeof(polandBorder[0]);

    for (int i = 0; i < polandPointsCount; ++i) {
        // Przeliczenie współrzędnych granicy na piksele za pomocą wbudowanej metody
        auto [x, y] = aircraftManager.ProjectCoordinateToScreen(polandBorder[i].lat, polandBorder[i].lon);

        if (i > 0) {
            // Rysowanie odcinka granicy między poprzednim a obecnym punktem
            backbuffer.drawLine(prevX, prevY, x, y, color);
        }

        prevX = x;
        prevY = y;
    }
}





void readSerialCommands() {
    // Sprawdzamy, czy w buforze Serial są jakieś nieprzeczytane dane
    while (Serial.available() > 0) {
        // Odczytujemy pojedynczy znak
        char incomingChar = Serial.read();

        // Reagujemy w zależności od tego, jaki to znak
        switch (incomingChar) {
            case '=':
            case '+':
                //Serial.println("Otrzymano PLUS (+)");
                aircraftManager.setRad(aircraftManager.getRad() + 10);
                g_lastZoomChange = millis();
                g_zoomChangeActive = true;
                break;
                
            case '-':
                //Serial.println("Otrzymano MINUS (-)");
                aircraftManager.setRad(aircraftManager.getRad() - 10);
                g_lastZoomChange = millis();
                g_zoomChangeActive = true;
                break;
                
            default:
                // Ignorujemy wszelkie inne znaki (litery, spacje, entery)
                break;
        }
    }
}