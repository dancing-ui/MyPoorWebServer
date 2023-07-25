cmake_minimum_required(VERSION 3.5)

project(PoorServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS -pthread)

#开启MOC、RCC和UIC，则会在预处理之前先进行MOC、RCC和UIC操作
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)
#set(CMAKE_AUTOUIC ON)

#设置UIC的查找路径，UIC默认会在引用ui头文件的源代码路径下查找ui文件，而我们的ui文件存放在/ui文件夹下
#set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/ui)

#设置可执行文件的输出路径，默认在哪个路径编译则会放在哪个路径，而我们是在/build下编译，要存放在/bin路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

#查找需要的Qt包
#find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

#将各个文件夹下的所有文件打包
FILE(GLOB ${PROJECT_NAME}_SOURCES "./src/*.cpp")
#FILE(GLOB ${PROJECT_NAME}_UI "./ui/*.ui")
#FILE(GLOB ${PROJECT_NAME}_QRC "./qrc/*.qrc")
FILE(GLOB ${PROJECT_NAME}_HEADER "./include/*")

#将/include路径包含到查找头文件路径
include_directories(${PROJECT_SOURCE_DIR}/include)

#直接将上述的所有文件进行编译和内部链接，ui和qrc以及包含Qt扩展语法的文件由于开启了开启MOC、RCC和UIC，因此会被自行处理
#add_executable(${PROJECT_NAME}
#          ${${PROJECT_NAME}_UI}
#          ${${PROJECT_NAME}_SOURCES}
#          ${${PROJECT_NAME}_QRC}
#          ${${PROJECT_NAME}_HEADER}
#          )

#直接将上述的所有文件进行编译和内部链接，仅包含.cpp和.h文件
add_executable(${PROJECT_NAME}
          ${${PROJECT_NAME}_SOURCES}
          ${${PROJECT_NAME}_HEADER}
          )


#和Qt的库进行链接
#target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Gui Qt5::Widgets)

# 把文件引入Qt creator，PoorServer是项目名
install(TARGETS PoorServer
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
