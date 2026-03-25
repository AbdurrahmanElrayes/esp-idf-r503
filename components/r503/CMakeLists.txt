idf_component_register(
    SRCS
        "src/r503.c"
        "src/r503_uart.c"
        "src/r503_packet.c"
    INCLUDE_DIRS
        "include"
        REQUIRES driver
)