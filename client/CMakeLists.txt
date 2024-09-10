cmake_minimum_required(VERSION 3.5)
project(client LANGUAGES CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



#查找和配置外部依赖库或工具
find_package(Qt5 COMPONENTS Widgets Network REQUIRED)

# 添加资源文件
qt5_add_resources(RESOURCE_FILES resources.qrc)

# 设置可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(client
  main.cpp
  mainwindow.cpp
  logindialog.cpp
  registdialog.cpp
  global.cpp
  httpmgr.cpp

  mainwindow.h
  logindialog.h
  registdialog.h
  global.h
  singleton.h
  httpmgr.h

  mainwindow.ui
  logindialog.ui
  registdialog.ui
  ${RESOURCE_FILES} # 添加资源文件
)

# 拷贝 config.ini 到 bin 目录
add_custom_command(
    TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/config.ini ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Copying config.ini to output directory"
)

# 将 config.ini 文件显式添加到项目中（仅为了 IDE 显示）
add_custom_target(config_files SOURCES ${CMAKE_SOURCE_DIR}/config.ini)

# 指定目标链接的库
target_link_libraries(client PRIVATE Qt5::Widgets Qt5::Network)
