#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <TinyPICO.h>
#include <Adafruit_MCP23X08.h>
#include <SPIFFS.h>
#include <esp_task_wdt.h>
#include "wf.h"
#include "PersistSettings.h"
#include "Config.h"
#include <map>
#include <ctime>

// Note, these are required because of https://community.platformio.org/t/identifier-is-undefined-setenv-tzset/16162/2
_VOID  _EXFUN(tzset,	(_VOID));
int	_EXFUN(setenv,(const char *__string, const char *__value, int __overwrite));

// Version info
#define MAJOR 1
#define MINOR 2
#define PATCH 0

// Debug Info
#define DEBUG 1
void debug(int level, const char *fmt, ...);

// Hardware board
TinyPICO TP = TinyPICO();
// Output Guages
#define WINDGAUGEPIN 25
#define WINDCHANNEL 0
#define TEMPGAUGEPIN 26
#define TEMPCHANNEL 1
// Input/Output Settings
#define BTNPIN1 33
#define BTNPIN2 32
#define LED1 27
#define LED1CHANNEL 2
#define LED2 15
#define LED2CHANNEL 3

// WeatherFlow Handler
WeatherFlow WF(Imperial);

// Persistent Settings Handler
PersistSettings<AppConfig> Settings(AppConfig::Version);

// Hardware I2C GPIO extender
Adafruit_MCP23X08 mcp;

// Function Prototypes
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255);
uint32_t scalePwmOutput(float dataVal, float minScale, float maxScale, float gain = 1);
uint8_t encodeWind(int windDir);
void RunCalibration(void);


// WiFi Parameters
bool bSoftApActive = false;
#define AP_MODE_SSID "WeatherFlowGauges"

// Webserver and Websockets
AsyncWebServer objWebServer(80);
AsyncWebSocket objWebSocket("/ws");
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
  void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(AwsFrameInfo *info, uint8_t *data, size_t len);
String webTemplateProcessor(const String& var);
void webServerSpiffsHandler(AsyncWebServerRequest *request);

enum CalMode { none, range };
CalMode CalibrationMode = none;


// ##################################################################
// # Setup
// ##################################################################

void setup() {

  // ==================================================
  // Hardware Init Stuff
  // ==================================================
  SPIFFS.begin();

  // ==================================================
  // Setup the serial port for logging, if debugging
  // ==================================================
  #ifdef DEBUG
  Serial.begin(9600);
  while(!Serial)
  {
    Serial.print('.');
  }
  #endif
  
  // ==================================================
  // Settings (non-volitale)
  // ==================================================
  Settings.Begin();

  // ==================================================
  // Button Setup & Reset Default Settings
  // ==================================================
  pinMode(BTNPIN1, INPUT_PULLUP);
  int iSwitchDebounce = 0;
  while( !digitalRead(BTNPIN1) ){
    // Stage 0 reset, debouncing for 5 seconds
    TP.DotStar_SetPixelColor(0xFF9A00);
    TP.DotStar_SetBrightness(150);
    TP.DotStar_SetPower(true);
    // Stage 1 reset
    if( iSwitchDebounce++ > 10 ){
      TP.DotStar_SetPixelColor(0xFF0000);
      Settings.ResetToDefault();
      delay(5000);
      ESP.restart();
    }
    delay(500);
  }

  // ==================================================
  // Environment Startup
  // ==================================================
  setenv("TZ", Settings.Config.TimeZone, true);
  tzset();

  // ==================================================
  // Wi-Fi Startup
  // ==================================================
  if( !Settings.Config.WiFi.ssid[0] || !Settings.Config.WiFi.pass[0] ){
    // ------------------------------------
    // Soft AP mode
    // ------------------------------------
    debug(1, "\n\rWi-Fi parameters not set, starting AP mode.");
    
    // Start the AP mode
    debug(1, "\n\rStarting Wi-Fi AP for SSID: WeatherFlow");
    if( !WiFi.softAP(AP_MODE_SSID) ){
      debug(1, "\n\rFailed to start AP mode!");
      while(1);
    }

    bSoftApActive = true;
    TP.DotStar_SetBrightness(150); 
  }
  else{
    // ------------------------------------
    // STA mode
    // ------------------------------------
    debug(1, "\n\rConecting to Wi-Fi: %s ...", Settings.Config.WiFi.ssid);
    WiFi.begin(Settings.Config.WiFi.ssid, Settings.Config.WiFi.pass);
    int iWiFiFailCount = 0;
    TP.DotStar_SetPixelColor(0x0000FF);
    while(WiFi.status() != WL_CONNECTED)
    {
      debug(1, ".");
      delay(500);
      // Five some feedback via the DotStar
      if( iWiFiFailCount++ % 2 ){TP.DotStar_SetBrightness(150);}
      else{TP.DotStar_SetBrightness(25);}
      
    }

    debug(1, "\n\rWi-Fi connected!");
    TP.DotStar_SetPixelColor(0x00FF00);
    TP.DotStar_SetBrightness(25);
    #ifdef DEBUG
    char  chIP[81];
    WiFi.localIP().toString().toCharArray(chIP, sizeof(chIP) - 1);
    debug(1, "\n\rIP Address: %s", chIP);
    #endif
  }

  // ==================================================
  // IP Services
  // ==================================================

  // Setup mDNS
  if ( !MDNS.begin("wxgauges") ){debug(1, "Failed to start mDNS responder!");}

  // Setup the webserver and websocket handling
  // FIXME: Handle firmware update
  //objWebServer.on("/update", HTTP_POST, NULL);
  objWebServer.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request){request->send(401);});
  objWebServer.on("/logged-out.html", HTTP_GET, webServerSpiffsHandler);
  objWebServer.onNotFound([](AsyncWebServerRequest *request){
    if( !request->authenticate(Settings.Config.Web.user, Settings.Config.Web.pass) )
      return request->requestAuthentication();
    webServerSpiffsHandler(request);
  });
  objWebSocket.onEvent(onWebSocketEvent);
  objWebServer.addHandler(&objWebSocket);
  objWebServer.begin();


  // ==================================================
  // PWM controls for gauge outputs
  // ==================================================
  
  // Wind PWM freq 5 kHz, 13-bit timer resolution
  ledcSetup(WINDCHANNEL, 5000, 13);
  ledcAttachPin(WINDGAUGEPIN, WINDCHANNEL);

  // Temp PWM freq 5 kHz, 13-bit timer resolution
  ledcSetup(TEMPCHANNEL, 5000, 13);
  ledcAttachPin(TEMPGAUGEPIN, TEMPCHANNEL);

  // LED PWM Setup, 5 kHz, 13-bit timer
  ledcSetup(LED1CHANNEL, 5000, 13);
  ledcAttachPin(LED1, LED1CHANNEL);
  ledcAnalogWrite(LED1CHANNEL, 0);
  ledcSetup(LED2CHANNEL, 5000, 13);
  ledcAttachPin(LED2, LED2CHANNEL);
  ledcAnalogWrite(LED2CHANNEL, 0);

  // ==================================================
  // Setup MCP23008 I2C GPIO Expander
  // ==================================================
  if (!mcp.begin_I2C()) {debug(1, "\n\rFailed to startup MCP23008!");}
  for(int i=0; i<8; i++)mcp.pinMode(i, OUTPUT);

  // ==================================================
  // Start listening for UDP messages
  // ==================================================
  if( !WF.Begin() )debug(1, "\n\rFailed to start WeatherFlow listener!");

  // ==================================================
  // Enable the watchdog timer
  // ==================================================
  esp_task_wdt_init(10, false);
  esp_task_wdt_add(NULL);
  
}

// ##################################################################
// # Main Loop
// ##################################################################

void loop() {
  static uint32_t u32WindPwm;
  static uint32_t u32TempPwm;
  static uint8_t u8WindDir;
  time_t ul32CurTime;
  static time_t ul32LastBlink;
  static bool bOneShot = false;
  static bool bGaugeLamp = false;


  // ================================================================
  // Watchdog timer reset & current time
  // ================================================================   
  time(&ul32CurTime);
  esp_task_wdt_reset();


  // ================================================================
  // Calibration modes
  // ================================================================
  if( CalibrationMode == range){
    debug(1, "\n\rRunning in calibration mode, current epoch time: %d", ul32CurTime);
    RunCalibration();
    delay(5000);
    return;
  }
  
  if( bSoftApActive ){
    // ================================================================
    // Wi-Fi in AP mode, allowing for user setup...
    // ================================================================
    // Cycle the DotStar color, just to give the user some feedback
    TP.DotStar_CycleColor(25);
  }
  else{
    // ================================================================
    // Wi-Fi in client mode, listening for WeatherFlow messages
    // ================================================================

    // Status via DotStar LED
    if( ul32CurTime%15 == 0 && !bOneShot ){
      bOneShot = true;
      TP.DotStar_SetPixelColor(0x00FF00);
      ul32LastBlink = ul32CurTime;
      // Log the current time
      char buf[128];
      strftime(buf, 128, "%c", localtime(&ul32CurTime));
      debug(1, "\n\rCurrent System Time: %s", buf); 
      debug(1, "\r\nFirmware Version: %d.%d.%d", MAJOR, MINOR, PATCH);
    }
    if( ul32CurTime - ul32LastBlink >= 1 ){
      bOneShot = false;
      TP.DotStar_SetPixelColor(0x0);
    }

    // ================================================================
    // WeatherFlow receiver loop function call, return indicates
    // new data is available
    // ================================================================
    if( WF.ReceiveLoop() ){
      debug(1, "\n\rReceived updated weather info...");
      
      // Check for Wind data
      if( WF.RapidWind().Valid() ){
        debug(1, "\n\rValid Rapid Wind data:");
        debug(1, "\n\r\tWind Speed: %f", WF.RapidWind().WindSpeed());
        debug(1, "\n\r\tWind Direction: %d", WF.RapidWind().WindDirection());

        u32WindPwm = scalePwmOutput(WF.RapidWind().WindSpeed(), Settings.Config.Wind.min,
          Settings.Config.Wind.max, Settings.Config.Wind.gain);
        debug(2, "\n\r\tWind PWM: %i", u32WindPwm);
        ledcAnalogWrite(WINDCHANNEL, u32WindPwm);

        if( WF.RapidWind().WindSpeed() >= Settings.Config.Wind.threshold ){ 
          u8WindDir = encodeWind(WF.RapidWind().WindDirection()); 
        }
        else{ 
          u8WindDir = 0x00;
        }
        debug(2, "\n\r\tWind direction code: 0x%02X", u8WindDir);
        mcp.writeGPIO(u8WindDir, 0);

        // Check if we need to update our system time
        if( abs(time(NULL) - WF.RapidWind().EpochTime()) > 10 ){
          debug(1, "\n\r\tUpdating the system time...");
          timeval WFtime;
          WFtime.tv_sec = WF.RapidWind().EpochTime();
          WFtime.tv_usec = 0;
          settimeofday(&WFtime, NULL);
          debug(2, "\n\r\t\tWF Epoch Time: %d", WF.RapidWind().EpochTime());
          debug(2, "\n\r\t\tSystem Time:   %d", time(NULL));
        }
      }

      // Check for valid Station data
      if( WF.ObservationTempest().Valid() ){
        debug(1, "\n\rValid Station Observation data:");
        debug(1, "\n\r\tAir Temperature: %f", WF.ObservationTempest().AirTemperature());
        u32TempPwm = scalePwmOutput(WF.ObservationTempest().AirTemperature(), Settings.Config.Temp.min,
          Settings.Config.Temp.max, Settings.Config.Temp.gain);
        debug(2, "\n\r\tTemp PWM: %i", u32TempPwm);
        ledcAnalogWrite(TEMPCHANNEL, u32TempPwm);
      }
    }
  
    // ================================================================
    // Check our current time, turn on the gauge lamps if needed
    // ================================================================
    tm *tmCurrentTime;
    tmCurrentTime = localtime(&ul32CurTime);
    if( !bGaugeLamp && tmCurrentTime->tm_hour == Settings.Config.GaugeLamps.OnHour 
        && tmCurrentTime->tm_min == Settings.Config.GaugeLamps.OnMinute ){
      bGaugeLamp = true;
      ledcAnalogWrite(LED1CHANNEL, scalePwmOutput(Settings.Config.GaugeLamps.LampBrightness, 0, 100, 1));
    }
    if( bGaugeLamp && tmCurrentTime->tm_hour == Settings.Config.GaugeLamps.OffHour && 
        tmCurrentTime->tm_min == Settings.Config.GaugeLamps.OffMinute ){
      bGaugeLamp = false;
      ledcAnalogWrite(LED1CHANNEL, 0);
    }
  }

}

// ##################################################################
// # Debug printer
// #
// # Debug logging to the serial port, message is only printed if 
// # the level is >= to DEBUG (i.e. debugging level).  If DEBUG is
// # undefined, code is deactivated.
// ##################################################################
void debug(int level, const char *fmt, ...){
  #ifdef DEBUG
  if( level <= DEBUG ){
    char chBuffer[256];
    va_list args;
    va_start(args,fmt);
    vsprintf(chBuffer, fmt, args);
    va_end(args);
    Serial.print(chBuffer);
  }
  #endif
}

// ####################################################################
// # Run Calibration
// #
// # Runs a simple calibration, steping through the gauge "steps",
// # on each call (i.e. once per loop).  This allows the gain values
// # to easily be "tuned."
// ####################################################################
void RunCalibration(void){
  static float fWindSpeed = 0;
  static float fAirTemp = 0;
  static int iWindDirectionDegrees = 0;
  uint32_t u32WindPwm;
  uint32_t u32TempPwm;
  uint8_t u8WindDir;

  u32WindPwm = scalePwmOutput(fWindSpeed, Settings.Config.Wind.min, 
    Settings.Config.Wind.max, Settings.Config.Wind.gain);
  debug(1, "\n\rWind PWM: %i", u32WindPwm);
  ledcAnalogWrite(WINDCHANNEL, u32WindPwm);

  fWindSpeed += Settings.Config.Wind.step;
  if( fWindSpeed > Settings.Config.Wind.max )fWindSpeed = Settings.Config.Wind.min;

  u32TempPwm = scalePwmOutput(fAirTemp, Settings.Config.Temp.min, 
    Settings.Config.Temp.max, Settings.Config.Temp.gain);
  debug(1, "\n\rTemp PWM: %i", u32TempPwm);
  ledcAnalogWrite(TEMPCHANNEL, u32TempPwm);
  
  fAirTemp += Settings.Config.Temp.step;
  if( fAirTemp > Settings.Config.Temp.max )fAirTemp = Settings.Config.Temp.min;

  iWindDirectionDegrees+= 15;
  if( iWindDirectionDegrees > 360 )iWindDirectionDegrees = 0;
  u8WindDir = encodeWind(iWindDirectionDegrees);
  debug(1, "\n\rWind direction code: 0x%02X", u8WindDir);
  mcp.writeGPIO(u8WindDir, 0);
}


// ##################################################################
// # PWM and Wind LED outputs
// ##################################################################

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

  if( windDir >= (int)348.75 || windDir < (int)11.25 ){ u8WindDir = 0x01; debug(3,"\n\rWind Dir: N"); }         // N
  else if( windDir >= (int)11.25 && windDir < (int)33.75 ){ u8WindDir = 0x03; debug(3,"\n\rWind Dir: NNE"); }   // NNE
  else if( windDir >= (int)33.75 && windDir < (int)56.25 ){ u8WindDir = 0x02; debug(3,"\n\rWind Dir: NE"); }    // NE
  else if( windDir >= (int)56.25 && windDir < (int)78.75 ){ u8WindDir = 0x06; debug(3,"\n\rWind Dir: ENE"); }   // ENE
  else if( windDir >= (int)78.75 && windDir < (int)101.25 ){ u8WindDir = 0x04; debug(3,"\n\rWind Dir: E"); }    // E
  else if( windDir >= (int)101.25 && windDir < (int)123.75 ){ u8WindDir = 0x0C; debug(3,"\n\rWind Dir: ESE"); } // ESE
  else if( windDir >= (int)123.75 && windDir < (int)146.25 ){ u8WindDir = 0x08; debug(3,"\n\rWind Dir: SE"); }  // SE
  else if( windDir >= (int)146.25 && windDir < (int)168.75 ){ u8WindDir = 0x18; debug(3,"\n\rWind Dir: SSE"); } // SSE
  else if( windDir >= (int)168.75 && windDir < (int)191.25 ){ u8WindDir = 0x10; debug(3,"\n\rWind Dir: S"); }   // S
  else if( windDir >= (int)191.25 && windDir < (int)213.75 ){ u8WindDir = 0x30; debug(3,"\n\rWind Dir: SSW"); } // SSW
  else if( windDir >= (int)213.75 && windDir < (int)236.25 ){ u8WindDir = 0x20; debug(3,"\n\rWind Dir: SW"); }  // SW
  else if( windDir >= (int)236.25 && windDir < (int)258.75 ){ u8WindDir = 0x60; debug(3,"\n\rWind Dir: WSW"); } // WSW
  else if( windDir >= (int)258.75 && windDir < (int)281.25 ){ u8WindDir = 0x40; debug(3,"\n\rWind Dir: W"); }   // W
  else if( windDir >= (int)281.25 && windDir < (int)303.75 ){ u8WindDir = 0xC0; debug(3,"\n\rWind Dir: WNW"); } // WNW
  else if( windDir >= (int)303.75 && windDir < (int)326.25 ){ u8WindDir = 0x80; debug(3,"\n\rWind Dir: NW"); }  // NW
  else{ u8WindDir = 0x81; debug(3,"\n\rWind Dir: NNW"); }                                                       // NNW

  return u8WindDir;
}

// ##################################################################
// # Web Services (http server and websockets)
// ##################################################################

void notFound(AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
}

void webServerSpiffsHandler(AsyncWebServerRequest *request){
  String path = request->url();
  String contentType;
  debug(2, "handleFileRead: %s", path);
  if (path.endsWith("/")) path += "index.html"; // Deal with the roots
  if (path.endsWith(".html")) contentType = "text/html";
  else if (path.endsWith(".css")) contentType = "text/css";
  else if (path.endsWith(".js")) contentType = "application/javascript";
  else if (path.endsWith(".ico")) contentType = "image/x-icon";
  else contentType = "text/plain";
  if( SPIFFS.exists(path) ){ 
    request->send(SPIFFS, path, contentType, false, webTemplateProcessor); 
  }
  else{ 
    request->send(404, "text/plain", "Not Found");
  }
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
    void *arg, uint8_t *data, size_t len){
  switch (type) {
  #ifdef DEBUG
    case WS_EVT_CONNECT:
      debug(1, "\r\nWebsocket connected (ClientID: %u, ClientIP:  %s)", client->id(), client->remoteIP().toString());
      break;
    case WS_EVT_DISCONNECT:
      debug(1, "\r\nWebscoked disconnected (ClientID: %u", client->id());
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  #endif
    case WS_EVT_DATA:
      handleWebSocketMessage( (AwsFrameInfo*) arg, data, len);
      break;
  }    
  return;
  }

void handleWebSocketMessage(AwsFrameInfo *info, uint8_t *data, size_t len){
  // Make sure this is a complete message and the is "text".
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;  // C-STR terminator
    debug(2, "\r\nRaw websocket payload: %s", (char*)data);
    DynamicJsonDocument jsonWsMsg(1000);
    if( deserializeJson(jsonWsMsg, (char*)data) == DeserializationError::Ok){
      // We've got a valid message folks
      const char *chMsgtype = jsonWsMsg["type"];
      const std::map<std::string,int> mapWsMsgTypes { {"updateSettings", 1}, {"updateWiFi", 2}, {"updateUser", 3} };
      switch( mapWsMsgTypes.find(std::string(chMsgtype))->second ){
        // Update System Settings
        case 1:{
            
            Settings.Config.Wind.min = jsonWsMsg["payload"]["wind"]["min"];
            Settings.Config.Wind.max = jsonWsMsg["payload"]["wind"]["max"];
            Settings.Config.Wind.step = jsonWsMsg["payload"]["wind"]["step"];
            Settings.Config.Wind.gain = jsonWsMsg["payload"]["wind"]["gain"];
            Settings.Config.Wind.threshold = jsonWsMsg["payload"]["wind"]["threshold"];

            Settings.Config.Temp.min = jsonWsMsg["payload"]["temp"]["min"];
            Settings.Config.Temp.max = jsonWsMsg["payload"]["temp"]["max"];
            Settings.Config.Temp.step = jsonWsMsg["payload"]["temp"]["step"];
            Settings.Config.Temp.gain = jsonWsMsg["payload"]["temp"]["gain"];

            if( strcmp("range", jsonWsMsg["payload"]["cal"]["mode"]) == 0 ){
              CalibrationMode = range;
            }
            if( strcmp("none", jsonWsMsg["payload"]["cal"]["mode"]) == 0 ){
              CalibrationMode = none;
            }
          }
          break;

        // updateWiFi
        case 2:{
            strcpy(Settings.Config.WiFi.ssid, jsonWsMsg["payload"]["wifi"]["ssid"]);
            strcpy(Settings.Config.WiFi.pass, jsonWsMsg["payload"]["wifi"]["pw"]);
            Settings.Write();
            debug(1, "\n\rGot Wi-Fi parameters, SSID: %s, and password: %s", 
              Settings.Config.WiFi.ssid, Settings.Config.WiFi.pass);
            delay(2000);
            ESP.restart();
          }
          break;

        //updateUser
        case 3:{
            strcpy(Settings.Config.Web.user, jsonWsMsg["payload"]["auth"]["user"]);
            strcpy(Settings.Config.Web.pass, jsonWsMsg["payload"]["auth"]["pass"]);
            Settings.Write();
            debug(1, "\n\rGot Auth parameters, user: %s, and password: %s", 
              Settings.Config.Web.user, Settings.Config.Web.pass);
          }
          break;

        default:{
            debug(1, "\r\nUnknown websocket message type: %s", chMsgtype);
          }
          break;  
      }
    }
  }
}

String webTemplateProcessor(const String& var){
  if( var == "WIFI_MODE"){
    if( bSoftApActive ){ return "AP Mode"; }
    else{ return "Station Mode"; }
  }
  else if( var == "WIFI_SSID" ){
    if( bSoftApActive){ return AP_MODE_SSID; }
    else{ return String(Settings.Config.WiFi.ssid); }
  }
  else if( var == "WIFI_IP_ADDR")return WiFi.localIP().toString();
  else if( var == "WIFI_RSSI" )return String(WiFi.RSSI());
  else if( var == "BAT_VOLT" )return String(TP.GetBatteryVoltage());
  else if( var == "MIN_WIND" )return String(Settings.Config.Wind.min);
  else if( var == "MAX_WIND" )return String(Settings.Config.Wind.max);
  else if( var == "STEP_WIND" )return String(Settings.Config.Wind.step);
  else if( var == "GAIN_WIND" )return String(Settings.Config.Wind.gain);
  else if( var == "THRESHOLD_WIND" )return String(Settings.Config.Wind.threshold);
  else if( var == "MIN_TEMP" )return String(Settings.Config.Temp.min);
  else if( var == "MAX_TEMP" )return String(Settings.Config.Temp.max);
  else if( var == "STEP_TEMP" )return String(Settings.Config.Temp.step);
  else if( var == "GAIN_TEMP" )return String(Settings.Config.Temp.gain);
  else return "N/A";
}

void notifyWsSystemStatus(void){

}