cmake_minimum_required(VERSION 3.5)

project(wfjchat LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)




set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Qt5 COMPONENTS Widgets REQUIRED)


# 添加资源文件
qt5_add_resources(RESOURCE_FILES resources.qrc)


add_executable(wfjchat
  main.cpp
  mainwindow.cpp
  logindialog.cpp
  registdialog.cpp
  mainwindow.h
  mainwindow.ui
  logindialog.ui
  registdialog.ui
  ${RESOURCE_FILES} # 添加资源文件
)


target_link_libraries(wfjchat PRIVATE Qt5::Widgets)
