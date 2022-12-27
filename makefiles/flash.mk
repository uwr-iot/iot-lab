BUILD_TARGET ?= esp32

# PORT = /ttyUSB0

# Flash memory layout
BOOTLOADER_START_ADDRESS        = 0x0000
BOOTLOADER_SIZE                 = 0x7000
PARTITION_TABLE_START_ADDRESS   = 0x8000
NVS_START_ADDRESS               = 0x9000
NVS_SIZE                        = 0x6000
APPLICATION_START_ADDRESS       = 0x10000
APPLICATION_SIZE                = 0x100000

# Build artifacts
APPLICATION_FILE    ?= iot-lab
BOOTLOADER_BIN      ?= ${BUILD_DIR}/bootloader/bootloader.bin
PARTITION_TABLE_BIN ?= ${BUILD_DIR}/partition_table/partition-table.bin
APPLICATION_BIN     ?= ${BUILD_DIR}/${APPLICATION_FILE}.bin

ARTIFACTS_DIR       ?= $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

ESPTOOL = esptool.py
ESPTOOL += --chip esp32s3

ifdef PORT
ESPTOOL += --port=${PORT}
endif

flash:
	${ESPTOOL} --baud 460800 --before=default_reset --after=hard_reset \
		write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB \
		${BOOTLOADER_START_ADDRESS}         ${BOOTLOADER_BIN} \
		${PARTITION_TABLE_START_ADDRESS}    ${PARTITION_TABLE_BIN} \
		${APPLICATION_START_ADDRESS}        ${APPLICATION_BIN}

flash-artifacts: export BOOTLOADER_BIN      := $(join ${ARTIFACTS_DIR}/bin/,$(notdir ${BOOTLOADER_BIN}))
flash-artifacts: export PARTITION_TABLE_BIN := $(join ${ARTIFACTS_DIR}/bin/,$(notdir ${PARTITION_TABLE_BIN}))
flash-artifacts: export APPLICATION_FILE    := application
flash-artifacts: export APPLICATION_BIN     := $(join ${ARTIFACTS_DIR}/bin/,$(notdir ${APPLICATION_BIN}))
flash-artifacts: flash

erase:
	${ESPTOOL} erase_flash

erase-bootloader:
	${ESPTOOL} erase_region ${BOOTLOADER_START_ADDRESS} ${BOOTLOADER_SIZE}

erase-application:
	${ESPTOOL} erase_region ${APPLICATION_START_ADDRESS} ${APPLICATION_SIZE}

erase-nvs:
	${ESPTOOL} erase_region ${NVS_START_ADDRESS} ${NVS_SIZE}

setup-flash:
	pip3 install esptool

.DEFAULT_GOAL := flash-artifacts
