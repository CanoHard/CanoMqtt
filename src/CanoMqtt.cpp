#include "CanoMqtt.h"

CanoMqtt *ref = nullptr;

CanoMqtt::~CanoMqtt()
{
  mqttClient.disconnect(true);
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  delete wifi_ssid;
  delete wifi_password;
  delete broker_ip;
  delete mqtt_user;
  delete mqtt_password;
  delete name;
  delete will_topic;
  delete will_message;
  delete ota_password;
  delete this;
  delete ref;
}

CanoMqtt::CanoMqtt(const char *wifi_ssid, const char *wifi_password, const char *broker_ip, const char *mqtt_user, const char *mqtt_password, const char *name)
{
  this->wifi_ssid = wifi_ssid;
  this->wifi_password = wifi_password;
  this->broker_ip = broker_ip;
  this->mqtt_user = mqtt_user;
  this->mqtt_password = mqtt_password;
  this->name = name;
}

CanoMqtt::CanoMqtt(const char *wifi_ssid, const char *wifi_password, const char *broker_ip, const char *mqtt_user,
                   const char *mqtt_password, const char *name, const char *ota_password)
{
  this->wifi_ssid = wifi_ssid;
  this->wifi_password = wifi_password;
  this->broker_ip = broker_ip;
  this->mqtt_user = mqtt_user;
  this->mqtt_password = mqtt_password;
  this->name = name;
  this->ota_password = ota_password;
}
CanoMqtt::CanoMqtt(const char *wifi_ssid, const char *wifi_password, const char *broker_ip, const char *mqtt_user,
                   const char *mqtt_password, const char *name, const char *will_topic, const char *will_message)
{
  this->wifi_ssid = wifi_ssid;
  this->wifi_password = wifi_password;
  this->broker_ip = broker_ip;
  this->mqtt_user = mqtt_user;
  this->mqtt_password = mqtt_password;
  this->name = name;
  this->will_topic = will_topic;
  this->will_message = will_message;
}

CanoMqtt::CanoMqtt(const char *wifi_ssid, const char *wifi_password, const char *broker_ip, const char *mqtt_user,
                   const char *mqtt_password, const char *name, const char *will_topic, const char *will_message, const char *ota_password)
{
  this->wifi_ssid = wifi_ssid;
  this->wifi_password = wifi_password;
  this->broker_ip = broker_ip;
  this->mqtt_user = mqtt_user;
  this->mqtt_password = mqtt_password;
  this->name = name;
  this->will_topic = will_topic;
  this->will_message = will_message;
  this->ota_password = ota_password;
}

//CALLBACKS
void CanoMqtt::connectToWifi()
{
  if (ref->debug)
  {
    Serial.println("Connecting to Wi-Fi...");
  }
  WiFi.begin(ref->wifi_ssid, ref->wifi_password);
}

void CanoMqtt::connectToMqtt()
{
  if (ref->debug)
  {
    Serial.println("Connecting to MQTT...");
  }
  ref->mqttClient.connect();
}

void CanoMqtt::onMqttConnect(bool ispresent)
{
  if (debug)
  {
    Serial.println("Connected to MQTT.");
    Serial.print("Session present: ");
    Serial.println(ispresent);
  }

  if (will_topic != nullptr)
  {
    mqttClient.publish(will_topic, 0, true, "online");
  }
  if (OnMqttConnect != nullptr)
  {
    OnMqttConnect(); //Call callback
  }
}
void CanoMqtt::onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  if (debug)
  {
    Serial.println("Disconnected from MQTT.");
  }

  if (WiFi.isConnected())
  {
#ifdef ARDUINO_ARCH_ESP32
    xTimerStart(mqttReconnectTimer, 0);
#else
    mqttReconnectTimer.once(2, connectToMqtt);
#endif
  }
  if (OnMqttDisconnect != nullptr)
  {
    OnMqttDisconnect(); //Call callback
  }
}
#ifdef ARDUINO_ARCH_ESP32
void CanoMqtt::WiFiEvent(WiFiEvent_t event)
{
  if (debug)
  {
    Serial.printf("[WiFi-event] event: %d\n", event);
  }
  switch (event)
  {
  case SYSTEM_EVENT_STA_GOT_IP:
    xTimerStop(wifiReconnectTimer, 0); // ensure we don't reconnect to WiFi
    if (debug)
    {
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    }
    setup_ota();
    connectToMqtt();
    if (OnWiFiConnect != nullptr)
    {
      OnWiFiConnect(); //Call callback
    }
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    if (debug)
    {
      Serial.println("WiFi lost connection");
    }

    xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    xTimerStart(wifiReconnectTimer, 0);
    if (OnWiFiDisconnect != nullptr)
    {
      OnWiFiDisconnect(); //Call callback
    }
    break;
  default:
    break;
  }
}
#else
void CanoMqtt::onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  if (debug)
  {
    Serial.println("Connected to Wi-Fi.");
  }

  connectToMqtt();
}
void CanoMqtt::onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  if (debug)
  {
    Serial.println("Disconnected from Wi-Fi.");
  }
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}
#endif

void CanoMqtt::Init()
{
  if (debug)
  {
    Serial.begin(9600);
  }

  ref = this;

  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
#ifdef ARDUINO_ARCH_ESP32
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
  WiFi.setHostname(name);
  WiFi.onEvent(std::bind(&CanoMqtt::WiFiEvent, this, std::placeholders::_1)); //Usando bind le paso la funcion correspodiente a esta clase
#else
  wifiConnectHandler = WiFi.onStationModeGotIP(std::bind(&CanoMqtt::onWifiConnect, this, std::placeholders::_1));
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(std::bind(&CanoMqtt::onWifiDisconnect, this, std::placeholders::_1));
  WiFi.hostname(name);
#endif

  //Serial.printf("\n Informacion %s, %s, %s, \n", mqtt_user, mqtt_password, broker_ip);
  mqttClient.setCredentials(mqtt_user, mqtt_password);
  mqttClient.onConnect(std::bind(&CanoMqtt::onMqttConnect, this, std::placeholders::_1));
  mqttClient.onDisconnect(std::bind(&CanoMqtt::onMqttDisconnect, this, std::placeholders::_1));
  mqttClient.setServer(broker_ip, MQTT_PORT);
  mqttClient.setClientId(name);
  mqttClient.setCleanSession(true);
  if (will_topic != nullptr)
  {
    mqttClient.setWill(will_topic, 0, true, will_message);
  }

  connectToWifi();
}

void CanoMqtt::setup_ota()
{
  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(name);
  if (ota_password != nullptr)
  {
    ArduinoOTA.setPassword(ota_password);
  }
  // No authentication by default
  if (ota_password != nullptr)
  {
    ArduinoOTA.setPassword(ota_password);
  }

  ArduinoOTA.onStart([&]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    if (OnOtaEvent != nullptr)
    {
      OnOtaEvent(OTA_ONSTART, 0);
    }
  });
  ArduinoOTA.onEnd([&]() {
    if (OnOtaEvent != nullptr)
    {
      OnOtaEvent(OTA_ONEND, 0);
    }
  });
  ArduinoOTA.onProgress([&](unsigned int progress, unsigned int total) {
    int po = (progress * 100) / total;
    if (OnOtaEvent != nullptr)
    {
      OnOtaEvent(OTA_ONPROGRESS, po);
    }
  });
  ArduinoOTA.onError([&](ota_error_t error) {
    if (OnOtaEvent != nullptr)
    {
      OnOtaEvent(OTA_ONERROR, 0);
    }
  });
  ArduinoOTA.begin();
}

void CanoMqtt::NetworkLoop()
{
  ArduinoOTA.handle();
}

//User methods
int CanoMqtt::WifiRSSI()
{
  return WiFi.RSSI();
}
bool CanoMqtt::IsMqttConnected()
{
  return mqttClient.connected();
}

bool CanoMqtt::IsWifiConnected()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    return true;
  }
  return false;
}

void CanoMqtt::Subscribe(char *topic, int qos)
{
  mqttClient.subscribe(topic, qos);
}

void CanoMqtt::UnSubscribe(char *topic)
{
  mqttClient.unsubscribe(topic);
}

void CanoMqtt::Publish(char *topic, int qos, bool retain, char *payload)
{
  mqttClient.publish(topic, qos, retain, payload);
}

//Set callbacks

void CanoMqtt::SetOnMqttConnect(void (*OnMqttConnect)())
{
  this->OnMqttConnect = OnMqttConnect;
}

void CanoMqtt::SetOnMqttDisconnect(void (*OnMqttDisconnect)())
{
  this->OnMqttDisconnect = OnMqttDisconnect;
}

void CanoMqtt::SetOnWiFiConnect(void (*OnWiFiConnect)())
{
  this->OnWiFiConnect = OnWiFiConnect;
}

void CanoMqtt::SetOnWiFiDisconnect(void (*OnWiFiDisconnect)())
{
  this->OnWiFiDisconnect = OnWiFiDisconnect;
}

void CanoMqtt::SetOnMqttMessage(void (*OnMqttMessage)(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total))
{
  //this->OnWiFiDisconnect = OnWiFiDisconnect;
  mqttClient.onMessage(OnMqttMessage);
}
void CanoMqtt::SetOnOtaEvent(void (*OnOtaEvent)(OtaEvent e, int p))
{
  this->OnOtaEvent = OnOtaEvent;
}

void CanoMqtt::SetDebug(bool t)
{
  debug = t;
}