#pragma once
#include "cloud_memory.h"
#include "platform/idf/runtime.h"
#include <ArduinoJson.h>
#include "cloud_endpoints.h"
namespace XTouchCloudToken {
template<class Request> int validate(DynamicJsonDocument &input, DynamicJsonDocument &out, const String &region, const String &token, String &reply, Request request){
 int code=-1;
  input.clear();input.shrinkToFit();
  code=request(String(XTouchCloudEndpoints::get(region.c_str())->api)+"/v1/user-service/my/profile",token,"",reply);
  XTouchCloudMemory::output(out);
  DynamicJsonDocument data(8192);
  if(code==200&&!deserializeJson(data,reply)) {
   String uid=data["uidStr"].as<String>();if(uid=="null"||uid.isEmpty())uid=data["uid"].as<String>();
   if(uid!="null"&&!uid.isEmpty()&&uid.length()<40){out["userId"]=uid.startsWith("u_")?uid:"u_"+uid;out["ok"]=true;}
  }
 return code;
}
}
