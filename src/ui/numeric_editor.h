#pragma once
#include "ui.h"
#ifdef __cplusplus
extern "C" {
#endif
struct ui_numeric_field {
    const char *name;
    unsigned value, min, max;
};
bool ui_numeric_active(void);
void ui_numeric_cancel(void);
void ui_numeric_commit(void);
typedef void (*ui_numeric_apply)(unsigned index, unsigned value);
void ui_numeric_show(const struct ui_numeric_field *fields, unsigned count, unsigned selected,
                     ui_numeric_apply apply, void (*cancel)(void), void (*more)(void));
#ifdef __cplusplus
}
#endif
