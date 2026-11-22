set(DYNA_GOODLUCKOS_SDL2_LIBRARY "" CACHE FILEPATH "ARMHF SDL2 shared library extracted from GoodLuckOS")
set(DYNA_GOODLUCKOS_SDL2_INCLUDE "" CACHE PATH "Directory containing the SDL2 header directory")
if(NOT EXISTS "${DYNA_GOODLUCKOS_SDL2_LIBRARY}" OR NOT EXISTS "${DYNA_GOODLUCKOS_SDL2_INCLUDE}/SDL2/SDL.h")
    message(FATAL_ERROR "Set DYNA_GOODLUCKOS_SDL2_LIBRARY and DYNA_GOODLUCKOS_SDL2_INCLUDE")
endif()
add_library(SDL2::SDL2 SHARED IMPORTED)
set_target_properties(SDL2::SDL2 PROPERTIES
    IMPORTED_LOCATION "${DYNA_GOODLUCKOS_SDL2_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${DYNA_GOODLUCKOS_SDL2_INCLUDE};${DYNA_GOODLUCKOS_SDL2_INCLUDE}/SDL2"
    # The firmware's SDL2 dependencies resolve against the firmware's libc.
    # The Ubuntu sysroot only supplies the link ABI for the game itself.
    INTERFACE_LINK_OPTIONS "LINKER:--allow-shlib-undefined")
set(SDL2_FOUND TRUE)
