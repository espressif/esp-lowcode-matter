idf_component_register(SRC_DIRS led utils ws2812 bp5758 .
                       INCLUDE_DIRS led utils ws2812 bp5758 .
                       REQUIRES sw_timer rmt i2c)

target_include_directories(
    ${COMPONENT_LIB} PRIVATE ${COMPONENT_INCLUDES}
)
