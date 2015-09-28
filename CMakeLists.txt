project(Reti)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)
cmake_policy(SET CMP0012 NEW)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.2")
		set(STD_REGEX_SUPPORTED FALSE)
	else()
		set(STD_REGEX_SUPPORTED TRUE)
	endif()
endif()

if($ENV{DISABLE_CCACHE})
	message("-- CCache disabled")
else()
	find_program(CCACHE_FOUND ccache)
	if(CCACHE_FOUND)
		message("-- CCache enabled")
		set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
		set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
	else()
		message("-- CCache not found")
	endif(CCACHE_FOUND)
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
)

qt4_add_resources(ui_res "src/res.qrc")

set(Reti_SRCS
	"src/CloneDialog.cpp"
	"src/Iface.cpp"
	"src/IfacesTable.cpp"
	"src/InfoDialog.cpp"
	"src/main.cpp"
	"src/MainWindow.cpp"
	"src/OSBridge.cpp"
	"src/ProgressDialog.cpp"
	"src/VirtualBoxBridge.cpp"
	"src/VirtualMachine.cpp"
	"src/VMSettings.cpp"
	"src/VMTabSettings.cpp"
	"src/UIMainEventListener.cpp"
)

set(nbdtool_SRCS "src/nbdtool.cpp")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVBOX_WITH_XPCOM_NAMESPACE_CLEANUP")

if(DEFINED ENV{PROGRAM_NAME})
	set(PROGRAM_NAME $ENV{PROGRAM_NAME})
else()
	set(PROGRAM_NAME "vb-ant")
endif()

message("-- Program name: ${PROGRAM_NAME}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPROGRAM_NAME=\"\\\"${PROGRAM_NAME}\\\"\"")

if(DEFINED ENV{OS_PARTITION_NUMBER})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOS_PARTITION_NUMBER=\"$ENV{OS_PARTITION_NUMBER}\"")
	message("-- OS partition number: $ENV{OS_PARTITION_NUMBER}")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOS_PARTITION_NUMBER=\"1\"")
	message("-- OS partition number: 1")
endif()

option(DEBUG "Enable debug output" OFF)
if(${DEBUG})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG_FLAG")
        message("-- Debug output: enabled")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -UDEBUG_FLAG")
        message("-- Debug output: disabled")
endif()

option(CONFIGURABLE_IP "Enable IP and subnet mask configuration" OFF)
if(${CONFIGURABLE_IP})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCONFIGURABLE_IP")
	message("-- IPv4 configuration: supported, enabled")
	if(STD_REGEX_SUPPORTED)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_IPv6")
		message("-- IPv6 configuration: supported, enabled")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -UENABLE_IPv6")
		message("-- IPv6 configuration: unsupported, wanted")
	endif()
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -UCONFIGURABLE_IP")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -UENABLE_IPv6")
	message("-- IPv4 configuration: supported, disabled")
	if(STD_REGEX_SUPPORTED)
		message("-- IPv6 configuration: supported, disabled")
	else()
		message("-- IPv6 configuration: unsupported, disabled")
	endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fshort-wchar -std=c++0x")

if(${DEBUG})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

set(SYSTEM_LIBS	"pthread" "kmod")
set(VBOX_LIB "/usr/lib/virtualbox/VBoxXPCOM.so")

qt4_automoc(${Reti_SRCS})

add_executable(${PROGRAM_NAME} ${Reti_SRCS} ${ui_src} ${ui_res})
add_executable(nbdtool ${nbdtool_SRCS})

target_link_libraries(${PROGRAM_NAME}
			${SYSTEM_LIBS}
			${QT_QTCORE_LIBRARY}
			${QT_QTGUI_LIBRARY}
			${VBOX_LIB}
)

target_link_libraries(nbdtool "kmod")
#install(TARGETS ${PROGRAM_NAME} RUNTIME DESTINATION bin)
install(TARGETS nbdtool RUNTIME DESTINATION bin)
