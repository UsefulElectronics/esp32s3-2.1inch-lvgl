cmake_minimum_required(VERSION 3.16)
add_compile_options(-fdiagnostics-color=always -DLV_LVGL_H_INCLUDE_SIMPLE -Wno-format -w)
# This example uses an extra component for common functions such as Wi-Fi and Ethernet connection.
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32s3_2.1inch_round_display)
