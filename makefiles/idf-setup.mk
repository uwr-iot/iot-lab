IDF_VERSION = v4.4.2

IDF_ROOT_DIR ?= ${PROJECT_DIR}
IDF_DIR = ${IDF_ROOT_DIR}/esp-idf-${IDF_VERSION}

IDF_EXPORT_ENVS = ${IDF_DIR}/export.sh

distclean-idf:
	rm -rf ${IDF_DIR}/

setup-idf:
	make idf-install-prerequisites
	make idf-download
	make idf-setup-tools

setup-idf-light:
	make idf-download
	make idf-setup-tools

idf-install-prerequisites:
	${SUDO} apt-get update \
	&& ${SUDO} apt-get install -y git wget flex bison gperf python3 python3-pip \
	                           python3-setuptools cmake ninja-build ccache \
	                           libffi-dev libssl-dev dfu-util libusb-1.0-0 \
	&& ${SUDO} apt-get install -y rustc

idf-download:
	if [[ ! -d ${IDF_DIR} ]]; then \
		wget https://dl.espressif.com/github_assets/espressif/esp-idf/releases/download/${IDF_VERSION}/esp-idf-${IDF_VERSION}.zip \
		&& unzip esp-idf-${IDF_VERSION}.zip -d ${IDF_ROOT_DIR} \
		&& rm esp-idf-${IDF_VERSION}.zip; \
	fi

idf-setup-tools:
	${IDF_DIR}/install.sh esp32s3

clean-idf:
	source $(IDF_EXPORT_ENVS) && idf.py clean
