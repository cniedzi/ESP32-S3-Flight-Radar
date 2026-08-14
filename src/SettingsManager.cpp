#include "SettingsManager.h"

SettingsManager::SettingsManager() {
    
}

void SettingsManager::Initialise() {
    preferences.begin("config", false);
    _Lat = preferences.getDouble("latitude", 52.010457);
    _Lon = preferences.getDouble("longitude", 20.537429);
    _Range = preferences.getInt("range", 60);
    _platform = static_cast<FlightPlatform>(preferences.getUChar("platform", static_cast<uint8_t>(FlightPlatform::ADSB_LOL)));
    _altitudeInMeters = preferences.getBool("altInM", false); // domyślnie feet
    _speedInKmh = preferences.getBool("spdInKmh", false);     // domyślnie knots
    _displayInfoText = preferences.getBool("infotext", true);
    _displayMemoryInfo = preferences.getBool("meminfo", true);
    _displayRssiInfo = preferences.getBool("rssiinfo", true);
    _displayRange = preferences.getBool("rangeinfo", true);
    _displayPlatform = preferences.getBool("platforminfo", true);
    _displayAircraftsUpdateIndicator = preferences.getBool("updateinfo", true);
    _displayAircraftsUpdateTimeRemaining = preferences.getBool("updatetime", true);

    // OPEN SKY
    size_t lenCid = preferences.getString("os_cid", _openSkyClientId, sizeof(_openSkyClientId));
    if (lenCid == 0) strlcpy(_openSkyClientId, "", sizeof(_openSkyClientId));
    size_t lenCsec = preferences.getString("os_csec", _openSkyClientSecret, sizeof(_openSkyClientSecret));
    if (lenCsec == 0) strlcpy(_openSkyClientSecret, "", sizeof(_openSkyClientSecret));
}

// ---------------------------------------------------------
// WSPÓŁRZĘDNE I ZASIĘG
// ---------------------------------------------------------

double SettingsManager::GetLatitude() const {
    return _Lat;
}

void SettingsManager::SetLatitude(double lat) {
    _Lat = lat;
    preferences.putDouble("latitude", lat);
}

double SettingsManager::GetLongitude() const {
    return _Lon;
}

void SettingsManager::SetLongitude(double lon) {
    _Lon = lon;
    preferences.putDouble("longitude", lon);
}

int SettingsManager::GetRange() const {
    return _Range;
}

void SettingsManager::SetRange(int range) {
    _Range = range;
    preferences.putInt("range", range);
}

// ---------------------------------------------------------
// PLATFORMA
// ---------------------------------------------------------

FlightPlatform SettingsManager::GetPlatform() const {
    return _platform;
}

void SettingsManager::SetPlatform(FlightPlatform platform) {
    _platform = platform;
    preferences.putUChar("platform", static_cast<uint8_t>(platform));
}

// ---------------------------------------------------------
// USTAWIENIA INTERFEJSU (UI)
// ---------------------------------------------------------

bool SettingsManager::GetAltitudeInMeters() const {
    return _altitudeInMeters;
}

void SettingsManager::SetAltitudeInMeters(bool inMeters) {
    _altitudeInMeters = inMeters;
    preferences.putBool("altInM", inMeters);
}

bool SettingsManager::GetSpeedInKmh() const {
    return _speedInKmh;
}

void SettingsManager::SetSpeedInKmh(bool inKmh) {
    _speedInKmh = inKmh;
    preferences.putBool("spdInKmh", inKmh);
}

bool SettingsManager::GetInfoTextVisible() const {
    return _displayInfoText;
}

void SettingsManager::SetInfoTextVisible(bool visible) {
    _displayInfoText = visible;
    preferences.putBool("infotext", visible);
}

bool SettingsManager::GetDisplayMemoryInfo() const {
    return _displayMemoryInfo;
}

void SettingsManager::SetDisplayMemoryInfo(bool visible) {
    _displayMemoryInfo = visible;
    preferences.putBool("meminfo", visible);
}

bool SettingsManager::GetDisplayRSSI() const {
    return _displayRssiInfo;
}

void SettingsManager::SetDisplayRSSI(bool visible) {
    _displayRssiInfo = visible;
    preferences.putBool("rssiinfo", visible);
}

bool SettingsManager::GetDisplayRange() const {
    return _displayRange;
}

void SettingsManager::SetDisplayRange(bool visible) {
    _displayRange = visible;
    preferences.putBool("rangeinfo", visible);
}

bool SettingsManager::GetDisplayPlatform() const {
    return _displayPlatform;
}

void SettingsManager::SetDisplayPlatform(bool visible) {
    _displayPlatform = visible;
    preferences.putBool("platforminfo", visible);
}

bool SettingsManager::GetDisplayAircraftsUpdateIndicator() const {
    return _displayAircraftsUpdateIndicator;
}

void SettingsManager::SetDisplayAircraftsUpdateIndicator(bool visible) {
    _displayAircraftsUpdateIndicator = visible;
    preferences.putBool("updateinfo", visible);
}

bool SettingsManager::GetDisplayAircraftsUpdateTimeRemaining() const {
    return _displayAircraftsUpdateTimeRemaining;
}

void SettingsManager::SetDisplayAircraftsUpdateTimeRemaining(bool visible) {
    _displayAircraftsUpdateTimeRemaining = visible;
    preferences.putBool("updatetime", visible);
}

// ---------------------------------------------------------
// OPEN SKY
// ---------------------------------------------------------

const char* SettingsManager::GetOpenSkyClientId() const {
    return _openSkyClientId;
}

void SettingsManager::SetOpenSkyClientId(const char* clientId) {
    strlcpy(_openSkyClientId, clientId, sizeof(_openSkyClientId));
    preferences.putString("os_cid", clientId);
}

const char* SettingsManager::GetOpenSkyClientSecret() const {
    return _openSkyClientSecret;
}

void SettingsManager::SetOpenSkyClientSecret(const char* clientSecret) {
    strlcpy(_openSkyClientSecret, clientSecret, sizeof(_openSkyClientSecret));
    preferences.putString("os_csec", clientSecret);
}
