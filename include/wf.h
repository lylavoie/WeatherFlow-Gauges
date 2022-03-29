#ifndef __WeatherFlow__
#define __WeatherFlow__

#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <map>

#define UDPRCRVSIZE 1460

enum wfUnits{
    Imperial,
    Metric
};

class wfRainStartEvent{
    private:
        unsigned long ulTimeEpoch;
        wfUnits units;
        bool valid;
    public: 
        wfRainStartEvent(){ ulTimeEpoch = 0; units = Imperial; valid = false; }
        wfRainStartEvent(wfUnits u){ units = u; ulTimeEpoch = 0; valid = false; }
        bool ParseMsg(JsonDocument&);
        unsigned long EpochTime(void){ return ulTimeEpoch; };
        bool Valid(void){ return valid; };
};

class wfLightningStrikeEvent{
    private:
        unsigned long ulTimeEpoch;
        float fDistance;
        unsigned int uiEnergy;
        wfUnits units;
        bool valid;
    public:
        wfLightningStrikeEvent(){ units = Imperial; uiEnergy = 0; fDistance = 0; valid = false; }
        wfLightningStrikeEvent(wfUnits u){ units = u; valid = false; }
        bool ParseMsg(JsonDocument&);
        unsigned long EpochTime(void){ return ulTimeEpoch; }
        unsigned int Energy(void){ return uiEnergy; }
        bool Valid(void){ return valid; }
        float Distance(void);
};

class wfRapidWind{
    private:
        unsigned long ulTimeEpoch;
        float fWindSpeed;
        unsigned int uiWindDirection;
        wfUnits units;
        bool valid;
    public:
        wfRapidWind() { ulTimeEpoch = 0; fWindSpeed = 0; uiWindDirection = 0; units = Imperial; valid = false; };
        wfRapidWind(wfUnits u) { ulTimeEpoch = 0; fWindSpeed = 0; uiWindDirection = 0; units = u; valid = false; };
        bool ParseMsg(JsonDocument&);
        float WindSpeed(void);
        unsigned int WindDirection(void){ return uiWindDirection; };
        unsigned long EpochTime(void){ return ulTimeEpoch; };
        bool Valid(void){ return valid; };
};

class wfObservationTempest{
    private:
        unsigned long ulTimeEpoch;
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
        wfObservationTempest(){ units = Imperial; init(); };
        wfObservationTempest(wfUnits u):wfObservationTempest(){ units = u; init(); };
        bool ParseMsg(JsonDocument&);
        bool Valid(void){ return valid; };
        unsigned long EpochTime(void){ return ulTimeEpoch; };
        float WindAverage(void);
        float WindLull(void);
        float WindGust(void);
        unsigned int WindDirection(void){ return uiWindDirection; };
        unsigned long WindSampleInterval(void){ return uiWindSampleInterval; };
        float StationPressure(void);
        float AirTemperature(void);
        float RelativeHumidity(void){ return fRelativeHumidity; };
        float Illuminance(void);
        unsigned int UV(void){ return uiUv; };
        float SolarRadiation(void);
        float RainOverPreviousMinute(void);
        PartType ParticipationType(void){ return static_cast<PartType>(uiParticipationType); };
        float LightningStrikeAverageDistance(void);
        unsigned int LightningStrikeCount(void){ return uiLightningStrikeCount; };
        float BatteryVoltage(void){ return fBatteryVoltage; };
        unsigned int ReportInterval(void){ return uiReportInterval; };
};

class wfStatusHub{
    private:
        unsigned int uiFirmwareVersion;
        unsigned long ulUptime;
        int iRssi;
        wfUnits units;
        bool valid;
    public:
        wfStatusHub(){ uiFirmwareVersion = 0; ulUptime = 0; iRssi = 0; units = Imperial; valid = false; };
        wfStatusHub(wfUnits u):wfStatusHub(){ units = u; }
        bool ParseMsg(JsonDocument&);
        unsigned int FirmwareVersion(void){ return uiFirmwareVersion; }
        unsigned long Uptime(void){ return ulUptime; }
        int RSSI(void){ return iRssi; }
        bool Valid(void){ return valid; }
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
        wfObservationTempest *myObservationTempest;
        wfStatusHub *myStatusHub;
        void init(void);
    public:
        WeatherFlow(){ units = Imperial; init(); }
        WeatherFlow(wfUnits u){ units = u; init(); }
        ~WeatherFlow();
        bool Begin(void);
        bool ReceiveLoop(void);
        wfRainStartEvent* RainStartEvent(void){ return myRainStartEvent; } 
        wfLightningStrikeEvent* LightningStrikeEvent(void){ return myLightningStrikeEvent; }
        wfRapidWind* RapidWind(void){ return myRapidWind; }
        wfObservationTempest* ObservationTempest(void){ return myObservationTempest; }
        wfStatusHub* StatusHub(void){ return myStatusHub; }
};    

#endif