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
  timerbtn.cpp
  clickedlabel.cpp
  resetdialog.cpp
  tcpmgr.cpp
  usermgr.cpp
  chatdialog.cpp
  clickedbtn.cpp
  customizeedit.cpp
  chatuserlist.cpp
  chatuserwid.cpp
  ListItemBase.cpp
  loadingdlg.cpp
  chatpage.cpp
  chatview.cpp
  chatitembase.cpp
  bubbleframe.cpp
  textbubble.cpp
  picturebubble.cpp
  messagetextedit.cpp
  statewidget.cpp
  searchlist.cpp
  userdata.cpp
  adduseritem.cpp
  findsuccessdlg.cpp
  applyfriend.cpp
  clickedoncelabel.cpp
  friendlabel.cpp
  contactuserlist.cpp
  conuseritem.cpp
  grouptipitem.cpp
  applyfriendpage.cpp
  applyfrienditem.cpp
  applyfriendlist.cpp

  mainwindow.h
  logindialog.h
  registdialog.h
  global.h
  singleton.h
  httpmgr.h
  timerbtn.h
  clickedlabel.h
  resetdialog.h
  tcpmgr.h
  usermgr.h
  chatdialog.h
  clickedbtn.h
  customizeedit.h
  chatuserlist.h
  chatuserwid.h
  ListItemBase.h
  loadingdlg.h
  chatpage.h
  chatview.h
  chatitembase.h
  bubbleframe.h
  textbubble.h
  picturebubble.h
  messagetextedit.h
  statewidget.h
  searchlist.h
  userdata.h
  adduseritem.h
  findsuccessdlg.h
  applyfriend.h
  clickedoncelabel.h
  friendlabel.h
  contactuserlist.h
  conuseritem.h
  grouptipitem.h
  applyfriendpage.h
  applyfrienditem.h
  applyfriendlist.h

  mainwindow.ui
  logindialog.ui
  registdialog.ui
  resetdialog.ui
  chatdialog.ui
  chatuserwid.ui
  loadingdlg.ui
  chatpage.ui
  adduseritem.ui
  findsuccessdlg.ui
  applyfriend.ui
  friendlabel.ui
  conuseritem.ui
  grouptipitem.ui
  applyfriendpage.ui
  applyfrienditem.ui
  ${RESOURCE_FILES} # 添加资源文件
)

# 拷贝 config.ini 到 bin 目录
add_custom_command(
    TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/config.ini ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
# 创建一个自定义目标来确保每次都复制 static 文件夹
add_custom_target(copy_static ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/static ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/static
)
add_dependencies(client copy_static)


# 将 config.ini 文件显式添加到项目中（仅为了 IDE 显示）
add_custom_target(config_files SOURCES ${CMAKE_SOURCE_DIR}/config.ini)
# 指定目标链接的库
target_link_libraries(client PRIVATE Qt5::Widgets Qt5::Network)

