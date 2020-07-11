idf_component_register(
    SRCS bitmap.c
         text.c
         unicode.c
         ${CMAKE_CURRENT_BINARY_DIR}/DejaVuSans-Bold-16.c
    INCLUDE_DIRS .
)

idf_build_get_property(python PYTHON)

add_custom_command(
    OUTPUT requirements.stamp
    COMMAND "${python}" -m pip install -r "${COMPONENT_DIR}/requirements.txt"
    COMMAND ${CMAKE_COMMAND} -E touch requriements.stamp
)

set(TOOLS "${CMAKE_CURRENT_SOURCE_DIR}/tools")
add_custom_command(OUTPUT DejaVuSans-Bold-16.c
    COMMAND ${TOOLS}/mkfont.py --range 0x20 0x7F "DejaVuSans-Bold" 16 DejaVuSans-Bold-16.bin
    COMMAND xxd -i DejaVuSans-Bold-16.bin DejaVuSans-Bold-16.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${TOOLS}/mkfont.py requirements.stamp
)
