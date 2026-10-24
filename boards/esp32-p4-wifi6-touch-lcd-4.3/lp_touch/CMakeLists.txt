# Keep LP compilation separate from the large HP/Arduino include graph.
# This also keeps the Windows ExternalProject command below cmd.exe limits.
idf_component_register(SRCS "embed.c" INCLUDE_DIRS "." REQUIRES ulp)
ulp_embed_binary(xtouch_lp_touch "main.c" "embed.c")
target_include_directories(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/xtouch_lp_touch")
