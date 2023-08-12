cmake_minimum_required(VERSION 3.20.1)

project(ChatClient VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# include current dir
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Network gui core  ) # opengl

# Proto file
# Generated sources
set(proto_srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/protoc/message.pb.cc")
set(proto_hdrs "${CMAKE_CURRENT_SOURCE_DIR}/src/protoc/message.pb.h")
add_custom_command(
    OUTPUT "${proto_srcs}" "${proto_hdrs}"
    COMMAND protoc
    ARGS  --cpp_out "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/src/protoc/message.proto"
        -I "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/protoc/message.proto"
)

######################Protobuf related ############################
set(Protobuf_INCLUDE_DIRS "D:/Software/install/protobuf3.20.0Debug/include")
set(Protobuf_LINK_DIRS "D:/Software/install/protobuf3.20.0Debug/lib/")
set(Protobuf_LIBRARIES libprotobufd.lib ) #libprotobufd
# FIXME use target_include_directories and target_link_directories instead
include_directories(${Protobuf_INCLUDE_DIRS})
link_directories(${Protobuf_LINK_DIRS})
# message("Protobuf_INCLUDE_DIRS: " ${Protobuf_INCLUDE_DIRS})
# message("Protobuf_INCLUDE_DIRS: " ${Protobuf_LIBRARIES})
######################!Protobuf related! ############################

######################OpenCV related ############################
find_package(OpenCV  REQUIRED
             PATHS "D:/Software/install/opencv/build"  # do not use chinese characters in path
             NO_DEFAULT_PATH )
include_directories(${OpenCV_inCLUDE_DIRS})
message("\nOpenCV_INCLUDE_DIRS: " ${OpenCV_INCLUDE_DIRS} "\n")
message("OpenCV_LIBS:+++++++++++ " ${OpenCV_LIBRARYS})
######################!OpenCV related! ############################


######################  ffmpeg related ############################
set(ffmpeg_INCLUDE_DIRS "D:/Software/install/ffmpeg6.0/include")
set(ffmpeg_LINK_DIRS "D:/Software/install/ffmpeg6.0/lib/")
set(ffmpeg_LIBRARIES swscale.lib avformat.lib avfilter.lib avdevice.lib avcodec.lib avutil.lib)


include_directories(${ffmpeg_INCLUDE_DIRS})
link_directories(${ffmpeg_LINK_DIRS})
######################! ffmpeg related ! ############################

####################### PortAudio  #############################
set(PortAudio_INCLUDE_DIRS "D:/Software/install/portaudio/include")
set(PortAudio_LINK_DIRS "D:/Software/install/portaudio/lib/")
set(PortAudio_LIBRARIES portaudio_x64 )

include_directories(${PortAudio_INCLUDE_DIRS})
link_directories(${PortAudio_LINK_DIRS})
#######################! PortAudio !#############################


######################  Opus related ############################
find_package(Opus  REQUIRED
             PATHS "D:/Software/install/opus/lib/cmake/Opus"  # do not use chinese characters in path
             NO_DEFAULT_PATH )
include_directories(${OPUS_INCLUDE_DIRS})

######################! Opus related ! ############################ D:\Software\install\opus-1.4\cmake
set(SRC_BASE
    src/tcpclient.cpp
    src/tcpclient.h
    src/config.cpp
    src/config.h
    src/cameravideo.h
    src/cameravideo.cpp
    src/mediacodec.h
    src/mediacodec.cpp
    src/avmessagemuxer.h
    src/avmessagemuxer.cpp
    src/portaudiox.h
    src/portaudiox.cpp

    src/codec/packet.cpp
    src/codec/packet.h
    src/codec/protobufCodec.cpp
    src/codec/protobufCodec.h
    src/codec/protobuf_fwd.h

    src/protoc/message.pb.h
    src/protoc/message.pb.cc
)

set(SRC_CHAT
    assets/img.qrc

    ui/logindialog.cpp
    ui/logindialog.h
    ui/logindialog.ui

    ui/chatwindow.cpp
    ui/chatwindow.h
    ui/chatwindow.ui
    ui/chatmessage/timemessage.ui
    ui/chatmessage/textmessage.ui
    ui/chatmessage/filemessage.ui

    ui/chatmessage/timemessage.h
    ui/chatmessage/timemessage.cpp
    ui/chatmessage/textmessage.h
    ui/chatmessage/textmessage.cpp
    ui/chatmessage/filemessage.h
    ui/chatmessage/filemessage.cpp
    ui/chatmessage/messagecommon.h

)

add_executable(ChatClient
    main.cpp
    # src/test/test_PortAudioX.cpp
    ${SRC_BASE}
    ${SRC_CHAT}
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
    ${x264_LIBRARIES}
    ${OPUS_LIBRARIES}
    ${ffmpeg_LIBRARIES}
    ${PortAudio_LIBRARIES}
)

set_target_properties(ChatClient PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE FALSE # set TRUE to disable console
)
