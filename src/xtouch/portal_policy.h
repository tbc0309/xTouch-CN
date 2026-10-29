#pragma once
#include "features/config.h"
inline bool xtouch_portal_transport_maintenance(bool reboot,bool uploading,bool cloudBusy){
 return reboot||uploading||(XTOUCH_PORTAL_RECLAIM_TLS_ON_READ&&cloudBusy);
}
