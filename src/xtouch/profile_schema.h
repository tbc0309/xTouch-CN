#pragma once
#include <ArduinoJson.h>
#include <ctype.h>
#include "profile_route.h"

namespace XTouchSchema {
constexpr size_t MaxAccounts = 50;
constexpr size_t MaxConfigBytes = 196608;
constexpr size_t MaxConfigCapacity = 262144;
inline bool labHost(const char *host) {
    return XTouchProfileRoute::lanHost(host);
}
inline bool text(JsonVariantConst value, size_t limit, bool required = false) {
    if (!value.is<const char *>())
        return !required && value.isNull();
    const char *s = value.as<const char *>();
    if ((required && !*s) || strlen(s) >= limit)
        return false;
    for (const uint8_t *p = (const uint8_t *)s; *p; ++p)
        if (*p < 32 || *p == 127)
            return false;
    return true;
}
inline bool asciiName(JsonVariantConst value) {
    if (!text(value, 64))
        return false;
    for (const unsigned char *p = (const unsigned char *)(value | ""); *p; p++)
        if (*p < 32 || *p > 126)
            return false;
    return true;
}
inline bool account(JsonVariantConst a) {
    if(a["simulator"].as<bool>())
        return text(a["id"],32,true) && asciiName(a["name"]) && labHost(a["mqttHost"]|"") &&
            a["mqttPort"].is<unsigned>() && a["mqttPort"].as<unsigned>()>0 && a["mqttPort"].as<unsigned>()<=65535 &&
            !strcmp(a["token"]|"","xtouch-lab") && !strcmp(a["userId"]|"","xtouch-lab") &&
            !a["rememberPassword"].as<bool>() && !strlen(a["passwordEnc"]|"") && !strlen(a["account"]|"");
    return a.is<JsonObjectConst>() && text(a["id"], 32, true) && text(a["token"], 2049) &&
           text(a["userId"], 48) && text(a["account"], 101) && text(a["name"], 64) &&
           text(a["passwordEnc"], 212) && text(a["tokenEnc"],2772) && text(a["password"],129) &&
           (!a.containsKey("tokenEncrypted") || a["tokenEncrypted"].is<bool>()) &&
           (!a.containsKey("passwordEncrypted") || a["passwordEncrypted"].is<bool>()) &&
           (!a.containsKey("rememberPassword") || a["rememberPassword"].is<bool>()) &&
           (!a.containsKey("loginCleared") || a["loginCleared"].is<bool>()) &&
           (!a.containsKey("signInMode") || !strcmp(a["signInMode"] | "", "password") || !strcmp(a["signInMode"] | "", "code")) &&
           (!strcmp(a["loginMethod"] | "token", "token") ||
            !strcmp(a["loginMethod"] | "token", "password")) &&
           (!strcmp(a["authMode"] | "token", "token") ||
            !strcmp(a["authMode"] | "token", "password")) &&
           (!strcmp(a["region"] | "cn", "cn") || !strcmp(a["region"] | "cn", "global"));
}
inline bool printer(JsonVariantConst p) {
    if (!p.is<JsonObjectConst>() || !text(p["name"], 64) || !text(p["printerModel"], 32) ||
        !text(p["serialNumber"], 32, true))
        return false;
    const char *mode = p["mode"] | "lan";
    if (!strcmp(mode, "cloud"))
        return text(p["accountId"], 32, true) && text(p["cameraLanHost"],64) &&
            (!strlen(p["cameraLanHost"]|"") || labHost(p["cameraLanHost"]));
    const char *host = p["host"] | "", *code = p["accessCode"] | "";
    if (strcmp(mode, "lan") || !text(p["host"], 64, true) ||
        !text(p["accessCodeEnc"],64) || (p.containsKey("accessCodeEncrypted")&&!p["accessCodeEncrypted"].is<bool>()) ||
        (*code?strlen(code)!=8:!strlen(p["accessCodeEnc"]|"")))
        return false;
    for (const char *c = host; *c; ++c)
        if (!(isalnum((unsigned char)*c) || *c == '.' || *c == '-' || *c == '_'))
            return false;
    for (const char *c = code; *c; ++c)
        if ((uint8_t)*c < 33 || (uint8_t)*c > 126)
            return false;
    return true;
}
inline bool lanCandidate(JsonVariantConst p) {
    if(!printer(p)||strlen(p["accessCode"]|"")!=8||strcmp(p["mode"]|"","lan")||!asciiName(p["name"])||!text(p["name"],64,true)||!text(p["printerModel"],32,true))return false;
    bool visible=false;for(const char *c=p["name"]|"";*c;++c)visible|=*c!=' ';
    for(const char *c=p["serialNumber"]|"";*c;++c)if(!isalnum((unsigned char)*c)&&*c!='_'&&*c!='-')return false;
    return visible;
}
inline bool root(JsonDocument &doc) {
    if (!text(doc["ssid"], 33, true) || !text(doc["pwd"], 65) || !doc["printers"].is<JsonArray>())
        return false;
    if (doc["printers"].size() > 100 ||
        (doc.containsKey("cloudAccounts") && !doc["cloudAccounts"].is<JsonArray>()) ||
        doc["cloudAccounts"].size() > MaxAccounts)
        return false;
    for (JsonVariantConst a : doc["cloudAccounts"].as<JsonArrayConst>()) {
        if (!account(a))
            return false;
        unsigned matches = 0;
        for (JsonVariantConst b : doc["cloudAccounts"].as<JsonArrayConst>())
            if (!strcmp(a["id"], b["id"] | ""))
                ++matches;
        if (matches != 1)
            return false;
    }
    for (JsonVariantConst p : doc["printers"].as<JsonArrayConst>()) {
        if (p.isNull())
            continue;
        if (!printer(p))
            return false;
        if (!strcmp(p["mode"] | "lan", "cloud")) {
            bool found = false;
            for (JsonVariantConst a : doc["cloudAccounts"].as<JsonArrayConst>())
                if (!strcmp(a["id"], p["accountId"])) {
                    if(a["simulator"].as<bool>() && strncmp(p["serialNumber"]|"","XTSIM",5))return false;
                    found = true;
                }
            if (!found)
                return false;
        }
    }
    if(doc.containsKey("activePrinter")&&!doc["activePrinter"].is<unsigned>())return false;
    unsigned active = doc["activePrinter"] | 0U;
    return doc["printers"].size() == 0 ? active == 0 : active < doc["printers"].size();
}
} // namespace XTouchSchema
