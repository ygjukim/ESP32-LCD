# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/yjkim/opt/esp/v5.2.1/esp-idf/components/bootloader/subproject"
  "/home/yjkim/works/esp32_ws/ESP_IDF_LGFX_test/build/bootloader"
  "/home/yjkim/works/esp32_ws/ESP_IDF_LGFX_test/build/bootloader-prefix"
  "/home/yjkim/works/esp32_ws/ESP_IDF_LGFX_test/build/bootloader-prefix/tmp"
  "/home/yjkim/works/esp32_ws/ESP_IDF_LGFX_test/build/bootloader-prefix/src/bootloader-stamp"
  "/home/yjkim/works/esp32_ws/ESP_IDF_LGFX_test/build/bootloader-prefix/src"
  "/home/yjkim/works/esp32_ws/ESP_IDF_LGFX_test/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/yjkim/works/esp32_ws/ESP_IDF_LGFX_test/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/yjkim/works/esp32_ws/ESP_IDF_LGFX_test/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
