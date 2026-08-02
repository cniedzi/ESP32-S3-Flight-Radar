struct Airport {
  char icao[5];      // 4 znaki kodu ICAO + 1 ukryty znak końca (tzw. null-terminator '\0')
  double latitude;   // Używamy double dla maksymalnej precyzji mapy (jak rozmawialiśmy wcześniej)
  double longitude;
};

const Airport airportList[] = {
    {"EPWA", 52.165727, 20.967111}, // Warszawa-Okęcie
    {"EPMO", 52.451175, 20.651959}, // Warszawa-Modlin
    {"EPKK", 50.077732, 19.784836}, // Kraków-Balice
    {"EPGD", 54.377569, 18.466222}, // Gdańsk-Rębiechowo
    {"EPKT", 50.474253, 19.079521}, // Katowice-Pyrzowice
    {"EPWR", 51.102683, 16.885834}, // Wrocław-Strachowice
    {"EPPO", 52.421031, 16.826322}, // Poznań-Ławica
    {"EPRZ", 50.109961, 22.019011}  // Rzeszów-Jasionka
};
