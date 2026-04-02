#include "idle_monitor.h"

#include <zephyr/kernel.h>
#include <zmk/display.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/wpm_state_changed.h>

static uint32_t last_activity_ms;
static bool initialized;

struct idle_activity_state {
    bool active;
};

static void idle_activity_update_cb(struct idle_activity_state state) {
    ARG_UNUSED(state);
    last_activity_ms = k_uptime_get_32();
}

static struct idle_activity_state idle_activity_get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    return (struct idle_activity_state){.active = true};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_idle_monitor_key, struct idle_activity_state,
                            idle_activity_update_cb, idle_activity_get_state)
ZMK_SUBSCRIPTION(widget_idle_monitor_key, zmk_keycode_state_changed);

ZMK_DISPLAY_WIDGET_LISTENER(widget_idle_monitor_wpm, struct idle_activity_state,
                            idle_activity_update_cb, idle_activity_get_state)
ZMK_SUBSCRIPTION(widget_idle_monitor_wpm, zmk_wpm_state_changed);

void prospector_idle_monitor_init(void) {
    if (initialized) {
        return;
    }

    initialized = true;
    last_activity_ms = k_uptime_get_32();
    widget_idle_monitor_key_init();
    widget_idle_monitor_wpm_init();
}

uint32_t prospector_idle_monitor_idle_ms(void) {
    return k_uptime_get_32() - last_activity_ms;
}
