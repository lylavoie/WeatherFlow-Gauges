#ifndef __AppConfig__
#define __AppConfig__

struct WiFiSettings{
    char ssid[32] = {0};
    char pass[32] = {0};
};

struct GaugeSettings{
    int min = 0;
    int max = 10;
    int step = 1;
    float gain = 1;  // Gain is now the 50% PWM output integer
    int threshold = 0;
    GaugeSettings(int m1, int m2, int s, float g, int t){
        gain = g;
        min = m1;
        max = m2;
        threshold = t;
        step = s;
    };
};

struct WebSettings{
    char user[32] = {0};
    char pass[32] = {0};
    WebSettings(const char *u, const char *p){
        strcpy(user, u);
        strcpy(pass, p);
    };
};

struct GaugeLampSettings{
    int LampBrightness = 100;
    int OnHour = 19;
    int OnMinute = 0;
    int OffHour = 7;
    int OffMinute = 0;
};

struct AppConfig{
    static const unsigned int Version = 3;
    WiFiSettings WiFi;
    GaugeSettings Wind = {0, 40, 10, 3900, 1};
    GaugeSettings Temp = {-10, 150, 15, 3800, 1};
    WebSettings Web = {"admin", "temp"};
    char TimeZone[32] = "EST+5EDT,M3.2.0/2,M11.1.0/2";
    GaugeLampSettings GaugeLamps;
};

#endif