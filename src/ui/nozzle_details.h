#pragma once
#include "ui.h"
#ifdef __cplusplus
extern "C" {
#endif
struct ui_nozzle_request {
    unsigned printer, index, target, action;
};
void ui_nozzle_details_show(void);
void ui_nozzle_actions_show(void);
void ui_hotend_details_show(void);
bool ui_nozzle_request_allowed(const struct ui_nozzle_request *r);
#ifdef __cplusplus
}
#endif
