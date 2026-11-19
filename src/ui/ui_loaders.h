
#ifndef _V3CONTROLCOMP_UI_LOADERS_H
#define _V3CONTROLCOMP_UI_LOADERS_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "ui.h"
#include "ui_msgs.h"

    void fillScreenData(int screen);
    void loadScreen(int screen);
    void showPrinterNotice(void);
    bool isPrinterNoticeActive(void);
    void initTopLayer();
    void ui_refresh_top_layer_language(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
