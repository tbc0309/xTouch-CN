#pragma once
#include "cloud_memory.h"
#include "platform/idf/runtime.h"
#include <ArduinoJson.h>
#include "cloud_auth.h"
#include "cloud_endpoints.h"
#include "vault.h"
namespace XTouchCloudLogin {
inline void wipe(String &value){for(size_t i=0;i<value.length();++i)value.setCharAt(i,0);value="";}
template<class Request, class Uid> int execute(DynamicJsonDocument &input, DynamicJsonDocument &out, const String &mode, const String &region, String &token, String &remembered, String &body, String &reply, Request request, Uid uidFromToken) {
 int code=-1;
 if(mode=="login"&&input["rememberPassword"].as<bool>())remembered=xtouch_vault_seal(input["password"]|"",String(input["accountId"]|"")+":"+region+":"+(input["account"]|""));
 if(mode=="login" || mode=="login_code" || mode=="login_tfa") {
  DynamicJsonDocument login(1024);login["account"]=input["account"];if(mode=="login"){login["password"]=input["password"];login["apiError"]="";}else login["code"]=input["code"];if(mode=="login_tfa"){login.clear();login["tfaKey"]=input["tfaKey"];login["tfaCode"]=input["code"];}serializeJson(login,body);input.clear();input.shrinkToFit();login.clear();login.shrinkToFit();
  code=request(mode=="login_tfa"?String(XTouchCloudEndpoints::get(region.c_str())->website)+"/api/sign-in/tfa":String(XTouchCloudEndpoints::get(region.c_str())->api)+"/v1/user-service/user/login","",body,reply);wipe(body);
  DynamicJsonDocument data(6144);
  if(code==200&&!deserializeJson(data,reply)) {
   token=data["accessToken"]|""; if(token.isEmpty())token=data["token"]|"";
   if(token.isEmpty()){token=data["data"]["accessToken"]|"";if(token.isEmpty())token=data["data"]["token"]|"";}
   if(!token.isEmpty()&&token.length()<=2048) {
    String uid;bool found=uidFromToken(token,uid);data.clear();data.shrinkToFit();wipe(reply);
    if(!found){code=request(String(XTouchCloudEndpoints::get(region.c_str())->api)+"/v1/user-service/my/profile",token,"",reply);DynamicJsonDocument profile(4096);
      if(code==200&&!deserializeJson(profile,reply)){uid=profile["uidStr"].as<String>();if(uid=="null"||uid.isEmpty())uid=profile["uid"].as<String>();found=uid!="null"&&!uid.isEmpty()&&uid.length()<40;}}
    XTouchCloudMemory::output(out);
    if(found){out["token"]=token;out["userId"]=uid.startsWith("u_")?uid:"u_"+uid;out["ok"]=true;if(remembered.length())out["passwordEnc"]=remembered;}
   }
   if(!out["ok"].as<bool>()) {
    if(!out.capacity())XTouchCloudMemory::output(out);
    const char *key=XTouchCloudAuth::field(data.as<JsonVariantConst>(),"tfaKey");
    if(strlen(key)<=512&&*key)out["tfaKey"]=key;
    out["error"]=XTouchCloudAuth::challenge(data.as<JsonVariantConst>());
   }
  }
 } else if(mode=="send_code") {
  bool phone=!strchr(input["account"]|"",'@');
  DynamicJsonDocument codeBody(512);codeBody[phone?"phone":"email"]=input["account"];codeBody["type"]="codeLogin";serializeJson(codeBody,body);
  input.clear();input.shrinkToFit();
  String url=String(XTouchCloudEndpoints::get(region.c_str())->api)+(phone?"/v1/user-service/user/sendsmscode":"/v1/user-service/user/sendemail/code");
  code=request(url,"",body,reply);wipe(body);XTouchCloudMemory::output(out);DynamicJsonDocument response(1024);
  if(code>=200&&code<300&&(reply.isEmpty()||(!deserializeJson(response,reply)&&!response["error"].is<const char *>()&&(!response.containsKey("success")||response["success"].as<bool>())&&(!response.containsKey("code")||response["code"].as<int>()==0))))out["ok"]=true;
 }
 return code;
}
}
