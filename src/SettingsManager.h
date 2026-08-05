#pragma once
#include <Preferences.h>

class SettingsManager {
private:
    Preferences preferences;

    double _Lat;
    double _Lon;
    int _Rad;
    bool _displayInfoText;
    bool _displayMemoryInfo;
    bool _displayRssiInfo;
    bool _displayRange;
    bool _displayAircraftsUpdateIndicator;

public:
    SettingsManager();
    ~SettingsManager() = default;

    // Funkcja do opcjonalnej inicjalizacji (np. resetowanie danych, jeśli to potrzebne)
    void Initialise();

    // ==========================================
    // GETTERY I SETTERY
    // ==========================================

    double GetLatitude();
    void SetLatitude(double lat);

    double GetLongitude();
    void SetLongitude(double lon);

    int GetRadius();
    void SetRadius(int rad);

    bool GetInfoTextVisible();
    void SetInfoTextVisible(bool visible);

    bool GetDisplayMemoryInfo();
    void SetDisplayMemoryInfo(bool visible);

    bool GetDisplayRSSI();
    void SetDisplayRSSI(bool visible);

    bool GetDisplayRange();
    void SetDisplayRange(bool visible);

    bool GetDisplayAircraftsUpdateIndicator();
    void SetDisplayAircraftsUpdateIndicator(bool visible);
        
};