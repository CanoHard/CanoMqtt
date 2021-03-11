#include <Arduino.h>

#include <CanoMqtt.h>

CanoMqtt c("Wifi-Name", "Wifi-Pass", "Broker-ip", "Broker-user", "Broker-pass", "Device-Name", "will-topic", "will-payload");

void onMqttConnect()
{

  Serial.println("Callback has been called");
  c.Subscribe("test/l", 0);
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
  char new_payload[len + 1];
  new_payload[len] = '\0';
  strncpy(new_payload, payload, len);
  Serial.println(new_payload);
  Serial.println(WiFi.localIP());
}

void Otacall(int e, int p){
  Serial.printf("\n %d , %d \n",e,p);
}

void setup()
{
  //Set callbacks
  c.SetOnMqttConnect(onMqttConnect);
  c.SetOnMqttMessage(&onMqttMessage);
  c.SetOnOtaEvent(Otacall);
  c.SetDebug(true); //Prints useful info in the serial monitor
  c.Init(); //Intialize CanoMqtt
  c.Publish("test/l",0,false,"hi");//Publish payload
}

void loop()
{
  c.NetworkLoop();
}
