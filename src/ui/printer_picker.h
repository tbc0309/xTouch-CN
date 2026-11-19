#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void ui_picker_draw_action(lv_draw_ctx_t *ctx,lv_area_t area,unsigned kind,bool pictures);
void ui_printer_picker_show(void);
void ui_printer_picker_close(void);
#ifdef __cplusplus
}
#endif
