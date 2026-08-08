#pragma once

#include <map>
#include <mutex>
#include <atomic>

#include "models/TrackedAircraft.h"
#include "SettingsManager.h"
#include "OpenSkyAuthTokenHandler.h"
#include "HttpRequestManager.h"
#include "LGFX.h"
#include "Config.h"


class AircraftManager
{
private:
    std::mutex _dataMutex;
    std::map<std::string, TrackedAircraft> trackedAircraft;
    std::function<void(int)> radiusChangedCallback;

    unsigned long lastFetch = 999999;

    SettingsManager& settings;
    OpenSkyAuthTokenHandler& authHandler;
    HttpRequestManager& http;
    LGFX& tft;

    void DrawRadarCircles(LGFX_Sprite& radarSprite) const;
    void DrawAircraftInfo(LGFX_Sprite& radarSprite, int x, int y, const TrackedAircraft& tracked) const;
    void DrawAircraft(LGFX_Sprite& radarSprite, int x, int y, const TrackedAircraft& tracked) const;
    
    

public:
    AircraftManager(SettingsManager& settingsManager, HttpRequestManager& httpManager, OpenSkyAuthTokenHandler& authHandlerManager, LGFX& tftGfx)
        : settings(settingsManager), authHandler(authHandlerManager), http(httpManager), tft(tftGfx)
    {
    }
    ~AircraftManager() = default;

    std::atomic<bool> isFetching{false};
    std::atomic<bool> isOpenSkyAuthenticated{false};
    std::atomic<bool> OpenSkyQuotaExceeded{false};


    void Initialise();
    void Update();
    void ForceUpdate();
    void Draw(LGFX_Sprite& radarSprite);
    std::pair<int, int> ProjectCoordinateToScreen(float predLat, float predLon) const;
    void setOnRadiusChanged(std::function<void(int)> cb) { radiusChangedCallback = cb; }
    void setRange(int newRad);
    
};