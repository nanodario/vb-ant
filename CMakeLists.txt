project(Reti)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)
set(CMAKE_AUTOMOC ON)
SET(QT_USE_QTXML TRUE)
include(${QT_USE_FILE})

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

qt4_wrap_ui(ui_src MainWindow.ui info_dialog.ui)

set(Reti_SRCS main.cpp MainWindow.cpp InfoDialog.cpp VMTabSettings.cpp IfacesTable.cpp Iface.cpp)
qt4_automoc(${Reti_SRCS})
add_executable(Reti ${Reti_SRCS} ${ui_src})
target_link_libraries(Reti ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})
install(TARGETS Reti RUNTIME DESTINATION bin)
