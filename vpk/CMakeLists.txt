cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	if(DEFINED ENV{VITASDK})
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
	else()
		message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
	endif()
endif()

set(SHORT_NAME vitastick)
project(${SHORT_NAME})
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Vitastick")
set(VITA_TITLEID  "VITASTICK")
set(VITA_VERSION "01.04")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

if (NOT ${RELEASE})
	add_definitions(-DENABLE_LOGGING)
endif()

include_directories(
	${PROJECT_SOURCE_DIR}/../skprx/uapi
)

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}/../skprx/build/stubs
)

add_executable(${SHORT_NAME}
	main.c
	debugScreenFont.c
)

target_link_libraries(${SHORT_NAME}
	vitastick_stub
	SceDisplay_stub
	SceCtrl_stub
	ScePower_stub
	SceTouch_stub
)

vita_create_self(${SHORT_NAME}.self ${SHORT_NAME})

vita_create_vpk(${SHORT_NAME}.vpk ${VITA_TITLEID} ${SHORT_NAME}.self
	VERSION ${VITA_VERSION}
	NAME ${VITA_APP_NAME}
	FILE	pkg/sce_sys/icon0.png sce_sys/icon0.png
		pkg/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
		pkg/sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
		pkg/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)

add_custom_target(vpksend
	COMMAND curl -T ${SHORT_NAME}.vpk ftp://$(PSVITAIP):1337/ux0:/
	DEPENDS ${SHORT_NAME}.vpk
)

add_custom_target(send
	COMMAND curl -T ${SHORT_NAME}.self ftp://$(PSVITAIP):1337/ux0:/app/${VITA_TITLEID}/eboot.bin
	DEPENDS ${SHORT_NAME}.self
)
