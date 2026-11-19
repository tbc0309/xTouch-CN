# Prevent disconnect/advertisement callbacks from restarting a stopping transport.
idf_component_get_property(PROTOCOMM_DIR protocomm COMPONENT_DIR)
idf_component_get_property(PROTOCOMM_LIB protocomm COMPONENT_LIB)
file(READ "${PROTOCOMM_DIR}/src/transports/protocomm_nimble.c" BLE_TRANSPORT_CODE)
set(BLE_ADVERTISE_START [=[simple_ble_advertise(void)
{
    int rc;
]=])
set(BLE_ADVERTISE_FIXED [=[simple_ble_advertise(void)
{
    if (!protocomm_ble_transport_active()) {
        return;
    }
    int rc;
]=])
set(BLE_STOP_START [=[    bool ble_callbacks_active = true;
    esp_err_t rc = ble_gap_adv_stop();]=])
set(BLE_STOP_FIXED [=[    bool ble_callbacks_active = true;
    // Disable callback-driven advertising before stopping the current advertisement.
    protoble_internal->pc_ble = NULL;
    esp_err_t rc = ble_gap_adv_stop();]=])
foreach(PART ADVERTISE STOP)
 string(FIND "${BLE_TRANSPORT_CODE}" "${BLE_${PART}_START}" BLE_PART_INDEX)
 if(BLE_PART_INDEX EQUAL -1)
  message(FATAL_ERROR "SDK NimBLE transport changed; review provisioning shutdown")
 endif()
 string(REPLACE "${BLE_${PART}_START}" "${BLE_${PART}_FIXED}" BLE_TRANSPORT_CODE "${BLE_TRANSPORT_CODE}")
endforeach()
set(BLE_TRANSPORT_FIXED "${CMAKE_CURRENT_BINARY_DIR}/board_protocomm_nimble.c")
file(CONFIGURE OUTPUT "${BLE_TRANSPORT_FIXED}" CONTENT "${BLE_TRANSPORT_CODE}" @ONLY)
get_target_property(PROTOCOMM_SOURCES ${PROTOCOMM_LIB} SOURCES)
list(FILTER PROTOCOMM_SOURCES EXCLUDE REGEX "/protocomm_nimble[.]c$")
set_property(TARGET ${PROTOCOMM_LIB} PROPERTY SOURCES "${PROTOCOMM_SOURCES}")
target_sources(${PROTOCOMM_LIB} PRIVATE "${BLE_TRANSPORT_FIXED}")
