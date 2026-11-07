# A removed SD-card slot must not suppress interrupts from the C6 SDIO slot.
# Generate the correction from the pinned SDK source; never modify the SDK tree.
idf_component_get_property(SDMMC_DIR esp_driver_sdmmc COMPONENT_DIR)
idf_component_get_property(SDMMC_LIB esp_driver_sdmmc COMPONENT_LIB)
file(READ "${SDMMC_DIR}/src/sd_host_sdmmc.c" SDMMC_HOST_CODE)
set(SDMMC_ISR_START [=[    sd_host_sdmmc_slot_t *slot = ctlr->slot[ctlr->cur_slot_id];

    if (slot == NULL) {
        uint32_t pending = sdmmc_ll_get_intr_status(ctlr->hal.dev);
        uint32_t dma_pending = sdmmc_ll_get_idsts_interrupt_raw(ctlr->hal.dev);
        sdmmc_ll_clear_interrupt(ctlr->hal.dev, pending);
        sdmmc_ll_clear_idsts_interrupt(ctlr->hal.dev, dma_pending);
        return;
    }

    sd_host_sdmmc_event_t event = {};
    bool need_yield = false;
    int higher_priority_task_awoken = pdFALSE;]=])
set(SDMMC_ISR_FIXED [=[    sd_host_sdmmc_slot_t *slot = ctlr->slot[ctlr->cur_slot_id];
    bool need_yield = false;
    int higher_priority_task_awoken = pdFALSE;

    // SDIO interrupts belong to their registered slot, not the last command's slot.
    uint32_t sdio_pending = sdmmc_ll_get_intr_status(ctlr->hal.dev) &
        (SDMMC_INTMASK_IO_SLOT1 | SDMMC_INTMASK_IO_SLOT0);
    if (sdio_pending) {
        sdmmc_ll_enable_interrupt(ctlr->hal.dev, sdio_pending, false);
        xSemaphoreGiveFromISR(ctlr->io_intr_sem, &higher_priority_task_awoken);
        for (unsigned i = 0; i < 2; ++i) {
            const uint32_t mask = i ? SDMMC_INTMASK_IO_SLOT1 : SDMMC_INTMASK_IO_SLOT0;
            sd_host_sdmmc_slot_t *io_slot = ctlr->slot[i];
            if ((sdio_pending & mask) && io_slot && io_slot->cbs.on_io_interrupt) {
                sd_host_evt_data_t edata = {};
                need_yield |= io_slot->cbs.on_io_interrupt(&io_slot->drv, &edata, io_slot->user_data);
            }
        }
    }
    if (slot == NULL) {
        uint32_t pending = sdmmc_ll_get_intr_status(ctlr->hal.dev);
        uint32_t dma_pending = sdmmc_ll_get_idsts_interrupt_raw(ctlr->hal.dev);
        sdmmc_ll_clear_interrupt(ctlr->hal.dev, pending &
            ~(SDMMC_INTMASK_IO_SLOT1 | SDMMC_INTMASK_IO_SLOT0));
        sdmmc_ll_clear_idsts_interrupt(ctlr->hal.dev, dma_pending);
        if (need_yield || higher_priority_task_awoken == pdTRUE) portYIELD_FROM_ISR();
        return;
    }

    sd_host_sdmmc_event_t event = {};]=])
set(SDMMC_ISR_OLD_IO [=[    uint32_t sdio_pending = (sdmmc_ll_get_intr_status(ctlr->hal.dev) & (SDMMC_INTMASK_IO_SLOT1 | SDMMC_INTMASK_IO_SLOT0));
    if (sdio_pending) {
        // disable the interrupt (no need to clear here, this is done in sdmmc_host_io_int_wait)
        sdmmc_ll_enable_interrupt(ctlr->hal.dev, sdio_pending, false);
        xSemaphoreGiveFromISR(ctlr->io_intr_sem, &higher_priority_task_awoken);
        if (slot->cbs.on_io_interrupt) {
            sd_host_evt_data_t edata = {};
            if (slot->cbs.on_io_interrupt(&slot->drv, &edata, slot->user_data)) {
                need_yield |= true;
            }
        }
    }
]=])
string(FIND "${SDMMC_HOST_CODE}" "${SDMMC_ISR_START}" ISR_START_INDEX)
string(FIND "${SDMMC_HOST_CODE}" "${SDMMC_ISR_OLD_IO}" ISR_IO_INDEX)
if(ISR_START_INDEX EQUAL -1 OR ISR_IO_INDEX EQUAL -1)
 message(FATAL_ERROR "SDK SDMMC interrupt dispatch changed; review the shared-slot fix")
endif()
string(REPLACE "${SDMMC_ISR_START}" "${SDMMC_ISR_FIXED}" SDMMC_HOST_CODE "${SDMMC_HOST_CODE}")
string(REPLACE "${SDMMC_ISR_OLD_IO}" "" SDMMC_HOST_CODE "${SDMMC_HOST_CODE}")
set(SDMMC_HOST_FIXED "${CMAKE_CURRENT_BINARY_DIR}/board_sd_host_sdmmc.c")
file(CONFIGURE OUTPUT "${SDMMC_HOST_FIXED}" CONTENT "${SDMMC_HOST_CODE}" @ONLY)
get_target_property(SDMMC_SOURCES ${SDMMC_LIB} SOURCES)
list(FILTER SDMMC_SOURCES EXCLUDE REGEX "/sd_host_sdmmc\\.c$")
set_property(TARGET ${SDMMC_LIB} PROPERTY SOURCES "${SDMMC_SOURCES}")
target_sources(${SDMMC_LIB} PRIVATE "${SDMMC_HOST_FIXED}")
