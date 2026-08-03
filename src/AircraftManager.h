#pragma once

#include <map>

#include "models/TrackedAircraft.h"
#include "ConfigurationWebServer.h"
#include "HttpRequestManager.h"
#include "LGFX.h"

class AircraftManager
{
private:
    double lat = 0.0;
    double lon = 0.0;
    int rad = 60;
    std::map<std::string, TrackedAircraft> trackedAircraft;

    bool displayInfoText = true;
    bool displayTriangles = true;

    unsigned long fetchInterval = 0;
    unsigned long lastFetch = 999999;

    ConfigurationWebServer& configServer;
    HttpRequestManager& http;
    LGFX& tft;

    void DrawRadarCircles(LGFX_Sprite& backbuffer) const;
    
    void DrawAircraftInfo(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked) const;
    void DrawAircraft(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked) const;

public:
    AircraftManager(ConfigurationWebServer& config, HttpRequestManager& httpManager, LGFX& tftGfx)
        : configServer(config), http(httpManager), tft(tftGfx)
    {
    }
    ~AircraftManager() = default;

    void Initialise();
    void Update();
    void Draw(LGFX_Sprite& backbuffer);
    std::pair<int, int> ProjectCoordinateToScreen(float predLat, float predLon) const;

    // Funkcja do odczytu zmiennej (Getter)
    int getRad() const { 
        return rad; 
    }

    // Funkcja do zapisu zmiennej (Setter)
    void setRad(int newRad) { 
        if (newRad > 0 && newRad <= 250) rad = newRad;
        else if (newRad > 250) rad = 250;
        else rad = 10;
    }

};