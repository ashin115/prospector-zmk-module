#include <lvgl.h>
#include <zephyr/kernel.h>

#include "modifier_indicator.h"
#include "wpm_meter.h"
#include "layer_display.h"
#include "battery_circles.h"
#include "output.h"
#include "display_colors.h"

#include <fonts.h>

#ifdef CONFIG_PROSPECTOR_CUSTOM_IDLE_FEATURE
#include "idle_monitor.h"
#include <brightness.h>

enum custom_idle_state {
    CUSTOM_IDLE_ACTIVE,
    CUSTOM_IDLE_DIMMED,
    CUSTOM_IDLE_SCREENSAVER,
};

static lv_obj_t *screensaver_label;
static lv_obj_t *screensaver_overlay;
static lv_timer_t *idle_timer;
static enum custom_idle_state idle_state = CUSTOM_IDLE_ACTIVE;
static int16_t saver_x = 8;
static int16_t saver_y = 8;
static int16_t saver_dx = 2;
static int16_t saver_dy = 2;

#define CUSTOM_SCREEN_WIDTH 260
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

static lv_obj_t *create_panel(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h) {
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, w, h);
    lv_obj_set_pos(panel, x, y);
    lv_obj_set_style_bg_color(panel, lv_color_hex(DISPLAY_COLOR_SCREEN_PANEL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(DISPLAY_COLOR_SCREEN_DIVIDER), LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_CLICKABLE);
    return panel;
}

#ifdef CONFIG_PROSPECTOR_CUSTOM_IDLE_FEATURE
static void screensaver_hide(void) {
    lv_obj_add_flag(screensaver_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void screensaver_show(void) {
    saver_x = 8;
    saver_y = 8;
    saver_dx = 2;
    saver_dy = 2;
    lv_obj_set_pos(screensaver_label, saver_x, saver_y);
    lv_obj_clear_flag(screensaver_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void screensaver_tick(void) {
    int16_t label_w = lv_obj_get_width(screensaver_label);
    int16_t label_h = lv_obj_get_height(screensaver_label);
    int16_t max_x = CUSTOM_SCREEN_WIDTH - label_w;
    int16_t max_y = CUSTOM_SCREEN_HEIGHT - label_h;

    saver_x += saver_dx;
    saver_y += saver_dy;

    if (saver_x <= 0 || saver_x >= max_x) {
        saver_dx = -saver_dx;
        saver_x += saver_dx;
    }

    if (saver_y <= 0 || saver_y >= max_y) {
        saver_dy = -saver_dy;
        saver_y += saver_dy;
    }

    lv_obj_set_pos(screensaver_label, saver_x, saver_y);
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
    screensaver_tick();
}
#endif

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(DISPLAY_COLOR_SCREEN_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, 255, LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(screen, 0, LV_PART_MAIN);

    create_panel(screen, 8, 4, 244, 30);
    create_panel(screen, 8, 38, 244, 126);
    create_panel(screen, 8, 168, 244, 68);

    zmk_widget_modifier_indicator_init(&modifier_indicator_widget, screen);
    lv_obj_set_pos(zmk_widget_modifier_indicator_obj(&modifier_indicator_widget), 15, 8);

    zmk_widget_wpm_meter_init(&wpm_meter_widget, screen);
    lv_obj_set_pos(zmk_widget_wpm_meter_obj(&wpm_meter_widget), 0, 42);

    zmk_widget_layer_display_init(&layer_display_widget, screen);
    lv_obj_set_pos(zmk_widget_layer_display_obj(&layer_display_widget), 10, 151);

    zmk_widget_battery_circles_init(&battery_circles_widget, screen);
    lv_obj_set_pos(zmk_widget_battery_circles_obj(&battery_circles_widget), 12, 173);

    zmk_widget_output_init(&output_widget, screen);
    lv_obj_set_pos(zmk_widget_output_obj(&output_widget), 132, 173);

#ifdef CONFIG_PROSPECTOR_CUSTOM_IDLE_FEATURE
    screensaver_overlay = lv_obj_create(screen);
    lv_obj_set_size(screensaver_overlay, CUSTOM_SCREEN_WIDTH, CUSTOM_SCREEN_HEIGHT);
    lv_obj_set_pos(screensaver_overlay, 0, 0);
    lv_obj_set_style_bg_color(screensaver_overlay, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screensaver_overlay, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(screensaver_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screensaver_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(screensaver_overlay, 0, LV_PART_MAIN);

    screensaver_label = lv_label_create(screensaver_overlay);
    lv_label_set_text(screensaver_label, "PROSPECTOR");
    lv_obj_set_style_text_font(screensaver_label, &FG_Medium_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(screensaver_label, lv_color_hex(0x8f8f8f), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screensaver_label, LV_OPA_TRANSP, LV_PART_MAIN);
    screensaver_hide();

    prospector_idle_monitor_init();
    idle_timer = lv_timer_create(custom_idle_timer_cb, IDLE_TIMER_PERIOD_MS, NULL);
    ARG_UNUSED(idle_timer);
#endif

    return screen;
}
