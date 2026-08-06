#pragma once

#include <map>
#include <mutex>
#include <atomic>

#include "models/TrackedAircraft.h"
#include "SettingsManager.h"
#include "HttpRequestManager.h"
#include "LGFX.h"
#include "Config.h"


class AircraftManager
{
private:
    std::mutex _dataMutex;
    std::atomic<bool> isFetching{false};
    std::map<std::string, TrackedAircraft> trackedAircraft;

    std::function<void(int)> radiusChangedCallback;

    bool displayInfoText = true;
    bool displayRange = true;
    bool displayAircraftsUpdateIndicator = true;
    bool displayMemoryInfo = true;
    bool displayRSSI = true;

    unsigned long fetchInterval = 0;
    unsigned long lastFetch = 999999;

    SettingsManager& settings;
    HttpRequestManager& http;
    LGFX& tft;

    void DrawRadarCircles(LGFX_Sprite& radarSprite) const;
    void DrawAircraftInfo(LGFX_Sprite& radarSprite, int x, int y, const TrackedAircraft& tracked) const;
    void DrawAircraft(LGFX_Sprite& radarSprite, int x, int y, const TrackedAircraft& tracked) const;
    char* separatorTysiecy_c(char* bufNum, uint32_t n);
    

public:
    AircraftManager(SettingsManager& settingsManager, HttpRequestManager& httpManager, LGFX& tftGfx)
        : settings(settingsManager), http(httpManager), tft(tftGfx)
    {
    }
    ~AircraftManager() = default;

    void Initialise();
    void Update();
    void ForceUpdate();
    void Draw(LGFX_Sprite& radarSprite);
    std::pair<int, int> ProjectCoordinateToScreen(float predLat, float predLon) const;
    void setOnRadiusChanged(std::function<void(int)> cb) { radiusChangedCallback = cb; }
    void setRad(int newRad);
    
};