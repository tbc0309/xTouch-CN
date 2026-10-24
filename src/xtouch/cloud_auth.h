#pragma once
#include <ArduinoJson.h>
#include <cstring>
namespace XTouchCloudAuth {
inline const char *field(JsonVariantConst response,const char *name){
 const char *value=response[name]|"";
 return *value?value:(response["data"][name]|"");
}
// A token-less failure is not automatically a verification-code challenge.
inline const char *challenge(JsonVariantConst response){
 const char *kind=field(response,"loginType"),*key=field(response,"tfaKey");
 if(!std::strcmp(kind,"tfa")||*key)return "two_factor_required";
 return !std::strcmp(kind,"verifyCode")?"verification_required":"authentication_rejected";
}
}
