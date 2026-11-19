// Only the camera decoder archive calls these functions (see camera_decoder.cmake).
// Espressif TinyH264 permits these data buffers in PSRAM but prefers internal RAM.
// Keep its metadata/reference allocations external; never consume the UI/DMA reserve.
#include <esp_heap_caps.h>
#include <stddef.h>
extern "C" void *xtouch_camera_decoder_malloc(size_t bytes) {
 return heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
}
extern "C" void *xtouch_camera_decoder_malloc_prefer(size_t bytes,size_t,...) {
 return xtouch_camera_decoder_malloc(bytes);
}
extern "C" void *xtouch_camera_decoder_calloc_prefer(size_t count,size_t bytes,size_t,...) {
 return heap_caps_calloc(count,bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
}
