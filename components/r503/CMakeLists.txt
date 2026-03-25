idf_component_register(
    SRCS
        "src/r503.c"
        "src/r503_core.c"
        "src/r503_packet.c"
        "src/r503_uart.c"
        "src/r503_info.c"
        "src/r503_template.c"
        "src/r503_highlevel.c"
        "src/r503_led.c"
    INCLUDE_DIRS
        "include"
    PRIV_INCLUDE_DIRS
        "src"
    REQUIRES driver
)
