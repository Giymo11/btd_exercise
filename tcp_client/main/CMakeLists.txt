idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    set(requires esp_event esp_stubs protocol_examples_common nvs_flash)
endif()

# idf_component_register(SRCS "main.c" "decode_jpeg.c" "tcp_client_v4.c" "tcp_client_main.c"
#                                 INCLUDE_DIRS "."
#                                 REQUIRES ${requires})

set(srcs "screen/btd_screen.c"
	"screen/decode_jpeg.c"
	"screen/decode_png.c"
	"screen/pngle.c"
    "btd_imu.c"
    "btd_lowpass.c"
    "btd_exercise2.c"
	)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")

