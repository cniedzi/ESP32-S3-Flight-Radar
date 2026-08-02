#include "AircraftManager.h"

constexpr int SCREEN_SIZE = 480;
constexpr int SCREEN_SIZE_DIV_2 = (SCREEN_SIZE / 2);

#include <ArduinoJson.h>

void AircraftManager::Initialise()
{
    // Pobranie konfiguracji środka radaru oraz promienia (teraz w milach morskich, np. 100 NM)
    lat = configServer.GetStoredString("latitude").toDouble();
    lon = configServer.GetStoredString("longitude").toDouble();
    rad = configServer.GetStoredString("radius").toDouble();

    // Konfiguracja widoczności elementów UI
    const String renderText = configServer.GetStoredString("infotext");
    const String renderTris = configServer.GetStoredString("triangle");
    if (!renderText.isEmpty()) displayInfoText = renderText == "true" ? true : false;
    if (!renderTris.isEmpty()) displayTriangles = renderTris == "true" ? true : false;

    // ADSB.lol nie ma rychliwych limitów OpenSky. Ustawiamy odświeżanie co 5 sekund (5000 ms).
    fetchInterval = 5000;
}

void AircraftManager::Update()
{
    unsigned long now = millis();



    // Cykl pobierania danych
    if (now - lastFetch >= fetchInterval) {
        lastFetch = now;

        String url = "https://api.adsb.lol/v2/lat/" + String(lat, 4) + "/lon/" + String(lon, 4) + "/dist/" + String((int)rad);
        Serial.println(url);

        // Zapytanie HTTP bez nagłówków autoryzacji
        HttpResult result = http.Get(url, {}, {});

        // Jeśli zapytanie się nie powiodło, pomijamy ten cykl
        if (!result.success) {
            Serial.print("[WARN] ADSB.lol API request failed: ");
            Serial.println(result.errorMessage);
            return;
        }

        // Parsowanie odpowiedzi JSON (ADSB.lol zwraca obiekt z tablicą "aircraft")
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, result.response);
        if (error) {
            Serial.print("[ERR] JSON deserialization failed: ");
            Serial.println(error.c_str());
            return;
        }

        auto aircraft = JsonParser::ParseArray<Aircraft>(doc["ac"]);

        Serial.printf("[DEBUG] Liczba samolotów w tablicy ac: %d\n", aircraft.size());

        if (!aircraft.empty()) {
            Serial.printf("[DEBUG] Pierwszy samolot - ICAO/Hex: %s, Lat: %.4f, Lon: %.4f\n", 
                        aircraft[0].icao24.c_str(), aircraft[0].latitude, aircraft[0].longitude);
        }



        now = millis(); // Aktualizacja znacznika czasu po parsowaniu

        for (auto& ac : aircraft) {
            auto it = trackedAircraft.find(ac.icao24);
            if (it == trackedAircraft.end())
                trackedAircraft.emplace(ac.icao24, TrackedAircraft{ ac, now });
            else
                it->second.Update(ac, now);
        }

        // Usunięcie samolotów, których już nie ma w nowym strumieniu danych
        for (auto it = trackedAircraft.begin(); it != trackedAircraft.end(); ) {
            bool aircraftPresent = std::any_of(aircraft.begin(), aircraft.end(), [&](const Aircraft& ac) { return ac.icao24 == it->first; });
            if (!aircraftPresent)
                it = trackedAircraft.erase(it);
            else
                ++it;
        }
    }
}

void AircraftManager::Draw(LGFX_Sprite& backbuffer)
{
    DrawRadarCircles(backbuffer);

    for (auto& [icao, tracked] : trackedAircraft) {
        if (tracked.state.onGround) continue;

        tracked.Tick();
        auto [predLat, predLon] = tracked.GetDisplayPosition();
        auto [x, y] = ProjectCoordinateToScreen(predLat, predLon);

        if (x < 0 || x > 480 || y < 80 || y > 400) {
            continue;
        }

        if (displayInfoText)
            DrawAircraftInfo(backbuffer, x, y, tracked);

        if (displayTriangles)
            DrawAircraftTriangle(backbuffer, x, y, tracked);
        else
            backbuffer.fillCircle(x, y, 3, lgfx::color565(0, 255, 0));
    }
}

void AircraftManager::DrawRadarCircles(LGFX_Sprite& backbuffer) const
{
    constexpr int CENTRE = SCREEN_SIZE_DIV_2 - 1;
    constexpr int OUTER = SCREEN_SIZE_DIV_2 - 1;

    backbuffer.drawCircle(CENTRE, CENTRE, OUTER, lgfx::color565(0, 64, 0));
    backbuffer.drawCircle(CENTRE, CENTRE, (OUTER / 3) * 2, lgfx::color565(0, 64, 0));
    backbuffer.drawCircle(CENTRE, CENTRE, OUTER / 3, lgfx::color565(0, 64, 0));
}

std::pair<int, int> AircraftManager::ProjectCoordinateToScreen(float predLat, float predLon) const
{
    const float dLon = predLon - lon;
    const float dLat = predLat - lat;

    // Przeliczenie promienia z mil morskich (rad) na przybliżone stopnie geograficzne (1 stopień $\approx$ 60 NM)
    const float radDeg = rad / 60.0f;

    const float normLon = (dLon + radDeg) / (2.0f * radDeg);
    const float normLat = (dLat + radDeg) / (2.0f * radDeg);

    const int x = static_cast<int>(normLon * SCREEN_SIZE);
    const int y = static_cast<int>(SCREEN_SIZE - (normLat * SCREEN_SIZE));

    return { x, y };
}

void AircraftManager::DrawAircraftInfo(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked) const
{
    const int lineHeight = tft.fontHeight() + 1;

    // Uwaga: ADSB.lol zwraca prędkość (gs) bezpośrednio w węzłach, a wysokość (alt_baro) w stopach.
    // Dostosuj poniższe linie w zależności od tego, jak model Aircraft przypisuje te pola z obiektu JSON ADSB.lol.
    int speed_kts = (int)(tracked.state.velocity / 0.514444f);
    int height_m = (int)(tracked.state.baroAltitude);

    backbuffer.setTextSize(1);
    backbuffer.setTextColor(lgfx::color565(0, 128, 0));
    backbuffer.drawString(tracked.state.callsign, x + 5, y + 5);
    backbuffer.setTextColor(TFT_CYAN);
    backbuffer.drawString(tracked.state.type, x + 5, y + 5 + lineHeight);
    backbuffer.setTextColor(TFT_GRAY);
    backbuffer.drawString(String(height_m) + "m", x + 5, y + 5 + lineHeight * 2);
    backbuffer.drawString(String(speed_kts) + "kts", x + 5, y + 5 + lineHeight * 3);
}

void AircraftManager::DrawAircraftTriangle(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked) const
{
    const float dx = std::sin(radians(tracked.state.trueTrack));
    const float dy = -std::cos(radians(tracked.state.trueTrack));
    const float px = -dy;
    const float py = dx;

    constexpr float TRIANGLE_LENGTH = 6.0f;
    constexpr float TRIANGLE_WIDTH = 4.0f;

    const float tipX = x + dx * TRIANGLE_LENGTH;
    const float tipY = y + dy * TRIANGLE_LENGTH;
    const float leftX = x - dx * TRIANGLE_LENGTH * 0.5f + px * TRIANGLE_WIDTH * 0.5f;
    const float leftY = y - dy * TRIANGLE_LENGTH * 0.5f + py * TRIANGLE_WIDTH * 0.5f;
    const float rightX = x - dx * TRIANGLE_LENGTH * 0.5f - px * TRIANGLE_WIDTH * 0.5f;
    const float rightY = y - dy * TRIANGLE_LENGTH * 0.5f - py * TRIANGLE_WIDTH * 0.5f;

    backbuffer.fillTriangle(tipX, tipY, leftX, leftY, rightX, rightY, lgfx::color565(128, 0, 0));
}