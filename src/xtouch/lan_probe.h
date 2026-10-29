#pragma once
#include <ArduinoJson.h>
#include "platform/idf/wifi.h"
#include "features/network/idf_mqtt.h"
#include <esp_system.h>

namespace XTouchLanProbe {
class DiscardReport : public Stream {
public:
 int available() override { return 0; }
 int read() override { return -1; }
 int peek() override { return -1; }
 void flush() override {}
 size_t write(uint8_t) override { return 1; }
};
// Runs in the existing bounded network worker. No printer commands are published.
inline String run(JsonVariantConst profile) {
 StaticJsonDocument<192> result;
 const char *error="lan_connection_failed";
 XTouchIdfMqtt mqtt;
 bool received=false;
 String topic=String("device/")+(profile["serialNumber"]|"")+"/report";
 if(WiFi.status()==WL_CONNECTED){
  mqtt.setServer(profile["host"]|"",8883);
  mqtt.setKeepAlive(10);
  mqtt.setCallback([&](char *name,uint8_t *,unsigned int){if(topic==name)received=true;});
  String client=String("XTC-")+String((uint32_t)ESP.getEfuseMac(),HEX)+"-"+String(esp_random(),HEX);
  if(mqtt.connect(client.c_str(),"bblp",profile["accessCode"]|"")) {
   error="lan_status_timeout";
   if(mqtt.subscribe(topic.c_str())) {
    const uint32_t started=millis();
    while(mqtt.connected()&&!received&&millis()-started<10000){mqtt.loop();delay(5);}
    if(received)result["ok"]=true;
   }
  } else if(mqtt.state()==4||mqtt.state()==5) error="lan_auth_rejected";
 }
 mqtt.disconnect();
 if(!result["ok"].as<bool>()){result["ok"]=false;result["error"]=error;}
 String output;serializeJson(result,output);return output;
}
}
