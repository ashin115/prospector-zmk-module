#include <lvgl.h>
#include <zephyr/kernel.h>

#include "modifier_indicator.h"
#include "wpm_meter.h"
#include "layer_display.h"
#include "battery_circles.h"
#include "output.h"

#include <fonts.h>

#ifdef CONFIG_PROSPECTOR_CUSTOM_IDLE_FEATURE
#include "idle_monitor.h"
#include "golden_forest.h"
#include <brightness.h>

enum custom_idle_state {
    CUSTOM_IDLE_ACTIVE,
    CUSTOM_IDLE_DIMMED,
    CUSTOM_IDLE_SCREENSAVER,
};

static lv_obj_t *screensaver_overlay;
static lv_obj_t *screensaver_img;
static lv_timer_t *idle_timer;
static enum custom_idle_state idle_state = CUSTOM_IDLE_ACTIVE;

#define CUSTOM_SCREEN_WIDTH 280
#define CUSTOM_SCREEN_HEIGHT 240
#define IDLE_TIMER_PERIOD_MS 100
#define CUSTOM_DIM_TIMEOUT_MS (CONFIG_PROSPECTOR_CUSTOM_IDLE_DIM_TIMEOUT_SEC * 1000U)
#define CUSTOM_SCREENSAVER_TIMEOUT_MS (CONFIG_PROSPECTOR_CUSTOM_IDLE_SCREENSAVER_TIMEOUT_SEC * 1000U)
#endif

static struct zmk_widget_modifier_indicator modifier_indicator_widget;
static struct zmk_widget_wpm_meter wpm_meter_widget;
static struct zmk_widget_layer_display layer_display_widget;
static struct zmk_widget_battery_circles battery_circles_widget;
static struct zmk_widget_output output_widget;

#ifdef CONFIG_PROSPECTOR_CUSTOM_IDLE_FEATURE
static void screensaver_zoom_cb(void *var, int32_t value) {
    lv_image_set_scale((lv_obj_t *)var, (uint16_t)value);
}

static void screensaver_hide(void) {
    lv_anim_del(screensaver_img, screensaver_zoom_cb);
    lv_image_set_scale(screensaver_img, 256);
    lv_obj_add_flag(screensaver_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void screensaver_show(void) {
    lv_obj_set_pos(screensaver_img, 0, 0);
    lv_obj_clear_flag(screensaver_overlay, LV_OBJ_FLAG_HIDDEN);

    /* Gentle zoom: slowly scale from 1.0x to ~1.09x and back */
    lv_image_set_pivot(screensaver_img, GOLDEN_FOREST_W / 2, GOLDEN_FOREST_H / 2);

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, screensaver_img);
    lv_anim_set_values(&anim, 256, 280);
    lv_anim_set_time(&anim, 10000);
    lv_anim_set_playback_time(&anim, 10000);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&anim, screensaver_zoom_cb);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
    lv_anim_start(&anim);
}

static void enter_active_state(void) {
    if (idle_state == CUSTOM_IDLE_ACTIVE) {
        return;
    }

    screensaver_hide();
    prospector_brightness_clear_override();
    idle_state = CUSTOM_IDLE_ACTIVE;
}

static void enter_dim_state(void) {
    if (idle_state != CUSTOM_IDLE_ACTIVE) {
        return;
    }

    prospector_brightness_set(CONFIG_PROSPECTOR_CUSTOM_IDLE_DIM_BRIGHTNESS);
    idle_state = CUSTOM_IDLE_DIMMED;
}

static void enter_screensaver_state(void) {
    if (idle_state == CUSTOM_IDLE_SCREENSAVER) {
        return;
    }

    prospector_brightness_set(CONFIG_PROSPECTOR_CUSTOM_IDLE_DIM_BRIGHTNESS);
    screensaver_show();
    idle_state = CUSTOM_IDLE_SCREENSAVER;
}

static void custom_idle_timer_cb(lv_timer_t *timer) {
    ARG_UNUSED(timer);

    uint32_t idle_ms = prospector_idle_monitor_idle_ms();

    if (idle_ms < CUSTOM_DIM_TIMEOUT_MS) {
        enter_active_state();
        return;
    }

    if (idle_ms < CUSTOM_SCREENSAVER_TIMEOUT_MS) {
        enter_dim_state();
        return;
    }

    enter_screensaver_state();
}
#endif

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, 255, LV_PART_MAIN);

    zmk_widget_modifier_indicator_init(&modifier_indicator_widget, screen);
    lv_obj_set_pos(zmk_widget_modifier_indicator_obj(&modifier_indicator_widget), 25, 8);

    zmk_widget_wpm_meter_init(&wpm_meter_widget, screen);
    lv_obj_set_pos(zmk_widget_wpm_meter_obj(&wpm_meter_widget), 10, 42);

    zmk_widget_layer_display_init(&layer_display_widget, screen);
    lv_obj_set_pos(zmk_widget_layer_display_obj(&layer_display_widget), 10, 142);

    zmk_widget_battery_circles_init(&battery_circles_widget, screen);
    lv_obj_set_pos(zmk_widget_battery_circles_obj(&battery_circles_widget), 11, 170);

    zmk_widget_output_init(&output_widget, screen);
    lv_obj_set_pos(zmk_widget_output_obj(&output_widget), 148, 170);

#ifdef CONFIG_PROSPECTOR_CUSTOM_IDLE_FEATURE
    screensaver_overlay = lv_obj_create(screen);
    lv_obj_remove_style_all(screensaver_overlay);
    lv_obj_set_size(screensaver_overlay, CUSTOM_SCREEN_WIDTH, CUSTOM_SCREEN_HEIGHT);
    lv_obj_set_pos(screensaver_overlay, 0, 0);
    lv_obj_set_style_bg_color(screensaver_overlay, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screensaver_overlay, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_add_flag(screensaver_overlay, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_clear_flag(screensaver_overlay, LV_OBJ_FLAG_SCROLLABLE);

    screensaver_img = lv_image_create(screensaver_overlay);
    lv_image_set_src(screensaver_img, &golden_forest_img);
    lv_obj_set_pos(screensaver_img, 0, 0);

    screensaver_hide();

    prospector_idle_monitor_init();
    idle_timer = lv_timer_create(custom_idle_timer_cb, IDLE_TIMER_PERIOD_MS, NULL);
    ARG_UNUSED(idle_timer);
#endif

    return screen;
}
