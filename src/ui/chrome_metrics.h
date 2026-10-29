#pragma once

/* Native-pixel chrome, shared by overlays rather than selected by board name.
 * Preserve a 320x240 reference canvas without stretching on wide/portrait ports. */
typedef struct {
    int reference_height;
    int edge;
    int control;
    int bar;
    int icon;
} ui_chrome_metrics_t;

static inline int ui_chrome_px(int reference_height, int design_pixels) {
    return (reference_height * design_pixels + 120) / 240;
}

static inline ui_chrome_metrics_t ui_chrome_metrics(int width, int height) {
    int reference = width * 3 / 4;
    if(height < reference) reference = height;
    if(reference < 120) reference = 120;
    ui_chrome_metrics_t m;
    m.reference_height = reference;
    m.edge = ui_chrome_px(reference, 8);
    /* Visual controls share one compact square; retain a separate outer
     * margin so intermediate canvases do not become dominated by chrome. */
    m.control = ui_chrome_px(reference, 28);
    m.bar = m.control + 2 * m.edge;
    m.icon = ui_chrome_px(reference, 14);
    return m;
}
