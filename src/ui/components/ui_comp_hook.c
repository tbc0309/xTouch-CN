#include "../ui.h"
#include "../layouts/shape.h"

void ui_comp_sidebarComponent_create_hook(lv_obj_t *comp)
{
    ui_text_guard_tree(comp);
    ui_layout_captions_tree(comp);
}
void ui_comp_homeComponent_create_hook(lv_obj_t *comp)
{
    ui_text_guard_tree(comp);
    ui_layout_captions_tree(comp);
}
void ui_comp_temperatureComponent_create_hook(lv_obj_t *comp)
{
    ui_text_guard_tree(comp);
    ui_layout_captions_tree(comp);
}
void ui_comp_controlComponent_create_hook(lv_obj_t *comp)
{
    ui_text_guard_tree(comp);
    ui_layout_captions_tree(comp);
}
void ui_comp_settingsComponent_create_hook(lv_obj_t *comp)
{
    ui_text_guard_tree(comp);
    ui_layout_captions_tree(comp);
}
void ui_comp_confirmPanel_create_hook(lv_obj_t *comp)
{
    ui_shape_dialog(comp,false);
    ui_text_guard_tree(comp);
    ui_layout_captions_tree(comp);
}
void ui_comp_hmsPanel_create_hook(lv_obj_t *comp)
{
    ui_shape_dialog(comp,true);
    ui_text_guard_tree(comp);
    ui_layout_captions_tree(comp);
}
void ui_comp_mainScreenStatus_create_hook(lv_obj_t *comp)
{
    ui_text_guard_tree(comp);
    ui_layout_captions_tree(comp);
}
void ui_comp_filamentComponent_create_hook(lv_obj_t *comp)
{
    ui_text_guard_tree(comp);
    ui_layout_captions_tree(comp);
}
