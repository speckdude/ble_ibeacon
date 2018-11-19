deps_config := \
	/home/matt/esp/esp-idf/components/app_trace/Kconfig \
	/home/matt/esp/esp-idf/components/aws_iot/Kconfig \
	/home/matt/esp/esp-idf/components/bt/Kconfig \
	/home/matt/esp/esp-idf/components/driver/Kconfig \
	/home/matt/esp/esp-idf/components/esp32/Kconfig \
	/home/matt/esp/esp-idf/components/esp_adc_cal/Kconfig \
	/home/matt/esp/esp-idf/components/esp_event/Kconfig \
	/home/matt/esp/esp-idf/components/esp_http_client/Kconfig \
	/home/matt/esp/esp-idf/components/esp_http_server/Kconfig \
	/home/matt/esp/esp-idf/components/ethernet/Kconfig \
	/home/matt/esp/esp-idf/components/fatfs/Kconfig \
	/home/matt/esp/esp-idf/components/freemodbus/Kconfig \
	/home/matt/esp/esp-idf/components/freertos/Kconfig \
	/home/matt/esp/esp-idf/components/heap/Kconfig \
	/home/matt/esp/esp-idf/components/libsodium/Kconfig \
	/home/matt/esp/esp-idf/components/log/Kconfig \
	/home/matt/esp/esp-idf/components/lwip/Kconfig \
	/home/matt/esp/esp-idf/components/mbedtls/Kconfig \
	/home/matt/esp/esp-idf/components/mdns/Kconfig \
	/home/matt/esp/esp-idf/components/mqtt/Kconfig \
	/home/matt/esp/esp-idf/components/nvs_flash/Kconfig \
	/home/matt/esp/esp-idf/components/openssl/Kconfig \
	/home/matt/esp/esp-idf/components/pthread/Kconfig \
	/home/matt/esp/esp-idf/components/spi_flash/Kconfig \
	/home/matt/esp/esp-idf/components/spiffs/Kconfig \
	/home/matt/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/home/matt/esp/esp-idf/components/vfs/Kconfig \
	/home/matt/esp/esp-idf/components/wear_levelling/Kconfig \
	/home/matt/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/matt/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/matt/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/matt/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)

ifneq "$(IDF_CMAKE)" "n"
include/config/auto.conf: FORCE
endif

$(deps_config): ;
