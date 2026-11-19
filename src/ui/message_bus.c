#include "message_bus.h"
#if LVGL_VERSION_MAJOR >= 9

/* UI-thread-only dispatch. Payloads remain borrowed for the synchronous call. */
typedef struct Subscription {
    struct Subscription *next;
    uint32_t id;
    lv_obj_t *object;
    lv_msg_subscribe_cb_t callback;
    void *data;
    bool active;
} Subscription;

static Subscription *subscriptions, *subscription_tail;
static unsigned dispatching;
static lv_event_code_t message_event;

lv_event_code_t ui_message_event_id(void)
{
    if (!message_event) message_event = (lv_event_code_t)lv_event_register_id();
    return message_event;
}

static void collect(void)
{
    if (dispatching) return;
    Subscription **link = &subscriptions;
    subscription_tail = NULL;
    while (*link) {
        Subscription *s = *link;
        if (!s->active) {
            *link = s->next;
            lv_free(s);
        } else {
            subscription_tail = s;
            link = &s->next;
        }
    }
}

void lv_msg_unsubscribe(void *node)
{
    for (Subscription *s = subscriptions; s; s = s->next) {
        if (s == node) { s->active = false; break; }
    }
    collect();
}

static void deleted(lv_event_t *event)
{
    lv_obj_t *object = lv_event_get_target_obj(event);
    for (Subscription *s = subscriptions; s; s = s->next) {
        if (s->object == object) s->active = false;
    }
    collect();
}

void *lv_msg_subscribe(uint32_t id, lv_msg_subscribe_cb_t callback, void *data)
{
    Subscription *s = lv_malloc(sizeof(*s));
    if (!s) return NULL;
    *s = (Subscription){.id = id, .callback = callback, .data = data, .active = true};
    /* Preserve the registration order used by LVGL 8. */
    if (subscription_tail) subscription_tail->next = s;
    else subscriptions = s;
    subscription_tail = s;
    return s;
}

void lv_msg_subsribe_obj(uint32_t id, lv_obj_t *object, void *data)
{
    if (!object) return;
    bool first = true;
    for (Subscription *s = subscriptions; s; s = s->next) {
        if (s->active && s->object == object) first = false;
    }
    Subscription *s = lv_msg_subscribe(id, NULL, data);
    if (!s) return;
    s->object = object;
    if (first) lv_obj_add_event_cb(object, deleted, LV_EVENT_DELETE, NULL);
}

void lv_msg_send(uint32_t id, const void *payload)
{
    ++dispatching;
    /* A callback may append subscriptions or delete objects. Keep existing nodes
     * alive and exclude newly appended nodes from this particular dispatch. */
    Subscription *last = subscription_tail;
    for (Subscription *s = subscriptions; s;) {
        Subscription *next = s->next;
        if (s->active && s->id == id) {
            lv_msg_t message = {id, payload, s->data};
            if (s->object) lv_obj_send_event(s->object, ui_message_event_id(), &message);
            else if (s->callback) s->callback(s, &message);
        }
        if (s == last) break;
        s = next;
    }
    --dispatching;
    collect();
}
#endif
