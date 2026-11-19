#pragma once
#include <lvgl.h>
#if LVGL_VERSION_MAJOR >= 9
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {uint32_t id;const void *payload;void *user_data;} lv_msg_t;
typedef void (*lv_msg_subscribe_cb_t)(void *,lv_msg_t *);
lv_event_code_t ui_message_event_id(void);
#define LV_EVENT_MSG_RECEIVED ui_message_event_id()
void *lv_msg_subscribe(uint32_t id,lv_msg_subscribe_cb_t callback,void *user_data);
void lv_msg_subsribe_obj(uint32_t id,lv_obj_t *object,void *user_data);
void lv_msg_unsubscribe(void *subscription);
void lv_msg_send(uint32_t id,const void *payload);
static inline uint32_t lv_msg_get_id(const lv_msg_t *m){return m?m->id:0;}
static inline const void *lv_msg_get_payload(const lv_msg_t *m){return m?m->payload:NULL;}
static inline void *lv_msg_get_user_data(const lv_msg_t *m){return m?m->user_data:NULL;}
static inline lv_msg_t *lv_event_get_msg(lv_event_t *e){return (lv_msg_t*)lv_event_get_param(e);}
#ifdef __cplusplus
}
#endif
#endif
