#include "models/Aircraft.h"

namespace JsonParser {

    // Parser dla ADSB.lol (format obiektowy z kluczami)
    Aircraft ParseADSB(const JsonVariant& state) {
        Aircraft a;

        strlcpy(a.icao24, state["hex"] | "", sizeof(a.icao24));
        strlcpy(a.callsign, state["flight"] | "", sizeof(a.callsign));
        strlcpy(a.type, state["t"] | "", sizeof(a.type));
        a.seen_pos = state["seen_pos"].isNull() ? 0 : (long)state["seen_pos"].as<float>();
        a.seen = state["seen"].isNull() ? 0 : (long)state["seen"].as<float>();
        a.longitude = state["lon"].isNull() ? 0.0f : state["lon"].as<float>();
        a.latitude = state["lat"].isNull() ? 0.0f : state["lat"].as<float>();
        
        if (!state["alt_baro"].isNull()) {
            if (state["alt_baro"].is<float>() || state["alt_baro"].is<int>()) {
                a.baroAltitude = state["alt_baro"].as<float>() * 0.3048f; // Stopy na metry
                a.onGround = false;
            } else if (state["alt_baro"] == "ground") {
                a.baroAltitude = 0.0f;
                a.onGround = true;
            }
        } else {
            a.onGround = false;
        }

        if (!state["gs"].isNull()) {
            a.velocity = state["gs"].as<float>() * 0.514444f; // Węzły na m/s
        } else {
            a.velocity = 0.0f;
        }
        a.trueTrack = state["track"].isNull() ? 0.0f : state["track"].as<float>();
        
        return a;
    }

    // Parser dla OpenSky (format tablicowy indeksowany numerycznie)
    Aircraft ParseOpenSky(const JsonVariant& state) {
        Aircraft a;

        strlcpy(a.icao24, state[0] | "", sizeof(a.icao24));
        strlcpy(a.callsign, state[1] | "", sizeof(a.callsign));
        a.type[0] = {'\0'};
        a.seen_pos = 0;
        a.seen = 0;
        a.longitude = state[5].isNull() ? 0.0f : state[5].as<float>();
        a.latitude = state[6].isNull() ? 0.0f : state[6].as<float>();
        
        // OpenSky zwraca wysokość i prędkość w natywnych jednostkach (metry i m/s)
        a.baroAltitude = state[7].isNull() ? 0.0f : state[7].as<float>();
        a.onGround = state[8].isNull() ? false : state[8].as<bool>();
        a.velocity = state[9].isNull() ? 0.0f : state[9].as<float>();
        a.trueTrack = state[10].isNull() ? 0.0f : state[10].as<float>();

        return a;
    }
}