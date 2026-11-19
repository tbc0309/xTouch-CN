#pragma once
#include <cstring>

namespace XTouchCloudEndpoints {
struct Region {const char *api,*website,*mqtt,*taskHistory;};
inline const Region *get(const char *region) {
 static const Region cn={"https://api.bambulab.cn","https://bambulab.cn","cn.mqtt.bambulab.com","https://api.bambulab.cn/v1/user-service/my/tasks?limit=50"};
 static const Region global={"https://api.bambulab.com","https://bambulab.com","us.mqtt.bambulab.com","https://api.bambulab.com/v1/user-service/my/tasks?limit=50"};
 if(!region)return nullptr;
 return !std::strcmp(region,"cn")?&cn:!std::strcmp(region,"global")?&global:nullptr;
}
}
