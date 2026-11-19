#pragma once
#include <stdbool.h>

/* A missing layer report must not be treated as an explicit layer zero. */
static inline bool xtouch_print_preparing(bool explicit_prepare, bool running,
                                          bool layer_known, int current_layer) {
    return explicit_prepare || (running && layer_known && current_layer == 0);
}
