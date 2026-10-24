#pragma once
#include <stdbool.h>
#include <stdint.h>
#define HMS_MAX_CODES 50
#ifdef __cplusplus
extern "C" {
#endif
void hms_reset(void);
bool hms_set_print_error(uint32_t code);
bool hms_set_health(const uint64_t *codes,unsigned count,bool truncated);
uint32_t hms_print_error(void);
unsigned hms_health_count(void);
uint64_t hms_health_at(unsigned index);
bool hms_health_truncated(void);
bool hms_has_pending(void);
void hms_dismiss(void);
uint32_t hms_revision(void);
unsigned hms_popup_timeout_ms(void);
bool hms_command_verification_failed(void);
/* Presentation filters never erase the raw fault used by command guards. */
bool hms_authorization_advisory_pending(void);
bool hms_health_visible(unsigned index);
uint32_t hms_visible_print_error(void);
#ifdef XTOUCH_ERROR_SELFTEST
bool hms_selftest(void);
#endif
#ifdef __cplusplus
}
#endif
