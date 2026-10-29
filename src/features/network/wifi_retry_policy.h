#pragma once
#include <stdint.h>

// One connection round is bounded; provisioning owns subsequent retries.
struct XTouchWifiRetry {
    uint32_t started=0,next=0;
    unsigned attempts=0;
    bool exhausted=false;
    void begin(uint32_t now){started=now?now:1;next=0;attempts=1;exhausted=false;}
    void connected(){started=next=attempts=0;exhausted=false;}
    void stopped(){connected();}
    void disconnected(uint32_t now){
        if(!started)started=now?now:1;
        if(attempts>=3){exhausted=true;next=0;}
        else next=now+(attempts>1?5000u:2000u);
    }
    bool due(uint32_t now){
        if(exhausted||!next||int32_t(now-next)<0)return false;
        next=0;++attempts;return true;
    }
};
