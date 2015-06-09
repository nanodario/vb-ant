project(Reti)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)
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
	Settings.cpp
	VirtualBoxBridge.cpp
	VirtualMachine.cpp
	VMTabSettings.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVBOX_WITH_XPCOM_NAMESPACE_CLEANUP")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fshort-wchar")

set(VBOX_LIB "/usr/lib/virtualbox/VBoxXPCOM.so")

qt4_automoc(${Reti_SRCS})
add_executable(Reti ${Reti_SRCS} ${ui_src} ${ui_res})
target_link_libraries(Reti
			${QT_QTCORE_LIBRARY}
			${QT_QTGUI_LIBRARY}
			${VBOX_LIB}
)
install(TARGETS Reti RUNTIME DESTINATION bin)
