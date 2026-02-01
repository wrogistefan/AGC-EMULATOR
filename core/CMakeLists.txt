cmake_minimum_required(VERSION 3.0)
project(agc_core C)

# includes and sources
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CORE_SRCS
	src/agc.c
	src/agc_cpu.c
	src/agc_memory.c
	src/agc_instructions.c
	src/agc_debug.c
)

add_executable(agc_emulator src/main.c ${CORE_SRCS})
