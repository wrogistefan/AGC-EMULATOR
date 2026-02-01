cmake_minimum_required(VERSION 3.16)
project(agc_emulator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)

# Główna biblioteka emulatora
add_library(agc_core
    core/src/agc.c
    core/src/agc_cpu.c
    core/src/agc_memory.c
    core/src/agc_instructions.c
    core/src/agc_debug.c
)

target_include_directories(agc_core PUBLIC
    core/include
)

# Główna aplikacja (jeśli chcesz mieć binarkę do testów)
add_executable(agc_main
    core/src/main.c
)

target_link_libraries(agc_main PRIVATE agc_core)

# JNI bridge (opcjonalnie)
add_library(agc_jni SHARED
    bridge/agc_jni.c
)

target_include_directories(agc_jni PRIVATE
    core/include
    bridge
)

target_link_libraries(agc_jni PRIVATE agc_core)

# Testy jednostkowe
enable_testing()

add_executable(test_basic
    tests/test_basic.c
)

target_link_libraries(test_basic PRIVATE agc_core)

add_test(NAME BasicTest COMMAND test_basic)
