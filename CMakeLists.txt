cmake_minimum_required(VERSION 3.28)

project(garuda-system-maintenance VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check https://doc.qt.io/qt/deployment-android.html for more information.
# They need to be set before the find_package( ...) calls below.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(QT NAMES Qt6 COMPONENTS)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Network LinguistTools REQUIRED)
find_package(KF6Notifications)
find_package(KF6StatusNotifierItem)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES en am ar bg ca cs da de el es et eu fa fi fil_PH fr he_IL hi hr hu id is it ja kk ko lt mk mr nb nl pl pt pt_BR ro ru sk sl sq sr sv sw tr uk zh_CN zh_TW)

set(PROJECT_SOURCES
        main.cpp
        tray.cpp
        tray.h
        tray.ui
        settingsdialog.cpp
        settingsdialog.h
        settingsdialog.ui
        baseagent.h
        agentmanager.cpp
        agentmanager.h
        forumagent.cpp
        forumagent.h
        packageagent.cpp
        packageagent.h
        updateagent.cpp
        updateagent.h
        snapshotagent.cpp
        snapshotagent.h
        ${TS_FILES}
)
list(TRANSFORM PROJECT_SOURCES PREPEND "src/")

qt_add_translations(garuda-system-maintenance
    TARGETS garuda-system-maintenance
    LUPDATE_OPTIONS "-source-language en"
    TS_OUTPUT_DIRECTORY "i18n/"
    LUPDATE_TARGET "lupdate"
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(garuda-system-maintenance
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(garuda-system-maintenance SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(garuda-system-maintenance
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(garuda-system-maintenance PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network KF6::Notifications KF6::StatusNotifierItem)

set_target_properties(garuda-system-maintenance PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(garuda-system-maintenance)
endif()

install(TARGETS garuda-system-maintenance
    RUNTIME DESTINATION bin)
