#include "models/Aircraft.h"

namespace JsonParser {
    template<>
    Aircraft Parse<Aircraft>(const JsonVariant& state) {
        Aircraft a;

        // Mapowanie kluczy obiektowych z ADSB.lol
        a.icao24 = state["hex"].isNull() ? "" : state["hex"].as<String>();
        a.callsign = state["flight"].isNull() ? "" : state["flight"].as<String>();
        a.type = state["t"].isNull() ? "" : state["t"].as<String>();
        a.originCountry = ""; 

        a.longitude = state["lon"].isNull() ? 0.0f : state["lon"].as<float>();
        a.latitude = state["lat"].isNull() ? 0.0f : state["lat"].as<float>();

        // alt_baro w ADSB.lol bywa liczbą (w stopach) lub napisem "ground"
        if (!state["alt_baro"].isNull()) {
            if (state["alt_baro"].is<float>() || state["alt_baro"].is<int>()) {
                // Konwersja stóp na metry (1 ft = 0.3048 m)
                a.baroAltitude = state["alt_baro"].as<float>() * 0.3048f;
                a.onGround = false;
            } else if (state["alt_baro"].as<String>() == "ground") {
                a.baroAltitude = 0.0f;
                a.onGround = true;
            }
        } else {
            a.onGround = false;
        }

        // gs to prędkość w węzłach. Przeliczamy na m/s (1 knot = 0.514444 m/s), 
        // żeby klasa TrackedAircraft poprawnie liczyła predykcję pozycji w metrach.
        if (!state["gs"].isNull()) {
            a.velocity = state["gs"].as<float>() * 0.514444f;
        } else {
            a.velocity = 0.0f;
        }

        a.trueTrack = state["track"].isNull() ? 0.0f : state["track"].as<float>();
        a.verticalRate = state["baro_rate"].isNull() ? 0.0f : state["baro_rate"].as<float>();
        a.squawk = state["squawk"].isNull() ? "" : state["squawk"].as<String>();

        return a;
    }
}