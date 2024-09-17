#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

set(OS_AGNOSTIC_BASE ${ZEPHYR_HAL_NORDIC_MODULE_DIR}/drivers/nrf_wifi)
set(FW_BINS_BASE ${ZEPHYR_HAL_NORDIC_MODULE_DIR}/zephyr/blobs/wifi_fw_bins)

zephyr_include_directories(
  inc
  ${OS_AGNOSTIC_BASE}/utils/inc
  ${OS_AGNOSTIC_BASE}/os_if/inc
  ${OS_AGNOSTIC_BASE}/bus_if/bus/qspi/inc
  ${OS_AGNOSTIC_BASE}/bus_if/bal/inc
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/inc
  ${OS_AGNOSTIC_BASE}/fw_load/mips/fw/inc
  ${OS_AGNOSTIC_BASE}/hw_if/hal/inc
  src/qspi/inc
  # for net_sprint_ll_addr
  ${ZEPHYR_BASE}/subsys/net/ip
  ${OS_AGNOSTIC_BASE}/hw_if/hal/inc/fw
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/inc/fw
)

zephyr_include_directories_ifdef(CONFIG_NRF70_RADIO_TEST
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/inc/radio_test
)

zephyr_include_directories_ifndef(CONFIG_NRF70_RADIO_TEST
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/inc/default
)

zephyr_library_sources_ifdef(CONFIG_NRF70_SR_COEX
  src/coex.c
)

zephyr_library_sources(
  ${OS_AGNOSTIC_BASE}/os_if/src/osal.c
  ${OS_AGNOSTIC_BASE}/utils/src/list.c
  ${OS_AGNOSTIC_BASE}/utils/src/queue.c
  ${OS_AGNOSTIC_BASE}/utils/src/util.c
  ${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_api.c
  ${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_fw_patch_loader.c
  ${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_interrupt.c
  ${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_mem.c
  ${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_reg.c
  ${OS_AGNOSTIC_BASE}/hw_if/hal/src/hpqm.c
  ${OS_AGNOSTIC_BASE}/hw_if/hal/src/pal.c
  ${OS_AGNOSTIC_BASE}/bus_if/bal/src/bal.c
  ${OS_AGNOSTIC_BASE}/bus_if/bus/qspi/src/qspi.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/cmd.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/event.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_api_common.c
  src/shim.c
  src/work.c
  src/timer.c
  src/fmac_main.c
  src/qspi/src/device.c
  src/qspi/src/rpu_hw_if.c
  src/qspi/src/ficr_prog.c
)

zephyr_library_sources_ifndef(CONFIG_NRF_WIFI_BUILD_ONLY_MODE
  src/fw_load.c
)

zephyr_library_sources_ifndef(CONFIG_NRF70_RADIO_TEST
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/rx.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_vif.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_util.c
  src/net_if.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/default/fmac_api.c
)

zephyr_library_sources_ifdef(CONFIG_NET_L2_WIFI_MGMT
  src/wifi_mgmt_scan.c
)

zephyr_library_sources_ifdef(CONFIG_NRF70_SYSTEM_MODE_COMMON
  src/wifi_mgmt.c
)

zephyr_library_sources_ifdef(CONFIG_NRF70_RADIO_TEST
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/radio_test/fmac_api.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_util.c
)

zephyr_library_sources_ifdef(CONFIG_NRF70_DATA_TX
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/tx.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_peer.c
)

zephyr_library_sources_ifdef(CONFIG_NRF70_STA_MODE
  src/wpa_supp_if.c
  src/wifi_mgmt.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_peer.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_util.c
)

zephyr_library_sources_ifdef(CONFIG_NRF70_AP_MODE
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_ap.c
)


# Without WPA supplicant we only support scan
zephyr_library_sources_ifdef(CONFIG_NRF70_STA_MODE
  src/wpa_supp_if.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_peer.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_util.c
)

zephyr_library_sources_ifdef(CONFIG_NRF70_ON_QSPI
  src/qspi/src/qspi_if.c
)

zephyr_library_sources_ifdef(CONFIG_NRF70_ON_SPI
  src/qspi/src/spi_if.c
)

zephyr_library_sources_ifdef(CONFIG_NRF70_UTIL
  src/wifi_util.c
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_ON_QSPI
# These are XIP related anomalies and aren't applicable for nRF7002 and cause
# throughput issues.
  -DNRF53_ERRATA_43_ENABLE_WORKAROUND=0
  -DNRF52_ERRATA_215_ENABLE_WORKAROUND=0
# nRF70 QSPI doesn't use 192MHz clock and most samples use 128MHz, this can cause anomaly 159
# but as its rare and not seen in most cases, we can disable it.
# Alternative is 128MHz CPU should be disabled that impacts Wi-Fi performance.
  -DNRF53_ERRATA_159_ENABLE_WORKAROUND=0
)

if (CONFIG_NRF_WIFI_BUILD_ONLY_MODE)
message(WARNING "
------------------------------------------------------------------------
Building only the nRF70 driver, skipping firmware patch.
This is only for building (CI) purposes and will not work on a real device.
------------------------------------------------------------------------
")
else()
# RPU FW patch binaries based on the selected configuration
if(CONFIG_NRF70_SYSTEM_MODE)
  set(NRF70_PATCH ${FW_BINS_BASE}/default/nrf70.bin)
elseif(CONFIG_NRF70_RADIO_TEST)
  set(NRF70_PATCH ${FW_BINS_BASE}/radio_test/nrf70.bin)
elseif(CONFIG_NRF70_SCAN_ONLY)
  set(NRF70_PATCH ${FW_BINS_BASE}/scan_only/nrf70.bin)
elseif (CONFIG_NRF70_SYSTEM_WITH_RAW_MODES)
  set(NRF70_PATCH ${FW_BINS_BASE}/system_with_raw/nrf70.bin)
else()
  # Error
  message(FATAL_ERROR "Unsupported nRF70 patch configuration")
endif()

if(NOT EXISTS ${NRF70_PATCH})
  message(FATAL_ERROR "
    ------------------------------------------------------------------------
    Missing blobs for nRF70 device driver, please install by running:
        $ west update
        $ west blobs fetch hal_nordic
    ------------------------------------------------------------------------")
endif()

zephyr_compile_definitions(
  -DCONFIG_NRF_WIFI_FW_BIN=${NRF70_PATCH}
)
endif()



# Translate the configuration to the OS agnostic code
zephyr_compile_definitions_ifdef(CONFIG_NRF_WIFI_LOW_POWER
  -DNRF_WIFI_LOW_POWER
)

zephyr_compile_definitions_ifdef(CONFIG_NRF_WIFI_RPU_RECOVERY
  -DNRF_WIFI_RPU_RECOVERY
)

zephyr_compile_definitions_ifdef(CONFIG_NRF_WIFI_AP_DEAD_DETECT_TIMEOUT
  -DNRF_WIFI_AP_DEAD_DETECT_TIMEOUT=${CONFIG_NRF_WIFI_AP_DEAD_DETECT_TIMEOUT}
)

zephyr_compile_definitions_ifdef(CONFIG_NRF_WIFI_IFACE_MTU
  -DNRF_WIFI_IFACE_MTU=${CONFIG_NRF_WIFI_IFACE_MTU}
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_STA_MODE
  -DNRF70_STA_MODE
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_DATA_TX
  -DNRF70_DATA_TX
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_RAW_DATA_TX
  -DNRF70_RAW_DATA_TX
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_RAW_DATA_RX
  -DNRF70_RAW_DATA_RX
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_PROMISC_DATA_RX
  -DNRF70_PROMISC_DATA_RX
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_TX_DONE_WQ_ENABLED
  -DNRF70_TX_DONE_WQ_ENABLED
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_RX_WQ_ENABLED
  -DNRF70_RX_WQ_ENABLED
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_UTIL
  -DNRF70_UTIL
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_RADIO_TEST
  -DNRF70_RADIO_TEST
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_TCP_IP_CHECKSUM_OFFLOAD
  -DNRF70_TCP_IP_CHECKSUM_OFFLOAD
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_RPU_EXTEND_TWT_SP
  -DNRF70_RPU_EXTEND_TWT_SP
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_SYSTEM_WITH_RAW_MODES
  -DNRF70_SYSTEM_WITH_RAW_MODES
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_SCAN_ONLY
  -DNRF70_SCAN_ONLY
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_SYSTEM_MODE
  -DNRF70_SYSTEM_MODE
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_2_4G_ONLY
  -DNRF70_2_4G_ONLY
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_LOG_VERBOSE
  -DNRF70_LOG_VERBOSE
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_AP_MODE
  -DNRF70_AP_MODE
)

zephyr_compile_definitions(
  -DNRF70_RX_NUM_BUFS=${CONFIG_NRF70_RX_NUM_BUFS}
  -DNRF70_MAX_TX_TOKENS=${CONFIG_NRF70_MAX_TX_TOKENS}
  -DNRF70_RX_MAX_DATA_SIZE=${CONFIG_NRF70_RX_MAX_DATA_SIZE}
  -DNRF70_MAX_TX_PENDING_QLEN=${CONFIG_NRF70_MAX_TX_PENDING_QLEN}
  -DNRF70_RPU_PS_IDLE_TIMEOUT_MS=${CONFIG_NRF70_RPU_PS_IDLE_TIMEOUT_MS}
  -DNRF70_REG_DOMAIN=${CONFIG_NRF70_REG_DOMAIN}
  -DNRF70_BAND_2G_LOWER_EDGE_BACKOFF_DSSS=${CONFIG_NRF70_BAND_2G_LOWER_EDGE_BACKOFF_DSSS}
  -DNRF70_BAND_2G_LOWER_EDGE_BACKOFF_HT=${CONFIG_NRF70_BAND_2G_LOWER_EDGE_BACKOFF_HT}
  -DNRF70_BAND_2G_LOWER_EDGE_BACKOFF_HE=${CONFIG_NRF70_BAND_2G_LOWER_EDGE_BACKOFF_HE}
  -DNRF70_BAND_2G_UPPER_EDGE_BACKOFF_DSSS=${CONFIG_NRF70_BAND_2G_UPPER_EDGE_BACKOFF_DSSS}
  -DNRF70_BAND_2G_UPPER_EDGE_BACKOFF_HT=${CONFIG_NRF70_BAND_2G_UPPER_EDGE_BACKOFF_HT}
  -DNRF70_BAND_2G_UPPER_EDGE_BACKOFF_HE=${CONFIG_NRF70_BAND_2G_UPPER_EDGE_BACKOFF_HE}
  -DNRF70_BAND_UNII_1_LOWER_EDGE_BACKOFF_HT=${CONFIG_NRF70_BAND_UNII_1_LOWER_EDGE_BACKOFF_HT}
  -DNRF70_BAND_UNII_1_LOWER_EDGE_BACKOFF_HE=${CONFIG_NRF70_BAND_UNII_1_LOWER_EDGE_BACKOFF_HE}
  -DNRF70_BAND_UNII_1_UPPER_EDGE_BACKOFF_HT=${CONFIG_NRF70_BAND_UNII_1_UPPER_EDGE_BACKOFF_HT}
  -DNRF70_BAND_UNII_1_UPPER_EDGE_BACKOFF_HE=${CONFIG_NRF70_BAND_UNII_1_UPPER_EDGE_BACKOFF_HE}
  -DNRF70_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HT=${CONFIG_NRF70_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HT}
  -DNRF70_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HE=${CONFIG_NRF70_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HE}
  -DNRF70_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HT=${CONFIG_NRF70_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HT}
  -DNRF70_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HE=${CONFIG_NRF70_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HE}
  -DNRF70_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HT=${CONFIG_NRF70_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HT}
  -DNRF70_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HE=${CONFIG_NRF70_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HE}
  -DNRF70_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HT=${CONFIG_NRF70_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HT}
  -DNRF70_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HE=${CONFIG_NRF70_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HE}
  -DNRF70_BAND_UNII_3_LOWER_EDGE_BACKOFF_HT=${CONFIG_NRF70_BAND_UNII_3_LOWER_EDGE_BACKOFF_HT}
  -DNRF70_BAND_UNII_3_LOWER_EDGE_BACKOFF_HE=${CONFIG_NRF70_BAND_UNII_3_LOWER_EDGE_BACKOFF_HE}
  -DNRF70_BAND_UNII_3_UPPER_EDGE_BACKOFF_HT=${CONFIG_NRF70_BAND_UNII_3_UPPER_EDGE_BACKOFF_HT}
  -DNRF70_BAND_UNII_3_UPPER_EDGE_BACKOFF_HE=${CONFIG_NRF70_BAND_UNII_3_UPPER_EDGE_BACKOFF_HE}
  -DNRF70_BAND_UNII_4_LOWER_EDGE_BACKOFF_HT=${CONFIG_NRF70_BAND_UNII_4_LOWER_EDGE_BACKOFF_HT}
  -DNRF70_BAND_UNII_4_LOWER_EDGE_BACKOFF_HE=${CONFIG_NRF70_BAND_UNII_4_LOWER_EDGE_BACKOFF_HE}
  -DNRF70_BAND_UNII_4_UPPER_EDGE_BACKOFF_HT=${CONFIG_NRF70_BAND_UNII_4_UPPER_EDGE_BACKOFF_HT}
  -DNRF70_BAND_UNII_4_UPPER_EDGE_BACKOFF_HE=${CONFIG_NRF70_BAND_UNII_4_UPPER_EDGE_BACKOFF_HE}
  -DNRF70_PCB_LOSS_2G=${CONFIG_NRF70_PCB_LOSS_2G}
  -DNRF70_PCB_LOSS_5G_BAND1=${CONFIG_NRF70_PCB_LOSS_5G_BAND1}
  -DNRF70_PCB_LOSS_5G_BAND2=${CONFIG_NRF70_PCB_LOSS_5G_BAND2}
  -DNRF70_PCB_LOSS_5G_BAND3=${CONFIG_NRF70_PCB_LOSS_5G_BAND3}
  -DNRF70_ANT_GAIN_2G=${CONFIG_NRF70_ANT_GAIN_2G}
  -DNRF70_ANT_GAIN_5G_BAND1=${CONFIG_NRF70_ANT_GAIN_5G_BAND1}
  -DNRF70_ANT_GAIN_5G_BAND2=${CONFIG_NRF70_ANT_GAIN_5G_BAND2}
  -DNRF70_ANT_GAIN_5G_BAND3=${CONFIG_NRF70_ANT_GAIN_5G_BAND3}
)

if(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP)
# Run patches from the external flash (XIP). No need to copy.
zephyr_code_relocate(FILES src/fw_load.c LOCATION EXTFLASH_RODATA NOCOPY)
endif()

if(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_STORE)
  message(STATUS "nRF WiFi FW patch binary will be stored in external flash")

  dt_chosen(ext_flash_dev PROPERTY nordic,pm-ext-flash)
  if(DEFINED ext_flash_dev)
    get_filename_component(qspi_node ${ext_flash_dev} DIRECTORY)
  else()
    dt_nodelabel(qspi_node NODELABEL "qspi")
  endif()
  if(DEFINED qspi_node)
    dt_reg_addr(xip_addr PATH ${qspi_node} NAME qspi_mm)
    if(NOT DEFINED xip_addr)
      message(FATAL_ERROR "Unable to find XIP area in dts")
    endif()
  else()
    message(FATAL_ERROR "QSPI node not found in dts")
  endif()

  if(CONFIG_PARTITION_MANAGER_ENABLED)
    if(NOT SYSBUILD)
      add_custom_target(nrf70_wifi_fw_patch_target
        DEPENDS ${CMAKE_BINARY_DIR}/zephyr/nrf70.hex
      )
      add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/zephyr/nrf70.hex
        COMMAND ${Python3_EXECUTABLE}
          -c "import sys; import intelhex; intelhex.bin2hex(sys.argv[1], sys.argv[2], int(sys.argv[3])) "
          ${NRF70_PATCH}
          ${CMAKE_BINARY_DIR}/zephyr/nrf70.hex
          $<TARGET_PROPERTY:partition_manager,nrf70_wifi_fw_XIP_ABS_ADDR>
          VERBATIM
        )
      # Delegate merging WiFi FW patch to mcuboot because we need to merge signed hex instead of raw nrf70.hex.
      if(NOT CONFIG_NRF_WIFI_FW_PATCH_DFU)
        set_property(
          GLOBAL PROPERTY
          nrf70_wifi_fw_PM_HEX_FILE
          ${CMAKE_BINARY_DIR}/zephyr/nrf70.hex
          )
        set_property(
          GLOBAL PROPERTY
          nrf70_wifi_fw_PM_TARGET
          nrf70_wifi_fw_patch_target
          )
      endif()
    endif()
  else()
    dt_nodelabel(nrf70_fw_node NODELABEL "nrf70_fw_partition")
    if(NOT DEFINED nrf70_fw_node)
      message(FATAL_ERROR "nrf70_fw_partition node not found in dts")
    endif()
    dt_prop(nrf70_fw_offset PATH ${nrf70_fw_node} PROPERTY reg INDEX 0)
    if(NOT DEFINED nrf70_fw_offset)
      message(FATAL_ERROR "nrf70_fw_partition.reg not found in dts")
    endif()
    math(EXPR NRF70_PATCH_ADDR "${xip_addr} + ${nrf70_fw_offset}")

    # Using python module instead of intelhex command line tool to handle windows
    execute_process(
      COMMAND ${Python3_EXECUTABLE}
        -c "import sys; import intelhex; intelhex.bin2hex(sys.argv[1], sys.argv[2], int(sys.argv[3]))"
        ${NRF70_PATCH}
        ${CMAKE_BINARY_DIR}/zephyr/nrf70.hex
        ${NRF70_PATCH_ADDR}
      RESULT_VARIABLE _bin2hex_result
      OUTPUT_VARIABLE _bin2hex_output
      ERROR_VARIABLE _bin2hex_error
      COMMAND_ECHO STDOUT
    )

    if(NOT _bin2hex_result EQUAL 0)
      message(FATAL_ERROR "Failed to convert binary patch to hex format: \n${_bin2hex_output}\n${_bin2hex_error}")
    endif()

    set_property(GLOBAL APPEND PROPERTY HEX_FILES_TO_MERGE
      ${CMAKE_BINARY_DIR}/zephyr/nrf70.hex ${CMAKE_BINARY_DIR}/zephyr/zephyr.hex)
    set_property(TARGET runners_yaml_props_target PROPERTY
      hex_file ${CMAKE_BINARY_DIR}/zephyr/merged.hex)
  endif()
endif()