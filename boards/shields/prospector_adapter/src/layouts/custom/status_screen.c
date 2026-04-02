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
#include "sleeping_cat.h"
#include <brightness.h>

enum custom_idle_state {
    CUSTOM_IDLE_ACTIVE,
    CUSTOM_IDLE_DIMMED,
    CUSTOM_IDLE_SCREENSAVER,
};

static lv_obj_t *screensaver_overlay;
static lv_obj_t *screensaver_canvas;
static lv_obj_t *screensaver_zzz;
static lv_timer_t *idle_timer;
static enum custom_idle_state idle_state = CUSTOM_IDLE_ACTIVE;
static int16_t saver_x = 8;
static int16_t saver_y = 8;
static int16_t saver_dx = 1;
static int16_t saver_dy = 1;
static int cat_frame_idx;
static int cat_frame_counter;

static uint8_t cat_canvas_buf[LV_CANVAS_BUF_SIZE(CAT_RENDER_W, CAT_RENDER_H, 32, 1)];

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

#ifdef CONFIG_PROSPECTOR_CUSTOM_IDLE_FEATURE
static void render_cat_frame(int frame) {
    lv_canvas_fill_bg(screensaver_canvas, lv_color_hex(0x000000), LV_OPA_COVER);

    lv_layer_t layer;
    lv_canvas_init_layer(screensaver_canvas, &layer);

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.radius = 0;

    for (int y = 0; y < CAT_ART_H; y++) {
        for (int x = 0; x < CAT_ART_W; x++) {
            uint8_t idx = cat_frames[frame][y][x];
            if (idx == 0) {
                continue;
            }
            rect_dsc.bg_color = lv_color_hex(cat_palette[idx]);
            lv_area_t area = {
                .x1 = x * CAT_SCALE,
                .y1 = y * CAT_SCALE,
                .x2 = (x + 1) * CAT_SCALE - 1,
                .y2 = (y + 1) * CAT_SCALE - 1,
            };
            lv_draw_rect(&layer, &rect_dsc, &area);
        }
    }

    lv_canvas_finish_layer(screensaver_canvas, &layer);
}

static void screensaver_zzz_opa_cb(void *obj, int32_t value) {
    lv_obj_set_style_text_opa((lv_obj_t *)obj, value, LV_PART_MAIN);
}

static void screensaver_start_zzz_anim(void) {
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, screensaver_zzz);
    lv_anim_set_values(&anim, 40, 200);
    lv_anim_set_time(&anim, 2000);
    lv_anim_set_playback_time(&anim, 2000);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&anim, screensaver_zzz_opa_cb);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
    lv_anim_start(&anim);
}

static void screensaver_stop_zzz_anim(void) {
    lv_anim_del(screensaver_zzz, screensaver_zzz_opa_cb);
}

static void screensaver_hide(void) {
    screensaver_stop_zzz_anim();
    lv_obj_add_flag(screensaver_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void screensaver_show(void) {
    saver_x = 8;
    saver_y = 16;
    saver_dx = 1;
    saver_dy = 1;
    cat_frame_idx = 0;
    cat_frame_counter = 0;
    render_cat_frame(0);
    lv_obj_set_pos(screensaver_canvas, saver_x, saver_y);
    lv_obj_set_pos(screensaver_zzz, saver_x + CAT_RENDER_W - 6, saver_y - 14);
    lv_obj_clear_flag(screensaver_overlay, LV_OBJ_FLAG_HIDDEN);
    screensaver_start_zzz_anim();
}

static void screensaver_tick(void) {
    /* Switch animation frame every 20 ticks (2 s) */
    cat_frame_counter++;
    if (cat_frame_counter >= 20) {
        cat_frame_counter = 0;
        cat_frame_idx = (cat_frame_idx + 1) % CAT_NUM_FRAMES;
        render_cat_frame(cat_frame_idx);
    }

    int16_t max_x = CUSTOM_SCREEN_WIDTH - CAT_RENDER_W;
    int16_t max_y = CUSTOM_SCREEN_HEIGHT - CAT_RENDER_H;

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

    lv_obj_set_pos(screensaver_canvas, saver_x, saver_y);
    lv_obj_set_pos(screensaver_zzz, saver_x + CAT_RENDER_W - 6, saver_y - 14);
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
    lv_obj_clear_flag(screensaver_overlay, LV_OBJ_FLAG_SCROLLABLE);

    screensaver_canvas = lv_canvas_create(screensaver_overlay);
    lv_canvas_set_buffer(screensaver_canvas, cat_canvas_buf,
                         CAT_RENDER_W, CAT_RENDER_H, LV_COLOR_FORMAT_ARGB8888);
    lv_canvas_fill_bg(screensaver_canvas, lv_color_hex(0x000000), LV_OPA_COVER);

    screensaver_zzz = lv_label_create(screensaver_overlay);
    lv_label_set_text(screensaver_zzz, "zzZ");
    lv_obj_set_style_text_font(screensaver_zzz, &FG_Medium_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(screensaver_zzz, lv_color_hex(0x607080), LV_PART_MAIN);

    screensaver_hide();

    prospector_idle_monitor_init();
    idle_timer = lv_timer_create(custom_idle_timer_cb, IDLE_TIMER_PERIOD_MS, NULL);
    ARG_UNUSED(idle_timer);
#endif

    return screen;
}
