#ifndef __WeatherFlow__
#define __WeatherFlow__

enum wfUnits{
    Imperial,
    Metric
};

class wfRainStartEvent{
    private:
        unsigned long ulTimeEpoch;
        wfUnits units;
    public: 
        wfRainStartEvent(){ ulTimeEpoch = 0; units = Imperial; }
        wfRainStartEvent(const char *);
        unsigned long EpochTime(void){ return ulTimeEpoch; };
};

class wfLightningStrikeEvent{
    private:
        unsigned long ulTimeEpoch;
        float fDistance;
        unsigned int uiEnergy;
        wfUnits units;
    public:
        wfLightningStrikeEvent(){ units = Imperial; }
        wfLightningStrikeEvent(const char *);
        unsigned long EpochTime(void){ return ulTimeEpoch; }
        unsigned int Energy(void){ return uiEnergy; }
        float Distance(void);
};

class wfRapidWind{
    private:
        unsigned long ulTimeEpoch;
        float fWindSpeed;
        unsigned int uiWindDirection;
        wfUnits units;
    public:
        wfRapidWind() { ulTimeEpoch = 0; fWindSpeed = 0; uiWindDirection = 0; units = Imperial; };
        wfRapidWind(const char *);
        float WindSpeed(void);
        unsigned int WindDirection(void){ return uiWindDirection; };
        unsigned long EpochTime(void){ return ulTimeEpoch; };
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
        float fUv;
        float fSolarRadiation;
        float fRainOverPrevMin;
        unsigned int uiParticipationType;
        float fLightningStrikeAvgDistance;
        float fLightningStrikeCount;
        float fBatteryVoltage;
        unsigned int uiReportInterval;
        wfUnits units;
    public:
        enum PartType{
            None,Rain,Hail,RainPlusHail
        };
        wfObservationTempest();
        wfObservationTempest(const char *);

};

class wfStatusHub{
    private:
        unsigned int uiFirmwareVersion;
        unsigned long ulUptime;
        int iRssi;
        wfUnits units;
    public:
        wfStatusHub(){ uiFirmwareVersion = 0; ulUptime = 0; iRssi = 0; units = Imperial; };
        wfStatusHub(const char *);
        unsigned int FirmwareVersion(void){ return uiFirmwareVersion; }
        unsigned long Uptime(void){ return ulUptime; }
        int RSSI(void){ return iRssi; }
};

#endif