#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "JsonParser.h"

struct Aircraft {
    char   icao24[8];           // hex ID samolotu
    char   callsign[10];        // numer rejsu (flight)
    char   type[8];
    long   seen_pos;    
    long   seen;     
    float  longitude;           // WGS-84 longitude
    float  latitude;            // WGS-84 latitude
    float  baroAltitude;        // wysokość w metrach (przeliczona ze stóp)
    bool   onGround;            // czy na ziemi
    float  velocity;            // prędkość w m/s (przeliczona z węzłów dla fizyki TrackedAircraft)
    float  trueTrack;           // kurs (heading)
};

namespace JsonParser {
    Aircraft ParseADSB(const JsonVariant& state);
    Aircraft ParseOpenSky(const JsonVariant& state);
}