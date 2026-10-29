#pragma once
#include <stdbool.h>
typedef enum { XTOUCH_SETUP_BACK_NONE, XTOUCH_SETUP_BACK_SETTINGS, XTOUCH_SETUP_BACK_METHODS } xtouch_setup_back_target;
static inline xtouch_setup_back_target xtouch_setup_back(bool configured,bool multipleMethods){
 return configured?XTOUCH_SETUP_BACK_SETTINGS:multipleMethods?XTOUCH_SETUP_BACK_METHODS:XTOUCH_SETUP_BACK_NONE;
}
