project(Reti)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)
find_package(ZLIB QUIET)
find_library(KMOD kmod)
cmake_policy(SET CMP0012 NEW)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.2")
		set(STD_REGEX_SUPPORTED FALSE)
	else()
		set(STD_REGEX_SUPPORTED TRUE)
	endif()
endif()

set(CMAKE_INSTALL_PREFIX "/usr")

option(EXAM_MODE "Enable exam mode" OFF)

option(DISABLE_CCACHE "Disable CCache" OFF)
if(DISABLE_CCACHE)
	message("-- CCache: disabled")
else()
	find_program(CCACHE_FOUND ccache)
	if(CCACHE_FOUND)
		message("-- CCache: enabled")
		set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
		set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
	else()
		message("-- CCache: wanted, not found")
	endif()
endif()

set(CMAKE_AUTOMOC ON)
SET(QT_USE_QTXML TRUE)
include(${QT_USE_FILE})

include_directories(${QT_INCLUDES}
			${CMAKE_CURRENT_BINARY_DIR}

			"virtualbox-4.3.10-dfsg/include/"
			"sdk/bindings/xpcom/idl"
			"sdk/bindings/xpcom/include"
			"sdk/bindings/xpcom/include/nsprpub"
			"sdk/bindings/xpcom/include/string"
			"sdk/bindings/xpcom/include/xpcom"
			"sdk/bindings/xpcom/include/ipcd"

# 			"virtualbox-4.3.10-dfsg/include/"
# 			"virtualbox-4.3.10-dfsg/src/VBox/Frontends/VirtualBox/src/globals/"
# 			"virtualbox-4.3.10-dfsg/src/VBox/Main/include/"
# 			"sdk/bindings/xpcom"
# 			"sdk/bindings/xpcom/cbinding"
# 			"sdk/bindings/xpcom/idl"
# 			"sdk/bindings/xpcom/include"
# 			"sdk/bindings/xpcom/include/ipcd"
# 			"sdk/bindings/xpcom/include/nsprpub"
# 			"sdk/bindings/xpcom/include/nsprpub/md"
# 			"sdk/bindings/xpcom/include/nsprpub/obsolete"
# 			"sdk/bindings/xpcom/include/nsprpub/private"
# 			"sdk/bindings/xpcom/include/string"
# 			"sdk/bindings/xpcom/include/xpcom"
)

qt4_wrap_ui(ui_src
	"src/MainWindow.ui"
	"src/CloneDialog.ui"
	"src/ProgressDialog.ui"
	"src/info_dialog.ui"
# 	"src/SummaryDialog.ui"
	"src/MachinesDialog.ui"
)

qt4_add_resources(ui_res "src/res.qrc")

set(Reti_SRCS
	"src/CloneDialog.cpp"
	"src/crc32.cpp"
	"src/Iface.cpp"
	"src/IfacesTable.cpp"
	"src/InfoDialog.cpp"
	"src/MachinesDialog.cpp"
	"src/main.cpp"
	"src/MainWindow.cpp"
	"src/OSBridge.cpp"
	"src/ProgressDialog.cpp"
	"src/SignalSpy.cpp"
	"src/SummaryDialog.cpp"
	"src/VirtualBoxBridge.cpp"
	"src/VirtualMachine.cpp"
	"src/VMSettings.cpp"
	"src/VMTabSettings.cpp"
	"src/UIMainEventListener.cpp"
)

add_definitions(-DVBOX_WITH_XPCOM_NAMESPACE_CLEANUP)

option(EXAM_MODE "Enable building settings for exam mode" OFF)
if(EXAM_MODE)
	message("-- Exam mode: enabled")
	add_definitions(-DEXAM_MODE)

	find_library(GPGME gpgme)
	if(GPGME STREQUAL "GPGME-NOTFOUND")
		message(FATAL_ERROR "-- libgpgme not found")
	endif()

	message("-- Found libgpgme at '${GPGME}'")
	set(GPGME_LIB ${GPGME})

	qt4_wrap_ui(ui_src
		"src/ExamDialog.ui")

	set(Reti_SRCS "${Reti_SRCS}"
		"src/ExamDialog.cpp"
		"src/GPGMEWrapper.cpp")

	set(PROGRAM_NAME "vb-ant")
	message("-- Program name: vb-ant")
	add_definitions(-DPROGRAM_NAME="vb-ant")

	message("-- OS partition number: 2")
	add_definitions(-DOS_PARTITION_NUMBER=2)

	message("-- IPv4 configuration: supported, enabled")
	add_definitions(-DCONFIGURABLE_IP)

	if(STD_REGEX_SUPPORTED)
		add_definitions(-DENABLE_IPv6)
		message("-- IPv6 configuration: supported, enabled")
	else(STD_REGEX_SUPPORTED)
		add_definitions(-UENABLE_IPv6)
		message("-- IPv6 configuration: unsupported, wanted")
	endif(STD_REGEX_SUPPORTED)

	message("-- IP validation: disabled")
	add_definitions(-UVALIDATE_IP)

else(EXAM_MODE)
	message("-- Exam mode: disabled")
	add_definitions(-UEXAM_MODE)

	set(PROGRAM_NAME "vb-ant" CACHE STRING "Selects program name")
	option(PROGRAM_NAME "Selects program name" "vb-ant")
	message("-- Program name: ${PROGRAM_NAME}")

	add_definitions(-DPROGRAM_NAME="${PROGRAM_NAME}")

	set(OS_PARTITION_NUMBER 2 CACHE STRING "Selects guest OS partition number")
	option(OS_PARTITION_NUMBER "Selects guest OS partition number" 2)
	if("${OS_PARTITION_NUMBER}" STREQUAL "")
		set(OS_PARTITION_NUMBER "2")
	endif()

	if(OS_PARTITION_NUMBER MATCHES "^0$" AND NOT OS_PARTITION_NUMBER MATCHES "^[[:digit:]]*$")
		message(FATAL_ERROR "OS_PARTITION_NUMBER must be a number greater than 0")
	else()
		add_definitions(-DOS_PARTITION_NUMBER=${OS_PARTITION_NUMBER})
		message("-- OS partition number: ${OS_PARTITION_NUMBER}")
	endif()

	option(CONFIGURABLE_IP "Enable IP and subnet mask configuration" OFF)
	if(CONFIGURABLE_IP)
		add_definitions(-DCONFIGURABLE_IP)
		message("-- IPv4 configuration: supported, enabled")
		if(STD_REGEX_SUPPORTED)
			add_definitions(-DENABLE_IPv6)
			message("-- IPv6 configuration: supported, enabled")
		else(STD_REGEX_SUPPORTED)
			add_definitions(-UENABLE_IPv6)
			message(WARNING "-- IPv6 configuration: unsupported, wanted")
		endif(STD_REGEX_SUPPORTED)
	else(CONFIGURABLE_IP)
		add_definitions(-UCONFIGURABLE_IP)
		add_definitions(-UENABLE_IPv6)
		message("-- IPv4 configuration: supported, disabled")
		if(STD_REGEX_SUPPORTED)
			message("-- IPv6 configuration: supported, disabled")
		else()
			message("-- IPv6 configuration: unsupported, disabled")
		endif()
	endif(CONFIGURABLE_IP)

	option(VALIDATE_IP "Enable IP and subnet mask validation" OFF)
	if(CONFIGURABLE_IP)
		if(VALIDATE_IP)
			add_definitions(-DVALIDATE_IP)
			message("-- IP validation: enabled")
		else(VALIDATE_IP)
			add_definitions(-UVALIDATE_IP)
			message("-- IP validation: disabled")
		endif(VALIDATE_IP)
	else(CONFIGURABLE_IP)
		add_definitions(-UVALIDATE_IP)
		message("-- IP validation: unavailable")
	endif(CONFIGURABLE_IP)
endif(EXAM_MODE)

qt4_automoc(${Reti_SRCS})

add_definitions(-fshort-wchar -std=c++0x)

option(DEBUG "Enable debug output" OFF)
if(DEBUG)
	add_definitions(-DDEBUG_FLAG -g)
	message("-- Debug output: enabled")
else()
	add_definitions(-UDEBUG_FLAG)
	message("-- Debug output: disabled")
endif()

if(ZLIB_FOUND)
	message("-- Found zlib at '${ZLIB_LIBRARIES}'")
endif(ZLIB_FOUND)

option(ZLIB "Enable zlib support" ON)
if(ZLIB)
	if(ZLIB_FOUND)
		message("-- Zlib support: enabled")
		add_definitions(-I${ZLIB_INCLUDE_DIR} -DUSE_ZLIB)
		set(Z_LIB ${ZLIB_LIBRARIES})
		set(Reti_SRCS "${Reti_SRCS}"
			"src/ZlibWrapper.cpp")
	else(ZLIB_FOUND)
		message(WARNING "-- Zlib support: wanted, unsupported")
		add_definitions(-UUSE_ZLIB)
	endif(ZLIB_FOUND)
else(ZLIB)
	message("-- Zlib support: disabled")
	add_definitions(-UUSE_ZLIB)
endif(ZLIB)

set(SYSTEM_LIBS "pthread")
set(VBOX_LIB "/usr/lib/virtualbox/VBoxXPCOM.so" CACHE STRING "VBoxXPCOM.so path")
option(VBOX_LIB "VBoxXPCOM.so path" "/usr/lib/virtualbox/VBoxXPCOM.so")
message("-- VBoxXPCOM.so path: ${VBOX_LIB}")

add_executable(${PROGRAM_NAME} ${Reti_SRCS} ${ui_src} ${ui_res})

target_link_libraries(${PROGRAM_NAME}
			${SYSTEM_LIBS}
			${QT_QTCORE_LIBRARY}
			${QT_QTGUI_LIBRARY}
			${VBOX_LIB}
			${Z_LIB}
			${GPGME_LIB}
)

if(NOT KMOD STREQUAL "KMOD-NOTFOUND")
	message("-- Found libkmod at '${KMOD}'")
	set(nbdtool_SRCS "src/nbdtool.cpp")
	add_executable(nbdtool ${nbdtool_SRCS})
	target_link_libraries(nbdtool "kmod")
#	install(TARGETS ${PROGRAM_NAME} RUNTIME DESTINATION bin)
	install(TARGETS nbdtool RUNTIME
		DESTINATION bin
		PERMISSIONS 	OWNER_READ OWNER_WRITE OWNER_EXECUTE
				GROUP_READ GROUP_EXECUTE
				WORLD_READ WORLD_EXECUTE
				SETUID SETGID
	)
else()
	message(WARNING "-- kmod not found, nbdtool will not be built")
endif()
