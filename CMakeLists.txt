cmake_minimum_required(VERSION 2.8)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER "arm-vita-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-vita-eabi-g++")

project(vitastick)

set(CMAKE_C_FLAGS "-Wl,-q -Wall -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

option(RELEASE "Release build" OFF)

if (RELEASE)
	add_definitions(-DRELEASE)
endif(RELEASE)

link_directories(
	${CMAKE_BINARY_DIR}
)

set(SOURCES
	main.c
	log.c
)

add_executable(${PROJECT_NAME}.elf
	${SOURCES}
)

target_link_libraries(${PROJECT_NAME}.elf
	SceSysclibForDriver_stub
	SceThreadmgrForDriver_stub
	SceCpuForDriver_stub
	SceCtrlForDriver_stub
	SceUdcdForDriver_stub
	SceIofilemgrForDriver_stub
)

set_target_properties(${PROJECT_NAME}.elf
	PROPERTIES LINK_FLAGS "-nostdlib"
	COMPILE_FLAGS "-D__VITA_KERNEL__"
)

add_custom_command(OUTPUT ${PROJECT_NAME}.velf
	COMMAND vita-elf-create -e ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.yml ${PROJECT_NAME}.elf ${PROJECT_NAME}.velf
	DEPENDS ${PROJECT_NAME}.elf
)

add_custom_command(OUTPUT ${PROJECT_NAME}.skprx
	COMMAND vita-make-fself ${PROJECT_NAME}.velf ${PROJECT_NAME}.skprx
	DEPENDS ${PROJECT_NAME}.velf
)

add_custom_target(skprx
	DEPENDS ${PROJECT_NAME}.skprx
)

add_custom_target(send
	COMMAND curl -T ${PROJECT_NAME}.skprx ftp://$(PSVITAIP):1337/ux0:/data/tai/kplugin.skprx
	DEPENDS ${PROJECT_NAME}.skprx
)
