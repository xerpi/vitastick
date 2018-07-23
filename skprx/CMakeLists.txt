cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	if(DEFINED ENV{VITASDK})
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
	else()
		message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
	endif()
endif()

project(vitastick)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O2 -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

option(RELEASE "Release build" OFF)

if (RELEASE)
	add_definitions(-DRELEASE)
endif(RELEASE)


add_executable(${PROJECT_NAME}
	main.c
	log.c
)

target_link_libraries(${PROJECT_NAME}
	SceSysclibForDriver_stub
	SceThreadmgrForDriver_stub
	SceCpuForDriver_stub
	SceCtrlForDriver_stub
	SceUdcdForDriver_stub
	SceIofilemgrForDriver_stub
)

vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME} CONFIG ${PROJECT_NAME}.yml UNSAFE)

vita_create_stubs(stubs ${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.yml KERNEL)

add_custom_target(send
	COMMAND curl -T ${PROJECT_NAME}.skprx ftp://$(PSVITAIP):1337/ux0:/tai/
	DEPENDS ${PROJECT_NAME}.skprx
)
