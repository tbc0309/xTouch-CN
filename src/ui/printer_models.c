#include "printer_models.h"
#include <string.h>
#include <ctype.h>
extern const lv_img_dsc_t printer_image_BL_P001;
extern const lv_img_dsc_t printer_image_BL_P002;
extern const lv_img_dsc_t printer_image_C11;
extern const lv_img_dsc_t printer_image_C12;
extern const lv_img_dsc_t printer_image_C13;
extern const lv_img_dsc_t printer_image_N1;
extern const lv_img_dsc_t printer_image_N2S;
extern const lv_img_dsc_t printer_image_N6;
extern const lv_img_dsc_t printer_image_N7;
extern const lv_img_dsc_t printer_image_N9;
extern const lv_img_dsc_t printer_image_O1C;
extern const lv_img_dsc_t printer_image_O1C2;
extern const lv_img_dsc_t printer_image_O1D;
extern const lv_img_dsc_t printer_image_O1E;
extern const lv_img_dsc_t printer_image_O1S;
typedef struct { const char *code; const char *name; const lv_img_dsc_t *image; bool heater; } PrinterModel;
static const PrinterModel models[] = {
{"BL-P001", "Bambu Lab X1 Carbon", &printer_image_BL_P001, false},
{"BL-P002", "Bambu Lab X1", &printer_image_BL_P002, false},
{"C11", "Bambu Lab P1P", &printer_image_C11, false},
{"C12", "Bambu Lab P1S", &printer_image_C12, false},
{"C13", "Bambu Lab X1E", &printer_image_C13, true},
{"N1", "Bambu Lab A1 mini", &printer_image_N1, false},
{"N2S", "Bambu Lab A1", &printer_image_N2S, false},
{"N6", "Bambu Lab X2D", &printer_image_N6, true},
{"N7", "Bambu Lab P2S", &printer_image_N7, false},
{"N9", "Bambu Lab A2L", &printer_image_N9, false},
{"O1C", "Bambu Lab H2C", &printer_image_O1C, true},
{"O1C2", "Bambu Lab H2C", &printer_image_O1C2, true},
{"O1D", "Bambu Lab H2D", &printer_image_O1D, true},
{"O1E", "Bambu Lab H2D Pro", &printer_image_O1E, true},
{"O1S", "Bambu Lab H2S", &printer_image_O1S, true},
};
static bool model_name_equal(const char *a,const char *b){
 while(*a||*b){while(*a==' ')a++;while(*b==' ')b++;if(tolower((unsigned char)*a)!=tolower((unsigned char)*b))return false;if(!*a)return true;a++;b++;}return true;
}
const char *ui_printer_model_code(const char *value) {
 if (!value) return "";
 if(!strcmp(value,"O2D"))return "O1E";
 if(model_name_equal(value,"X1C"))return "BL-P001";
 if (!strcmp(value,"3DPrinter-X1")) return "BL-P002";
 if (!strcmp(value,"3DPrinter-X1-Carbon")) return "BL-P001";
 for (unsigned i=0;i<sizeof(models)/sizeof(models[0]);++i) if (!strcmp(value,models[i].code) || !strcmp(value,models[i].name) || model_name_equal(value,models[i].name+10)) return models[i].code;
 return "";
}
const char *ui_printer_model_short_name(const char *value) {
 const char *code=ui_printer_model_code(value);
 if (!strcmp(code,"BL-P001")) return "X1C";
 if (!strcmp(code,"N1")) return "A1mini";
 if (!strcmp(code,"O1E")) return "H2DPro";
 for (unsigned i=0;i<sizeof(models)/sizeof(models[0]);++i)
  if (!strcmp(code,models[i].code)) return models[i].name+10; // Skip the common vendor prefix.
 return "?";
}
const lv_img_dsc_t *ui_printer_model_image(const char *value) {
 ui_printer_image_decoder_init();
 const char *code=ui_printer_model_code(value);
 for (unsigned i=0;i<sizeof(models)/sizeof(models[0]);++i) if (!strcmp(code,models[i].code)) return models[i].image;
 return NULL;
}
bool ui_printer_model_chamber_heater(const char *value) {
 const char *code=ui_printer_model_code(value);
 for (unsigned i=0;i<sizeof(models)/sizeof(models[0]);++i) if (!strcmp(code,models[i].code)) return models[i].heater;
 return false;
}
