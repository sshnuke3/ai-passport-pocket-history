# 口袋历史·上下五千年

FoloToy AI Passport 玩法（play）。在 ESP32-C3 设备上以「纸墨朱砂」国风 UI 呈现中华上下五千年脉络。

## 设备与目标

- 硬件：FoloToy AI Passport（ESP32-C3，8MB Flash，无 PSRAM，ST7789 240×320 RGB565）
- 图形：LVGL v9.5.0
- 构建：ESP-IDF 5.5.3

## 功能

- 五种模式：主页 / 朝代 / 人物 / 典故 / 随机
- 内置内容（公有领域）：18 个朝代、36 位人物、24 则典故
- 中文子集字体：取自 NotoSansSC，经 `lv_font_conv` 子集化（`--bpp 4 --no-compress`）
- 上电直启：编译期定义 `BOOT_DIRECT_PLAY "口袋历史"`

## 目录

- `main/demo_pocket_history.c`：玩法实现（enter/exit/key，自绘 UI）
- `main/pocket_history_data.h`：朝代 / 人物 / 典故数据
- `assets/fonts/`：子集化中文字体（`app_font_16.c`、`app_font_20.c`）
- `tools/gen_ph_font.py`：字体生成脚本（需 `lv_font_conv` 与本机 NotoSansSC 字体）

## 构建与烧录

按 FoloToy AI Passport 标准流程构建，产物为 `build/FoloToy-AI-Passport-full.bin`，
通过 web-flasher 在 offset `0x0` 烧录。

## 许可

MIT License，见 [LICENSE](LICENSE)。
