project(Reti)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)

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

qt4_wrap_ui(ui_src MainWindow.ui info_dialog.ui)

qt4_add_resources(ui_res res.qrc)

set(Reti_SRCS
	Iface.cpp
	IfacesTable.cpp
	InfoDialog.cpp
	main.cpp
	MainWindow.cpp
	OSBridge.cpp
	VirtualBoxBridge.cpp
	VirtualMachine.cpp
	VMSettings.cpp
	VMTabSettings.cpp
	UIMainEventListener.cpp
)

set(nbdtool_SRCS nbdtool.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVBOX_WITH_XPCOM_NAMESPACE_CLEANUP")

if($ENV{PROGRAM_NAME})
	set(PROGRAM_NAME ${PROGRAM_NAME})
else()
	set(PROGRAM_NAME "vb-ant")
endif()

message("-- Program name: ${PROGRAM_NAME}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPROGRAM_NAME=\"\\\"${PROGRAM_NAME}\\\"\"")

if($ENV{OS_PARTITION_NUMBER})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOS_PARTITION_NUMBER=$ENV{OS_PARTITION_NUMBER}")
	message("-- OS partition number: $ENV{OS_PARTITION_NUMBER}")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOS_PARTITION_NUMBER=1")
	message("-- OS partition number: 1")
endif()

if($ENV{DEBUG_FLAG})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG_FLAG")
        message("-- Debug output: enabled")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -UDEBUG_FLAG")
        message("-- Debug output: disabled")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fshort-wchar")
set(SYSTEM_LIBS
	"pthread"
	"kmod"
)

if($ENV{CONFIGURABLE_IP})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCONFIGURABLE_IP")
	message("-- Enabled IP configuration")
else()
	message("-- Disabled IP configuration")
endif()

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

target_link_libraries(nbdtool
			"kmod")
install(TARGETS ${PROGRAM_NAME} RUNTIME DESTINATION bin)
install(TARGETS nbdtool RUNTIME DESTINATION bin)
