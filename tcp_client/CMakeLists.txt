# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

if("${IDF_TARGET}" STREQUAL "linux")
    set(COMPONENTS main)
endif()

set(EXTRA_COMPONENT_DIRS components/)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(btd_tima)


# Create a SPIFFS image from the contents of the 'font' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash
#spiffs_create_partition_image(storage1 fonts FLASH_IN_PROJECT)
#spiffs_create_partition_image(storage2 images FLASH_IN_PROJECT)
#spiffs_create_partition_image(storage3 icons FLASH_IN_PROJECT)


