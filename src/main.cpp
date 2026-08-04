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
#include "driver/touch_pad.h"
#include <atomic>


#define TOUCH_THRESHOLD 100000
#define TOUCH_DEBOUNCE_MS 300
#define TOUCH_ZOOM_IN TOUCH_PAD_NUM4
#define TOUCH_ZOOM_OUT TOUCH_PAD_NUM5
#define AIRPORT_COLOR 0x2bf8
#define HOME_LAT 52.010457
#define HOME_LON 20.537429
#define SCREEN_SIZE 480
#define SCREEN_SIZE_DIV_2 SCREEN_SIZE / 2


void drawHome(LGFX_Sprite& backbuffer);
void drawAirports(LGFX_Sprite& backbuffer);
void drawPolandMap(LGFX_Sprite& backbuffer, uint16_t color = TFT_DARKGREY);
void readSerialCommands();
void commandZoomIn();
void commandZoomOut();
void touchTask(void *pvParameters);
void aircraftsUpdateTask(void *pvParameters);


bool g_restartNeeded = false;
unsigned long g_lastZoomChange = 0;
std::atomic<bool> g_zoomChangeActive{false};
std::atomic<bool> g_requestZoomIn{false};
std::atomic<bool> g_requestZoomOut{false};


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

  touch_pad_init();
  touch_pad_config(TOUCH_ZOOM_IN);
  touch_pad_config(TOUCH_ZOOM_OUT);
  touch_filter_config_t filter_info = {
      .mode = TOUCH_PAD_FILTER_IIR_16, // Mocny filtr kroczący
      .debounce_cnt = 1,
      .noise_thr = 0,
      .jitter_step = 4,
      .smh_lvl = TOUCH_PAD_SMOOTH_IIR_2
  };
  touch_pad_filter_set_config(&filter_info);
  touch_pad_filter_enable();
  touch_pad_fsm_start(); //Uruchomienie ciągłego pomiaru w tle



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

  xTaskCreatePinnedToCore(
      touchTask,                // Nazwa naszej funkcji
      "TouchTask",              // Nazwa systemowa do debugowania
      2048,                     // Pamięć RAM dla zadania (2KB wystarczy z zapasem)
      NULL,                     // Brak parametrów startowych
      1,                        // Priorytet (1 = niski)
      NULL,                     // Uchwyt zadania (nie potrzebujemy)
      0                         // Odpalamy na Rdzeniu 0 (odciążamy główny Rdzeń 1)
  );

  xTaskCreatePinnedToCore(
      aircraftsUpdateTask,      // Nazwa funkcji
      "AircraftsUpdateTask",    // Nazwa systemowa
      8192,                     // Sieć i JSON zżerają sporo pamięci! Dajmy tu solidne 8KB
      NULL,
      1,                        // Niski priorytet
      NULL,
      0                         // Odpalamy na Rdzeniu 0 (obok zadania Touch)
  );

}




void loop()
{
  if (g_restartNeeded) {
    delay(500);
    ESP.restart();
  }
  
  readSerialCommands();

  if (g_requestZoomIn) {
      g_requestZoomIn = false; // Natychmiastowe zrzucenie flagi
      commandZoomIn();
  }
  else if (g_requestZoomOut) {
      g_requestZoomOut = false; // Natychmiastowe zrzucenie flagi
      commandZoomOut();
  }

  if (g_zoomChangeActive && (millis() - g_lastZoomChange >= 1000)) g_zoomChangeActive = false;
  
  backbuffer.fillScreen(TFT_BLACK);
  drawPolandMap(backbuffer);

  aircraftManager.Draw(backbuffer);
  
  drawAirports(backbuffer);
  drawHome(backbuffer);

  backbuffer.setCursor(0, 80); backbuffer.setTextColor(TFT_RED); backbuffer.printf("Free heap: %d", ESP.getFreeHeap());
  backbuffer.setCursor(0, 90); backbuffer.setTextColor(TFT_RED); backbuffer.printf("Free PSRAM: %d", ESP.getFreePsram());
  backbuffer.setCursor(0, 100); backbuffer.setTextColor(TFT_RED); backbuffer.printf("RSSI: %d", WiFi.RSSI());
  
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
        // Przeliczenie współrzędnych granicy na piksele ekranu
        auto [x, y] = aircraftManager.ProjectCoordinateToScreen(polandBorder[i].lat, polandBorder[i].lon);
        if (i > 0) {
            backbuffer.drawLine(prevX, prevY, x, y, color); // Rysowanie odcinka granicy między poprzednim a obecnym punktem
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
                commandZoomOut();
                break;
                
            case '-':
                //Serial.println("Otrzymano MINUS (-)");
                commandZoomIn();
                break;
                
            default:
                // Ignorujemy wszelkie inne znaki (litery, spacje, entery)
                break;
        }
    }
}



void commandZoomIn() {
  aircraftManager.setRad(aircraftManager.getRad() - 10);
  g_lastZoomChange = millis();
  g_zoomChangeActive = true;    
}



void commandZoomOut() {
  aircraftManager.setRad(aircraftManager.getRad() + 10);
  g_lastZoomChange = millis();
  g_zoomChangeActive = true;    
}



void touchTask(void *pvParameters) {
    // Te zmienne żyją teraz lokalnie, tylko wewnątrz tego wątku
    unsigned long localLastTouchTime = 0;
    
    // Nieskończona pętla zadania RTOS
    for(;;) {
        if (millis() - localLastTouchTime >= TOUCH_DEBOUNCE_MS) {
            uint32_t touchInValue = 0;
            uint32_t touchOutValue = 0;
            touch_pad_filter_read_smooth(TOUCH_ZOOM_IN, &touchInValue);
            touch_pad_filter_read_smooth(TOUCH_ZOOM_OUT, &touchOutValue);
            if (touchInValue >= TOUCH_THRESHOLD) {
                g_requestZoomIn = true;
                localLastTouchTime = millis();
            }
            else if (touchOutValue >= TOUCH_THRESHOLD) {
                g_requestZoomOut = true;
                localLastTouchTime = millis();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}



void aircraftsUpdateTask(void *pvParameters) {
    for(;;) {
        // Zabezpieczenie przed pobieraniem danych zaraz po kliknięciu Zoom
        if (!g_zoomChangeActive) {
            // Wewnątrz Update() samoloty będą już zabezpieczone Mutexem!
            aircraftManager.Update(); 
        }
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}