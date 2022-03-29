#include "wf.h"
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <map>

/* *********************************************
/  Rain Start Event Class
/
/  *********************************************/

// Parse the JSON message for the event data.
bool wfRainStartEvent::ParseMsg(JsonDocument& jsonMsg){
    // Check that we've good the right message components
    if( !jsonMsg.containsKey("evt") ){
        valid = false;
        return false;
    };
    // Parse the message array
    uint8_t count = 0; 
    for( JsonVariant RainStart : jsonMsg["evt"].as<JsonArray>() ){
        switch(count){
            case 0:{
                this->ulTimeEpoch = RainStart.as<unsigned long>();
            }
            break;
            default:{
                valid = false;
                return false;
            }
            break;
        }
        count++;
    }
    // Quick check that all parts of the message array were received.
    if( count == 1 ){ valid = true; return true; }
    else { valid = false; return false; }
}

/* *********************************************
/  Lightning Strike Event Class
/
/  *********************************************/

// Parse the JSON message for the strike data.
bool wfLightningStrikeEvent::ParseMsg(JsonDocument& jsonMsg){
// Check that we've good the right message components
    if( !jsonMsg.containsKey("evt") ){
        valid = false;
        return false;
    };
    // Parse the message array
    uint8_t count = 0; 
    for( JsonVariant LightningStrike : jsonMsg["evt"].as<JsonArray>() ){
        switch(count){
            case 0:{
                this->ulTimeEpoch = LightningStrike.as<unsigned long>();
            }
            break;
            case 1:{
                this->fDistance = LightningStrike.as<float>();
            }
            break;
            case 2:{
                this->uiEnergy = LightningStrike.as<unsigned int>();
            }
            break;
            default:{
                valid = false;
                return false;
            }
            break;
        }
        count++;
    }
    // Quick check that all parts of the message array were received.
    if( count == 3 ){ valid = true; return true; }
    else { valid = false; return false; }
}

// Returns the distance to the strike in one of the following units:
// Imperial - miles
// Metric - kilometers
float wfLightningStrikeEvent::Distance(void){
    switch (units){
        case Imperial:
            return fDistance / 1.609;
            break;
        
        default:  // Metric
            return fDistance;
            break;
    }
}

/* *********************************************
/  Rapid Wind Class
/
/  *********************************************/

// Parse the JSON message for the rapid wind data.
bool wfRapidWind::ParseMsg(JsonDocument& jsonMsg){
    // Check that we've good the right message components
    if( !jsonMsg.containsKey("ob") ){
        valid = false;
        return false;
    };
    // Parse the message array
    uint8_t count = 0; 
    for( JsonVariant RapidWind : jsonMsg["ob"].as<JsonArray>() ){
        switch(count){
            case 0:{
                this->ulTimeEpoch = RapidWind.as<unsigned long>();
            }
            break;
            case 1:{
                this->fWindSpeed = RapidWind.as<float>();
            }
            break;
            case 2:{
                this->uiWindDirection = RapidWind.as<float>();
            }
            break;
            default:{
                valid = false;
                return false;
            }
            break;
        }
        count++;
    }
    // Quick check that all parts of the message array were received.
    if( count == 3 ){ valid = true; return true; }
    else { valid = false; return false; }
}

// Returns the wind speed in one of the following units:
// Imperial - miles per hour
// Metric - meters per second
float wfRapidWind::WindSpeed(){
    switch(units){
        case Imperial:
            return fWindSpeed * 2.237;
            break;
            
        default: // Metric
            return fWindSpeed;
            break;
    }
}


/* *********************************************
/  Observation Tempest Class
/
/  *********************************************/
void wfObservationTempest::init(void){
    ulTimeEpoch = 0;
    fWindLull = 0;
    fWindAverage = 0;
    fWindGust = 0;
    uiWindDirection = 0;
    uiWindSampleInterval = 0;
    fStationPressure = 0;
    fAirTemp = 0;
    fRelativeHumidity = 0;
    fIlluminance = 0;
    uiUv = 0;
    fSolarRadiation = 0;
    fRainOverPrevMin = 0;
    uiParticipationType = 0;
    fLightningStrikeAvgDistance = 0;
    uiLightningStrikeCount = 0;
    fBatteryVoltage = 0;
    uiReportInterval = 0;
    valid = false;
}

// Parse the JSON message for the station data.
bool wfObservationTempest::ParseMsg(JsonDocument& jsonMsg){
    // Check that we've good the right message components
    if( !jsonMsg.containsKey("obs") ){
        valid = false;
        return false;
    };
    // Parse the message array
    uint8_t count = 0; 
    for( JsonVariant TempestObs : jsonMsg["obs"][0].as<JsonArray>() ){
        switch(count){
            case 0: this->ulTimeEpoch = TempestObs.as<unsigned long>();
            break;
            case 1: this->fWindLull = TempestObs.as<float>();
            break;
            case 2: this->fWindAverage = TempestObs.as<float>();
            break;
            case 3: this->fWindGust = TempestObs.as<float>();
            break;
            case 4: this->uiWindDirection = TempestObs.as<unsigned int>();
            break;
            case 5: this->uiWindSampleInterval = TempestObs.as<unsigned int>();
            break;
            case 6: this->fStationPressure = TempestObs.as<float>();
            break;
            case 7: this->fAirTemp = TempestObs.as<float>();
            break;
            case 8: this->fRelativeHumidity = TempestObs.as<float>();
            break;
            case 9: this->fIlluminance = TempestObs.as<float>();
            break;
            case 10: this->uiUv = TempestObs.as<unsigned int>();
            break;
            case 11: this->fSolarRadiation = TempestObs.as<float>();
            break;
            case 12: this->fRainOverPrevMin = TempestObs.as<float>();
            break;
            case 13: this->uiParticipationType = TempestObs.as<unsigned int>();
            break;
            case 14: this->fLightningStrikeAvgDistance = TempestObs.as<float>();
            break;
            case 15: this->uiLightningStrikeCount = TempestObs.as<unsigned int>();
            break;
            case 16: this->fBatteryVoltage = TempestObs.as<float>();
            break;
            case 17: this->uiReportInterval = TempestObs.as<unsigned int>();
            break;
            default:{
                valid = false;
                return false;
            }
            break;
        }
        count++;
    }
    // Quick check that all parts of the message array were received.
    if( count == 18 ){ valid = true; return true; }
    else { valid = false; return false; }
}

// Average Wind Speed in one of the following units:
// Imperial - miles per hour
// Metric - meters per second
float wfObservationTempest::WindAverage(void){
    switch (units){
        case Imperial:
            return fWindAverage * 2.237; 
            break;
        default:  // Metric
            return fWindAverage;
            break;
    }
}

// Wind Lull Speed in one of the following units:
// Imperial - miles per hour
// Metric - meters per second
float wfObservationTempest::WindLull(void){
    switch (units){
        case Imperial:
            return fWindLull * 2.237; 
            break;
        default:  // Metric
            return fWindLull;
            break;
    }
}

// Wind Gust Speed in one of the following units:
// Imperial - miles per hour
// Metric - meters per second
float wfObservationTempest::WindGust(void){
    switch (units){
        case Imperial:
            return fWindGust * 2.237; 
            break;
        default:  // Metric
            return fWindGust;
            break;
    }
}

// Station Pressure in one fo the following units:
// Imperial - inches of mercury
// Metric - millibar
float wfObservationTempest::StationPressure(void){
    switch (units){
        case Imperial:
            return fStationPressure / 33.864; 
            break;
        default:  // Metric
            return fStationPressure;
            break;
    }
}

// Air Temperature in one fo the following units:
// Imperial - Fahrenheit
// Metric - Celsius
float wfObservationTempest::AirTemperature(void){
    switch (units){
        case Imperial:
            return (fAirTemp * (9/5)) + 32; 
            break;
        default:  // Metric
            return fAirTemp;
            break;
    }
}

// Illuminance in one of the following units:
// Imperial - foot-candle
// Metric - Lux
float wfObservationTempest::Illuminance(void){
    switch (units){
        case Imperial:
            return fIlluminance / 10.764; 
            break;
        default:  // Metric
            return fIlluminance;
            break;
    }
}

// Solar Radiation in one of the following units:
// Imperial - Watts per square foot
// Metric - Watts per square meter
float wfObservationTempest::SolarRadiation(void){
    switch (units){
        case Imperial:
            return fSolarRadiation / 10.764; 
            break;
        default:  // Metric
            return fSolarRadiation;
            break;
    }
}

// Rain amount observed over the past one minute in the following:
// Imperial - inches
// Metric - millimeters
float wfObservationTempest::RainOverPreviousMinute(void){
    switch (units){
        case Imperial:
            return fRainOverPrevMin / 25.4; 
            break;
        default:  // Metric
            return fRainOverPrevMin;
            break;
    }
}

// Average distance to lightning strikes in the following units:
// Imperial - miles
// Metric - kilometers
float wfObservationTempest::LightningStrikeAverageDistance(void){
    switch (units){
        case Imperial:
            return fLightningStrikeAvgDistance / 1.609; 
            break;
        default:  // Metric
            return fLightningStrikeAvgDistance;
            break;
    }
}


/* *********************************************
/  Status Hub Class
/
/  *********************************************/

// Parse the JSON message for the hub status data.
bool wfStatusHub::ParseMsg(JsonDocument& jsonMsg){
    // FIXME: implement this function...
    return false;
}

/* *********************************************
/  Main WeatherFlow Class
/
/  *********************************************/
void WeatherFlow::init(void){
    myRainStartEvent = new wfRainStartEvent(this->units);
    myLightningStrikeEvent = new wfLightningStrikeEvent(this->units);
    myRapidWind = new wfRapidWind(this->units);
    myObservationTempest = new wfObservationTempest(this->units);
    myStatusHub = new wfStatusHub(this->units);
    UDPrcvr = new WiFiUDP();
}

WeatherFlow::~WeatherFlow(){
    UDPrcvr->stop();
    delete UDPrcvr;
    delete myRainStartEvent;
    delete myLightningStrikeEvent;
    delete myRapidWind;
    delete myObservationTempest;
    delete myStatusHub;
}

// Set up the UDP receiver, listening for the broadcast 
// UDP packets. Returns true on successful startup.
bool WeatherFlow::Begin(void){
    // Start listening for the UDP broadcast messages from the 
    // WeatherFlow systems.
    return UDPrcvr->begin(50222);
}

// Receiver Loop function, should be called once per loop to handle
// receipt and processing of one or more UDP packets.  Returns true
// if a UDP packet was received and processed, indicating the state
// has been updated in one or more 
bool WeatherFlow::ReceiveLoop(void){
    char chUdpBuffer[UDPRCRVSIZE]= {0};
    //DynamicJsonDocument jsonMsg(UDPRCRVSIZE);
    StaticJsonDocument<UDPRCRVSIZE> jsonMsg;

    // Check if we received a packet
    if( UDPrcvr->parsePacket() ){
        
        // Pull the packet from the buffer, and begin parsing the JSON message
        UDPrcvr->read(chUdpBuffer, UDPRCRVSIZE);
        if( deserializeJson(jsonMsg, chUdpBuffer) == DeserializationError::Ok){
            // Select the message type to parse        
            switch ( mapMsgTypes.find(std::string(jsonMsg["type"].as<const char*>()))->second ){
                case evt_precip: return this->myRainStartEvent->ParseMsg(jsonMsg);
                break;
                case evt_strike: return this->myLightningStrikeEvent->ParseMsg(jsonMsg);
                break;
                case rapid_wind: return this->myRapidWind->ParseMsg(jsonMsg);
                break;
                case obs_st: return this->myObservationTempest->ParseMsg(jsonMsg);                
                break;
                case hub_status: return this->myStatusHub->ParseMsg(jsonMsg);
                break;
                // FIXME: Deal with other message types
                default: return false;
                break;
            }
        }
    }
    return false;
}
