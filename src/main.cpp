#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <StreamUtils.h>
#include <TinyPICO.h>
#include <Adafruit_MCP23X08.h>
#include <SPIFFS.h>
#include <Update.h>
#include <map>

// Version info
#define MAJOR 1
#define MINOR 0
#define PATCH 0

// Debug Info
#define DEBUG 1
int debug(int level, const char *fmt, ...);

// Hardware board
TinyPICO TP = TinyPICO();
// Output Guages
#define WINDGUAGEPIN 25
#define WINDCHANNEL 0
#define TEMPGUAGEPIN 26
#define TEMPCHANNEL 1
// Input Settings
#define BTNPIN1 33
#define BTNPIN2 32
#define LED1 27
#define LED1CHANNEL 2
#define LED2 15
#define LED2CHANNEL 3

// Hardware I2C GPIO extender
Adafruit_MCP23X08 mcp;

struct GaugeConfig{
  float min;
  float max;
  float step;
  float gain;
  float threshold;
};

struct FullConfig{
  GaugeConfig WindConfig;
  GaugeConfig TempConfig;
  char ssid[32];
  char wifipass[32];
  char username[32];
  char userpass[32];
};

FullConfig SystemConfig;

// Function Prototypes
void setDefaultSettings(void);
void ExtractConfig(FullConfig &newConfig, DynamicJsonDocument SettingsDoc);
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255);
uint32_t scalePwmOutput(float dataVal, float minScale, float maxScale, float gain = 1);
uint8_t encodeWind(int windDir);


// WiFi Parameters
#define SETTINGS_SIZE 512
EepromStream objEepromStream(0, SETTINGS_SIZE);
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

// UDP receiver for WeatherFlow
WiFiUDP UDPrcvr;
#define UDPPORT 50222
#define UDPRCRVSIZE 1460

enum CalMode { none, fixed, range };
CalMode CalibrationMode = none;

// The weather (in variables)
float fWindSpeedMph = 0;
int iWindDirectionDegrees = 0;
float fAirTempF = 0;



// ##################################################################
// # Setup
// ##################################################################

void setup() {

  // ==================================================
  // Hardware Init Stuff
  // ==================================================
  EEPROM.begin(SETTINGS_SIZE);
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
  DynamicJsonDocument jsonSettings(SETTINGS_SIZE);
  deserializeJson(jsonSettings, objEepromStream);
  ExtractConfig(SystemConfig, jsonSettings);

  // ==================================================
  // Button Setup & Reset Request
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
      setDefaultSettings();
      delay(5000);
      ESP.restart();
    }
    delay(500);
  }

  // ==================================================
  // Wi-Fi Startup
  // ==================================================
  if( !SystemConfig.ssid || !SystemConfig.wifipass ){
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
    debug(1, "\n\rConecting to Wi-Fi: %s ...", SystemConfig.ssid);
    WiFi.begin(SystemConfig.ssid, SystemConfig.wifipass);
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
    if( !request->authenticate(SystemConfig.username, SystemConfig.userpass) )
      return request->requestAuthentication();
    webServerSpiffsHandler(request);
  });
  objWebSocket.onEvent(onWebSocketEvent);
  objWebServer.addHandler(&objWebSocket);
  objWebServer.begin();

  // ==================================================
  // OTA Firmware handling
  // ==================================================

  // Setup OTA firmware handling debug
  #ifdef DEBUG
  // Debug OTA start notice
  ArduinoOTA.onStart([]() { Serial.println("OTA Starting"); });

  // Debug OTA end notice
  ArduinoOTA.onEnd([]() { Serial.println("OTA Finished"); });

  // Debug OTA progress
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  #endif
  // Start the OTA process
  //ArduinoOTA.
  ArduinoOTA.begin();

  // ==================================================
  // PWM controls for gauge outputs
  // ==================================================
  
  // Wind PWM freq 5 kHz, 13-bit timer resolution
  ledcSetup(WINDCHANNEL, 5000, 13);
  ledcAttachPin(WINDGUAGEPIN, WINDCHANNEL);

  // Temp PWM freq 5 kHz, 13-bit timer resolution
  ledcSetup(TEMPCHANNEL, 5000, 13);
  ledcAttachPin(TEMPGUAGEPIN, TEMPCHANNEL);

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
  if( !UDPrcvr.begin(UDPPORT) )debug(1, "\n\rFailed to open UDP listening port!");
}

// ##################################################################
// # Main Loop
// ##################################################################

void loop() {
  static char chUdpBuffer[UDPRCRVSIZE]= {0};
  DynamicJsonDocument jsonMsg(UDPRCRVSIZE);
  static uint32_t u32WindPwm;
  static uint32_t u32TempPwm;
  static uint8_t u8WindDir;
  time_t ul32CurTime;
  static time_t ul32LastBlink;

  // Get the current time
  time(&ul32CurTime);

  // Check for any incoming OTA updates
  ArduinoOTA.handle();

  // ==============================================================
  // Calibration modes
  // ==============================================================
  if( CalibrationMode == range ){
    debug(1, "\n\rWind is %f Mph at %i degrees.", fWindSpeedMph, iWindDirectionDegrees);
    u32WindPwm = scalePwmOutput(fWindSpeedMph, SystemConfig.WindConfig.min, SystemConfig.WindConfig.max, SystemConfig.WindConfig.gain);
    debug(1, "\n\rWind PWM: %i", u32WindPwm);
    ledcAnalogWrite(WINDCHANNEL, u32WindPwm);

    fWindSpeedMph += SystemConfig.WindConfig.step;
    if( fWindSpeedMph > SystemConfig.WindConfig.max )fWindSpeedMph = SystemConfig.WindConfig.min;

    debug(1, "\n\rAir temp is %f degrees F.", fAirTempF);\
    u32TempPwm = scalePwmOutput(fAirTempF, SystemConfig.TempConfig.min, SystemConfig.TempConfig.max, SystemConfig.TempConfig.gain);
    debug(1, "\n\rTemp PWM: %i", u32TempPwm);
    ledcAnalogWrite(TEMPCHANNEL, u32TempPwm);
    
    fAirTempF += SystemConfig.TempConfig.step;
    if( fAirTempF > SystemConfig.TempConfig.max )fAirTempF = SystemConfig.TempConfig.min;

    iWindDirectionDegrees+= 15;
    if( iWindDirectionDegrees > 360 )iWindDirectionDegrees = 0;
    u8WindDir = encodeWind(iWindDirectionDegrees);
    debug(1, "\n\rWind direction code: 0x%02X", u8WindDir);
    mcp.writeGPIO(u8WindDir, 0);

    delay(3000);
    return;
  }

  if( CalibrationMode == fixed ){
    fWindSpeedMph = 30;
    debug(1, "\n\rWind is %f Mph at %i degrees.", fWindSpeedMph, iWindDirectionDegrees);
    u32WindPwm = scalePwmOutput(fWindSpeedMph, SystemConfig.WindConfig.min, SystemConfig.WindConfig.max, SystemConfig.WindConfig.gain);
    debug(2, "\n\rWind PWM: %i", u32WindPwm);
    ledcAnalogWrite(WINDCHANNEL, u32WindPwm);

    fAirTempF = 80;
    debug(1, "\n\rAir temp is %f degrees F.", fAirTempF);\
    u32TempPwm = scalePwmOutput(fAirTempF, SystemConfig.TempConfig.min, SystemConfig.TempConfig.max, SystemConfig.TempConfig.gain);
    debug(2, "\n\rTemp PWM: %i", u32TempPwm);
    ledcAnalogWrite(TEMPCHANNEL, u32TempPwm);

    delay(5000);
    return;
  }

  // ================================================================
  // Wi-Fi in client mode, listening for WeatherFlow messages
  // ================================================================
  if( !bSoftApActive ){
    // Status via DotStar LED
    if( ul32CurTime%15 == 0 ){
      TP.DotStar_SetPixelColor(0x00FF00);
      ul32LastBlink = ul32CurTime;
    }
    if( ul32CurTime - ul32LastBlink >= 1 )TP.DotStar_SetPixelColor(0x0);

    // Check if we received a packet
    if( UDPrcvr.parsePacket() ){
      debug(2, "\n\rReceived a packet...");
      UDPrcvr.read(chUdpBuffer, UDPRCRVSIZE);
      debug(4, "\n\rPacket:\n\r  %s", chUdpBuffer);
      if( deserializeJson(jsonMsg, chUdpBuffer) == DeserializationError::Ok){
        debug(2, "\n\rJSON ok...");
        const char *obsType = jsonMsg["type"];
        const std::map<std::string,int> mapMsgTypes { {"rapid_wind", 1}, {"obs_st", 2} };
        switch ( mapMsgTypes.find(std::string(obsType))->second )
        {
        case 1:{
          debug(2, "\n\rReceived rapid_wind type");
          fWindSpeedMph = jsonMsg["ob"][1];
          iWindDirectionDegrees = jsonMsg["ob"][2];
          debug(1, "\n\rWind is %f Mph at %i degrees.", fWindSpeedMph, iWindDirectionDegrees);
          u32WindPwm = scalePwmOutput(fWindSpeedMph, SystemConfig.WindConfig.min, SystemConfig.WindConfig.max, SystemConfig.WindConfig.gain);
          debug(2, "\n\rWind PWM: %i", u32WindPwm);
          ledcAnalogWrite(WINDCHANNEL, u32WindPwm);
          if( fWindSpeedMph >= SystemConfig.WindConfig.threshold ){ u8WindDir = encodeWind(iWindDirectionDegrees); }
          else{ u8WindDir = 0x00; }
          debug(2, "\n\rWind direction code: 0x%02X", u8WindDir);
          mcp.writeGPIO(u8WindDir, 0);
        }
          break;
        
        case 2:{
          debug(2, "\n\rReceived obs_st type");
          float fAirTempC = jsonMsg["obs"][0][7];
          fAirTempF = 32 + 9*fAirTempC/5;
          debug(1, "\n\rAir temp is %f degrees F.", fAirTempF);
          u32TempPwm = scalePwmOutput(fAirTempF, SystemConfig.TempConfig.min, SystemConfig.TempConfig.max, SystemConfig.TempConfig.gain);
          debug(2, "\n\rTemp PWM: %i", u32TempPwm);
          ledcAnalogWrite(TEMPCHANNEL, u32TempPwm);
          }
          break;

        default:{
          debug(3, "\n\rMessage type (%s) is a don't care", obsType);
          }
          break;
        }
      }

      // Zero out our buffer, so we're good for the next UDP packet
      memset(chUdpBuffer, 0, UDPRCRVSIZE);
    }
  }

  // ================================================================
  // Wi-Fi in AP mode, listening for WeatherFlow messages
  // ================================================================
  if( bSoftApActive ){
    // Cycle the DotStar color, just to give the user some feedback
    TP.DotStar_CycleColor(25);
  }
}

// ##################################################################
// # Debug printer
// #
// # Debug logging to the serial port, message is only printed if 
// # the level is >= to DEBUG (i.e. debugging level).  If DEBUG is
// # undefined, code is deactivated.
// ##################################################################
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

// ##################################################################
// # Configuration Handlers
// ##################################################################

// # Default Settings
void setDefaultSettings(void){
  DynamicJsonDocument jsonDefaultSettings(SETTINGS_SIZE);
  debug(1, "Resaving default values to EEPROM...");
  jsonDefaultSettings.clear();
  jsonDefaultSettings["wind"]["min"] = 0;
  jsonDefaultSettings["wind"]["max"] = 40;
  jsonDefaultSettings["wind"]["step"] = 10;
  jsonDefaultSettings["wind"]["gain"] = 0.93;
  jsonDefaultSettings["wind"]["threshold"] = 1;
  jsonDefaultSettings["temp"]["min"] = -10;
  jsonDefaultSettings["temp"]["max"] = 110;
  jsonDefaultSettings["temp"]["step"] = 15;
  jsonDefaultSettings["temp"]["gain"] = 0.88;
  jsonDefaultSettings["auth"]["user"] = "admin";
  jsonDefaultSettings["auth"]["pass"] = "temp";
  serializeJson(jsonDefaultSettings, objEepromStream);
  objEepromStream.flush();
}

// Load the configuration structure
void ExtractConfig(FullConfig &newConfig, DynamicJsonDocument SettingsDoc){
  // Wind
  newConfig.WindConfig.min =  SettingsDoc["wind"]["min"];
  newConfig.WindConfig.max = SettingsDoc["wind"]["max"];
  newConfig.WindConfig.step = SettingsDoc["wind"]["step"];
  newConfig.WindConfig.gain = SettingsDoc["wind"]["gain"];
  newConfig.WindConfig.threshold = SettingsDoc["wind"]["threshold"];
  // Temp
  newConfig.TempConfig.min = SettingsDoc["temp"]["min"];
  newConfig.TempConfig.max = SettingsDoc["temp"]["max"];
  newConfig.TempConfig.step = SettingsDoc["temp"]["step"];
  newConfig.TempConfig.gain = SettingsDoc["temp"]["gain"];
  newConfig.TempConfig.threshold = 0;
  // User Login
  strlcpy(newConfig.username, SettingsDoc["auth"]["user"] | "admin", sizeof(newConfig.username));
  strlcpy(newConfig.userpass, SettingsDoc["auth"]["pass"] | "temp", sizeof(newConfig.userpass));
  // WiFi
  strlcpy(newConfig.ssid, SettingsDoc["wifi"]["ssid"] | "", sizeof(newConfig.ssid));
  strlcpy(newConfig.wifipass, SettingsDoc["wifi"]["pw"] | "", sizeof(newConfig.wifipass));
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

  if( windDir >= (int)348.75 || windDir < (int)11.25 ){ u8WindDir = 0x01; debug(3,"\n\rWind Dir: N"); }               // N
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
            DynamicJsonDocument jsonSettings(SETTINGS_SIZE);
            deserializeJson(jsonSettings, objEepromStream);
            jsonSettings["wind"] = jsonWsMsg["payload"]["wind"];
            jsonSettings["temp"] = jsonWsMsg["payload"]["temp"];
            if( strcmp("fixed", jsonWsMsg["payload"]["cal"]["mode"]) == 0 ){
              CalibrationMode = fixed;
              fAirTempF = jsonWsMsg["payload"]["cal"]["cal_fixed_temp"];
              fWindSpeedMph = jsonWsMsg["payload"]["cal"]["cal_fixed_wind"];
            }
            if( strcmp("range", jsonWsMsg["payload"]["cal"]["mode"]) == 0 ){
              CalibrationMode = range;
            }
            if( strcmp("none", jsonWsMsg["payload"]["cal"]["mode"]) == 0 ){
              CalibrationMode = none;
            }
            serializeJson(jsonSettings, objEepromStream);
            objEepromStream.flush();
            ExtractConfig(SystemConfig, jsonSettings);
          }
          break;

        // updateWiFi
        case 2:{
            DynamicJsonDocument jsonSettings(SETTINGS_SIZE);
            deserializeJson(jsonSettings, objEepromStream);
            jsonSettings["wifi"] = jsonWsMsg["payload"]["wifi"];
            debug(1, "\n\rGot Wi-Fi parameters, SSID: %s, and password: %s", jsonSettings["wifi"]["ssid"], jsonSettings["wifi"]["pw"]);
            serializeJson(jsonSettings, objEepromStream);
            objEepromStream.flush();
            delay(2000);
            ESP.restart();
          }
          break;

        //updateUser
        case 3:{
            DynamicJsonDocument jsonSettings(SETTINGS_SIZE);
            deserializeJson(jsonSettings, objEepromStream);
            jsonSettings["auth"] = jsonWsMsg["payload"]["auth"];
            debug(1, "\n\rGot Auth parameters, user: %s, and password: %s", jsonSettings["auth"]["user"], jsonSettings["auth"]["pass"]);
            serializeJson(jsonSettings, objEepromStream);
            objEepromStream.flush();
            ExtractConfig(SystemConfig, jsonSettings);
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
    else{ return String(SystemConfig.ssid); }
  }
  else if( var == "WIFI_IP_ADDR")return WiFi.localIP().toString();
  else if( var == "WIFI_RSSI" )return String(WiFi.RSSI());
  else if( var == "BAT_VOLT" )return String(TP.GetBatteryVoltage());
  else if( var == "MIN_WIND" )return String(SystemConfig.WindConfig.min);
  else if( var == "MAX_WIND" )return String(SystemConfig.WindConfig.max);
  else if( var == "STEP_WIND" )return String(SystemConfig.WindConfig.step);
  else if( var == "GAIN_WIND" )return String(SystemConfig.WindConfig.gain);
  else if( var == "THRESHOLD_WIND" )return String(SystemConfig.WindConfig.threshold);
  else if( var == "MIN_TEMP" )return String(SystemConfig.TempConfig.min);
  else if( var == "MAX_TEMP" )return String(SystemConfig.TempConfig.max);
  else if( var == "STEP_TEMP" )return String(SystemConfig.TempConfig.step);
  else if( var == "GAIN_TEMP" )return String(SystemConfig.TempConfig.gain);
  else return "N/A";
}

void notifyWsSystemStatus(void){

}