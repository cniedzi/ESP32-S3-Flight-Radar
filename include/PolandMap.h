// Definicja punktów gdzieś w pliku (np. u góry pliku lub w pliku z konfiguracją)
struct GeoPoint {
    float lat;
    float lon;
};

const GeoPoint polandBorder[] = {
    // === WYBRZEŻE BAŁTYKU ===
    {53.91f, 14.24f}, {53.98f, 14.61f}, {54.02f, 14.77f}, {54.10f, 15.08f}, 
    {54.18f, 15.57f}, {54.26f, 16.14f}, {54.34f, 16.48f}, {54.55f, 16.85f}, 
    {54.74f, 17.34f}, {54.83f, 17.72f}, {54.83f, 18.20f}, 
    {54.83f, 18.30f}, // Jastrzębia Góra (Północ)
    {54.79f, 18.42f}, {54.60f, 18.80f}, // Cypel Helski
    {54.60f, 18.50f}, {54.45f, 18.55f}, {54.35f, 18.66f}, // Zatoka i Gdańsk
    {54.33f, 19.14f}, {54.43f, 19.90f}, // Mierzeja Wiślana (Piaski)

    // === GRANICA Z ROSJĄ (Obwód Królewiecki) ===
    {54.36f, 19.98f}, {54.37f, 20.30f}, {54.33f, 20.65f}, {54.34f, 21.05f}, 
    {54.32f, 21.50f}, {54.34f, 22.10f}, {54.36f, 22.75f}, // Trójstyk PL-RU-LT (Wisztyniec)

    // === GRANICA Z LITWĄ ===
    {54.25f, 23.00f}, 
    {54.15f, 23.40f}, // Ogrodniki / Sejny
    {53.95f, 23.53f}, // Trójstyk PL-LT-BY (Stanowisko / Mara)

    // === GRANICA Z BIAŁORUSIĄ ===
    {53.81f, 23.55f}, // Lipszczany
    {53.60f, 23.60f}, // Okolice Grodna (granica po polskiej stronie)
    {53.51f, 23.65f}, // Kuźnica
    {53.26f, 23.77f}, // Krynki
    {53.12f, 23.89f}, // Bobrowniki
    {52.95f, 23.91f}, // Wschodni brzeg Zbiornika Siemianówka
    {52.70f, 23.85f}, // Białowieża
    {52.50f, 23.35f}, // Połowce / Czeremcha
    {52.28f, 23.16f}, // Niemirów (początek granicy rzeką Bug)
    
    // === GRANICA Z UKRAINĄ ===
    {52.08f, 23.58f}, // Terespol
    {51.85f, 23.59f}, {51.52f, 23.63f}, {51.25f, 23.83f}, // Sobibór
    {51.05f, 23.83f}, 
    {50.85f, 24.12f}, // Kolano Bugu w Hrubieszowie (najbardziej na wschód)
    {50.70f, 24.03f}, {50.55f, 24.03f}, {50.40f, 23.76f}, {50.20f, 23.50f}, // Roztocze
    {50.05f, 23.22f}, {49.95f, 23.10f}, {49.77f, 22.84f}, {49.60f, 22.74f}, 
    {49.33f, 22.70f}, {49.10f, 22.85f}, 
    {49.00f, 22.85f}, // Szczyt Opołonek (najbardziej na południe)

    // === GRANICA ZE SŁOWACJĄ ===
    {49.08f, 22.53f}, // Trójstyk PL-UA-SK (Krzemieniec)
    {49.16f, 22.15f}, {49.36f, 21.53f}, {49.44f, 21.05f}, {49.38f, 20.65f}, 
    {49.40f, 20.33f}, {49.37f, 19.98f}, 
    {49.20f, 19.98f}, // Tatry (Rysy / Kasprowy)
    {49.25f, 19.78f}, {49.33f, 19.75f}, {49.50f, 19.45f}, {49.50f, 19.12f}, // Babia Góra
    {49.43f, 18.97f}, 
    {49.52f, 18.85f}, // Trójstyk PL-SK-CZ (Trzycatek)

    // === GRANICA Z CZECHAMI ===
    {49.61f, 18.81f}, {49.75f, 18.63f}, // Cieszyn
    {49.90f, 18.52f}, {49.92f, 18.25f}, {50.03f, 18.00f}, {50.25f, 17.70f}, 
    {50.31f, 17.33f}, {50.40f, 16.90f}, {50.15f, 16.65f}, {50.10f, 16.48f}, 
    {50.40f, 16.25f}, // Worek Międzyleski
    {50.45f, 16.03f}, {50.62f, 16.03f}, {50.68f, 15.93f}, {50.73f, 15.75f}, // Śnieżka
    {50.80f, 15.42f}, {50.95f, 15.15f}, 
    {50.87f, 14.98f}, // Trójstyk PL-CZ-DE (Porajów / Worek Turoszowski)

    // === GRANICA Z NIEMCAMI (Odra i Nysa Łużycka) ===
    {51.05f, 14.98f}, {51.20f, 15.00f}, {51.40f, 14.90f}, {51.55f, 14.73f}, 
    {51.75f, 14.75f}, {52.05f, 14.72f}, {52.20f, 14.65f}, {52.35f, 14.55f}, // Słubice
    {52.60f, 14.62f}, {52.75f, 14.45f}, 
    {52.88f, 14.12f}, // Osinów Dolny (najbardziej na zachód)
    {53.05f, 14.28f}, {53.25f, 14.40f}, {53.40f, 14.45f}, {53.53f, 14.55f}, // Szczecin
    {53.75f, 14.28f}, {53.85f, 14.20f},

    // Zamknięcie konturu (powrót do Świnoujścia)
    {53.91f, 14.24f}
};