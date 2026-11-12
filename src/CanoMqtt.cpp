#include "CanoMqtt.h"

// Made by Pablo Cano

// The WiFi events were renamed in the ESP32 Arduino core 2.x
#if defined(ARDUINO_ARCH_ESP32) && defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 2
#define SYSTEM_EVENT_STA_GOT_IP ARDUINO_EVENT_WIFI_STA_GOT_IP
#define SYSTEM_EVENT_STA_DISCONNECTED ARDUINO_EVENT_WIFI_STA_DISCONNECTED
#endif
CanoMqtt *ref = nullptr;

CanoMqtt::~CanoMqtt()
{
  // The strings are owned by the user and the object may not live in the heap,
  // so nothing is deleted here: only stop what this object started.
#ifdef ARDUINO_ARCH_ESP32
  if (wifiReconnectTimer != nullptr)
  {
    xTimerDelete(wifiReconnectTimer, 0);
  }
#else
  wifiReconnectTimer.detach();
#endif
  mqttClient.disconnect(true);
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  if (ref == this)
  {
    ref = nullptr;
  }
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

// CALLBACKS
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
  ref->lastMqttAttempt = millis();
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
    OnMqttConnect(); // Call callback
  }
}
void CanoMqtt::onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  if (debug)
  {
    Serial.println("Disconnected from MQTT.");
  }

  // The reconnection is handled by NetworkLoop()
  if (OnMqttDisconnect != nullptr)
  {
    OnMqttDisconnect(); // Call callback
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
      OnWiFiConnect(); // Call callback
    }
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    if (debug)
    {
      Serial.println("WiFi lost connection");
    }

    xTimerStart(wifiReconnectTimer, 0);
    if (OnWiFiDisconnect != nullptr)
    {
      OnWiFiDisconnect(); // Call callback
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
  if (OnWiFiConnect != nullptr)
  {
    OnWiFiConnect(); // Call callback
  }
  setup_ota();
  connectToMqtt();
}
void CanoMqtt::onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  if (debug)
  {
    Serial.println("Disconnected from Wi-Fi.");
  }
  if (OnWiFiDisconnect != nullptr)
  {
    OnWiFiDisconnect(); // Call callback
  }
  wifiReconnectTimer.once(Wifi_Reconnect_Time, connectToWifi);
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
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(Wifi_Reconnect_Time * 1000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
  WiFi.setHostname(name);
  WiFi.onEvent(std::bind(&CanoMqtt::WiFiEvent, this, std::placeholders::_1)); // Usando bind le paso la funcion correspodiente a esta clase
#else
  wifiConnectHandler = WiFi.onStationModeGotIP(std::bind(&CanoMqtt::onWifiConnect, this, std::placeholders::_1));
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(std::bind(&CanoMqtt::onWifiDisconnect, this, std::placeholders::_1));
  WiFi.hostname(name);
#endif

  // Serial.printf("\n Informacion %s, %s, %s, \n", mqtt_user, mqtt_password, broker_ip);
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

  ArduinoOTA.onStart([&]()
                     {
                       String type;
                       if (ArduinoOTA.getCommand() == U_FLASH)
                         type = "sketch";
                       else // U_SPIFFS
                         type = "filesystem";
                       if (OnOtaEvent != nullptr)
                       {
                         OnOtaEvent(OTA_ONSTART, 0);
                       } });
  ArduinoOTA.onEnd([&]()
                   {
                     if (OnOtaEvent != nullptr)
                     {
                       OnOtaEvent(OTA_ONEND, 0);
                     } });
  ArduinoOTA.onProgress([&](unsigned int progress, unsigned int total)
                        {
                          int po = (progress * 100) / total;
                          if (OnOtaEvent != nullptr)
                          {
                            OnOtaEvent(OTA_ONPROGRESS, po);
                          } });
  ArduinoOTA.onError([&](ota_error_t error)
                     {
                       if (OnOtaEvent != nullptr)
                       {
                         OnOtaEvent(OTA_ONERROR, 0);
                       } });
  ArduinoOTA.begin();
}

void CanoMqtt::NetworkLoop()
{
  ArduinoOTA.handle();

  // MQTT reconnection. It is done here and not from the disconnect callback because
  // the callback is not always fired, and the client can get stuck in the
  // CONNECTING/DISCONNECTING state, where connect() does nothing.
  if (!WiFi.isConnected() || mqttClient.connected())
  {
    pendingConnect = false;
    return;
  }

  if (pendingConnect)
  {
    pendingConnect = false;
    connectToMqtt();
    return;
  }

  if (millis() - lastMqttAttempt >= Mqtt_Reconnect_Time * 1000UL)
  {
    if (debug)
    {
      Serial.println("MQTT not connected, retrying");
    }
    lastMqttAttempt = millis();
    mqttClient.disconnect(true); // Drop whatever is left of the previous attempt
    pendingConnect = true;       // And connect on the next call, once it has been cleaned up
  }
}

// User methods
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

void CanoMqtt::Subscribe(const char *topic, int qos)
{
  mqttClient.subscribe(topic, qos);
}

void CanoMqtt::UnSubscribe(const char *topic)
{
  mqttClient.unsubscribe(topic);
}

void CanoMqtt::Publish(const char *topic, int qos, bool retain, const char *payload)
{
  mqttClient.publish(topic, qos, retain, payload);
}

// Set callbacks

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
  // this->OnWiFiDisconnect = OnWiFiDisconnect;
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

AsyncMqttClient *CanoMqtt::getInternalMqtt()
{
  return &mqttClient;
}