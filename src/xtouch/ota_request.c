#include "ota_request.h"
#include "portal.h"
static bool pending;
static unsigned taps;
static uint32_t last;
void xtouch_ota_version_tap(lv_event_t *event)
{
    if(lv_event_get_code(event)==LV_EVENT_DELETE){taps=0;return;}
    if(lv_event_get_code(event)!=LV_EVENT_SHORT_CLICKED)return;
    uint32_t now=lv_tick_get();if(now-last>800)taps=0;last=now;
    if(++taps==8){taps=0;pending=false;xtouch_portal_request_reset();}
}
bool xtouch_ota_take_request(void){if(taps==3 && lv_tick_elaps(last)>900){taps=0;pending=true;}else if(taps && lv_tick_elaps(last)>1200)taps=0;bool value=pending;pending=false;return value;}
