cmake_minimum_required(VERSION 3.20)
project(DynaBrothersRecomp VERSION 0.0.1 LANGUAGES C CXX)
enable_testing()
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(RECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/upstream/segagenesisrecomp")
set(RUNNER_ROOT "${RECOMP_ROOT}/runner")
find_package(SDL2 REQUIRED CONFIG)
include("${RECOMP_ROOT}/cmake/GenesisRecompRunner.cmake")
genesisrecomp_runner_sources(RUNNER_SOURCES TRACE ON REVERSE_DEBUG OFF)

function(add_dyna target prefix rom crc entry vint hint)
    if(prefix MATCHES "^db2")
        set(_display "Dyna Brothers 2 Special")
    else()
        set(_display "Dyna Brothers")
    endif()
    file(GLOB PARTS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/generated/${prefix}/${prefix}_part*.c")
    if(NOT PARTS)
        message(FATAL_ERROR "Generate ${prefix} before configuring")
    endif()
    add_executable(${target} ${RUNNER_SOURCES} ${PARTS}
        "generated/${prefix}/${prefix}_dispatch.c"
        "generated/${prefix}/${prefix}_layout.c"
        "generated/${prefix}/${prefix}_cycles.c"
        "${RUNNER_ROOT}/game_cycles.c" games/dyna_spec.c)
    target_include_directories(${target} PRIVATE
        "${RUNNER_ROOT}" "${RUNNER_ROOT}/include" "${RUNNER_ROOT}/video"
        "${RUNNER_ROOT}/external/ymfm/src" "${RUNNER_ROOT}/external/superzazu"
        "${RUNNER_ROOT}/external/clowncommon" "${RECOMP_ROOT}/recompiler/src"
        "${CMAKE_SOURCE_DIR}/generated/${prefix}" "C:/msys64/ucrt64/include")
    target_compile_definitions(${target} PRIVATE
        INCLUDE_GENERATED_CODE=1 ENABLE_RECOMPILED_CODE=1 OWN_BACKEND=1 GEN_ENABLE_TRACE=1
        DEFAULT_DEBUG_PORT=4390 DYNA_NAME="${target}" DYNA_CRC=${crc}
        DYNA_DISPLAY_NAME="${_display}"
        DYNA_SIZE=${rom} DYNA_ENTRY=${entry} DYNA_VINT=${vint} DYNA_HINT=${hint})
    target_link_libraries(${target} PRIVATE SDL2::SDL2main SDL2::SDL2)
    if(WIN32)
        target_link_libraries(${target} PRIVATE comdlg32)
    endif()
    genesisrecomp_runner_target(${target})
endfunction()

add_dyna(DynaBrothers1EN db1 1048576 0xC1E5C0DAu 0x200u 0x312u 0x340u)
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/db2special/db2special_decls.h")
    add_dyna(DynaBrothers2JP db2special 2621440 0xBA3554BCu 0x738u 0x44Au 0x68Cu)
    target_compile_definitions(DynaBrothers2JP PRIVATE DYNA_DB2=1)
endif()
add_executable(DynaRamContractTest tests/db2_ram_test.c games/db2_ram.c
    "${RECOMP_ROOT}/external/m68k-recomp-core/runtime/m68k_step.c")
target_include_directories(DynaRamContractTest PRIVATE "${RECOMP_ROOT}/external/m68k-recomp-core/runtime")
if(MSVC)
    target_compile_options(DynaRamContractTest PRIVATE /UNDEBUG)
else()
    target_compile_options(DynaRamContractTest PRIVATE -UNDEBUG)
endif()
add_test(NAME db2_ram_instruction_contract COMMAND DynaRamContractTest)
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/db1step/db1step_module.c")
    add_dyna(DynaBrothers1AOT db1 1048576 0xC1E5C0DAu 0x200u 0x312u 0x340u)
    get_target_property(_sources DynaBrothers1AOT SOURCES)
    list(FILTER _sources EXCLUDE REGEX "games/dyna_spec.c$")
    set_property(TARGET DynaBrothers1AOT PROPERTY SOURCES "${_sources}")
    file(GLOB STEP_PARTS CONFIGURE_DEPENDS "generated/db1step/db1step_part*.c")
    target_sources(DynaBrothers1AOT PRIVATE games/step_adapter.c games/db2_ram.c ${STEP_PARTS}
        generated/db1step/db1step_module.c
        "${RECOMP_ROOT}/external/m68k-recomp-core/runtime/m68k_step.c")
    target_include_directories(DynaBrothers1AOT PRIVATE generated/db1step
        "${RECOMP_ROOT}/external/m68k-recomp-core/runtime")
    target_compile_definitions(DynaBrothers1AOT PRIVATE DYNA_STEP_HEADER="db1step.h" DYNA_STEP_MODULE=db1step_module)
    if(EXISTS "${CMAKE_SOURCE_DIR}/generated/db1jp/db1jp_module.c")
        file(GLOB JP_PARTS CONFIGURE_DEPENDS "generated/db1jp/db1jp_part*.c")
        target_sources(DynaBrothers1AOT PRIVATE ${JP_PARTS} generated/db1jp/db1jp_module.c)
        target_include_directories(DynaBrothers1AOT PRIVATE generated/db1jp)
        target_compile_definitions(DynaBrothers1AOT PRIVATE DYNA_HAS_JP=1)
    endif()
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/db2step/db2step_module.c")
    add_dyna(DynaBrothers2AOT db2special 2621440 0xBA3554BCu 0x738u 0x44Au 0x68Cu)
    get_target_property(_sources DynaBrothers2AOT SOURCES)
    list(FILTER _sources EXCLUDE REGEX "games/dyna_spec.c$")
    set_property(TARGET DynaBrothers2AOT PROPERTY SOURCES "${_sources}")
    file(GLOB STEP_PARTS CONFIGURE_DEPENDS "generated/db2step/db2step_part*.c")
    target_sources(DynaBrothers2AOT PRIVATE games/step_adapter.c games/db2_ram.c ${STEP_PARTS}
        generated/db2step/db2step_module.c
        "${RECOMP_ROOT}/external/m68k-recomp-core/runtime/m68k_step.c")
    target_include_directories(DynaBrothers2AOT PRIVATE generated/db2step
        "${RECOMP_ROOT}/external/m68k-recomp-core/runtime")
    target_compile_definitions(DynaBrothers2AOT PRIVATE DYNA_STEP_HEADER="db2step.h" DYNA_STEP_MODULE=db2step_module)
    if(EXISTS "${CMAKE_SOURCE_DIR}/generated/db2overlay/db2overlay_module.c")
        file(GLOB OVERLAY_PARTS CONFIGURE_DEPENDS "generated/db2overlay/db2overlay_part*.c")
        target_sources(DynaBrothers2AOT PRIVATE ${OVERLAY_PARTS} generated/db2overlay/db2overlay_module.c)
        target_include_directories(DynaBrothers2AOT PRIVATE generated/db2overlay)
        target_compile_definitions(DynaBrothers2AOT PRIVATE DYNA_HAS_OVERLAY=1)
    endif()
endif()
