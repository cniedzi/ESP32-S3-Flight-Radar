#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

#include "JsonParser.h"

struct Aircraft {
    String icao24;          // hex ID samolotu
    String callsign;        // numer rejsu (flight)
    String type;
    long   seen_pos;    
    long   seen;     
    float  longitude;       // WGS-84 longitude
    float  latitude;        // WGS-84 latitude
    float  baroAltitude;    // wysokość w metrach (przeliczona ze stóp)
    bool   onGround;        // czy na ziemi
    float  velocity;        // prędkość w m/s (przeliczona z węzłów dla fizyki TrackedAircraft)
    float  trueTrack;       // kurs (heading)
    float  verticalRate;    
    float  geoAltitude;     
    bool   spi;             
    int    positionSource;  
    int    category;        
};

namespace JsonParser {
    template<>
    Aircraft Parse<Aircraft>(const JsonVariant& state);
}