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

#ifndef __WeatherFlow__
#define __WeatherFlow__

#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <map>

#define UDPRCRVSIZE 1460
#define SN_SIZE 20

enum wfUnits{
    Imperial,
    Metric
};

class wfRainStartEvent{
    private:
        String strSerialNumber;
        String strHubSerialNumber;
        time_t ulTimeEpoch;
        wfUnits units;
        bool valid;
    public: 
        wfRainStartEvent();
        wfRainStartEvent(wfUnits);
        bool ParseMsg(JsonDocument&);
        time_t EpochTime(void);
        bool Valid(void);
        String SerialNumber(void);
        String HubSerialNumber(void);
};

class wfLightningStrikeEvent{
    private:
        String strSerialNumber;
        String strHubSerialNumber;
        time_t ulTimeEpoch;
        float fDistance;
        unsigned int uiEnergy;
        wfUnits units;
        bool valid;
    public:
        wfLightningStrikeEvent();
        wfLightningStrikeEvent(wfUnits);
        bool ParseMsg(JsonDocument&);
        time_t EpochTime(void);
        unsigned int Energy(void);
        bool Valid(void);
        float Distance(void);
        String SerialNumber(void);
        String HubSerialNumber(void);
};

class wfRapidWind{
    private:
        String strSerialNumber;
        String strHubSerialNumber;
        time_t ulTimeEpoch;
        float fWindSpeed;
        unsigned int uiWindDirection;
        wfUnits units;
        bool valid;
    public:
        wfRapidWind();
        wfRapidWind(wfUnits);
        bool ParseMsg(JsonDocument&);
        float WindSpeed(void);
        unsigned int WindDirection(void);
        time_t EpochTime(void);
        bool Valid(void);
        String SerialNumber(void);
        String HubSerialNumber(void);
};

class wfObservationAir{
    private:
        wfUnits units;
        String strSerialNumber;
        String strHubSerialNumber;
        unsigned int uiFirmwareVersion;
        time_t ulTimeEpoch;
        float fStationPressure;
        float fAirTemp;
        float fRelativeHumidity;
        unsigned int uiLightningStrikeCount;
        float fLightningStrikeAvgDistance;
        float fBatteryVoltage;
        unsigned int uiReportInterval;
        bool valid;
        void init(void);
    public:
        wfObservationAir();
        wfObservationAir(wfUnits u);
        bool ParseMsg(JsonDocument&);
        bool Valid(void);
        time_t EpochTime(void);
        float StationPressure(void);
        float AirTemperature(void);
        float RelativeHumidity(void);
        unsigned int LightningStrikeCount(void);
        float LightningStrikeAvgDistance(void);
        float BatteryVoltage(void);
        unsigned int ReportInterval(void);
        String SerialNumber(void);
        String HubSerialNumber(void);
        unsigned int FirmwareVersion(void);
};

class wfObservationSky{
    private:
        wfUnits units;
        String strSerialNumber;
        String strHubSerialNumber;
        unsigned int uiFirmwareVersion;
        time_t ulTimeEpoch;
        float fIlluminance;
        unsigned int uiUv;
        float fRainOverPrevMin;
        float fWindLull;
        float fWindAverage;
        float fWindGust;
        unsigned int uiWindDirection;
        float fBatteryVoltage;
        unsigned int uiReportInterval;
        float fSolarRadiation;
        float fLocalDayRainAccumulation;
        unsigned int uiParticipationType;
        unsigned int uiWindSampleInterval;
        bool valid;
        void init(void);
    public:
        enum PartType{
            None,Rain,Hail
        };
        wfObservationSky();
        wfObservationSky(wfUnits u);
        bool ParseMsg(JsonDocument&);
        bool Valid(void);
        time_t EpochTime(void);
        float Illuminance(void);
        unsigned int UV(void);
        float RainOverPreviousMinute(void);
        float WindLull(void);
        float WindAverage(void);
        float WindGust(void);
        unsigned int WindDirection(void);
        float BatteryVoltage(void);
        unsigned int ReportInterval(void);
        float SolarRadiation(void);
        float LocalDayRainAccumulation(void);
        PartType ParticipationType(void);
        unsigned long WindSampleInterval(void);
        String SerialNumber(void);
        String HubSerialNumber(void);
        unsigned int FirmwareVersion(void);
};

class wfObservationTempest{
    private:
        String strSerialNumber;
        String strHubSerialNumber;
        unsigned int uiFirmwareVersion;
        time_t ulTimeEpoch;
        float fWindLull;
        float fWindAverage;
        float fWindGust;
        unsigned int uiWindDirection;
        unsigned int uiWindSampleInterval;
        float fStationPressure;
        float fAirTemp;
        float fRelativeHumidity;
        float fIlluminance;
        unsigned int uiUv;
        float fSolarRadiation;
        float fRainOverPrevMin;
        unsigned int uiParticipationType;
        float fLightningStrikeAvgDistance;
        unsigned int uiLightningStrikeCount;
        float fBatteryVoltage;
        unsigned int uiReportInterval;
        wfUnits units;
        bool valid;
        void init(void);
    public:
        enum PartType{
            None,Rain,Hail,RainPlusHail
        };
        wfObservationTempest();
        wfObservationTempest(wfUnits u);
        bool ParseMsg(JsonDocument&);
        bool Valid(void);
        time_t EpochTime(void);
        float WindAverage(void);
        float WindLull(void);
        float WindGust(void);
        unsigned int WindDirection(void);
        unsigned long WindSampleInterval(void);
        float StationPressure(void);
        float AirTemperature(void);
        float RelativeHumidity(void);
        float Illuminance(void);
        unsigned int UV(void);
        float SolarRadiation(void);
        float RainOverPreviousMinute(void);
        PartType ParticipationType(void);
        float LightningStrikeAverageDistance(void);
        unsigned int LightningStrikeCount(void);
        float BatteryVoltage(void);
        unsigned int ReportInterval(void);
        String SerialNumber(void);
        String HubSerialNumber(void);
        unsigned int FirmwareVersion(void);
};

class wfDeviceStatus{
    private:
        String strSerialNumber;
        String strHubSerialNumber;
        unsigned int uiFirmwareVersion;
        time_t ulTimeStamp;
        time_t ulUptime;
        unsigned int uiVoltage;
        int iRssi;
        int iHubRssi;
        unsigned int uiSensorStatus;
        unsigned int uiDebug;
        bool valid;
    public:
        enum DebugStatus{ Disabled, Enabled };
        enum SensorStatusFlags{ 
            SensorsOk =            0b000000000, 
            LightningFailed =      0b000000001,
            LightningNoise =       0b000000010,
            LightningDisturber =   0b000000100,
            PressureFailed =       0b000001000,
            TemperatureFailed =    0b000010000,
            RhFailed =             0b000100000,
            WindFailed =           0b001000000,
            PrecipitationFailed =  0b010000000,
            LightUvFailed =        0b100000000,
            PowerBoosterDepleted = 0x00008000,
            PowerBoosterShorePower = 0x00010000
        };
        wfDeviceStatus();
        bool ParseMsg(JsonDocument&);
        bool Valid(void);
        String SerialNumber(void);
        String HubSerialNumber(void);
        unsigned int FirmwareVersion(void);
        time_t TimeStamp(void);
        time_t Uptime(void);
        unsigned int Voltage(void);
        int RSSI(void);
        int HubRSSI(void);
        unsigned int SensorStatus(void);
        DebugStatus Debug(void);
};

class wfHubStatus{
    private:
        String strHubSerialNumber;
        String strFirmwareVersion;
        time_t ulTimeStamp;
        time_t ulUptime;
        int iRssi;
        String strResetFlags;
        unsigned int uiSequence;
        bool valid;
    public:
        wfHubStatus();
        bool ParseMsg(JsonDocument&);
        String HubSerialNumber(void);
        String FirmwareVersion(void);
        time_t TimeStamp(void);
        time_t Uptime(void);
        String ResetFlags(void);
        unsigned int Sequence(void);
        int RSSI(void);
        bool Valid(void);
};

class WeatherFlow{
    private:
        enum MsgType{
            evt_precip,
            evt_strike,
            rapid_wind,
            obs_air,
            obs_sky,
            obs_st,
            device_status,
            hub_status
        };
        const std::map<std::string,MsgType> mapMsgTypes { 
            {"evt_precip", evt_precip},
            {"evt_strike", evt_strike},
            {"rapid_wind", rapid_wind}, 
            {"obs_air", obs_air},
            {"obs_sky", obs_sky},
            {"obs_st", obs_st},
            {"device_status", device_status},
            {"hub_status", hub_status}
            };
        WiFiUDP *UDPrcvr;
        wfUnits units;
        wfRainStartEvent *myRainStartEvent;
        wfLightningStrikeEvent *myLightningStrikeEvent;
        wfRapidWind *myRapidWind;
        wfObservationAir *myObservationAir;
        wfObservationSky *myObservationSky;
        wfObservationTempest *myObservationTempest;
        wfDeviceStatus *myDeviceStatus;
        wfHubStatus *myHubStatus;
        void init(void);
    public:
        WeatherFlow();
        WeatherFlow(wfUnits u);
        ~WeatherFlow();
        bool Begin(void);
        bool ReceiveLoop(void);
        void SetUnits(wfUnits);
        wfUnits GetUnits(void);
        wfRainStartEvent RainStartEvent(void);
        wfLightningStrikeEvent LightningStrikeEvent(void);
        wfRapidWind RapidWind(void);
        wfObservationAir ObservationAir(void);
        wfObservationSky ObservationSky(void);
        wfObservationTempest ObservationTempest(void);
        wfDeviceStatus DeviceStatus(void);
        wfHubStatus HubStatus(void);
};    

#endif