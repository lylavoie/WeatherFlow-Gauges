#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <map>

// Debug Info
#define DEBUG 1
int debug(int level, const char *fmt, ...);

// Function Prototypes
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255);
uint32_t scalePwmOutput(float dataVal, float minScale, float maxScale, float gain = 1);
uint8_t encodeWind(int windDir);

// WiFi Parameters
const char chSSID[] = "IOT";
const char chPassword[] = "IotRecordsRule";

// UDP receiver for WeatherFlow
WiFiUDP UDPrcvr;
#define UDPPORT 50222
#define UDPRCRVSIZE 1460
StaticJsonDocument<UDPRCRVSIZE> jsonMsg; 
const std::map<std::string,int> mapMsgTypes { {"rapid_wind", 1}, {"obs_st", 2} };

// Output Guages
#define WINDGUAGEPIN 26
#define WINDCHANNEL 0
#define WINDMIN 0
#define WINDMAX 50
#define WINDGAIN 1
#define WINDDIRTHRESHOLD 1
#define TEMPGUAGEPIN 25
#define TEMPCHANNEL 1
#define TEMPMIN -10
#define TEMPMAX 115
#define TEMPGAIN 1

// The weather (in variables)
float fWindSpeedMph = 0;
int iWindDirectionDegrees = 0;
float fAirTempC = 0;
float fAirTempF = 0;

void setup() {
  // Setup the serial port for logging, if debugging
  #ifdef DEBUG
  Serial.begin(9600);
  while(!Serial)
  {
    Serial.print('.');
  }
  #endif
  
  // Setup the Wi-Fi connection
  debug(1, "\nConecting to Wi-Fi: %s ...", chSSID);
  WiFi.begin(chSSID, chPassword);
  while(WiFi.status() != WL_CONNECTED)
  {
      debug(1, ".");
      delay(500);
  }

  debug(1, "\nWi-Fi connected!");
  #ifdef DEBUG
  char  chIP[81];
  WiFi.localIP().toString().toCharArray(chIP, sizeof(chIP) - 1);
  debug(1, "IP Address: %s", chIP);
  #endif

  // Setup up the guage outputs

  // Wind PWM freq 5 kHz, 13-bit timer resolution
  ledcSetup(WINDCHANNEL, 5000, 13);
  ledcAttachPin(WINDGUAGEPIN, WINDCHANNEL);

  // Temp PWM freq 5 kHz, 13-bit timer resolution
  ledcSetup(TEMPCHANNEL, 5000, 13);
  ledcAttachPin(TEMPGUAGEPIN, TEMPCHANNEL);

  // Start listening for UDP messages
  if( !UDPrcvr.begin(UDPPORT) )debug(1, "\nFailed to open UDP listening port!");
}

void loop() {
  static char chUdpBuffer[UDPRCRVSIZE]= {0};
  static uint32_t u32WindPwm;
  static uint32_t u32TempPwm;
  static uint8_t u8WindDir;

  if( 0 ){
    debug(1, "\nWind is %f Mph at %i degrees.", fWindSpeedMph, iWindDirectionDegrees);
    u32WindPwm = scalePwmOutput(fWindSpeedMph, WINDMIN, WINDMAX, WINDGAIN);
    debug(2, "\nWind PWM: %i", u32WindPwm);
    ledcAnalogWrite(WINDCHANNEL, u32WindPwm);

    fWindSpeedMph += 10;
    if( fWindSpeedMph > 50 )fWindSpeedMph = 0;

    delay(5000);
    return;
  }

  // Check if we received a packet
  if( UDPrcvr.parsePacket() ){
    debug(2, "\nReceived a packet...");
    UDPrcvr.read(chUdpBuffer, UDPRCRVSIZE);
    debug(4, "\nPacket:\n  %s", chUdpBuffer);
    if( deserializeJson(jsonMsg, chUdpBuffer) == DeserializationError::Ok){
      debug(2, "\nJSON ok...");
      const char *obsType = jsonMsg["type"];
      switch ( mapMsgTypes.find(std::string(obsType))->second )
      {
      case 1:{
        debug(2, "\nReceived rapid_wind type");
        fWindSpeedMph = jsonMsg["ob"][1];
        iWindDirectionDegrees = jsonMsg["ob"][2];
        debug(1, "\nWind is %f Mph at %i degrees.", fWindSpeedMph, iWindDirectionDegrees);
        u32WindPwm = scalePwmOutput(fWindSpeedMph, WINDMIN, WINDMAX, WINDGAIN);
        debug(2, "\nWind PWM: %i", u32WindPwm);
        ledcAnalogWrite(WINDCHANNEL, u32WindPwm);
        if( fWindSpeedMph >= (float)WINDDIRTHRESHOLD )u8WindDir = encodeWind(iWindDirectionDegrees);
        else u8WindDir = 0x00;
        debug(2, "\nWind direction code: 0x%02X", u8WindDir);

        // FIXME: Write the wind direction out to the SPI driver
      }
        break;
      
      case 2:{
        debug(2, "\nReceived obs_st type");
        fAirTempC = jsonMsg["obs"][0][7];
        fAirTempF = 32 + 9*fAirTempC/5;
        debug(1, "\nAir temp is %f degrees F.", fAirTempF);\
        u32TempPwm = scalePwmOutput(fAirTempF, TEMPMIN, TEMPMAX, TEMPGAIN);
        debug(2, "\nTemp PWM: %i", u32TempPwm);
        ledcAnalogWrite(TEMPCHANNEL, u32TempPwm);
        }
        break;

      default:{
        debug(3, "\nMessage type (%s) is a don't care", obsType);
        }
        break;
      }
    }
    memset(chUdpBuffer, 0, UDPRCRVSIZE);
  }

}

// Debug logging to the serial port, message is only printed if the level 
// is >= to DEBUG (i.e. debugging level).  If DEBUG is undefined, code
// is deactivated.
int debug(int level, const char *fmt, ...){
  #ifdef DEBUG
  if( level <= DEBUG ){
    int iNumChar = 0;
    char chBuffer[256];
    va_list args;
    va_start(args,fmt);
    iNumChar = vsprintf(chBuffer, fmt, args);
    va_end(args);
    Serial.print(chBuffer);
    return iNumChar;
  }
  #endif
  return 0;
}

// Analog PMW control, similar to Arduino analogWrite
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax) {
  // calculate duty, 8191 from 2 ^ 13 - 1
  uint32_t duty = (8191 / valueMax) * min(value, valueMax);

  // write duty to LEDC
  ledcWrite(channel, duty);
}

// Scale our guage values, as needed
uint32_t scalePwmOutput(float dataVal, float minScale, float maxScale, float gain){
  float fRange = maxScale - minScale;
  float fScaler = 255 / fRange;
  uint32_t u32Output = (dataVal - minScale) * fScaler * gain;
  return min(u32Output, (uint32_t)255);
}

// Encode wind direction to the output for the LED driver.
// Assumes 8 LEDs, connected to 8-bit register
uint8_t encodeWind(int windDir){
  uint8_t u8WindDir;

  if( windDir >= (int)348.75 || windDir < (int)11.25 ){ u8WindDir = 0x01; debug(3,"\nWind Dir: N"); }               // N
        else if( windDir >= (int)11.25 && windDir < (int)33.75 ){ u8WindDir = 0x03; debug(3,"\nWind Dir: NNE"); }   // NNE
        else if( windDir >= (int)33.75 && windDir < (int)56.25 ){ u8WindDir = 0x02; debug(3,"\nWind Dir: NE"); }    // NE
        else if( windDir >= (int)56.25 && windDir < (int)78.75 ){ u8WindDir = 0x06; debug(3,"\nWind Dir: ENE"); }   // ENE
        else if( windDir >= (int)78.75 && windDir < (int)101.25 ){ u8WindDir = 0x04; debug(3,"\nWind Dir: E"); }    // E
        else if( windDir >= (int)101.25 && windDir < (int)123.75 ){ u8WindDir = 0x0C; debug(3,"\nWind Dir: ESE"); } // ESE
        else if( windDir >= (int)123.75 && windDir < (int)146.25 ){ u8WindDir = 0x08; debug(3,"\nWind Dir: SE"); }  // SE
        else if( windDir >= (int)146.25 && windDir < (int)168.75 ){ u8WindDir = 0x18; debug(3,"\nWind Dir: SSE"); } // SSE
        else if( windDir >= (int)168.75 && windDir < (int)191.25 ){ u8WindDir = 0x10; debug(3,"\nWind Dir: S"); }   // S
        else if( windDir >= (int)191.25 && windDir < (int)213.75 ){ u8WindDir = 0x30; debug(3,"\nWind Dir: SSW"); } // SSW
        else if( windDir >= (int)213.75 && windDir < (int)236.25 ){ u8WindDir = 0x20; debug(3,"\nWind Dir: SW"); }  // SW
        else if( windDir >= (int)236.25 && windDir < (int)258.75 ){ u8WindDir = 0x60; debug(3,"\nWind Dir: WSW"); } // WSW
        else if( windDir >= (int)258.75 && windDir < (int)281.25 ){ u8WindDir = 0x40; debug(3,"\nWind Dir: W"); }   // W
        else if( windDir >= (int)281.25 && windDir < (int)303.75 ){ u8WindDir = 0xC0; debug(3,"\nWind Dir: WNW"); } // WNW
        else if( windDir >= (int)303.75 && windDir < (int)326.25 ){ u8WindDir = 0x80; debug(3,"\nWind Dir: NW"); }  // NW
        else{ u8WindDir = 0x81; debug(3,"\nWind Dir: NNW"); }                                                       // NNW

  return u8WindDir;
}