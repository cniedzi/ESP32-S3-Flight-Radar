#include "AircraftManager.h"
#include "AircraftIcon.h"
#include <unordered_set>
#include <WiFi.h>


#define RADAR_SIZE DISPLAY_WIDTH
#define DISPLAY_WIDTH_DIV_2 DISPLAY_WIDTH / 2
#define DISPLAY_HEIGHT_DIV_2 DISPLAY_HEIGHT / 2
#define FETCH_INTERVAL_ADSB 5000 //ms


void AircraftManager::Initialise() {}


void AircraftManager::Update()
{
    unsigned long now = millis();
    FlightPlatform platform = settings.GetPlatform();

    static FlightPlatform lastPlatform = platform;
    if (platform != lastPlatform) {
        std::lock_guard<std::mutex> lock(_dataMutex);
        trackedAircraft.clear();
        lastPlatform = platform;
    }

    unsigned long fetchInterval = FETCH_INTERVAL_ADSB;

    String token = "";
    if (platform == FlightPlatform::OpenSky) {
        constexpr int MS_PER_DAY = 24 * 60 * 60 * 1000;
        constexpr int ANONYMOUS_TOKENS_PER_DAY = 400;
        constexpr int AUTHED_TOKENS_PER_DAY = 4000;
        constexpr int TOKEN_BUFFER = 3;
        int dailyRequestBudget = ANONYMOUS_TOKENS_PER_DAY - TOKEN_BUFFER;
        // Sprawdzamy token dla OpenSky
        token = authHandler.GetValidToken(settings.GetOpenSkyClientId(), settings.GetOpenSkyClientSecret());
        if (!token.isEmpty()) {
            dailyRequestBudget = AUTHED_TOKENS_PER_DAY - TOKEN_BUFFER;
            isOpenSkyAuthenticated = true;
        }
        else isOpenSkyAuthenticated = false;
        fetchInterval = MS_PER_DAY / dailyRequestBudget; // Wyliczony budżet tylko dla OpenSky
    }

    // Cykl pobierania danych
    if (now - lastFetch >= fetchInterval) {
        char url[256];
        isFetching = true;
        std::vector<std::pair<String, String>> headers = {};

        if (platform == FlightPlatform::ADSB_LOL) {
            snprintf(url, sizeof(url), "https://api.adsb.lol/v2/lat/%.4f/lon/%.4f/dist/%d", settings.GetLatitude(), settings.GetLongitude(), settings.GetRange());
        } else {
            if (!token.isEmpty()) headers.push_back({ "Authorization", "Bearer " + token });

            // OpenSky wymaga prostokąta (bounding box) wyliczonego z pozycji i zasięgu w km
            float lat = settings.GetLatitude();
            float lon = settings.GetLongitude();
            int rangeNm = settings.GetRange();
            float rangeKm = rangeNm * 1.852f; // Przeliczamy mile morskie na kilometry (1 nm = 1.852 km)
            float latDelta = rangeKm / 111.0f;
            float lonDelta = rangeKm / (111.0f * cos(lat * 0.01745329251f));

            snprintf(url, sizeof(url), "https://opensky-network.org/api/states/all?lamin=%.4f&lamax=%.4f&lomin=%.4f&lomax=%.4f",
                     lat - latDelta, lat + latDelta, lon - lonDelta, lon + lonDelta);
        }

        // Alokator PSRAM dla dokumentu JSON
        PsramJsonAllocator psramAllocator;

        // Dokument ląduje w całości w PSRAM
        JsonDocument doc(&psramAllocator);

        HttpResult result = http.GetJson(url, doc, headers);

        if (!result.success) {
            Serial.print("[WARN] API/JSON Error: ");
            Serial.println(result.errorMessage);
            isFetching = false;
            return;
        }

        // Zbiór pomocniczy do śledzenia aktywnych ICAO w tej paczce (do usuwania "duchów")
        std::unordered_set<std::string> fetchedIcaos;

        // Pobieramy tablicę JSON bezpośrednio z dokumentu
        JsonArray array;
        if (platform == FlightPlatform::ADSB_LOL) {
            array = doc["ac"].as<JsonArray>();
        }
        else {
            if (doc["states"].isNull()) {
                Serial.println("[INFO] OpenSky: Brak samolotów w zadanym obszarze.");
                isFetching = false;
                lastFetch = millis();
                return;
            }
            array = doc["states"].as<JsonArray>();
        }

        {
            std::lock_guard<std::mutex> lock(_dataMutex);
            // Bezpośrednia pętla: JSON -> pojedynczy Aircraft -> trackedAircraft
            for (JsonVariant item : array) {
                // Parsujemy pojedynczy element w locie
                Aircraft ac;
                if (platform == FlightPlatform::ADSB_LOL) ac = JsonParser::ParseADSB(item);
                else ac = JsonParser::ParseOpenSky(item);

                // Jawnie konwertujemy String na std::string przed wrzuceniem do setu
                fetchedIcaos.insert(ac.icao24);

                // Od razu aktualizujemy lub dodajemy do głównej bazy
                auto it = trackedAircraft.find(ac.icao24);
                if (it == trackedAircraft.end()) {
                    trackedAircraft.emplace(ac.icao24, TrackedAircraft{ ac, now });
                }
                else {
                    it->second.Update(ac, now);
                }
            }

            now = millis();

            // Usunięcie samolotów, których już nie ma w nowym strumieniu danych
            for (auto it = trackedAircraft.begin(); it != trackedAircraft.end(); ) {
                if (fetchedIcaos.find(it->first) == fetchedIcaos.end()) {
                    it = trackedAircraft.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
        lastFetch = now;
        isFetching = false;
    }
}



void AircraftManager::Draw(LGFX_Sprite& radarSprite)
{
    std::lock_guard<std::mutex> lock(_dataMutex);
   
    DrawRadarCircles(radarSprite);
    for (auto& [icao, tracked] : trackedAircraft) {
        if (tracked.state.onGround) continue;
        tracked.Tick();
        auto [predLat, predLon] = tracked.GetDisplayPosition();
        auto [x, y] = ProjectCoordinateToScreen(predLat, predLon);

        if (x < -50 || x > DISPLAY_WIDTH + 50 || y < -50 || y > DISPLAY_HEIGHT + 50) continue;

        if (settings.GetInfoTextVisible()) DrawAircraftInfo(radarSprite, x, y, tracked);

        DrawAircraft(radarSprite, x, y, tracked);
    }
}



void AircraftManager::DrawRadarCircles(LGFX_Sprite& radarSprite) const
{
    constexpr int CENTRE_X = DISPLAY_WIDTH_DIV_2;
    constexpr int CENTRE_Y = DISPLAY_HEIGHT_DIV_2;
    constexpr int OUTER = DISPLAY_WIDTH_DIV_2 - 1;

    int r1 = OUTER / 3;
    int r2 = (2 * OUTER) / 3;
    int r3 = OUTER;

    // Rysowanie okręgów
    radarSprite.drawCircle(CENTRE_X, CENTRE_Y, r3, lgfx::color565(0, 64, 0));
    radarSprite.drawCircle(CENTRE_X, CENTRE_Y, r2, lgfx::color565(0, 64, 0));
    radarSprite.drawCircle(CENTRE_X, CENTRE_Y, r1, lgfx::color565(0, 64, 0));

    // Konfiguracja stylu tekstu
    radarSprite.setTextSize(1);
    radarSprite.setTextColor(lgfx::color565(0, 128, 0), TFT_BLACK);
    
    // Ustawienie centrowania tekstu
    radarSprite.setTextDatum(middle_center); 

    // Obliczenie wartości zasięgu
    int range1 = static_cast<int>(settings.GetRange() / 3.0f + 0.5f);
    int range2 = static_cast<int>((settings.GetRange() * 2.0f) / 3.0f + 0.5f);
    int range3 = static_cast<int>(settings.GetRange());

    // Kąt w radianach
    constexpr float angleRad = PI / 5.5f; 
    float sinA = sin(angleRad);
    float cosA = cos(angleRad);

    struct RangeCircle {
        int radius;
        int value;
    };
    RangeCircle ranges[] = {
        {r1, range1},
        {r2, range2},
        {r3, range3}
    };

    // Wyświetlenie napisów pod kątem 30 stopni
    for (const auto& rc : ranges) {
        int x = CENTRE_X + static_cast<int>(static_cast<float>(rc.radius) * cosA);
        int y = CENTRE_Y - static_cast<int>(static_cast<float>(rc.radius) * sinA);
        
        // Lekkie odsunięcie na zewnątrz okręgu
        int labelX = x + static_cast<int>(6.0f * cosA);
        int labelY = y - static_cast<int>(6.0f * sinA);

        radarSprite.setCursor(labelX, labelY);
        radarSprite.printf("%dnm", rc.value);
    }
    
}




std::pair<int, int> AircraftManager::ProjectCoordinateToScreen(float predLat, float predLon) const
{
    const float dLon = predLon - settings.GetLongitude();
    const float dLat = predLat - settings.GetLatitude();

    // Korekta długości geograficznej ze względu na szerokość
    const float dLonCorrected = dLon * cos(radians(settings.GetLatitude()));

    // Przeliczenie promienia z mil morskich (nm) na stopnie geograficzne
    const float radDeg = settings.GetRange() / 60.0f;

    // Używamy dLonCorrected zamiast surowego dLon
    const float normLon = (dLonCorrected + radDeg) / (2.0f * radDeg);
    const float normLat = (dLat + radDeg) / (2.0f * radDeg);

    const int x = static_cast<int>(normLon * RADAR_SIZE);
    const int y = static_cast<int>(RADAR_SIZE - (normLat * RADAR_SIZE) - ((RADAR_SIZE - DISPLAY_HEIGHT) / 2));

    return { x, y };
}

void AircraftManager::DrawAircraftInfo(LGFX_Sprite& radarSprite, int x, int y, const TrackedAircraft& tracked) const
{
    const int lineHeight = tft.fontHeight() + 1;
    uint8_t currentLine = 0;
    char buffer[32];

    // ADSB.lol zwraca prędkość (gs) bezpośrednio w węzłach, a wysokość (alt_baro) w stopach.
    int speed_kts = static_cast<int>(round(tracked.state.velocity / 0.514444f));
    int speed_kmh = static_cast<int>(round(tracked.state.velocity * 3.6f));
    int height_m = static_cast<int>(round(tracked.state.baroAltitude));
    int height_ft = static_cast<int>(round(tracked.state.baroAltitude * 3.28084f));

    radarSprite.setTextSize(1);
    radarSprite.setTextDatum(top_left);

    // Callsign (jeśli nie jest pusty)
    if (tracked.state.callsign[0] != '\0') {
        radarSprite.setTextColor(lgfx::color565(0, 128, 0));
        radarSprite.drawString(tracked.state.callsign, x + 5, y + 5 + lineHeight * currentLine);
        currentLine++;
    }

    // Type (jeśli nie jest pusty)
    if (tracked.state.type[0] != '\0') {
        radarSprite.setTextColor(TFT_CYAN);
        radarSprite.drawString(tracked.state.type, x + 5, y + 5 + lineHeight * currentLine);
        currentLine++;
    }

    // Altitude
    radarSprite.setTextColor(TFT_GRAY);
    if (settings.GetAltitudeInMeters()) {
        snprintf(buffer, sizeof(buffer), "%dm", height_m);
    } else {
        snprintf(buffer, sizeof(buffer), "%dft", height_ft);
    }
    radarSprite.drawString(buffer, x + 5, y + 5 + lineHeight * currentLine);
    currentLine++;

    // Speed
    if (settings.GetSpeedInKmh()) {
        snprintf(buffer, sizeof(buffer), "%dkm/h", speed_kmh);
    } else {
        snprintf(buffer, sizeof(buffer), "%dkts", speed_kts);
    }
    radarSprite.drawString(buffer, x + 5, y + 5 + lineHeight * currentLine);
    currentLine++;

}



void AircraftManager::DrawAircraft(LGFX_Sprite& radarSprite, int x, int y, const TrackedAircraft& tracked) const
{
    // Wymiary obrazka samolotu
    constexpr int32_t IMG_WIDTH = 14;
    constexpr int32_t IMG_HEIGHT = 14;

    // Ustawienie punktu obrotu dokładnie na środku obrazka
    const float pivotX = IMG_WIDTH / 2.0f;
    const float pivotY = IMG_HEIGHT / 2.0f;

    // Pobranie kąta (kierunku lotu) bezpośrednio z danych samolotu
    float angle = tracked.state.trueTrack;

    const unsigned short* aircraftSprite = AIRCRAFT;
    if (tracked.state.baroAltitude > 10000) {
        aircraftSprite = AIRCRAFT_HIGH_ALT;
    }

    // Rysowanie na buforze
    radarSprite.pushImageRotateZoom(
        x, y,                  // Punkt na ekranie, w którym znajdzie się środek obrazka
        pivotX, pivotY,        // Punkt zakotwiczenia (obrotu) na samym obrazku
        angle,                 // Kąt obrotu w stopniach
        1.0f, 1.0f,            // Skala (1.0 = oryginalny rozmiar)
        IMG_WIDTH, IMG_HEIGHT, // Rozmiary tablicy pikseli
        aircraftSprite,        // Tablica danych
        TFT_BLACK              // Przezroczysty kolor tła
    );
}



void AircraftManager::ForceUpdate() {
    std::lock_guard<std::mutex> lock(_dataMutex);
    lastFetch = 0; // Zerujemy timer, dzięki czemu następne wywołanie Update() wykona się natychmiast
}



void AircraftManager::setRange(int newRad) { 
  if (newRad > 0 && newRad <= 250) settings.SetRange(newRad);
  else if (newRad > 250) settings.SetRange(250);
  else settings.SetRange(10);
  
  if (radiusChangedCallback) radiusChangedCallback(settings.GetRange());
}