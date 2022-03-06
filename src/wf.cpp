#include "wf.h"

/* *********************************************
/  Rain Start Event Class
/
/  *********************************************/

wfRainStartEvent::wfRainStartEvent(const char *JsonString){

}

/* *********************************************
/  Lightning Strike Event Class
/
/  *********************************************/

wfLightningStrikeEvent::wfLightningStrikeEvent(const char *JsonString){

}

float wfLightningStrikeEvent::Distance(void){
    switch (units){
        case Imperial:
            return fDistance / 1609;
            break;
        
        default:  // Metric
            return fDistance;
            break;
    }
}

/* *********************************************
/  Lightning Strike Event Class
/
/  *********************************************/

wfRapidWind::wfRapidWind(const char *JsonString){

}

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

wfObservationTempest::wfObservationTempest(const char *JsonString){

}

/* *********************************************
/  Status Hub Class
/
/  *********************************************/
wfStatusHub::wfStatusHub(const char *JsonString){
    
}