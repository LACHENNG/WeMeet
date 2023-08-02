cmake_minimum_required(VERSION 3.20.1)

project(ChatClient VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# include current dir
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Network gui core  ) # opengl


######################Protobuf related ############################
set(Protobuf_INCLUDE_DIRS "D:/Software/install/protobuf3.20.0Debug/include")
set(Protobuf_LINK_DIRS "D:/Software/install/protobuf3.20.0Debug/lib/")
set(Protobuf_LIBRARIES libprotobufd.lib )
# FIXME use target_include_directories and target_link_directories instead
include_directories(${Protobuf_INCLUDE_DIRS})
link_directories(${Protobuf_LINK_DIRS})
# message("Protobuf_INCLUDE_DIRS: " ${Protobuf_INCLUDE_DIRS})
# message("Protobuf_INCLUDE_DIRS: " ${Protobuf_LIBRARIES})
######################!Protobuf related! ############################

######################opencv related ############################
find_package(OpenCV  REQUIRED
             PATHS "D:/Software/install/opencv/build"  # do not use chinese characters in path
             NO_DEFAULT_PATH )
include_directories(${OpenCV_inCLUDE_DIRS})
message("\nOpenCV_INCLUDE_DIRS: " ${OpenCV_INCLUDE_DIRS} "\n")
#message("OpenCV_LIBS: " ${OpenCV_LIBS})
######################!OpenCV related! ############################

set(PROJECT_SOURCES
    main.cpp
    assets/img.qrc

    ui/chatwindow.cpp
    ui/chatwindow.h
    ui/chatwindow.ui
    src/cameravideo.h
    src/cameravideo.cpp
    ui/chatmessage/qnchatmessage.h
    ui/chatmessage/qnchatmessage.cpp

    src/tcpclient.cpp
    src/tcpclient.h
    src/config.cpp
    src/config.h

    src/codec/packet.cpp
    src/codec/packet.h
    src/codec/protobufCodec.cpp
    src/codec/protobufCodec.h
    src/codec/protobuf_fwd.h

    src/protoc/message.pb.h
    src/protoc/message.pb.cc
)

add_executable(ChatClient
    ${PROJECT_SOURCES}
    src/codec/protobuf_fwd.h
)

target_link_libraries(ChatClient PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt::Gui
    Qt::Core
#    Qt::OpenGL
    ${OpenCV_LIBS}
    ${Protobuf_LIBRARIES}
    ws2_32 # winsock2
)

set_target_properties(ChatClient PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
