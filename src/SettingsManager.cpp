#include "SettingsManager.h"

SettingsManager::SettingsManager() {
    
}

void SettingsManager::Initialise() {
    preferences.begin("config", false); // Otwieramy raz na starcie w trybie odczyt/zapis
    _Lat = preferences.getDouble("latitude", 52.010457);
    _Lon = preferences.getDouble("longitude", 20.537429);
    _Rad = preferences.getInt("radius", 60);
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

int SettingsManager::GetRadius() {
    return _Rad;
}

void SettingsManager::SetRadius(int rad) {
    _Rad = rad;
    preferences.putInt("radius", rad);
}

// ---------------------------------------------------------
// USTAWIENIA INTERFEJSU (UI)
// ---------------------------------------------------------

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