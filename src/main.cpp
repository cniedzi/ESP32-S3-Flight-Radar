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
SettingsManager settingsManager;
AircraftManager aircraftManager(settingsManager, http, tft);
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
  
  radarSprite.pushSprite(0, 0);
  delay(10);

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
        char label[9];
        snprintf(label, sizeof(label), " %s ", airport.icao);
        radarSprite.drawString(label, x + 10, y - 10);
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
  aircraftManager.setRad(settingsManager.GetRange() - 10);
  g_lastZoomChange = millis();
  g_zoomChangeActive = true;    
}



void commandZoomOut() {
  aircraftManager.setRad(settingsManager.GetRange() + 10);
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
