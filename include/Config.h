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
    float gain = 1;
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

struct AppConfig{
    static const unsigned int Version = 1;
    WiFiSettings WiFi;
    GaugeSettings Wind = {10, 50, 10, 0.93, 1};
    GaugeSettings Temp = {-10, 150, 15, 0.88, 1};
    WebSettings Web = {"admin", "temp"};
};

#endif