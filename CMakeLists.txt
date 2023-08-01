cmake_minimum_required(VERSION 3.5)

project(ChatClient VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# include current dir
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

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



###################### some mismatch when invloving MSVC in QT######
##################### ref to :https://blog.csdn.net/qq_42123832/article/details/115347309?spm=1001.2014.3001.5506#
if(MSVC)
    # Use the static C library for all build types
    foreach(var
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
      )
      if(${var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${var} "${${var}}")
      endif()
    endforeach()
endif(MSVC)


set(PROJECT_SOURCES
    main.cpp
    assets/img.qrc

    ui/chatwindow.cpp
    ui/chatwindow.h
    ui/chatwindow.ui
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

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ChatClient
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ChatClient APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(ChatClient SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(ChatClient
            ${PROJECT_SOURCES}
            src/codec/protobuf_fwd.h
        )

    endif()

endif()


target_link_libraries(ChatClient PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    ${Protobuf_LIBRARIES}
    ws2_32 # winsock2
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ChatClient)
endif()
set_target_properties(ChatClient PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ChatClient
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    message("!!!!!!!!!!!!!!!!")
    qt_finalize_executable(ChatClient)
endif()
