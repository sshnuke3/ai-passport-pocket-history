// main/demo_pocket_history.c —— 《口袋历史·上下五千年》离线内容库玩法
// 形态:开机直达(见 main.c BOOT_DIRECT_PLAY)。自建 UI,不沿用 demo 测试外壳。
// 交互:首页 上/下选择、确定进入;列表页 上/下翻页、长按确定返回首页;随机卷 上/下/确定 均换一卷。
#include "demo.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"   // bsp_lvgl_lock / bsp_lvgl_unlock
#include "lvgl.h"
#include "pocket_history_data.h"
#include "esp_random.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// ---- 主题色(自建设计,纸底墨字朱红印) ----
#define PH_PAPER   0xEFE6D4
#define PH_INK     0x2B2A28
#define PH_ACCENT  0xB23A2E
#define PH_DIM     0x9A9184
#define PH_CARD    0xE4D8BF
#define PH_CARD_HI 0xF7EEDC

LV_FONT_DECLARE(app_font_16);
LV_FONT_DECLARE(app_font_20);
static lv_font_t s_font16;
static lv_font_t s_font20;
static bool s_fonts_ready;

typedef enum { MODE_HOME, MODE_DYNASTY, MODE_FIGURE, MODE_TALE, MODE_RANDOM } ph_mode_t;

static lv_obj_t *s_scr;
static ph_mode_t s_mode = MODE_HOME;
static int s_sel;            // 首页选中项 0..3
static int s_idx;            // 当前条目下标(列表或随机)
static ph_mode_t s_rnd_kind; // 随机卷当前命中的池(朝代/先贤/典故)
static uint32_t s_rng;       // xorshift 状态

static const char *MODULE_NAME[4] = { "朝代长河", "先贤列传", "典故菁华", "随机一卷" };

static uint32_t ph_rand(void) {
    uint32_t x = s_rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    s_rng = x ? x : 0x9E3779B9u;
    return s_rng;
}

// 随机:从三个池合并里挑一条,只落 s_rnd_kind + s_idx,不改变 s_mode(保持 RANDOM 态)
static void ph_random_pick(void) {
    int total = PH_DYNASTY_COUNT + PH_FIGURE_COUNT + PH_TALE_COUNT;
    int r = (int)(ph_rand() % (uint32_t)total);
    if (r < PH_DYNASTY_COUNT) {
        s_rnd_kind = MODE_DYNASTY; s_idx = r;
    } else if (r < PH_DYNASTY_COUNT + PH_FIGURE_COUNT) {
        s_rnd_kind = MODE_FIGURE; s_idx = r - PH_DYNASTY_COUNT;
    } else {
        s_rnd_kind = MODE_TALE; s_idx = r - PH_DYNASTY_COUNT - PH_FIGURE_COUNT;
    }
}

static void ph_fonts_init(void) {
    if (s_fonts_ready) return;
    s_font16 = app_font_16; s_font16.fallback = &lv_font_montserrat_14;
    s_font20 = app_font_20; s_font20.fallback = &lv_font_montserrat_20;
    s_fonts_ready = true;
}

static lv_obj_t *ph_label(lv_obj_t *parent, const lv_font_t *font, lv_color_t color,
                          const char *text, lv_align_t align, int x, int y, int w) {
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_label_set_text(l, text);
    if (w > 0) lv_obj_set_width(l, w);
    lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_align(l, align, x, y);
    return l;
}

// 右上角电量;-1 优雅降级为不显示
static void ph_battery(lv_obj_t *parent) {
    int soc = bsp_battery_soc();
    if (soc < 0) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", soc);
    lv_color_t col = (soc < 20) ? lv_color_hex(PH_ACCENT) : lv_color_hex(PH_DIM);
    ph_label(parent, &s_font16, col, buf, LV_ALIGN_TOP_RIGHT, -14, 16, 0);
}

static void ph_hint(lv_obj_t *parent, const char *text) {
    ph_label(parent, &s_font16, lv_color_hex(PH_DIM), text, LV_ALIGN_BOTTOM_MID, 0, -8, 224);
}

static void ph_render_home(void) {
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(PH_PAPER), 0);
    ph_label(s_scr, &s_font20, lv_color_hex(PH_INK), "口袋历史", LV_ALIGN_TOP_MID, 0, 12, 224);
    ph_label(s_scr, &s_font16, lv_color_hex(PH_DIM), "上下五千年 · 随身读史", LV_ALIGN_TOP_MID, 0, 44, 224);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *it = lv_label_create(s_scr);
        lv_obj_set_style_text_font(it, &s_font16, 0);
        lv_label_set_text(it, MODULE_NAME[i]);
        lv_obj_set_width(it, 200);
        lv_obj_set_height(it, 34);
        lv_label_set_long_mode(it, LV_LABEL_LONG_MODE_CLIP);
        lv_obj_set_style_text_align(it, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_bg_opa(it, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(it, lv_color_hex(i == s_sel ? PH_CARD_HI : PH_CARD), 0);
        lv_obj_set_style_radius(it, 8, 0);
        lv_obj_set_style_border_width(it, 2, 0);
        lv_obj_set_style_border_color(it, lv_color_hex(i == s_sel ? PH_ACCENT : PH_DIM), 0);
        lv_obj_set_style_text_color(it, lv_color_hex(i == s_sel ? PH_ACCENT : PH_INK), 0);
        lv_obj_set_style_pad_top(it, 6, 0);   // 近似垂直居中(卡高34,16px字行高约20)
        lv_obj_align(it, LV_ALIGN_TOP_MID, 0, 76 + i * 44);
    }
    ph_battery(s_scr);
    ph_hint(s_scr, "上/下 选择      确定 进入");
}

static void ph_render_entry(const char *module, int idx, int count,
                            const char *name, const char *detail) {
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(PH_PAPER), 0);

    char head[40];
    snprintf(head, sizeof(head), "%s  %d/%d", module, idx + 1, count);
    ph_label(s_scr, &s_font16, lv_color_hex(PH_DIM), head, LV_ALIGN_TOP_LEFT, 14, 16, 180);

    ph_label(s_scr, &s_font20, lv_color_hex(PH_ACCENT), name, LV_ALIGN_TOP_MID, 0, 46, 224);
    ph_label(s_scr, &s_font16, lv_color_hex(PH_INK), detail, LV_ALIGN_TOP_LEFT, 16, 92, 210);

    ph_battery(s_scr);
    ph_hint(s_scr, "上/下 翻页      长按 返回");
}

static int ph_count(ph_mode_t m) {
    switch (m) {
        case MODE_DYNASTY: return PH_DYNASTY_COUNT;
        case MODE_FIGURE:  return PH_FIGURE_COUNT;
        case MODE_TALE:    return PH_TALE_COUNT;
        default:           return 0;
    }
}

static const char *ph_entry_name(ph_mode_t m, int i) {
    if (m == MODE_DYNASTY) return PH_DYNASTIES[i].name;
    if (m == MODE_FIGURE)  return PH_FIGURES[i].name;
    if (m == MODE_TALE)    return PH_TALES[i].title;
    return "";
}

static void ph_build_detail(char *buf, size_t cap, ph_mode_t m, int i) {
    buf[0] = '\0';
    if (m == MODE_DYNASTY) {
        const ph_dynasty_t *d = &PH_DYNASTIES[i];
        snprintf(buf, cap, "起止  %s\n都城  %s\n\n%s", d->span, d->capital, d->note);
    } else if (m == MODE_FIGURE) {
        const ph_figure_t *f = &PH_FIGURES[i];
        snprintf(buf, cap, "时代  %s\n\n%s", f->era, f->bio);
    } else if (m == MODE_TALE) {
        snprintf(buf, cap, "%s", PH_TALES[i].text);
    }
    size_t l = strlen(buf);
    if (l && buf[l - 1] == '\n') buf[l - 1] = '\0';   // 去尾换行,避免 LVGL 多算一行
}

static const char *ph_modname(ph_mode_t m) {
    return (m == MODE_DYNASTY) ? "朝代长河" : (m == MODE_FIGURE) ? "先贤列传" : "典故菁华";
}

// 单一渲染入口:清屏重建(同屏复用,避免建新删旧的堆峰)
static void ph_render(void) {
    if (!s_scr) return;
    lv_obj_clean(s_scr);
    if (s_mode == MODE_HOME) { ph_render_home(); return; }

    ph_mode_t m = (s_mode == MODE_RANDOM) ? s_rnd_kind : s_mode;
    static char detail[384];
    ph_build_detail(detail, sizeof(detail), m, s_idx);
    ph_render_entry(ph_modname(m), s_idx, ph_count(m), ph_entry_name(m, s_idx), detail);
}

void demo_pocket_history_enter(void) {
    ph_fonts_init();
    if (!s_scr) {
        s_scr = lv_obj_create(NULL);
        lv_obj_remove_style_all(s_scr);
        lv_obj_set_size(s_scr, 240, 320);
        lv_obj_clear_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    }
    s_rng = esp_random();
    if (!s_rng) s_rng = 0x1234567u;
    if (s_mode == MODE_RANDOM) ph_random_pick();
    ph_render();
    lv_screen_load(s_scr);
}

void demo_pocket_history_exit(void) {
    if (s_scr) { lv_obj_delete(s_scr); s_scr = NULL; }
    s_mode = MODE_HOME; s_sel = 0; s_idx = 0;
}

void demo_pocket_history_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    bool changed = false;

    if (ev == BSP_BTN_LONG && btn == BSP_BTN_OK) {
        if (s_mode != MODE_HOME) { s_mode = MODE_HOME; changed = true; }
    } else if (ev == BSP_BTN_CLICK) {
        if (btn == BSP_BTN_UP || btn == BSP_BTN_DOWN) {
            int dir = (btn == BSP_BTN_UP) ? -1 : 1;
            if (s_mode == MODE_HOME) {
                s_sel = (s_sel + dir + 4) % 4;
                changed = true;
            } else if (s_mode == MODE_RANDOM) {
                ph_random_pick();
                changed = true;
            } else {
                int n = ph_count(s_mode);
                s_idx = (s_idx + dir + n) % n;
                changed = true;
            }
        } else if (btn == BSP_BTN_OK) {
            if (s_mode == MODE_HOME) {
                switch (s_sel) {
                    case 0: s_mode = MODE_DYNASTY; s_idx = 0; break;
                    case 1: s_mode = MODE_FIGURE;  s_idx = 0; break;
                    case 2: s_mode = MODE_TALE;    s_idx = 0; break;
                    default: s_mode = MODE_RANDOM; ph_random_pick(); break;
                }
                changed = true;
            } else if (s_mode == MODE_RANDOM) {
                ph_random_pick();
                changed = true;
            }
        }
    }

    if (changed && bsp_lvgl_lock(300)) {
        ph_render();
        bsp_lvgl_unlock();
    }
}
