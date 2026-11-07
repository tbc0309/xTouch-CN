#include "browser_internal.h"
#include "browser.h"
#include "features/filename_font.h"
#include "slice_stats.h"
#include "ui/ui.h"
#include "ui/chrome_metrics.h"
#include "ui/printer_picker.h"
#include "ui/settings_gesture.h"
#include "ui/message_style.h"
#include "xtouch/printers.h"
#include <memory>
// This overlay uses native-pixel chrome and never legacy coordinate transforms.
#undef lv_obj_create
#undef lv_obj_set_size
#undef lv_obj_set_pos
#undef lv_obj_set_height
#undef lv_obj_set_style_radius
// This module measures native pixels; bypass all legacy canvas wrappers.
#undef lv_obj_set_pos
#undef lv_obj_set_size
#undef lv_obj_align
#undef lv_obj_get_coords
#undef lv_obj_get_content_coords
#undef lv_indev_get_point
#undef lv_text_get_size
#undef lv_draw_rect
#undef lv_draw_label
#undef lv_draw_img
#undef lv_qrcode_create
#undef lv_obj_set_width
#undef lv_obj_set_height
#undef lv_obj_set_x
#undef lv_obj_set_y
#undef lv_obj_set_style_width
#undef lv_obj_set_style_height
#undef lv_obj_set_style_max_height
#undef lv_obj_set_style_pad_all
#undef lv_obj_set_style_pad_ver
#undef lv_obj_set_style_pad_left
#undef lv_obj_set_style_pad_right
#undef lv_obj_set_style_pad_top
#undef lv_obj_set_style_pad_bottom
#undef lv_obj_set_style_pad_row
#undef lv_obj_set_style_pad_column
#undef lv_obj_set_style_radius
#undef lv_obj_set_style_border_width
#undef lv_obj_set_style_text_line_space
#undef lv_obj_create
#undef lv_obj_set_style_text_font
#undef lv_obj_get_width
#undef lv_obj_get_height
#undef lv_obj_get_x_aligned
#undef lv_obj_get_y_aligned
#undef lv_obj_scroll_to_x
#undef lv_obj_scroll_to_y
LV_FONT_DECLARE(ui_font_text_24);
LV_FONT_DECLARE(xtouch_files_font_14);
LV_FONT_DECLARE(xtouch_files_font_18);
LV_FONT_DECLARE(xtouch_files_font_28);
namespace xtouch_files {
namespace {
const char *words[Count][8] = {
    {"打印文件", "列印檔案", "Print files", "Fichiers", "Druckdateien", "Файлы печати",
     "印刷ファイル", "인쇄 파일"},
    {"正在读取…", "正在讀取…", "Loading…", "Chargement…", "Laden…", "Загрузка…", "読み込み中…",
     "불러오는 중…"},
    {"刷新", "重新整理", "Refresh", "Actualiser", "Aktualisieren", "Обновить", "更新", "새로 고침"},
    {"关闭", "關閉", "Close", "Fermer", "Schließen", "Закрыть", "閉じる", "닫기"},
    {"列表", "列表", "List", "Liste", "Liste", "Список", "一覧", "목록"},
    {"图片", "圖片", "Pictures", "Images", "Bilder", "Изображения", "画像", "이미지"},
    {"上一页", "上一頁", "Previous", "Précédent", "Zurück", "Назад", "前へ", "이전"},
    {"下一页", "下一頁", "Next", "Suivant", "Weiter", "Далее", "次へ", "다음"},
    {"暂无文件", "暫無檔案", "No files", "Aucun fichier", "Keine Dateien", "Нет файлов",
     "ファイルなし", "파일 없음"},
    {"读取失败", "讀取失敗", "Read failed", "Échec de lecture", "Lesefehler", "Ошибка чтения",
     "読み込み失敗", "읽기 실패"},
    {"显示上次缓存；请刷新后打印", "顯示上次快取；請重新整理後列印",
     "Cached list; refresh before printing", "Liste en cache ; actualisez",
     "Cache; vor Druck aktualisieren", "Кэш; обновите перед печатью",
     "キャッシュ表示：印刷前に更新", "캐시 목록: 인쇄 전 새로 고침"},
    {"内存不足", "記憶體不足", "Not enough memory", "Mémoire insuffisante", "Nicht genug Speicher",
     "Недостаточно памяти", "メモリ不足", "메모리 부족"},
    {"需要当前设备的局域网连接", "需要目前裝置的區域網路連線",
     "Current printer LAN connection required", "Connexion LAN requise",
     "LAN-Verbindung erforderlich", "Требуется подключение LAN", "プリンターのLAN接続が必要",
     "프린터 LAN 연결 필요"},
    {"重新打印", "重新列印", "Print again", "Réimprimer", "Erneut drucken", "Повторная печать",
     "再印刷", "다시 인쇄"},
    {"盘号", "列印板", "Plate", "Plateau", "Platte", "Пластина", "プレート", "플레이트"},
    {"使用AMS当前配置", "使用AMS目前設定", "Use current AMS mapping", "Utiliser AMS actuel",
     "Aktuelle AMS-Zuordnung", "Текущая схема AMS", "現在のAMS設定", "현재 AMS 설정"},
    {"确认打印", "確認列印", "Confirm print", "Confirmer", "Druck bestätigen", "Начать печать",
     "印刷を確認", "인쇄 확인"},
    {"取消", "取消", "Cancel", "Annuler", "Abbrechen", "Отмена", "キャンセル", "취소"},
    {"打印机忙，暂不可重新打印", "印表機忙碌，暫不可重新列印", "Printer busy; cannot start",
     "Imprimante occupée", "Drucker beschäftigt", "Принтер занят", "プリンター使用中",
     "프린터 사용 중"},
    {"请求已发送，等待打印机状态", "請求已傳送，等待印表機狀態", "Request sent; awaiting printer",
     "Envoyé ; attente imprimante", "Gesendet; warte auf Drucker", "Отправлено; ожидание принтера",
     "送信済み：応答待ち", "전송됨: 프린터 응답 대기"},
    {"请求未发送", "請求未傳送", "Request not sent", "Demande non envoyée", "Nicht gesendet",
     "Не отправлено", "未送信", "전송되지 않음"},
    {"使用已装入耗材", "使用已裝入耗材", "Loaded filament", "Filament chargé", "Geladenes Filament", "Заправленный материал", "装填済みフィラメント", "장착된 필라멘트"},
    {"可能需要局域网及开发者模式", "可能需要區域網路及開發者模式", "May require LAN + Developer mode", "Mode LAN + développeur possible", "Ggf. LAN- und Entwicklermodus", "Может требоваться LAN + режим разработчика", "LAN・開発者モードが必要な場合あり", "LAN 및 개발자 모드가 필요할 수 있음"},
    {"打印机已接受请求", "印表機已接受請求", "Printer accepted request", "Demande acceptée",
     "Anfrage angenommen", "Запрос принят", "要求を受理しました", "프린터가 요청을 수락함"},
    {"打印机拒绝请求", "印表機拒絕請求", "Printer rejected request", "Demande refusée",
     "Anfrage abgelehnt", "Запрос отклонён", "要求が拒否されました", "프린터가 요청을 거부함"},
    {"未收到打印机确认，请核对本机状态", "未收到印表機確認，請核對本機狀態",
     "No confirmation; check the printer", "Sans confirmation ; vérifiez l’imprimante",
     "Keine Bestätigung; Drucker prüfen", "Нет подтверждения; проверьте принтер",
     "応答なし：プリンターを確認", "응답 없음: 프린터 상태 확인"},
    {"原盘号未知", "原列印板未知", "Original plate unknown", "Plateau d’origine inconnu", "Ursprüngliche Platte unbekannt", "Исходная пластина неизвестна", "元のプレート不明", "원본 플레이트 알 수 없음"},
    {"时间", "時間", "Time", "Durée", "Dauer", "Время", "時間", "시간"},
    {"克重", "克重", "Weight", "Poids", "Gewicht", "Вес", "重量", "무게"}};
lv_obj_t *overlay = nullptr, *body = nullptr, *status = nullptr, *footer = nullptr,
         *detail = nullptr, *plateInput = nullptr;
std::unique_ptr<Page> displayed;
Entry selected, pendingPrint;
unsigned pendingPlate = 1, pendingEpoch = 0, selectedPlate = 0;
bool printRequested = false;
bool selectedFresh=false,pendingFresh=false;
bool pictures = false, closing = false;
lv_obj_t *artSlots[pageSize]{};
unsigned pageNumber = 0;
lv_obj_t *previousButton=nullptr,*nextPageButton=nullptr;
ui_chrome_metrics_t chrome{};
ui_scroll_guard_t *fileGuard=nullptr;
lv_obj_t *detailArt=nullptr,*confirmButton=nullptr;
lv_obj_t *detailValues[3]{};
unsigned selectedIndex=0;
std::unique_ptr<Page> selectedPreview;
const lv_font_t *font();
void clearSelectedPreview(){
    if(selectedPreview)lv_img_cache_invalidate_src(&selectedPreview->items[0].image);
    if(detailArt)lv_obj_clean(detailArt);
    selectedPreview.reset();
}
void coverPlaceholder(lv_obj_t *parent,const Item &item);
void paintDetail() {
    if(!detailArt || !displayed || selectedIndex>=displayed->count)return;
    lv_obj_clean(detailArt);auto &item=(selectedPreview && selectedPreview->plate==selectedPlate)?selectedPreview->items[0]:displayed->items[selectedIndex];
    bool matching=selectedPlate<=1 || item.entry.plate==selectedPlate || (selectedPreview && selectedPreview->plate==selectedPlate);
    if(matching && item.pixels){auto im=lv_image_create(detailArt);lv_image_set_src(im,&item.image);lv_obj_update_layout(detailArt);unsigned z=std::min(384u,std::min(unsigned(lv_obj_get_content_width(detailArt))*256/item.image.header.w,unsigned(lv_obj_get_content_height(detailArt))*256/item.image.header.h));lv_image_set_scale(im,z);lv_obj_center(im);}
    else coverPlaceholder(detailArt,item);
}
uint32_t layoutChanged=0;
bool settled(){return lv_tick_elaps(layoutChanged)>=300 && !ui_scroll_busy(300);}
const lv_font_t *font() {
#if XTOUCH_DISPLAY_HEIGHT >= 400 && XTOUCH_DISPLAY_WIDTH >= 640
    return xtouch_filename_font(&xtouch_files_font_28);
#else
    return xtouch_filename_font(&xtouch_files_font_14);
#endif
}
lv_obj_t *label(lv_obj_t *parent, const char *caption) {
    auto l = lv_label_create(parent);
    lv_label_set_text(l, caption);
    lv_label_set_long_mode(l, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(l, lv_pct(100));
    lv_obj_set_style_text_font(l, font(), 0);
    return l;
}
// Details share fixed label/value columns; only an overflowing value scrolls.
lv_obj_t *parameter(lv_obj_t *parent,const char *caption,const std::string &value){
    auto row=lv_obj_create(parent);lv_obj_remove_style_all(row);
    lv_obj_set_size(row,lv_pct(100),std::max<int>(font()->line_height+2,ui_chrome_px(chrome.reference_height,19)));
    lv_obj_set_flex_flow(row,LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row,chrome.edge/2,0);
    lv_obj_set_flex_align(row,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row,LV_OBJ_FLAG_SCROLLABLE);
    int keyWidth=0;
    for(unsigned id:{unsigned(Plate),unsigned(Time),unsigned(Weight)}){
        auto ink=ui_layout_ink(font(),text(id));keyWidth=std::max<int>(keyWidth,lv_area_get_width(&ink)+4);
    }
    auto key=label(row,caption);lv_obj_set_width(key,keyWidth);
    lv_label_set_long_mode(key,LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(key,lv_color_hex(UI_COLOR_TEXT_SECONDARY),0);
    auto field=label(row,value.c_str());lv_obj_set_width(field,0);lv_obj_set_flex_grow(field,1);
    lv_obj_set_style_text_align(field,LV_TEXT_ALIGN_RIGHT,0);
    return field;
}
void coverPlaceholder(lv_obj_t *parent,const Item &item) {
    auto l=label(parent,busy && !item.coverRead?text(Loading):(archive(item.entry.path)?"3MF":"G-code"));
    lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);lv_obj_center(l);
}
lv_obj_t *button(lv_obj_t *parent, const char *caption, lv_event_cb_t callback) {
    auto b = lv_button_create(parent);
    lv_obj_set_style_bg_color(b, lv_color_hex(UI_COLOR_BUTTON), 0);
    lv_obj_set_style_pad_all(b, chrome.edge / 2, 0);
    lv_obj_set_height(b, chrome.control);
    auto l = label(b, caption);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, callback, LV_EVENT_CLICKED, nullptr);
    return b;
}
void closeDetail(lv_event_t *event) {
    bool resume=event && detail;
    if (detail)
        lv_obj_delete(detail);
    detail = plateInput = detailArt = confirmButton = nullptr;
    for(auto &value:detailValues)value=nullptr;
    clearSelectedPreview();
    if(resume && open)request(pageNumber,false);
}
bool idle() {
    return !bambuStatus.non_fdm_mode && bambuStatus.print_status != XTOUCH_PRINT_STATUS_RUNNING &&
           bambuStatus.print_status != XTOUCH_PRINT_STATUS_PAUSED &&
           bambuStatus.print_status != XTOUCH_PRINT_STATUS_PREPARE;
}
void confirm(lv_event_t *) {
    if(ui_scroll_busy(300))return;
    if (!displayed || !selectedFresh || !xtouch_printer_online() || !idle() ||
        printBusy() || !open || source.epoch != epoch ||
        strcmp(source.serial, xtouch_printer_serial())) {
        lv_label_set_text(status, text(Busy));
        return;
    }
    unsigned plate = selectedPlate;
    if (plate < 1 || plate > 999)
        return;
    pendingPrint = selected;
    pendingFresh = selectedFresh;
    pendingPlate = plate;
    pendingEpoch = epoch;
    printRequested = true;
    preparePrint();
    closeDetail(nullptr);
    lv_label_set_text(status, text(Loading));
}
void choose(lv_event_t *event) {
    if (!settled() || !displayed)
        return;
    unsigned i = (uintptr_t)lv_event_get_user_data(event);
    if (i >= displayed->count)
        return;
    selectedIndex=i;
    selected = displayed->items[i].entry;
    selectedFresh = displayed->fresh;
    selectedPlate = selected.plate?selected.plate:(archive(selected.path)?0:1);
    closeDetail(nullptr);
    detail = lv_obj_create(overlay);
    lv_obj_set_width(detail, std::min<int>(lv_display_get_horizontal_resolution(nullptr)-2*chrome.edge,ui_chrome_px(chrome.reference_height,270)));
    lv_obj_set_height(detail, LV_SIZE_CONTENT);
    lv_obj_set_style_max_height(detail, lv_display_get_vertical_resolution(nullptr) * 9 / 10, 0);
    lv_obj_set_scroll_dir(detail, LV_DIR_VER);
    lv_obj_center(detail);
    lv_obj_set_style_bg_color(detail, lv_color_hex(UI_COLOR_BUTTON), 0);
    lv_obj_set_style_pad_all(detail, chrome.edge, 0);
    lv_obj_set_style_pad_row(detail,chrome.edge/2,0);
    lv_obj_set_style_border_width(detail,0,0);
    lv_obj_set_style_radius(detail,chrome.edge,0);
    lv_obj_set_flex_flow(detail, LV_FLEX_FLOW_COLUMN);
    auto slash=selected.path.find_last_of('/');label(detail,selected.path.substr(slash+1).c_str());
    int square=ui_chrome_px(chrome.reference_height,96);
    auto content=lv_obj_create(detail);lv_obj_remove_style_all(content);
    lv_obj_set_size(content,lv_pct(100),square);lv_obj_set_flex_flow(content,LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(content,chrome.edge,0);lv_obj_clear_flag(content,LV_OBJ_FLAG_SCROLLABLE);
    auto options=lv_obj_create(content);lv_obj_remove_style_all(options);
    lv_obj_set_size(options,0,square);lv_obj_set_flex_grow(options,1);lv_obj_set_flex_flow(options,LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(options,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(options,chrome.edge/2,0);lv_obj_clear_flag(options,LV_OBJ_FLAG_SCROLLABLE);
    detailValues[0]=parameter(options,text(Plate),selectedPlate?std::to_string(selectedPlate):"--");
    detailValues[1]=parameter(options,text(Time),selected.hasTime?statsCaption(selected.seconds,true,0,false):"--");
    detailValues[2]=parameter(options,text(Weight),selected.hasWeight?statsCaption(0,false,selected.grams,true):"--");
    auto material=label(options,text(External));lv_obj_set_style_text_color(material,lv_color_hex(UI_COLOR_TEXT_SECONDARY),0);
    detailArt=lv_obj_create(content);lv_obj_remove_style_all(detailArt);
    lv_obj_set_size(detailArt,square,square);lv_obj_clear_flag(detailArt,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(detailArt,lv_color_hex(0x444444),0);lv_obj_set_style_bg_opa(detailArt,LV_OPA_COVER,0);
    lv_obj_set_style_radius(detailArt,chrome.edge/2,0);
    auto controls=lv_obj_create(detail);lv_obj_remove_style_all(controls);
    lv_obj_set_size(controls,lv_pct(100),chrome.control);lv_obj_set_flex_flow(controls,LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(controls,chrome.edge,0);lv_obj_clear_flag(controls,LV_OBJ_FLAG_SCROLLABLE);
    auto no=button(controls,text(Cancel),closeDetail);lv_obj_set_width(no,0);lv_obj_set_flex_grow(no,1);ui_action_style(no,UI_ACTION_SECONDARY);
    auto yes=button(controls,text(Confirm),confirm);confirmButton=yes;lv_obj_set_width(yes,0);lv_obj_set_flex_grow(yes,1);ui_action_style(yes,UI_ACTION_PRIMARY);
    refreshPrintAction();
    lv_obj_update_layout(detail);lv_obj_center(detail);paintDetail();ui_settings_gesture_bind(detail);
    if(!selectedFresh || busy || !displayed->items[i].pixels)requestDetail(selected,pageNumber,i,selectedPlate?selectedPlate:1);

}
void refresh(lv_event_t *) {
    if (!settled()) return;
    if (busy)
        return;
    lv_label_set_text(status, text(Loading));
    request(pageNumber, true);
}
void toggle(lv_event_t *event) {
    if (!settled()) return;
    pictures = !pictures;
    lv_obj_invalidate(lv_event_get_target_obj(event));
    if(displayed){auto page=displayed.release();redraw(page);}
}
void refreshPageActions() {
    const bool canPrevious=displayed && pageNumber>0;
    const bool canNext=displayed && displayed->total>0 && pageNumber<(displayed->total-1)/pageSize;
    for(unsigned i=0;i<2;++i){
        auto b=i?nextPageButton:previousButton;
        if(!b)continue;
        if(i?canNext:canPrevious)lv_obj_clear_state(b,LV_STATE_DISABLED);
        else lv_obj_add_state(b,LV_STATE_DISABLED);
    }
}
void previous(lv_event_t *) {
    if (!settled()) return;
    if (pageNumber) {
        --pageNumber;
        showCachedPage(pageNumber);
        request(pageNumber, false);
    }
}
void next(lv_event_t *) {
    if (!settled()) return;
    if (displayed && (pageNumber + 1) * pageSize < displayed->total) {
        ++pageNumber;
        showCachedPage(pageNumber);
        request(pageNumber, false);
    }
}
void pickerAction(lv_event_t *event) {
    auto ctx=lv_event_get_draw_ctx(event);lv_area_t a;lv_obj_get_coords(lv_event_get_target_obj(event),&a);
    ui_picker_draw_action(ctx,a,(uintptr_t)lv_event_get_user_data(event),pictures);
}
void exit(lv_event_t *) {
    close();
}
void releasePage() {
    if (displayed)
        for (auto &item : displayed->items)
            lv_img_cache_invalidate_src(&item.image);
    displayed.reset();
}
void deleted(lv_event_t *) {
    closeDetail(nullptr);
    if (body)
        lv_obj_clean(body);
    releasePage();
    overlay = body = status = footer = detail = plateInput = nullptr;
    previousButton=nextPageButton=nullptr;
    fileGuard=nullptr;
    open = false;
    ++epoch;
}
} // namespace
const char *text(unsigned key) {
    unsigned language = xtouch_language_get();
    return words[key < Count ? key : Failed][language < 8 ? language : 2];
}
void fitTool(lv_obj_t *b) {
    auto l=lv_obj_get_child(b,0);lv_obj_set_width(l,LV_SIZE_CONTENT);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_LEFT,0);auto f=&ui_font_text_24;
    auto ink=ui_layout_ink(f,lv_label_get_text(l));int span=std::max(lv_area_get_width(&ink),lv_area_get_height(&ink));
    if(span>0)ui_text_scale(l,f,(chrome.icon*256+span/2)/span);
    lv_obj_update_layout(b);ui_layout_ink_place(l,0,0,lv_obj_get_content_width(b),lv_obj_get_content_height(b));
}
void show() {
    if(overlay)return;
    closing=false;pageNumber=0;
    int w=lv_display_get_horizontal_resolution(nullptr),h=lv_display_get_vertical_resolution(nullptr);
    chrome=ui_chrome_metrics(w,h);
    overlay=lv_obj_create(lv_layer_top());lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay,w,h);lv_obj_set_pos(overlay,0,0);
    lv_obj_set_style_bg_color(overlay,lv_color_hex(0x444444),0);
    lv_obj_set_style_bg_opa(overlay,LV_OPA_COVER,0);lv_obj_set_style_text_color(overlay,lv_color_white(),0);
    lv_obj_clear_flag(overlay,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(overlay,deleted,LV_EVENT_DELETE,nullptr);
    int buttons=3*chrome.control+2*chrome.edge;
    auto title=label(overlay,text(Title));
    lv_obj_set_pos(title,chrome.edge,(chrome.bar-font()->line_height)/2);
    lv_obj_set_width(title,w-3*chrome.edge-buttons);
    lv_obj_t *actions[]={button(overlay,LV_SYMBOL_REFRESH,refresh),button(overlay,"",toggle),button(overlay,"",exit)};
    for(unsigned i=0;i<3;++i) {
        auto b=actions[i];lv_obj_remove_style_all(b);
        lv_obj_set_size(b,chrome.control,chrome.control);
        lv_obj_set_pos(b,w-chrome.edge-(3-i)*chrome.control-(2-i)*chrome.edge,chrome.edge);
        lv_obj_set_style_radius(b,chrome.control/6,0);lv_obj_set_style_bg_color(b,lv_color_hex(0x555555),0);
        lv_obj_set_style_bg_opa(b,LV_OPA_COVER,0);lv_obj_clear_flag(b,LV_OBJ_FLAG_SCROLLABLE);
        if(i) {
            lv_obj_clean(b);lv_obj_set_style_bg_opa(b,LV_OPA_TRANSP,0);
            lv_obj_add_event_cb(b,pickerAction,LV_EVENT_DRAW_MAIN,(void*)(uintptr_t)(i==2?1:0));
        } else {
            fitTool(b);
        }
    }
    int foot=chrome.control+2*chrome.edge;
    body=lv_obj_create(overlay);lv_obj_remove_style_all(body);
    lv_obj_set_pos(body,chrome.edge,chrome.bar);
    lv_obj_set_size(body,w-2*chrome.edge,h-chrome.bar-foot);
    lv_obj_set_scroll_dir(body,LV_DIR_VER);lv_obj_set_scrollbar_mode(body,LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_flex_flow(body,LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(body,chrome.edge/2,0);
    fileGuard=ui_scroll_guard_create(body);
    footer=lv_obj_create(overlay);lv_obj_remove_style_all(footer);
    lv_obj_set_pos(footer,chrome.edge,h-foot+chrome.edge);
    lv_obj_set_size(footer,w-2*chrome.edge,chrome.control);
    lv_obj_clear_flag(footer,LV_OBJ_FLAG_SCROLLABLE);
    int bw=chrome.control*2;
    auto prev=button(footer,LV_SYMBOL_LEFT,previous),nextButton=button(footer,LV_SYMBOL_RIGHT,next);
    previousButton=prev;nextPageButton=nextButton;
    lv_obj_set_size(prev,bw,chrome.control);lv_obj_set_pos(prev,0,0);
    lv_obj_set_size(nextButton,bw,chrome.control);lv_obj_set_pos(nextButton,w-2*chrome.edge-bw,0);
    for(auto b:{prev,nextButton}) {
        lv_obj_set_style_pad_all(b,0,0);lv_obj_set_style_shadow_width(b,0,0);
        ui_action_style(b,UI_ACTION_PRIMARY);
        lv_obj_set_style_bg_color(b,lv_color_hex(UI_COLOR_BUTTON),LV_STATE_DISABLED);
        lv_obj_set_style_opa(b,LV_OPA_40,LV_STATE_DISABLED);
        lv_obj_set_style_bg_opa(b,LV_OPA_COVER,LV_STATE_PRESSED);
        lv_obj_set_style_translate_y(b,0,LV_STATE_PRESSED);
        fitTool(b);ui_scroll_guard_add(fileGuard,b);
    }
    refreshPageActions();
    status=label(footer,text(Loading));
    lv_obj_set_pos(status,bw+chrome.edge,(chrome.control-font()->line_height)/2);
    lv_obj_set_width(status,w-4*chrome.edge-2*bw);
    lv_obj_set_style_text_align(status,LV_TEXT_ALIGN_CENTER,0);
}
void paintArt(unsigned i) {
    if(!displayed || i>=displayed->count || !artSlots[i]) return;
    auto art=artSlots[i]; auto &item=displayed->items[i]; lv_obj_clean(art);
    if(item.pixels) {
        auto image=lv_image_create(art); lv_image_set_src(image,&item.image);
        lv_obj_add_flag(image,LV_OBJ_FLAG_EVENT_BUBBLE); lv_obj_update_layout(art);
        unsigned zoom=std::min(256u,std::min(unsigned(std::max<int>(1,lv_obj_get_content_width(art)))*256/item.image.header.w,
            unsigned(std::max<int>(1,lv_obj_get_content_height(art)))*256/item.image.header.h));
        lv_image_set_scale(image,zoom);lv_obj_center(image);
    } else coverPlaceholder(art,item);
}
bool catalogReloadReady(){return !detail && settled();}
void refreshDetail(){
    if(displayed)for(unsigned i=0;i<displayed->count;++i)paintArt(i);
    paintDetail();
    refreshPrintAction();
}
void refreshPrintAction(){
    if(!confirmButton)return;
    bool blocked=!selectedPlate || printBusy() || !displayed || !selectedFresh || !idle() || !xtouch_printer_online();
    if(blocked==lv_obj_has_state(confirmButton,LV_STATE_DISABLED))return;
    if(blocked)lv_obj_add_state(confirmButton,LV_STATE_DISABLED);
    else lv_obj_clear_state(confirmButton,LV_STATE_DISABLED);
}
void updateParameters(const Entry &entry){
    if(!detail || selected.path!=entry.path || selected.size!=entry.size || selected.stamp!=entry.stamp || selected.plate!=entry.plate)return;
    selected.seconds=entry.seconds;selected.grams=entry.grams;selected.hasTime=entry.hasTime;selected.hasWeight=entry.hasWeight;
    if(detailValues[1])lv_label_set_text(detailValues[1],selected.hasTime?statsCaption(selected.seconds,true,0,false).c_str():"--");
    if(detailValues[2])lv_label_set_text(detailValues[2],selected.hasWeight?statsCaption(0,false,selected.grams,true).c_str():"--");
}
void updateImage(Page *page) {
    std::unique_ptr<Page> incoming(page);
    if(!displayed || incoming->number!=displayed->number || incoming->imageIndex>=displayed->count) return;
    if(incoming->detailOnly) {
        if(!detailArt || incoming->imageIndex!=selectedIndex || incoming->plate!=(selectedPlate?selectedPlate:1) || incoming->items[0].entry.path!=selected.path || incoming->items[0].entry.size!=selected.size || incoming->items[0].entry.stamp!=selected.stamp || incoming->items[0].entry.plate!=selected.plate)return;
        selectedFresh=incoming->fresh;
        updateParameters(incoming->items[0].entry);
        if(incoming->plate!=1){if(incoming->items[0].pixels){clearSelectedPreview();selectedPreview=std::move(incoming);}paintDetail();return;}
        if(!incoming->items[0].pixels){displayed->items[selectedIndex].coverRead=incoming->items[0].coverRead;paintArt(selectedIndex);paintDetail();return;}
    }
    unsigned i=incoming->imageIndex; auto &dst=displayed->items[i];auto &src=incoming->items[0];
    if(dst.entry.path!=src.entry.path || dst.entry.plate!=src.entry.plate || dst.entry.size!=src.entry.size || dst.entry.stamp!=src.entry.stamp) return;
    dst.entry.seconds=src.entry.seconds;dst.entry.grams=src.entry.grams;dst.entry.hasTime=src.entry.hasTime;dst.entry.hasWeight=src.entry.hasWeight;
    updateParameters(src.entry);
    dst.coverRead=src.coverRead;
    if(!src.pixels){paintArt(i);if(detailArt && selectedIndex==i)paintDetail();return;}
    lv_img_cache_invalidate_src(&dst.image);free(dst.pixels);
    dst.pixels=src.pixels;dst.image=src.image;src.pixels=nullptr;
    paintArt(i);
    if(detailArt && selectedIndex==i)paintDetail();
}
void redraw(Page *page) {
    std::unique_ptr<Page> incoming(page);
    if (!overlay)
        return;
    layoutChanged=lv_tick_get();
    closeDetail(nullptr);
    for(auto &a:artSlots) a=nullptr;
    lv_obj_clean(body);
    bool fresh = incoming->refresh ? incoming->fresh : (displayed?displayed->fresh:incoming->fresh);
    incoming->fresh = fresh;
    releasePage();
    displayed = std::move(incoming);
    pageNumber = displayed->number;
    refreshPageActions();
    const char *message = displayed->result == 4   ? text(LocalOnly)
                          : displayed->result == 3 ? text(NoMemory)
                          : displayed->result == 2 ? text(Stale)
                          : displayed->result      ? text(Failed)
                          : displayed->count       ? ""
                                                   : text(Empty);
    char caption[180];
    snprintf(caption, sizeof(caption), "%u / %u  %s", pageNumber + 1,
             std::max(1u, (displayed->total + pageSize - 1) / pageSize), message);
    lv_label_set_text(status, caption);
    lv_obj_set_style_text_color(status,
                                lv_color_hex(displayed->result ? UI_COLOR_WARNING : 0xFFFFFF), 0);
    lv_obj_update_layout(body);
    int width = lv_obj_get_content_width(body),
        cols = pictures ? 4 : 1,
        gap = chrome.edge / 2;
    int cardHeight=pictures ? (lv_obj_get_content_height(body)-gap)/2 : std::max<int>(font()->line_height+12,(lv_obj_get_content_height(body)-3*gap)/4);
    for (unsigned i = 0; i < displayed->count; ++i) {
        auto &item = displayed->items[i];
        auto card = lv_obj_create(body);
        lv_obj_set_size(card, (width - gap * (cols - 1)) / cols,
                        cardHeight);
        lv_obj_set_style_pad_all(card, chrome.edge / 2, 0);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x555555), 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x777777), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_radius(card,5*std::max<int>(1,std::min(lv_display_get_horizontal_resolution(nullptr)/320,lv_display_get_vertical_resolution(nullptr)/240)),0);
        lv_obj_set_flex_align(card,pictures?LV_FLEX_ALIGN_START:LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(card, choose, LV_EVENT_SHORT_CLICKED, (void *)(uintptr_t)i);
        if (pictures) {
            auto art=lv_obj_create(card); lv_obj_remove_style_all(art);
            lv_obj_set_size(art,lv_pct(100),std::max<int>(1,cardHeight-font()->line_height-2*chrome.edge));
            lv_obj_clear_flag(art,LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_flag(art,LV_OBJ_FLAG_EVENT_BUBBLE);
            artSlots[i]=art;paintArt(i);
        }
        auto slash = item.entry.path.find_last_of('/');
        std::string caption=item.entry.path.substr(slash+1);
        auto name = label(card,caption.c_str());
        lv_obj_set_style_text_font(
            name, font(), 0);
        lv_obj_set_style_text_align(name, pictures ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_add_flag(name, LV_OBJ_FLAG_EVENT_BUBBLE);

        lv_obj_set_style_bg_color(card,lv_color_hex(UI_COLOR_BLUE_PRESSED),LV_STATE_PRESSED);
        ui_scroll_guard_add(fileGuard,card);
    }
}
void close() {
    if (closing)
        return;
    closing = true;
    printRequested = false;
    finishPrintPreparation();
    open = false;
    ++epoch;
    if (overlay)
        lv_obj_delete(overlay);
}
void replyResult(unsigned key, unsigned error, bool authorization) {
    if (status) {
        if(authorization && error)lv_label_set_text_fmt(status,"%s  %u  %s",text(key),error,text(Permissions));
        else if(authorization)lv_label_set_text_fmt(status,"%s  %s",text(key),text(Permissions));
        else if(error)lv_label_set_text_fmt(status,"%s  %u",text(key),error);
        else lv_label_set_text(status,text(key));
        lv_obj_set_style_text_color(status,
                                    lv_color_hex(authorization || key == NoReply ? UI_COLOR_WARNING
                                                 : key == Rejected               ? UI_COLOR_ERROR
                                                                                 : 0xFFFFFF),
                                    0);
    }
}
void printResult(bool sent) {
    if (status) {
        lv_obj_set_style_text_color(status, lv_color_hex(sent ? 0xFFFFFF : UI_COLOR_ERROR), 0);
        lv_label_set_text(status, text(sent ? Sent : SendFailed));
    }
}
bool takePrint(Entry &entry, unsigned &plate, bool &ams) {
    if (!printRequested)
        return false;
    printRequested = false;
    finishPrintPreparation();
    if (!open || pendingEpoch != epoch || strcmp(source.serial, xtouch_printer_serial()) ||
        !xtouch_files_available() || !pendingPlate || !displayed || !pendingFresh || !idle())
        return false;
    entry = pendingPrint;
    plate = pendingPlate;
    ams = false;
    return true;
}
} // namespace xtouch_files
