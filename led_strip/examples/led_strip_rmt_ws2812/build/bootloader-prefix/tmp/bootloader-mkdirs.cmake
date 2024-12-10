# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/Tanya/esp/esp-idf/components/bootloader/subproject"
  "/Users/Tanya/led_strip/examples/led_strip_rmt_ws2812/build/bootloader"
  "/Users/Tanya/led_strip/examples/led_strip_rmt_ws2812/build/bootloader-prefix"
  "/Users/Tanya/led_strip/examples/led_strip_rmt_ws2812/build/bootloader-prefix/tmp"
  "/Users/Tanya/led_strip/examples/led_strip_rmt_ws2812/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/Tanya/led_strip/examples/led_strip_rmt_ws2812/build/bootloader-prefix/src"
  "/Users/Tanya/led_strip/examples/led_strip_rmt_ws2812/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/Tanya/led_strip/examples/led_strip_rmt_ws2812/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/Tanya/led_strip/examples/led_strip_rmt_ws2812/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()