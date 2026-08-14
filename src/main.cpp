//  FLIGHTRADAR ESP32
//  C. Niedziński 2026

#include <Arduino.h>
#include <WiFiManager.h>
#include <driver/touch_sens.h>
#include <atomic>
#include "LGFX.h"
#include "ConfigurationWebServer.h"
#include "HttpRequestManager.h"
#include "AircraftManager.h"
#include "models/Aircraft.h"
#include "models/TrackedAircraft.h"
#include "Airports.h"
#include "PolandMap.h"
#include "Home.h"
#include "SettingsManager.h"
#include "Config.h"


#define WAITING_FOR_WIFI_TIME 5000 //ms
#define TOUCH_THRESHOLD 4000
#define TOUCH_DEBOUNCE_MS 300
#define TOUCH_ZOOM_IN 4
#define TOUCH_ZOOM_OUT 5
#define AIRPORT_COLOR 0x2bf8
#define CONFIG_PORTAL_TIMEOUT 180 //s


void displayUIElements(LGFX_Sprite& radarSprite);
void drawHome(LGFX_Sprite& radarSprite);
void drawAirports(LGFX_Sprite& radarSprite);
void drawPolandMap(LGFX_Sprite& radarSprite, uint16_t color = TFT_DARKGREY);
void readSerialCommands();
void commandZoomIn();
void commandZoomOut();
void aircraftsUpdateTask(void *pvParameters);
void configureTouchSensor();
void touchTask(void *pvParameters);
bool isTouchedOnStartup();
char* separatorTysiecy_c(char* bufNum, uint32_t n);


unsigned long g_lastZoomChange = 0;
std::atomic<bool> g_zoomChangeActive{false};
std::atomic<bool> g_requestZoomIn{false};
std::atomic<bool> g_requestZoomOut{false};

touch_sensor_handle_t g_touchSensHandle = NULL;
touch_channel_handle_t g_touchChanZoomIn = NULL;
touch_channel_handle_t g_touchChanZoomOut = NULL;

LGFX tft;
LGFX_Sprite radarSprite(&tft);
HttpRequestManager http;
OpenSkyAuthTokenHandler authHandler(http);
SettingsManager settingsManager;
AircraftManager aircraftManager(settingsManager, http, authHandler, tft);
ConfigurationWebServer configServer(settingsManager, aircraftManager);




void setup()
{
  Serial.begin(115200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  configureTouchSensor();

  WiFi.begin();
  WiFiManager wm;

  tft.init();
  tft.setSwapBytes(true);
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);

  if (isTouchedOnStartup()) {
    constexpr const char* apName = "Flightradar-Setup";
    const int lineHeight = tft.fontHeight() + 10;
    tft.drawCenterString("- SETUP -", tft.width() / 2, tft.height() / 2 - lineHeight);
    tft.drawCenterString("Connect to this WiFi hotspot:", tft.width() / 2, tft.height() / 2);
    tft.drawCenterString(apName, tft.width() / 2, tft.height() / 2 + lineHeight);
    wm.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
    wm.startConfigPortal(apName);
    ESP.restart();
  }

  tft.drawCentreString("Connecting to WiFi...", tft.width() / 2, tft.height() / 2);

  radarSprite.setPsram(true);
  radarSprite.setColorDepth(8);
  radarSprite.setSwapBytes(tft.getSwapBytes());
  radarSprite.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  
  unsigned long waitingForWiFiStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - waitingForWiFiStart > WAITING_FOR_WIFI_TIME) {
      ESP.restart();
    }
    delay(10);
  }

  tft.fillScreen(TFT_BLACK);
  tft.drawCentreString("Initializing...", tft.width() / 2, tft.height() / 2);

  settingsManager.Initialise();
  configServer.Initialise();
  aircraftManager.Initialise();

  xTaskCreatePinnedToCore(
      touchTask,
      "TouchTask",
      2048,
      NULL,
      1,
      NULL,
      0
  );

  xTaskCreatePinnedToCore(
      aircraftsUpdateTask,
      "AircraftsUpdateTask",
      8192,
      NULL,
      1,
      NULL,
      0
  );

}



void loop()
{
  readSerialCommands();

  if (g_requestZoomIn) {
      g_requestZoomIn = false;
      commandZoomIn();
  }
  else if (g_requestZoomOut) {
      g_requestZoomOut = false;
      commandZoomOut();
  }

  if (g_zoomChangeActive && (millis() - g_lastZoomChange >= 1000)) {
    g_zoomChangeActive = false;
    aircraftManager.ForceUpdate();
  }
  
  radarSprite.fillScreen(TFT_BLACK);
  drawPolandMap(radarSprite);
  drawAirports(radarSprite);
  drawHome(radarSprite);
  aircraftManager.Draw(radarSprite);
  displayUIElements(radarSprite);
  radarSprite.pushSprite(0, 0);
  delay(10);

}



void displayUIElements(LGFX_Sprite& radarSprite) {
    
    // Aircraft data platform
    if (settingsManager.GetDisplayPlatform()) {
        int x = 360;
        int y = 0;
        int16_t color = 0xa2e3;
        char text[10];
        if (settingsManager.GetPlatform() == FlightPlatform::ADSB_LOL) strlcpy(text, "ADSB.lol", sizeof(text));
        else {
            strlcpy(text, "OpenSky", sizeof(text));
            if (aircraftManager.isOpenSkyAuthenticated.load()) color = 0x0315;
            else {
                if (!aircraftManager.OpenSkyQuotaExceeded) color = 0xf3c4;
                else {
                    color = TFT_RED;
                }
            }
        }
        radarSprite.setTextColor(TFT_WHITE);
        int length = radarSprite.textWidth(text) + 11;
        radarSprite.fillRoundRect(x - length / 2, y, length, radarSprite.fontHeight() + 3, 4, color);
        radarSprite.setTextDatum(middle_center);
        radarSprite.drawString(text, x + 1, y + radarSprite.fontHeight() / 2 + 2);
        if (settingsManager.GetPlatform() == FlightPlatform::OpenSky && !aircraftManager.isOpenSkyAuthenticated.load()) {
            if (aircraftManager.OpenSkyQuotaExceeded) {
                radarSprite.fillCircle(x + length / 2 + 10, y + radarSprite.fontHeight() / 2 + 1, radarSprite.fontHeight() / 2 + 1, TFT_RED);
                radarSprite.drawString("!", x + length / 2 + 11, y + 6);
            }
        }
    }

    // Memory parameters
    int currentY = 0;
    if (settingsManager.GetDisplayMemoryInfo()) {
        char buf[32];
        const uint8_t lineHeight = radarSprite.fontHeight() + 3;
        radarSprite.setTextDatum(top_left);
        radarSprite.setCursor(0, currentY); radarSprite.setTextColor(TFT_ORANGE); radarSprite.printf("Free heap: %sB", separatorTysiecy_c(buf, ESP.getFreeHeap())); currentY += lineHeight;
        radarSprite.setCursor(0, currentY); radarSprite.setTextColor(TFT_ORANGE); radarSprite.printf("Free PSRAM: %sB", separatorTysiecy_c(buf, ESP.getFreePsram())); currentY += lineHeight;
        
    }

    // WiFi RSSI
    if (settingsManager.GetDisplayRSSI()) {
        radarSprite.setTextDatum(top_left);
        radarSprite.setCursor(0, currentY);
        radarSprite.setTextColor(TFT_ORANGE);radarSprite.printf("RSSI: %ddBm", WiFi.RSSI());
    }

    // Range
    if (settingsManager.GetDisplayRange()) {
        int x = 240;
        int y = 0;
        char rangeText[15];
        snprintf(rangeText, sizeof(rangeText), "Range %dnm", settingsManager.GetRange());
        radarSprite.setTextColor(TFT_WHITE);
        int length = radarSprite.textWidth(rangeText) + 11;
        radarSprite.fillRoundRect(x - length / 2, y, length, radarSprite.fontHeight() + 3, 4, TFT_MAGENTA);
        radarSprite.setTextDatum(middle_center);
        radarSprite.drawString(rangeText, x + 1, y + radarSprite.fontHeight() / 2 + 2);
    }

    // Update remaining time
    if (settingsManager.GetDisplayAircraftsUpdateTimeRemaining() && settingsManager.GetDisplayAircraftsUpdateIndicator()) {
        int x = DISPLAY_WIDTH;
        int y = 0;
        char buf[15];
        int updateTimeRemaining = aircraftManager.getSecondsUntilNextUpdate();
        if (!aircraftManager.isFetching.load() && updateTimeRemaining > 0) {
            snprintf(buf, sizeof(buf), "%ds", updateTimeRemaining);
            radarSprite.setTextColor(TFT_DARKGREY, TFT_BLACK);
            radarSprite.setTextDatum(middle_right);
            radarSprite.drawString(buf, x + 1, y + radarSprite.fontHeight() / 2 + 2);
        }
    }

    // Update indicator
    if (settingsManager.GetDisplayAircraftsUpdateIndicator()) {
        if (aircraftManager.isFetching.load()) radarSprite.fillCircle(DISPLAY_WIDTH - 7, radarSprite.fontHeight() / 2 + 2, 5, tft.color565(6, 85, 150));
    }

}



void drawHome(LGFX_Sprite& radarSprite)
{
    const uint8_t HOME_WIDTH = 18;  
    const uint8_t HOME_HEIGHT = 15;
    auto [x, y] = aircraftManager.ProjectCoordinateToScreen(settingsManager.GetLatitude(), settingsManager.GetLongitude());
    radarSprite.pushImage(x - HOME_WIDTH / 2, y - HOME_HEIGHT / 2 - 1 , HOME_WIDTH, HOME_HEIGHT, HOME, TFT_BLACK);
}



void drawAirports(LGFX_Sprite& radarSprite) {
    for (const auto& airport : airportList) {
        auto [x, y] = aircraftManager.ProjectCoordinateToScreen(airport.latitude, airport.longitude);
        
        radarSprite.drawCircle(x, y, 3, AIRPORT_COLOR);
        radarSprite.setTextColor(TFT_WHITE, AIRPORT_COLOR);
        uint8_t text_dx = 5;
        uint8_t text_dy = 15;
        int length = radarSprite.textWidth(airport.icao) + 5;
        radarSprite.fillRoundRect(x + text_dx, y - text_dy, length, radarSprite.fontHeight() + 3, 2, AIRPORT_COLOR);
        radarSprite.setTextDatum(middle_center);
        radarSprite.drawString(airport.icao, x + text_dx + length / 2 + 1, y - text_dy + radarSprite.fontHeight() / 2 + 2);

    }
}



// Funkcja rysująca obrys na buforze (sprite) LovyanGFX
void drawPolandMap(LGFX_Sprite& radarSprite, uint16_t color) {
    int prevX = -1;
    int prevY = -1;
    const int polandPointsCount = sizeof(polandBorder) / sizeof(polandBorder[0]);

    for (int i = 0; i < polandPointsCount; ++i) {
        auto [x, y] = aircraftManager.ProjectCoordinateToScreen(polandBorder[i].lat, polandBorder[i].lon);
        if (i > 0) {
            radarSprite.drawLine(prevX, prevY, x, y, color);
        }
        prevX = x;
        prevY = y;
    }
}



void readSerialCommands() {
    while (Serial.available() > 0) {
        char incomingChar = Serial.read();

        switch (incomingChar) {
            case '=':
            case '+':
                commandZoomOut();
                break;
                
            case '-':
                commandZoomIn();
                break;
                
            default:
                break;
        }
    }
}



void commandZoomIn() {
  aircraftManager.setRange(settingsManager.GetRange() - 10);
  g_lastZoomChange = millis();
  g_zoomChangeActive = true;    
}



void commandZoomOut() {
  aircraftManager.setRange(settingsManager.GetRange() + 10);
  g_lastZoomChange = millis();
  g_zoomChangeActive = true;    
}



void aircraftsUpdateTask(void *pvParameters) {
    for(;;) {
        if (!g_zoomChangeActive) {
            aircraftManager.Update();
        }
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}



void configureTouchSensor() {
    touch_sensor_sample_config_t touchSampleCfg = TOUCH_SENSOR_V2_DEFAULT_SAMPLE_CONFIG(16, TOUCH_VOLT_LIM_L_0V5, TOUCH_VOLT_LIM_H_2V7);
    touch_sensor_config_t touchSensCfg = TOUCH_SENSOR_DEFAULT_BASIC_CONFIG(1, &touchSampleCfg);
    ESP_ERROR_CHECK(touch_sensor_new_controller(&touchSensCfg, &g_touchSensHandle));

    touch_channel_config_t touchChanCfg = {
      .active_thresh = {500},
      .charge_speed = TOUCH_CHARGE_SPEED_7,
      .init_charge_volt = TOUCH_INIT_CHARGE_VOLT_DEFAULT,
    };
    ESP_ERROR_CHECK(touch_sensor_new_channel(g_touchSensHandle, TOUCH_ZOOM_IN, &touchChanCfg, &g_touchChanZoomIn));
    ESP_ERROR_CHECK(touch_sensor_new_channel(g_touchSensHandle, TOUCH_ZOOM_OUT, &touchChanCfg, &g_touchChanZoomOut));

    touch_sensor_filter_config_t touchFilterCfg = {
        .benchmark = {
            .filter_mode = TOUCH_BM_IIR_FILTER_16,
            .jitter_step = 4,
            .denoise_lvl = 0,
        },
        .data = {
            .smooth_filter = TOUCH_SMOOTH_IIR_FILTER_2,
            .active_hysteresis = 0,
            .debounce_cnt = 1,
        },
    };
    ESP_ERROR_CHECK(touch_sensor_config_filter(g_touchSensHandle, &touchFilterCfg));

    ESP_ERROR_CHECK(touch_sensor_enable(g_touchSensHandle));
    ESP_ERROR_CHECK(touch_sensor_start_continuous_scanning(g_touchSensHandle));
    delay(50);
}



void touchTask(void *pvParameters) {
    unsigned long localLastTouchTime = 0;
    
    for(;;) {
        if (millis() - localLastTouchTime >= TOUCH_DEBOUNCE_MS) {
            uint32_t touchInValue = 0;
            uint32_t touchOutValue = 0;
            touch_channel_read_data(g_touchChanZoomIn, TOUCH_CHAN_DATA_TYPE_SMOOTH, &touchInValue);
            touch_channel_read_data(g_touchChanZoomOut, TOUCH_CHAN_DATA_TYPE_SMOOTH, &touchOutValue);
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



bool isTouchedOnStartup() {
    bool result = false;
    uint32_t touchInValue = 0;
    uint32_t touchOutValue = 0;
    touch_channel_read_data(g_touchChanZoomIn, TOUCH_CHAN_DATA_TYPE_SMOOTH, &touchInValue);
    touch_channel_read_data(g_touchChanZoomOut, TOUCH_CHAN_DATA_TYPE_SMOOTH, &touchOutValue);
    if (touchInValue >= TOUCH_THRESHOLD || touchOutValue >= TOUCH_THRESHOLD) result = true;
    return result;
}



char* separatorTysiecy_c(char* bufNum, uint32_t n) {
  int i = 15;
  bufNum[i--] = '\0';
    
  if (n == 0) {
    bufNum[0] = '0';
    bufNum[1] = '\0';
    return bufNum;
  }

  uint8_t count = 0;
  while (n > 0) {
    if (count == 3) {
      bufNum[i--] = ' ';
        count = 0;
    }
    bufNum[i--] = (n % 10) + '0';
    n /= 10;
    count++;
  }
  int startIdx = i + 1;
  int dlugosc = 15 - startIdx;
  memmove(bufNum, &bufNum[startIdx], dlugosc + 1);
  return bufNum;
}

