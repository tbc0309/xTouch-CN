# The small SPI screen benefits from rendering tiles in internal SRAM.
# Compose and transfer the completed update in PSRAM (see display.cpp).
target_compile_definitions(${COMPONENT_LIB} PRIVATE
 XTOUCH_DRAW_BUFFER_INTERNAL=1 XTOUCH_REFRESH_MS=16 XTOUCH_TOUCH_MS=8)
