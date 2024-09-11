


DISTFILES +=  config.ini：

//在 qmake 项目中，DISTFILES 用来指明非编译文件（如配置文件、文档等），
//确保它们在项目构建或者打包时也会被包含。



add_custom_target(config_files SOURCES config.ini)

//创建一个名为 config_files 的自定义目标
//SOURCES config.ini：这里指定了 config.ini 作为目标的“源文件”。
//add_custom_target，可以将 config.ini 文件显式添加到项目中，并结合其他 CMake 命令来控制该文件的使用或分发。


//#CMAKE_BINARY_DIR就是build目录  而CMAKE_SOURCE_DIR就是cmakelist所在的目录
//#CMAKE_RUNTIME_OUTPUT_DIRECTORY用来指定可执行文件的生成目录