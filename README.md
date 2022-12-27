# Introduction to Internet of Things

This repository contains customized ESP-IDF-based project setup dedicated
for *Introduction to Internet of Things* Hands-on sessions.

## Preconditions

1. Add user user the following groups: `sudo`, `plugdev` and `dialout`.

        $ sudo usermod -a -G sudo ${USER}
        $ sudo usermod -a -G plugdev ${USER}
        $ sudo usermod -a -G dialout ${USER}

2. Install `build-essential` package.

        $ sudo apt-get update && sudo apt-get install build-essential

3. Clone repository.

        $ git clone git@github.com:uwr-iot/iot-lab.git

4. Download & install project dependencies.

        $ cd iot-lab
        $ make setup

## Source code compilation, installation and debugging

Use following `make` targets:

* `make all` - to compile project,
* `make flash` - to install firmware on main CPU,
* `make monitor` - to open debug console.

## More info

Complete documentation for ESP-IDF can be found [here](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32s3/index.html).
