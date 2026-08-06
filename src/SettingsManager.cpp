#include "SettingsManager.h"

SettingsManager::SettingsManager() {
    
}

void SettingsManager::Initialise() {
    preferences.begin("config", false);
    _Lat = preferences.getDouble("latitude", 52.010457);
    _Lon = preferences.getDouble("longitude", 20.537429);
    _Range = preferences.getInt("range", 60);
    _altitudeInMeters = preferences.getBool("altInM", false); // domyślnie feet
    _speedInKmh = preferences.getBool("spdInKmh", false);     // domyślnie knots
    _displayInfoText = preferences.getBool("infotext", true);
    _displayMemoryInfo = preferences.getBool("meminfo", true);
    _displayRssiInfo = preferences.getBool("rssiinfo", true);
    _displayRange = preferences.getBool("rangeinfo", true);
    _displayAircraftsUpdateIndicator = preferences.getBool("updateinfo", true);
}

// ---------------------------------------------------------
// WSPÓŁRZĘDNE I ZASIĘG
// ---------------------------------------------------------

double SettingsManager::GetLatitude() {
    return _Lat;
}

void SettingsManager::SetLatitude(double lat) {
    _Lat = lat;
    preferences.putDouble("latitude", lat);
}

double SettingsManager::GetLongitude() {
    return _Lon;
}

void SettingsManager::SetLongitude(double lon) {
    _Lon = lon;
    preferences.putDouble("longitude", lon);
}

int SettingsManager::GetRange() {
    return _Range;
}

void SettingsManager::SetRange(int range) {
    _Range = range;
    preferences.putInt("range", range);
}

// ---------------------------------------------------------
// USTAWIENIA INTERFEJSU (UI)
// ---------------------------------------------------------

bool SettingsManager::GetAltitudeInMeters() {
    return _altitudeInMeters;
}

void SettingsManager::SetAltitudeInMeters(bool inMeters) {
    _altitudeInMeters = inMeters;
    preferences.putBool("altInM", inMeters);
}

bool SettingsManager::GetSpeedInKmh() {
    return _speedInKmh;
}

void SettingsManager::SetSpeedInKmh(bool inKmh) {
    _speedInKmh = inKmh;
    preferences.putBool("spdInKmh", inKmh);
}

bool SettingsManager::GetInfoTextVisible() {
    return _displayInfoText;
}

void SettingsManager::SetInfoTextVisible(bool visible) {
    _displayInfoText = visible;
    preferences.putBool("infotext", visible);
}

bool SettingsManager::GetDisplayMemoryInfo() {
    return _displayMemoryInfo;
}

void SettingsManager::SetDisplayMemoryInfo(bool visible) {
    _displayMemoryInfo = visible;
    preferences.putBool("meminfo", visible);
}

bool SettingsManager::GetDisplayRSSI() {
    return _displayRssiInfo;
}

void SettingsManager::SetDisplayRSSI(bool visible) {
    _displayRssiInfo = visible;
    preferences.putBool("rssiinfo", visible);
}

bool SettingsManager::GetDisplayRange() {
    return _displayRange;
}

void SettingsManager::SetDisplayRange(bool visible) {
    _displayRange = visible;
    preferences.putBool("rangeinfo", visible);
}

bool SettingsManager::GetDisplayAircraftsUpdateIndicator() {
    return _displayAircraftsUpdateIndicator;
}

void SettingsManager::SetDisplayAircraftsUpdateIndicator(bool visible) {
    _displayAircraftsUpdateIndicator = visible;
    preferences.putBool("updateinfo", visible);
}