#pragma once
#include <Arduino.h>

class WebServerDsp {
public:
    WebServerDsp();
    bool begin();
    void loop();

    bool isWifiConnected() const;
    bool isApMode() const { return _isApMode; }
    String getIpAddress() const;

private:
    bool _isApMode;
    void setupRoutes();
};

extern WebServerDsp webServerDsp;
