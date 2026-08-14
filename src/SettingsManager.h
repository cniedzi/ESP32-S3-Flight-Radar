#pragma once
#include <Preferences.h>

enum class FlightPlatform {
        ADSB_LOL,
        OpenSky
};

class SettingsManager {
private:
    
    Preferences preferences;
    
    double _Lat;
    double _Lon;
    int _Range;
    FlightPlatform _platform;
    bool _altitudeInMeters;
    bool _speedInKmh;
    bool _displayInfoText;
    bool _displayMemoryInfo;
    bool _displayRssiInfo;
    bool _displayRange;
    bool _displayPlatform;
    bool _displayAircraftsUpdateIndicator;
    bool _displayAircraftsUpdateTimeRemaining;
    char _openSkyClientId[64];
    char _openSkyClientSecret[64];



public:
    SettingsManager();
    ~SettingsManager() = default;

    void Initialise();

    // ==========================================
    // GETTERY I SETTERY
    // ==========================================

    double GetLatitude() const;
    void SetLatitude(double lat);

    double GetLongitude() const;
    void SetLongitude(double lon);

    int GetRange() const;
    void SetRange(int range);

    FlightPlatform GetPlatform() const;
    void SetPlatform(FlightPlatform platform);

    bool GetAltitudeInMeters() const;
    void SetAltitudeInMeters(bool inMeters);

    bool GetSpeedInKmh() const;
    void SetSpeedInKmh(bool inKmh);

    bool GetInfoTextVisible() const;
    void SetInfoTextVisible(bool visible);

    bool GetDisplayMemoryInfo() const;
    void SetDisplayMemoryInfo(bool visible);

    bool GetDisplayRSSI() const;
    void SetDisplayRSSI(bool visible);

    bool GetDisplayRange() const;
    void SetDisplayRange(bool visible);

    bool GetDisplayPlatform() const;
    void SetDisplayPlatform(bool visible);

    bool GetDisplayAircraftsUpdateIndicator() const;
    void SetDisplayAircraftsUpdateIndicator(bool visible);

    bool GetDisplayAircraftsUpdateTimeRemaining() const;
    void SetDisplayAircraftsUpdateTimeRemaining(bool visible);

    // Gettery i settery dla OpenSky
    const char* GetOpenSkyClientId() const;
    void SetOpenSkyClientId(const char* clientId);

    const char* GetOpenSkyClientSecret() const;
    void SetOpenSkyClientSecret(const char* clientSecret);
        
};