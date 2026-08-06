#include "AircraftManager.h"
#include "AircraftIcon.h"
#include <unordered_set>
#include <WiFi.h>


#define RADAR_SIZE DISPLAY_WIDTH
#define DISPLAY_WIDTH_DIV_2 DISPLAY_WIDTH / 2
#define DISPLAY_HEIGHT_DIV_2 DISPLAY_HEIGHT / 2
#define FETCH_INTERVAL 5000 //ms


void AircraftManager::Initialise()
{
}



void AircraftManager::Update()
{
    unsigned long now = millis();

    // Cykl pobierania danych
    if (now - lastFetch >= FETCH_INTERVAL) {
        
        isFetching = true;

        String url = "https://api.adsb.lol/v2/lat/" + String(settings.GetLatitude(), 4) + "/lon/" + String(settings.GetLongitude(), 4) + "/dist/" + String(settings.GetRange());

        // 0. Alokator PSRAM dla dokumentu JSON
        PsramJsonAllocator psramAllocator;

        // 1. Dokument ląduje w całości w PSRAM
        JsonDocument doc(&psramAllocator);

        HttpResult result = http.GetJson(url, doc);

        if (!result.success) {
            Serial.print("[WARN] API/JSON Error: ");
            Serial.println(result.errorMessage);
            isFetching = false;
            return;
        }

        // Zbiór pomocniczy do śledzenia aktywnych ICAO w tej paczce (do usuwania "duchów")
        std::unordered_set<std::string> fetchedIcaos;

        // Pobieramy tablicę JSON bezpośrednio z dokumentu
        JsonArray array = doc["ac"].as<JsonArray>();

        {
            std::lock_guard<std::mutex> lock(_dataMutex);
            // 2. Bezpośrednia pętla: JSON -> pojedynczy Aircraft -> trackedAircraft
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
            }

            now = millis(); // Aktualizacja znacznika czasu

            // 3. Usunięcie samolotów, których już nie ma w nowym strumieniu danych
            for (auto it = trackedAircraft.begin(); it != trackedAircraft.end(); ) {
                // Konwertujemy klucz z mapy (Arduino String) na std::string do wyszukiwania w secie
                if (fetchedIcaos.find(std::string(it->first.c_str())) == fetchedIcaos.end()) {
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
    int currentY = 0;
    const uint8_t lineHeight = radarSprite.fontHeight() + 3;
    if (settings.GetDisplayMemoryInfo()) {
        char buf[15];
        radarSprite.setTextDatum(top_left);
        radarSprite.setCursor(0, currentY); radarSprite.setTextColor(TFT_ORANGE); radarSprite.printf("Free heap: %sB", separatorTysiecy_c(buf, ESP.getFreeHeap())); currentY += lineHeight;
        radarSprite.setCursor(0, currentY); radarSprite.setTextColor(TFT_ORANGE); radarSprite.printf("Free PSRAM: %sB", separatorTysiecy_c(buf, ESP.getFreePsram())); currentY += lineHeight;
        
    }
    if (settings.GetDisplayRSSI()) {
        radarSprite.setTextDatum(top_left);
        radarSprite.setCursor(0, currentY);
        radarSprite.setTextColor(TFT_ORANGE);radarSprite.printf("RSSI: %ddBm", WiFi.RSSI());
    }
    if (settings.GetDisplayRange()) {
        radarSprite.setTextDatum(top_center);
        radarSprite.setTextColor(TFT_WHITE, TFT_MAGENTA);
        char rangeText[15];
        snprintf(rangeText, sizeof(rangeText), " Range %dnm ", settings.GetRange());
        radarSprite.drawString(rangeText, radarSprite.width() / 2, 0);
        radarSprite.setTextDatum(top_left);
    }
    if (settings.GetDisplayAircraftsUpdateIndicator()) {
        if (isFetching.load()) radarSprite.fillCircle(DISPLAY_WIDTH - 6, 6, 5, tft.color565(6, 85, 150));
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
        int x = CENTRE_X + static_cast<int>(static_cast<float>(rc.radius) * cosA);
        int y = CENTRE_Y - static_cast<int>(static_cast<float>(rc.radius) * sinA);
        
        // Lekkie odsunięcie na zewnątrz okręgu (np. o 6 pikseli)
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

    // KLUCZOWO: Korekta długości geograficznej ze względu na szerokość (np. w Polsce ok. 0.61)
    const float dLonCorrected = dLon * cos(radians(settings.GetLatitude()));

    // Przeliczenie promienia z mil morskich (NM) na stopnie geograficzne (1 stopień $\approx$ 60 NM)
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

    // Uwaga: ADSB.lol zwraca prędkość (gs) bezpośrednio w węzłach, a wysokość (alt_baro) w stopach.
    // Dostosuj poniższe linie w zależności od tego, jak model Aircraft przypisuje te pola z obiektu JSON ADSB.lol.
    int speed_kts = static_cast<int>(round(tracked.state.velocity / 0.514444f));
    int speed_kmh = static_cast<int>(round(tracked.state.velocity * 3.6f));
    int height_m = static_cast<int>(round(tracked.state.baroAltitude));
    int height_ft = static_cast<int>(round(tracked.state.baroAltitude * 3.28084f));

    radarSprite.setTextSize(1);
    radarSprite.setTextDatum(top_left);
    radarSprite.setTextColor(lgfx::color565(0, 128, 0));
    radarSprite.drawString(tracked.state.callsign, x + 5, y + 5);
    radarSprite.setTextColor(TFT_CYAN);
    radarSprite.drawString(tracked.state.type, x + 5, y + 5 + lineHeight);
    radarSprite.setTextColor(TFT_GRAY);
    if (settings.GetAltitudeInMeters()) radarSprite.drawString(String(height_m) + "m", x + 5, y + 5 + lineHeight * 2);
    else radarSprite.drawString(String(height_ft) + "ft", x + 5, y + 5 + lineHeight * 2);
    if (settings.GetSpeedInKmh()) radarSprite.drawString(String(speed_kmh) + "km/h", x + 5, y + 5 + lineHeight * 3);
    else radarSprite.drawString(String(speed_kts) + "kts", x + 5, y + 5 + lineHeight * 3);

}



void AircraftManager::DrawAircraft(LGFX_Sprite& radarSprite, int x, int y, const TrackedAircraft& tracked) const
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
    radarSprite.pushImageRotateZoom(
        x, y,                  // Punkt na ekranie, w którym znajdzie się środek obrazka
        pivotX, pivotY,        // Punkt zakotwiczenia (obrotu) na samym obrazku
        angle,                 // Kąt obrotu w stopniach
        1.0f, 1.0f,            // Skala (1.0 = oryginalny rozmiar)
        IMG_WIDTH, IMG_HEIGHT, // Rozmiary tablicy pikseli
        aircraftSprite,        // Tablica danych (np. wygenerowana z obrazka)
        TFT_BLACK              // Przezroczysty kolor tła (czarny nie będzie rysowany)
    );
}



void AircraftManager::ForceUpdate() {
    std::lock_guard<std::mutex> lock(_dataMutex);
    lastFetch = 0; // Zerujemy timer, dzięki czemu następne wywołanie Update() wykona się natychmiast
}



// Separator tysięcy w liczbie - wersja z tablicą znaków
char* AircraftManager::separatorTysiecy_c(char* bufNum, uint32_t n) {
  int i = 15;
  bufNum[i--] = '\0';
    
  if (n == 0) {
    bufNum[0] = '0';
    bufNum[1] = '\0';
    return bufNum;
  }

  uint8_t count = 0;
  while (n > 0) {
    if (count == 3) {
      bufNum[i--] = ' ';
        count = 0;
    }
    bufNum[i--] = (n % 10) + '0';
    n /= 10;
    count++;
  }
  int startIdx = i + 1;
  int dlugosc = 15 - startIdx;
  memmove(bufNum, &bufNum[startIdx], dlugosc + 1);
  return bufNum;
}



// Funkcja do zapisu zmiennej (Setter)
void AircraftManager::setRad(int newRad) { 
  if (newRad > 0 && newRad <= 250) settings.SetRange(newRad);
  else if (newRad > 250) settings.SetRange(250);
  else settings.SetRange(10);
  
  if (radiusChangedCallback) radiusChangedCallback(settings.GetRange());
}