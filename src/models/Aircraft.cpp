#include "models/Aircraft.h"

namespace JsonParser {
    template<>
    Aircraft Parse<Aircraft>(const JsonVariant& state) {
        Aircraft a;

        strlcpy(a.icao24, state["hex"] | "", sizeof(a.icao24));
        strlcpy(a.callsign, state["flight"] | "", sizeof(a.callsign));
        strlcpy(a.type, state["t"] | "", sizeof(a.type));
        a.seen_pos = state["seen_pos"].isNull() ? 0 : (long)state["seen_pos"].as<float>();;
        a.seen = state["seen"].isNull() ? 0 : (long)state["seen"].as<float>();
        a.longitude = state["lon"].isNull() ? 0.0f : state["lon"].as<float>();
        a.latitude = state["lat"].isNull() ? 0.0f : state["lat"].as<float>();
        if (!state["alt_baro"].isNull()) {
            if (state["alt_baro"].is<float>() || state["alt_baro"].is<int>()) {
                a.baroAltitude = state["alt_baro"].as<float>() * 0.3048f; // Konwersja stóp na metry (1 ft = 0.3048 m)
                a.onGround = false;
            } else if (state["alt_baro"].as<String>() == "ground") {
                a.baroAltitude = 0.0f;
                a.onGround = true;
            }
        } else {
            a.onGround = false;
        }

        if (!state["gs"].isNull()) {
            a.velocity = state["gs"].as<float>() * 0.514444f; // gs to prędkość w węzłach. Przeliczamy na m/s (1 knot = 0.514444 m/s), żeby klasa TrackedAircraft poprawnie liczyła predykcję pozycji w metrach
        } else {
            a.velocity = 0.0f;
        }
        a.trueTrack = state["track"].isNull() ? 0.0f : state["track"].as<float>();
        return a;
    }
}