/**
 * WeatherFlow Tempest UDP API v171
 * 
 * Copyright: Lincoln Lavoie, lincoln.lavoie@gmail.com, 2022
 * 
 * License: Apache License v2.0
 * 
 * Implements a UDP receiver for WeatherFlow Tempest UDP messages
 * for the Tempest, Sky, and AIR weather stations and receiviers.
 * 
 * Usage:
 * 1. Begin() method should be called once to setup the UDP receiver,
 *    only after the Wi-Fi connect has been established.
 * 2. ReceiveLoop() method should be called within the main Loop() to,
 *    check for new UDP messages.  The return code from this method
 *    can be used to determine is new data is available.
 * 3. Sub-classes are available for each message type that may be
 *    received.  Each class included a read-only Valid() property
 *    that will be set to true once the object contains value data.
 * 4. Changing the units of the WeatherFlow() object will reset all
 *    available data (i.e. all Valid() properties will be set to 
 *    false for all sub-classes).
 *
 * 
 * Notes:
 * 1. Class design should allow for future version of the API, assuming
 *    only new parameters are added and new values are only added to the
 *    end of the array.
 * 2. The design assumes only 1 reporting station of each time (i.e. two 
 *    Tempest units connected to 1 Hub is not currenly supported).
 * 3. The design assumes the complete JSON message payload is contained
 *    in a single UDP payload (i.e. no message fragmentation).
 * 4. If constructed with the default WeatherFlow(), the units are set to
 *    Imperial (US) units.
 * 
 */

#include "wf.h"
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <map>

/* *********************************************
/  Rain Start Event Class
/
/  *********************************************/
wfRainStartEvent::wfRainStartEvent(){ 
    ulTimeEpoch = 0;
    units = Imperial;
    valid = false;
    strSerialNumber.clear();
    strHubSerialNumber.clear();
}

wfRainStartEvent::wfRainStartEvent(wfUnits u){
    units = u;
    ulTimeEpoch = 0;
    valid = false;
    strSerialNumber.clear();
    strHubSerialNumber.clear(); 
}

// Time of the rain start event, in Epoch seconds.
time_t wfRainStartEvent::EpochTime(void){ return ulTimeEpoch; };

// Event data is valid.
bool wfRainStartEvent::Valid(void){ return valid; };
        
// Serial number of the reporting station, as a String.
String wfRainStartEvent::SerialNumber(void){ return strSerialNumber; };
        
// Serial number of the receiving hub, as a String.
String wfRainStartEvent::HubSerialNumber(void){ return strHubSerialNumber; };

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
                this->ulTimeEpoch = RainStart.as<time_t>();
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
    if( count >= 1 ){ valid = true; return true; }
    else { valid = false; return false; }
}




/* *********************************************
/  Lightning Strike Event Class
/
/  *********************************************/
wfLightningStrikeEvent::wfLightningStrikeEvent(){ 
    units = Imperial;
    uiEnergy = 0;
    fDistance = 0;
    valid = false;
    strSerialNumber.clear();
    strHubSerialNumber.clear();
}

wfLightningStrikeEvent::wfLightningStrikeEvent(wfUnits u){ 
    units = u;
    uiEnergy = 0;
    fDistance = 0;
    valid = false;
    strSerialNumber.clear();
    strHubSerialNumber.clear();
}

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
                this->ulTimeEpoch = LightningStrike.as<time_t>();
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
    if( count >= 3 ){ valid = true; return true; }
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

// Time of the strike event, in Epoch seconds.
time_t wfLightningStrikeEvent::EpochTime(void){ return ulTimeEpoch; }

// Detected energy of the strike
unsigned int wfLightningStrikeEvent::Energy(void){ return uiEnergy; }

// Event data is valid.
bool wfLightningStrikeEvent::Valid(void){ return valid; }

// Serial number of the reporting station, as a String.
String wfLightningStrikeEvent::SerialNumber(void){ return strSerialNumber; };
        
// Serial number of the receiving hub, as a String.
String wfLightningStrikeEvent::HubSerialNumber(void){ return strHubSerialNumber; };




/* *********************************************
/  Rapid Wind Class
/
/  *********************************************/
wfRapidWind::wfRapidWind(){
    units = Imperial;
    ulTimeEpoch = 0;
    fWindSpeed = 0;
    uiWindDirection = 0;
     valid = false;
     strSerialNumber.clear();
     strHubSerialNumber.clear();
}

wfRapidWind::wfRapidWind(wfUnits u){
    units = u;
    ulTimeEpoch = 0;
    fWindSpeed = 0;
    uiWindDirection = 0;
     valid = false;
     strSerialNumber.clear();
     strHubSerialNumber.clear();
}

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
                this->ulTimeEpoch = RapidWind.as<time_t>();
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
    if( count >= 3 ){ valid = true; return true; }
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

// Wind direction in degrees
unsigned int wfRapidWind::WindDirection(void){ return uiWindDirection; };

// Time of the strike event, in Epoch seconds.
time_t wfRapidWind::EpochTime(void){ return ulTimeEpoch; };

// Event data is valid.
bool wfRapidWind::Valid(void){ return valid; };

// Serial number of the reporting station, as a String.
String wfRapidWind::SerialNumber(void){ return strSerialNumber; };
        
// Serial number of the receiving hub, as a String.
String wfRapidWind::HubSerialNumber(void){ return strHubSerialNumber; };




/* *********************************************
/  Observation AIR Class
/
/  *********************************************/
wfObservationAir::wfObservationAir(){
    units = Imperial;
    this->init();
}

wfObservationAir::wfObservationAir(wfUnits u){
    units = u;
    this->init();
}

void wfObservationAir::init(void){
    strSerialNumber.clear() ;
    strHubSerialNumber.clear();
    uiFirmwareVersion = 0;
    ulTimeEpoch = 0;
    fStationPressure = 0;
    fAirTemp = 0;
    fRelativeHumidity = 0;
    uiLightningStrikeCount =0;
    fLightningStrikeAvgDistance = 0;
    fBatteryVoltage = 0;
    uiReportInterval = 0;
    valid = false;
}

// Parse the JSON message for the station data.
bool wfObservationAir::ParseMsg(JsonDocument& jsonMsg){
    // Check that we've good the right message components
    if( !jsonMsg.containsKey("obs") ){
        valid = false;
        return false;
    };

    // Station info (serial numbers, firmware, etc.)
    strSerialNumber = jsonMsg["serial_number"].as<String>();
    strHubSerialNumber = jsonMsg["hub_sn"].as<String>();
    uiFirmwareVersion = jsonMsg["firmware_revision"].as<unsigned int>();


    // Parse the message array
    uint8_t count = 0; 
    for( JsonVariant AirObs : jsonMsg["obs"][0].as<JsonArray>() ){
        switch(count){
            case 0: this->ulTimeEpoch = AirObs.as<time_t>();
            break;
            case 1: this->fStationPressure = AirObs.as<float>();
            break;
            case 2: this->fAirTemp = AirObs.as<float>();
            break;
            case 3: this->fRelativeHumidity = AirObs.as<float>();
            break;
            case 4: this->uiLightningStrikeCount = AirObs.as<unsigned int>();
            break;
            case 5: this->fLightningStrikeAvgDistance = AirObs.as<float>();
            break;
            case 6: this->fBatteryVoltage = AirObs.as<float>();
            break;
            case 7: this->uiReportInterval = AirObs.as<unsigned int>();
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
    if( count >= 8 ){ valid = true; return true; }
    else { valid = false; return false; }
}

// Station Pressure in one fo the following units:
// Imperial - inches of mercury
// Metric - millibar
float wfObservationAir::StationPressure(void){
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
float wfObservationAir::AirTemperature(void){
    switch (units){
        case Imperial:
            return (fAirTemp * (9/5)) + 32; 
            break;
        default:  // Metric
            return fAirTemp;
            break;
    }
}

// Average distance to lightning strikes in the following units:
// Imperial - miles
// Metric - kilometers
float wfObservationAir::LightningStrikeAvgDistance(void){
    switch (units){
        case Imperial:
            return fLightningStrikeAvgDistance / 1.609; 
            break;
        default:  // Metric
            return fLightningStrikeAvgDistance;
            break;
    }
}

// Observation data is valid.
bool wfObservationAir::Valid(void){ return valid; };

// Time of the observatin in Epoch seconds.
time_t wfObservationAir::EpochTime(void){ return ulTimeEpoch; };

// Relative humidity as a percentage.
float wfObservationAir::RelativeHumidity(void){ return fRelativeHumidity; };

// Lightening strike count as an unsigned integer
unsigned int wfObservationAir::LightningStrikeCount(void){ return uiLightningStrikeCount; };

// Station battery voltage as a float.
float wfObservationAir::BatteryVoltage(void){ return fBatteryVoltage; };

// Report interval of of the data in the observation in seconds as unsigned integer.
unsigned int wfObservationAir::ReportInterval(void){ return uiReportInterval; };

// Serial number of the reporting station, as a String.
String wfObservationAir::SerialNumber(void){ return strSerialNumber; };
        
// Serial number of the receiving hub, as a String.
String wfObservationAir::HubSerialNumber(void){ return strHubSerialNumber; };

// Firmware version of the station as an unsigned integer
unsigned int wfObservationAir::FirmwareVersion(void){ return uiFirmwareVersion; };




/* *********************************************
/  Observation Tempest Class
/
/  *********************************************/
wfObservationSky::wfObservationSky(){
    units = Imperial;
    this->init();
}

wfObservationSky::wfObservationSky(wfUnits u){
    units = u;
    this->init();
}

void wfObservationSky::init(void){
    strSerialNumber.clear();
    strHubSerialNumber.clear();
    uiFirmwareVersion = 0;
    ulTimeEpoch = 0;
    fIlluminance = 0;
    uiUv = 0 ;
    fRainOverPrevMin = 0;
    fWindLull = 0;
    fWindAverage = 0;
    fWindGust = 0;
    uiWindDirection = 0;
    fBatteryVoltage = 0;
    uiReportInterval = 0;
    fSolarRadiation = 0;
    fLocalDayRainAccumulation = 0;
    uiParticipationType = 0;
    uiWindSampleInterval = 0;
    valid = false;
}

// Parse the JSON message for the station data.
bool wfObservationSky::ParseMsg(JsonDocument& jsonMsg){
    // Check that we've good the right message components
    if( !jsonMsg.containsKey("obs") ){
        valid = false;
        return false;
    };

    // Station info (serial numbers, firmware, etc.)
    strSerialNumber = jsonMsg["serial_number"].as<String>();
    strHubSerialNumber = jsonMsg["hub_sn"].as<String>();
    uiFirmwareVersion = jsonMsg["firmware_revision"].as<unsigned int>();

    // Parse the message array
    uint8_t count = 0; 
    for( JsonVariant SkyObs : jsonMsg["obs"][0].as<JsonArray>() ){
        switch(count){
            case 0: this->ulTimeEpoch = SkyObs.as<time_t>();
            break;
            case 1: this->fIlluminance = SkyObs.as<float>();
            break;
            case 2: this->uiUv = SkyObs.as<unsigned int>();
            break;
            case 3: this->fRainOverPrevMin = SkyObs.as<float>();
            break;
            case 4: this->fWindLull = SkyObs.as<float>();
            break;
            case 5: this->fWindAverage = SkyObs.as<float>();
            break;
            case 6: this->fWindGust = SkyObs.as<float>();
            break;
            case 7: this->uiWindDirection = SkyObs.as<unsigned int>();
            break;
            case 8: this->fBatteryVoltage = SkyObs.as<float>();
            break;
            case 9: this->uiReportInterval = SkyObs.as<unsigned int>();
            break;
            case 10: this->fSolarRadiation = SkyObs.as<float>();
            break;
            case 11: this->fLocalDayRainAccumulation = SkyObs.as<float>();
            break;
            case 12: this->uiParticipationType = SkyObs.as<unsigned int>();
            break;
            case 13: this->uiWindSampleInterval = SkyObs.as<unsigned int>();
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
    if( count >= 14 ){ valid = true; return true; }
    else { valid = false; return false; }
}

// Illuminance in one of the following units:
// Imperial - foot-candle
// Metric - Lux
float wfObservationSky::Illuminance(void){
    switch (units){
        case Imperial:
            return fIlluminance / 10.764; 
            break;
        default:  // Metric
            return fIlluminance;
            break;
    }
}

// Rain amount observed over the past one minute in the following:
// Imperial - inches
// Metric - millimeters
float wfObservationSky::RainOverPreviousMinute(void){
    switch (units){
        case Imperial:
            return fRainOverPrevMin / 25.4; 
            break;
        default:  // Metric
            return fRainOverPrevMin;
            break;
    }
}

// Average Wind Speed in one of the following units:
// Imperial - miles per hour
// Metric - meters per second
float wfObservationSky::WindAverage(void){
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
float wfObservationSky::WindLull(void){
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
float wfObservationSky::WindGust(void){
    switch (units){
        case Imperial:
            return fWindGust * 2.237; 
            break;
        default:  // Metric
            return fWindGust;
            break;
    }
}

// Solar Radiation in one of the following units:
// Imperial - Watts per square foot
// Metric - Watts per square meter
float wfObservationSky::SolarRadiation(void){
    switch (units){
        case Imperial:
            return fSolarRadiation / 10.764; 
            break;
        default:  // Metric
            return fSolarRadiation;
            break;
    }
}

// Rain amount observed over the past local day:
// Imperial - inches
// Metric - millimeters
float wfObservationSky::LocalDayRainAccumulation(void){
    switch (units){
        case Imperial:
            return fLocalDayRainAccumulation / 25.4; 
            break;
        default:  // Metric
            return fLocalDayRainAccumulation;
            break;
    }
}

// Observation data is valid.
bool wfObservationSky::Valid(void){ return valid; };

// Time of the observatin in Epoch seconds.
time_t wfObservationSky::EpochTime(void){ return ulTimeEpoch; };

// Observed UV index as unsigned integer.
unsigned int wfObservationSky::UV(void){ return uiUv; };

// Observed wind direction in degrees as unsigned integer.
unsigned int wfObservationSky::WindDirection(void){ return uiWindDirection; };

// Station battery voltage as float.
float wfObservationSky::BatteryVoltage(void){ return fBatteryVoltage; };

// Report interval for this observation in seconds as unsigned integer.
unsigned int wfObservationSky::ReportInterval(void){ return uiReportInterval; };

// Observed participation type
wfObservationSky::PartType wfObservationSky::ParticipationType(void){ return static_cast<PartType>(uiParticipationType); };

// Wind sample interval in seconds as unsigned integer.
unsigned long wfObservationSky::WindSampleInterval(void){ return uiWindSampleInterval; };

// Reporting station serial number as string.
String wfObservationSky::SerialNumber(void){ return strSerialNumber; };

// Receiving hub serial number as string. 
String wfObservationSky::HubSerialNumber(void){ return strHubSerialNumber; };

// Station firmware version as unsigned integer.
unsigned int wfObservationSky::FirmwareVersion(void){ return uiFirmwareVersion; };




/* *********************************************
/  Observation Tempest Class
/
/  *********************************************/
wfObservationTempest::wfObservationTempest(){
    units = Imperial;
    this->init();
}

wfObservationTempest::wfObservationTempest(wfUnits u){
    units = u;
    this->init();
}

void wfObservationTempest::init(void){
    strSerialNumber.clear();
    strHubSerialNumber.clear();
    uiFirmwareVersion = 0;
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

    // Station info (serial numbers, firmware, etc.)
    strSerialNumber = jsonMsg["serial_number"].as<String>();
    strHubSerialNumber = jsonMsg["hub_sn"].as<String>();
    uiFirmwareVersion = jsonMsg["firmware_revision"].as<unsigned int>();

    // Parse the message array
    uint8_t count = 0; 
    for( JsonVariant TempestObs : jsonMsg["obs"][0].as<JsonArray>() ){
        switch(count){
            case 0: this->ulTimeEpoch = TempestObs.as<time_t>();
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
    if( count >= 18 ){ valid = true; return true; }
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

// Observation data is valid
bool wfObservationTempest::Valid(void){ return valid; };

// Observation time as Epoch seconds
time_t wfObservationTempest::EpochTime(void){ return ulTimeEpoch; };

// Observed wind direction in degrees as unsigned integer.
unsigned int wfObservationTempest::WindDirection(void){ return uiWindDirection; };

// Wind sample interval in seconds as unsigned integer.
unsigned long wfObservationTempest::WindSampleInterval(void){ return uiWindSampleInterval; };

// Observed relative humidity percentage as float.
float wfObservationTempest::RelativeHumidity(void){ return fRelativeHumidity; };

// Observed UV index as unsigned integer.
unsigned int wfObservationTempest::UV(void){ return uiUv; };

// Observed participation type.
wfObservationTempest::PartType wfObservationTempest::ParticipationType(void){ return static_cast<PartType>(uiParticipationType); };

// Observed lightning strike count as unsigned integer.
unsigned int wfObservationTempest::LightningStrikeCount(void){ return uiLightningStrikeCount; };

// Station battery voltage as float.
float wfObservationTempest::BatteryVoltage(void){ return fBatteryVoltage; };

// Report interval in seconds as unsigned seconds. 
unsigned int wfObservationTempest::ReportInterval(void){ return uiReportInterval; };

// Reporting station serial number as String.
String wfObservationTempest::SerialNumber(void){ return strSerialNumber; };

// Receiving hub serial number as String.
String wfObservationTempest::HubSerialNumber(void){ return strHubSerialNumber; };

// Station firmware number as unsigned integer.
unsigned int wfObservationTempest::FirmwareVersion(void){ return uiFirmwareVersion; };




/* *********************************************
/  Status Device Class
/
/  *********************************************/
wfDeviceStatus::wfDeviceStatus(){
    strSerialNumber.clear();
    strHubSerialNumber.clear();
    uiFirmwareVersion = 0;
    ulTimeStamp = 0;
    ulUptime = 0;
    uiVoltage = 0;
    iRssi = -999;
    iHubRssi = -999;
    uiSensorStatus = 0;
    uiDebug = 0;
    valid = false;
}

// Parse the JSON message for the device status data.
bool wfDeviceStatus::ParseMsg(JsonDocument& jsonMsg){
    strSerialNumber = jsonMsg["serial_number"].as<String>();
    strHubSerialNumber = jsonMsg["hub_sn"].as<String>();
    ulTimeStamp = jsonMsg["timestamp"].as<time_t>();
    ulUptime = jsonMsg["uptime"].as<time_t>();
    uiVoltage = jsonMsg["voltage"].as<unsigned int>();
    uiFirmwareVersion = jsonMsg["firmware_revision"].as<unsigned int>();
    iRssi = jsonMsg["rssi"].as<int>();
    iHubRssi = jsonMsg["hub_rssi"].as<int>();
    uiSensorStatus = jsonMsg["sensor_status"].as<unsigned int>();
    uiDebug = jsonMsg["debug"].as<unsigned int>();
    valid = true;
    return valid;
}

// Status data is valid.
bool wfDeviceStatus::Valid(void){ return valid; }

// Reporting station serial number as String.
String wfDeviceStatus::SerialNumber(void){ return strSerialNumber; };

// Receiving hub serial number as String.
String wfDeviceStatus::HubSerialNumber(void){ return strHubSerialNumber; };

// Station firmware version as unsigned integer.
unsigned int wfDeviceStatus::FirmwareVersion(void){ return uiFirmwareVersion; };

// Status time stamp as Epoch seconds.
time_t wfDeviceStatus::TimeStamp(void){ return ulTimeStamp; };

// Station uptime in seconds.
time_t wfDeviceStatus::Uptime(void){ return ulUptime; };

// Station battery voltage as unsigned integer.
unsigned int wfDeviceStatus::Voltage(void){ return uiVoltage; };

// Station wireless RSSI as integer.
int wfDeviceStatus::RSSI(void){ return iRssi; };

// Hub wireless RSSI as integer.
int wfDeviceStatus::HubRSSI(void){ return iHubRssi; };

// Station sensor status
unsigned int wfDeviceStatus::SensorStatus(void){ return uiSensorStatus; };

// Station debug status
wfDeviceStatus::DebugStatus wfDeviceStatus::Debug(void){ return static_cast<DebugStatus>(uiDebug); };




/* *********************************************
/  Status Hub Class
/
/  *********************************************/
wfHubStatus::wfHubStatus(){
    strHubSerialNumber.clear();
    strFirmwareVersion.clear();
    ulTimeStamp = 0;
    ulUptime = 0;
    iRssi = -999;
    strResetFlags.clear();
    uiSequence = 0;
    valid = false;
}

// Parse the JSON message for the hub status data.
bool wfHubStatus::ParseMsg(JsonDocument& jsonMsg){
    strHubSerialNumber = jsonMsg["serial_number"].as<String>();
    strFirmwareVersion = jsonMsg["firmware_revision"].as<String>();
    ulTimeStamp = jsonMsg["timestamp"].as<time_t>();
    ulUptime = jsonMsg["uptime"].as<time_t>();
    iRssi = jsonMsg["rssi"].as<int>();
    strResetFlags = jsonMsg["reset_flags"].as<String>();
    uiSequence = jsonMsg["seq"].as<unsigned int>();
    valid = true;
    return valid;
}

// Hub serial number as String.
String wfHubStatus::HubSerialNumber(void){ return strHubSerialNumber; };

// Hub firmware version as String.
String wfHubStatus::FirmwareVersion(void){ return strFirmwareVersion; };

// Status time stamp in Epoch seconds.
time_t wfHubStatus::TimeStamp(void){ return ulTimeStamp; };

// Hub uptime in seconds.
time_t wfHubStatus::Uptime(void){ return ulUptime; };

// Hub last reset cause, comma separated string.
String wfHubStatus::ResetFlags(void){ return strResetFlags; };

// Sequence number as unsigned integer.
unsigned int wfHubStatus::Sequence(void){ return uiSequence; };

// Hub wireless RSSI as integer.
int wfHubStatus::RSSI(void){ return iRssi; };

// Hub status data is valud.
bool wfHubStatus::Valid(void){ return valid; };



/* *********************************************
/  Main WeatherFlow Class
/
/  *********************************************/
WeatherFlow::WeatherFlow(){
    units = Imperial;
    this->init();
}

WeatherFlow::WeatherFlow(wfUnits u){
    units = u;
    this->init();
}

void WeatherFlow::init(void){
    myRainStartEvent = new wfRainStartEvent(this->units);
    myLightningStrikeEvent = new wfLightningStrikeEvent(this->units);
    myRapidWind = new wfRapidWind(this->units);
    myObservationAir = new wfObservationAir(this->units);
    myObservationSky = new wfObservationSky(this->units);
    myObservationTempest = new wfObservationTempest(this->units);
    myDeviceStatus = new wfDeviceStatus();
    myHubStatus = new wfHubStatus();
    UDPrcvr = new WiFiUDP();
}

WeatherFlow::~WeatherFlow(){
    UDPrcvr->stop();
    delete UDPrcvr;
    delete myRainStartEvent;
    delete myLightningStrikeEvent;
    delete myRapidWind;
    delete myObservationAir;
    delete myObservationSky;
    delete myObservationTempest;
    delete myDeviceStatus;
    delete myHubStatus;
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
                case obs_air: return this->myObservationAir->ParseMsg(jsonMsg);
                break;
                case obs_sky: return this-myObservationSky->ParseMsg(jsonMsg);
                break;
                case obs_st: return this->myObservationTempest->ParseMsg(jsonMsg);                
                break;
                case device_status: return this->myDeviceStatus->ParseMsg(jsonMsg);
                break;
                case hub_status: return this->myHubStatus->ParseMsg(jsonMsg);
                break;
                default: return false;
                break;
            }
        }
    }
    return false;
}

// Change the current units of the WeatherFlow objects and sub-classes.
// Note, this clears all currently received data from the sub-classes.
void WeatherFlow::SetUnits(wfUnits u){
    delete myRainStartEvent;
    delete myLightningStrikeEvent;
    delete myRapidWind;
    delete myObservationAir;
    delete myObservationSky;
    delete myObservationTempest;

    this->units = u;

    myRainStartEvent = new wfRainStartEvent(this->units);
    myLightningStrikeEvent = new wfLightningStrikeEvent(this->units);
    myRapidWind = new wfRapidWind(this->units);
    myObservationAir = new wfObservationAir(this->units);
    myObservationSky = new wfObservationSky(this->units);
    myObservationTempest = new wfObservationTempest(this->units);
}

// Current units of the WeatherFlow object data.
wfUnits WeatherFlow::GetUnits(void){ return units; };

// Latest rain start event data.
wfRainStartEvent WeatherFlow::RainStartEvent(void){ return *myRainStartEvent; };

// Latest lightning strike event data.
wfLightningStrikeEvent WeatherFlow::LightningStrikeEvent(void){ return *myLightningStrikeEvent; };

// Latest rapid wind data.
wfRapidWind WeatherFlow::RapidWind(void){ return *myRapidWind; }

// Latest AIR station observation data.
wfObservationAir WeatherFlow::ObservationAir(void){ return *myObservationAir; };

// Latest SKY station observation data.
wfObservationSky WeatherFlow::ObservationSky(void){ return *myObservationSky; };

// Latest Tempest station observation data.
wfObservationTempest WeatherFlow::ObservationTempest(void){ return *myObservationTempest; };

// Latest device status data.
wfDeviceStatus WeatherFlow::DeviceStatus(void){ return *myDeviceStatus; };

// Latest hub status data.
wfHubStatus WeatherFlow::HubStatus(void){ return *myHubStatus; };