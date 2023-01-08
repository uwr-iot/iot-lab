SHELL = /bin/bash

include makefiles/variables.mk
include makefiles/flash.mk
include makefiles/idf-setup.mk

SRCS := \
	$(SRC_DIR_NAME)/app_main.c

menuconfig:
	source $(IDF_EXPORT_ENVS) && idf.py menuconfig

setup:
	make setup-idf
	make setup-flash

setup-light:
	make setup-idf-light

all:
	source $(IDF_EXPORT_ENVS) && idf.py build

clean:
	make clean-idf

distclean:
	rm -rf $(BUILD_DIR)

monitor:
	source $(IDF_EXPORT_ENVS) && idf.py monitor --port=${PORT}

doc:
	source $(IDF_EXPORT_ENVS) && idf.py docs

help:
	@echo "Build project targets are:"
	@echo ""
	@echo "  - setup                     create build environment and install all required deps"
	@echo "  - setup-light               create build environment"
	@echo "  - all                       compile source code"
	@echo "  - flash                     install firmware on the target"
	@echo "  - monitor                   open CPU debug console"


.PHONY: menuconfig setup flash clean distclean help
.DEFAULT_GOAL := help
