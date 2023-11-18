
#ifndef __SIGMA_CONNECT_H__
#define __SIGMA_CONNECT_H__

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>

#define SIGMA_TCP_PORT 8086
#define SIGMA_TCP_BUFFER_SIZE 256
#define SIGMA_TCP_COMMAND_READ 0x0a
#define SIGMA_TCP_COMMAND_WRITE 0x0b

class SigmaTcpServer 
{    
private:
    WiFiServer sigma_tcp_server;
    TwoWire i2s_device;

public:
    SigmaTcpServer();

    ~SigmaTcpServer();

    void setup_sigma_tcp_server();
    void handle_sigma_tcp_client();
};

#endif