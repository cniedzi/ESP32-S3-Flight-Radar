#include "AircraftManager.h"
#include <unordered_set>
#include "AircraftIcon.h"


constexpr int SCREEN_SIZE = 480;
constexpr int SCREEN_SIZE_DIV_2 = (SCREEN_SIZE / 2);


void AircraftManager::Initialise()
{
    // Pobranie konfiguracji środka radaru oraz promienia (teraz w milach morskich, np. 100 NM)
    lat = configServer.GetStoredDouble("latitude", 0.0);
    lon = configServer.GetStoredDouble("longitude", 0.0);
    rad = configServer.GetStoredInt("radius", 60);
    
    // Konfiguracja widoczności parametrów samolotów
    displayInfoText = configServer.GetStoredBool("infotext", true);

    fetchInterval = 5000;
}





void AircraftManager::Update()
{
    unsigned long now = millis();

    // Cykl pobierania danych
    if (now - lastFetch >= fetchInterval) {
        lastFetch = now;

        String url = "https://api.adsb.lol/v2/lat/" + String(lat, 4) + "/lon/" + String(lon, 4) + "/dist/" + String((int)rad);

        // 0. Alokator PSRAM dla dokumentu JSON
        PsramJsonAllocator psramAllocator;

        // 1. Dokument ląduje w całości w PSRAM
        JsonDocument doc(&psramAllocator);

        HttpResult result = http.GetJson(url, doc);

        if (!result.success) {
            Serial.print("[WARN] API/JSON Error: ");
            Serial.println(result.errorMessage);
            return;
        }

        // Zbiór pomocniczy do śledzenia aktywnych ICAO w tej paczce (do usuwania "duchów")
        std::unordered_set<std::string> fetchedIcaos;

        // Pobieramy tablicę JSON bezpośrednio z dokumentu
        JsonArray array = doc["ac"].as<JsonArray>();

        // 2. Bezpośrednia pętla: JSON -> pojedynczy Aircraft -> trackedAircraft
        int count = 0;
        for (JsonObject item : array) {
            // Parsujemy pojedynczy element w locie
            Aircraft ac = JsonParser::Parse<Aircraft>(item);

            // Jawnie konwertujemy Arduino String na std::string przed wrzuceniem do setu
            fetchedIcaos.insert(ac.icao24);

            // Od razu aktualizujemy lub dodajemy do głównej bazy
            auto it = trackedAircraft.find(ac.icao24);
            if (it == trackedAircraft.end()) {
                trackedAircraft.emplace(ac.icao24, TrackedAircraft{ ac, now });
            }
            else {
                it->second.Update(ac, now);
            }

            // Co 15 przetworzonych samolotów dajemy "odetchnąć" systemowi (zapobiega WDT)
            if (++count % 15 == 0) {
                yield(); 
            }
        }

        now = millis(); // Aktualizacja znacznika czasu

        // 3. Usunięcie samolotów, których już nie ma w nowym strumieniu danych
        int cleanCount = 0;
        for (auto it = trackedAircraft.begin(); it != trackedAircraft.end(); ) {
            // Konwertujemy klucz z mapy (Arduino String) na std::string do wyszukiwania w secie
            if (fetchedIcaos.find(std::string(it->first.c_str())) == fetchedIcaos.end()) {
                it = trackedAircraft.erase(it);
            }
            else {
                ++it;
            }
            // Dajemy odetchnąć systemowi co 20 usuniętych/sprawdzonych elementów
            if (++cleanCount % 20 == 0) {
                yield();
            }
        }
        yield();
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

        if (x < 0 || x > 480 || y < 80 || y > 400) continue;

        if (displayInfoText) DrawAircraftInfo(backbuffer, x, y, tracked);

        DrawAircraft(backbuffer, x, y, tracked);
    }
}




void AircraftManager::DrawRadarCircles(LGFX_Sprite& backbuffer) const
{
    constexpr int CENTRE = SCREEN_SIZE_DIV_2 - 1;
    constexpr int OUTER = SCREEN_SIZE_DIV_2 - 1;

    int r1 = OUTER / 3;
    int r2 = (2 * OUTER) / 3;
    int r3 = OUTER;

    // Rysowanie okręgów
    backbuffer.drawCircle(CENTRE, CENTRE, r3, lgfx::color565(0, 64, 0));
    backbuffer.drawCircle(CENTRE, CENTRE, r2, lgfx::color565(0, 64, 0));
    backbuffer.drawCircle(CENTRE, CENTRE, r1, lgfx::color565(0, 64, 0));

    // Konfiguracja stylu tekstu
    backbuffer.setTextSize(1);
    backbuffer.setTextColor(lgfx::color565(0, 128, 0), TFT_BLACK);
    
    // Ustawienie centrowania tekstu
    backbuffer.setTextDatum(middle_center); 

    // Obliczenie wartości zasięgu
    int range1 = static_cast<int>(rad / 3.0f + 0.5f);
    int range2 = static_cast<int>((rad * 2.0f) / 3.0f + 0.5f);
    int range3 = static_cast<int>(rad);

    // Kąt w radianach (30 stopni = PI / 6)
    constexpr float angleRad = PI / 6.0f; 
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
        int x = CENTRE + static_cast<int>(static_cast<float>(rc.radius) * cosA);
        int y = CENTRE - static_cast<int>(static_cast<float>(rc.radius) * sinA);
        
        // Lekkie odsunięcie na zewnątrz okręgu (np. o 6 pikseli)
        int labelX = x + static_cast<int>(6.0f * cosA);
        int labelY = y - static_cast<int>(6.0f * sinA);

        backbuffer.setCursor(labelX, labelY);
        backbuffer.printf("%dnm", rc.value);
    }
    
    backbuffer.setTextDatum(top_right);
    backbuffer.setTextColor(TFT_BLACK, 0xdc82);
    char rangeText[15];
    snprintf(rangeText, sizeof(rangeText), " Range %dnm ", (int)rad);
    backbuffer.drawString(rangeText, backbuffer.width(), 80);

    // Reset datum na domyślne
    backbuffer.setTextDatum(top_left);
}




std::pair<int, int> AircraftManager::ProjectCoordinateToScreen(float predLat, float predLon) const
{
    const float dLon = predLon - lon;
    const float dLat = predLat - lat;

    // KLUCZOWO: Korekta długości geograficznej ze względu na szerokość (np. w Polsce ok. 0.61)
    const float dLonCorrected = dLon * cos(radians(lat));

    // Przeliczenie promienia z mil morskich (NM) na stopnie geograficzne (1 stopień $\approx$ 60 NM)
    const float radDeg = rad / 60.0f;

    // Używamy dLonCorrected zamiast surowego dLon
    const float normLon = (dLonCorrected + radDeg) / (2.0f * radDeg);
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



void AircraftManager::DrawAircraft(LGFX_Sprite& backbuffer, int x, int y, const TrackedAircraft& tracked) const
{
    // Zdefiniuj wymiary obrazka samolotu (zaktualizuj jeśli tablica ma inne wymiary)
    constexpr int32_t IMG_WIDTH = 14;
    constexpr int32_t IMG_HEIGHT = 14;

    // Ustawienie punktu obrotu dokładnie na środku obrazka
    const float pivotX = IMG_WIDTH / 2.0f;
    const float pivotY = IMG_HEIGHT / 2.0f;

    // Pobranie kąta (kierunku lotu) bezpośrednio z danych samolotu
    float angle = tracked.state.trueTrack;

    // Opcjonalnie: Jeśli chciałbyś zachować różne kolory w zależności od wysokości,
    // musisz użyć odpowiedniej tablicy.
    const unsigned short* aircraftSprite = AIRCRAFT;
    if (tracked.state.baroAltitude > 10000) {
        aircraftSprite = AIRCRAFT_HIGH_ALT;
    }

    // Rysowanie na buforze
    backbuffer.pushImageRotateZoom(
        x, y,                  // Punkt na ekranie, w którym znajdzie się środek obrazka
        pivotX, pivotY,        // Punkt zakotwiczenia (obrotu) na samym obrazku
        angle,                 // Kąt obrotu w stopniach
        1.0f, 1.0f,            // Skala (1.0 = oryginalny rozmiar)
        IMG_WIDTH, IMG_HEIGHT, // Rozmiary tablicy pikseli
        aircraftSprite,        // Tablica danych (np. wygenerowana z obrazka)
        TFT_BLACK              // Przezroczysty kolor tła (czarny nie będzie rysowany)
    );
}