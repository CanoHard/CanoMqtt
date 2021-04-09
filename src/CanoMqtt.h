
//CanoMqtt is an library that creates an abstraction layer that handles wifi, mqtt and ota
//Made by Pablo Cano

#ifndef CANO_MQTT_H
#define CANO_MQTT_H

#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#include <Ticker.h>
#endif

#include <ArduinoOTA.h>
#include <AsyncMqttClient.h>

class CanoMqtt
{

public:
    CanoMqtt(const char *wifi_ssid, const char *wifi_password, const char *broker_ip, const char *mqtt_user, const char *mqtt_password, const char *name, const char *ota_password);
    CanoMqtt(const char *wifi_ssid, const char *wifi_password, const char *broker_ip, const char *mqtt_user, const char *mqtt_password, const char *name);
    CanoMqtt(const char *wifi_ssid, const char *wifi_password, const char *broker_ip, const char *mqtt_user, const char *mqtt_password, const char *name, const char *will_topic, const char *will_message);
    CanoMqtt(const char *wifi_ssid, const char *wifi_password, const char *broker_ip, const char *mqtt_user, const char *mqtt_password, const char *name, const char *will_topic, const char *will_message, const char *ota_password);
    ~CanoMqtt(); // destructor
    //
    typedef int OtaEvent;
    static const OtaEvent OTA_ONSTART = 1;
    static const OtaEvent OTA_ONEND = 2;
    static const OtaEvent OTA_ONPROGRESS = 3;
    static const OtaEvent OTA_ONERROR = 4;
    //User methods
    int WifiRSSI();
    bool IsMqttConnected();
    bool IsWifiConnected();
    void Init();
    void NetworkLoop();
    void Subscribe(const char *topic,const int qos);
    void UnSubscribe(const char *topic);
    void Publish(const char *topic, int qos, bool retain,const char *payload);
    void SetDebug(bool t);
    //Set callbacks
    void SetOnMqttConnect(void (*OnMqttConnect)());
    void SetOnMqttDisconnect(void (*OnMqttDisconnect)());
    void SetOnWiFiConnect(void (*OnWiFiConnect)());
    void SetOnWiFiDisconnect(void (*OnWiFiDisconnect)());
    void SetOnMqttMessage(void (*OnMqttMessage)(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total));
    void SetOnOtaEvent(void (*OnOtaEvent)(OtaEvent e, int p));
    //

private:
    const char *wifi_ssid = nullptr;
    const char *wifi_password = nullptr;
    const char *broker_ip = nullptr;
    const char *mqtt_user = nullptr;
    const char *mqtt_password = nullptr;
    const char *name = nullptr;
    const char *will_topic = nullptr;
    const char *will_message = nullptr;
    const char *ota_password = nullptr;

    //Callbacks functions
    static void connectToWifi();
    static void connectToMqtt();
    void onMqttConnect(bool);
    void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
#ifdef ARDUINO_ARCH_ESP32
    void WiFiEvent(WiFiEvent_t event);
#else
    void onWifiConnect(const WiFiEventStationModeGotIP &event);
    void onWifiDisconnect(const WiFiEventStationModeDisconnected &event);

#endif
    void setup_ota();

    //User Callbacks

    void (*OnMqttConnect)() = nullptr;
    void (*OnMqttDisconnect)() = nullptr;
    void (*OnWiFiConnect)() = nullptr;
    void (*OnWiFiDisconnect)() = nullptr;
    void (*OnOtaEvent)(OtaEvent e, int p) = nullptr;

    AsyncMqttClient mqttClient;

#ifdef ARDUINO_ARCH_ESP32
    TimerHandle_t mqttReconnectTimer;
    TimerHandle_t wifiReconnectTimer;
#else
    Ticker mqttReconnectTimer;
    WiFiEventHandler wifiConnectHandler;
    WiFiEventHandler wifiDisconnectHandler;
    Ticker wifiReconnectTimer;
#endif

    bool debug = false;

#define MQTT_PORT 1883
#define Wifi_Reconnect_Time 25 //Seconds for each attempt
#define Mqtt_Reconnect_Time 25 //Seconds for each attempt
};

#endif