cmake_minimum_required(VERSION 3.16)
project(unturnedCmdGen VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# When cross-compiling for Windows on Linux, the Qt6::moc/uic/rcc targets point
# at Windows .exe binaries that cannot run on the host. Point them at the host
# Qt's tools instead (moc output is platform-independent C++).
if(CMAKE_CROSSCOMPILING)
    find_program(HOST_MOC moc
        PATHS "${QT_HOST_PATH}/lib/qt6/libexec" NO_DEFAULT_PATH)
    find_program(HOST_UIC uic
        PATHS "${QT_HOST_PATH}/lib/qt6/libexec" NO_DEFAULT_PATH)
    find_program(HOST_RCC rcc
        PATHS "${QT_HOST_PATH}/lib/qt6/libexec" NO_DEFAULT_PATH)
    if(HOST_MOC)
        set_target_properties(Qt6::moc PROPERTIES IMPORTED_LOCATION "${HOST_MOC}")
        message(STATUS "Cross build: Qt6::moc -> ${HOST_MOC}")
    endif()
    if(HOST_UIC)
        set_target_properties(Qt6::uic PROPERTIES IMPORTED_LOCATION "${HOST_UIC}")
    endif()
    if(HOST_RCC)
        set_target_properties(Qt6::rcc PROPERTIES IMPORTED_LOCATION "${HOST_RCC}")
    endif()
endif()

# --- Test target -----------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 COMPONENTS Test QUIET)
    if(Qt6Test_FOUND)
        set(TEST_SOURCES tests/test_command_gen.cpp)
        set(TEST_HEADERS)
        add_executable(test_command_gen ${TEST_SOURCES} ${TEST_HEADERS}
            src/controller/CommandController.cpp
            src/model/Command.cpp
            src/model/AppData.cpp
            src/util/JsonHelper.cpp
        )
        target_include_directories(test_command_gen PRIVATE src)
        target_link_libraries(test_command_gen PRIVATE Qt6::Core Qt6::Test)
        add_test(NAME command_gen COMMAND test_command_gen
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
endif()

# --- GUI render smoke test (optional) ---------------------------------------
option(BUILD_GUI_TEST "Build the headless GUI render test" OFF)
if(BUILD_GUI_TEST)
    add_executable(test_gui_render
        tests/test_gui_render.cpp
        resources.qrc
        src/model/Command.cpp
        src/model/AppData.cpp
        src/controller/CommandController.cpp
        src/controller/TableController.cpp
        src/controller/AppController.cpp
        src/controller/RconClient.cpp
        src/ui/MainWindow.cpp
        src/ui/CommandListWidget.cpp
        src/ui/ParameterWidget.cpp
        src/ui/OutputWidget.cpp
        src/ui/SettingsDialog.cpp
        src/ui/ImportDialog.cpp
        src/ui/TableManagerDialog.cpp
        src/ui/ModManagerDialog.cpp
        src/util/JsonHelper.cpp
        src/util/Translator.cpp
    )
    target_include_directories(test_gui_render PRIVATE src)
    target_link_libraries(test_gui_render PRIVATE
        Qt6::Core Qt6::Widgets Qt6::Network Qt6::Test)
    add_test(NAME gui_render COMMAND test_gui_render
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()


set(SOURCES
    src/main.cpp
    src/model/Command.cpp
    src/model/AppData.cpp
    src/controller/CommandController.cpp
    src/controller/TableController.cpp
    src/controller/AppController.cpp
    src/controller/RconClient.cpp
    src/ui/MainWindow.cpp
    src/ui/CommandListWidget.cpp
    src/ui/ParameterWidget.cpp
    src/ui/OutputWidget.cpp
    src/ui/SettingsDialog.cpp
    src/ui/ImportDialog.cpp
    src/ui/TableManagerDialog.cpp
    src/ui/ModManagerDialog.cpp
    src/util/JsonHelper.cpp
    src/util/Translator.cpp
)

set(HEADERS
    src/model/Command.h
    src/model/AppData.h
    src/controller/CommandController.h
    src/controller/TableController.h
    src/controller/AppController.h
    src/controller/RconClient.h
    src/ui/MainWindow.h
    src/ui/CommandListWidget.h
    src/ui/ParameterWidget.h
    src/ui/OutputWidget.h
    src/ui/SettingsDialog.h
    src/ui/ImportDialog.h
    src/ui/TableManagerDialog.h
    src/ui/ModManagerDialog.h
    src/model/ModInfo.h
    src/util/JsonHelper.h
    src/util/Translator.h
)

add_executable(unturnedCmdGen ${SOURCES} ${HEADERS}
    resources.qrc)

# The installed binary is "UnturnedCmdGen" (capitalized), matching the desktop
# entry and the package branding.
set_target_properties(unturnedCmdGen PROPERTIES OUTPUT_NAME "UnturnedCmdGen")

target_include_directories(unturnedCmdGen PRIVATE src)

target_link_libraries(unturnedCmdGen PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# Locate assets relative to the executable (build dir copies them next to the binary)
add_custom_command(TARGET unturnedCmdGen POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:unturnedCmdGen>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/translations
        $<TARGET_FILE_DIR:unturnedCmdGen>/translations
    COMMENT "Copying assets and translations next to executable"
)

install(TARGETS unturnedCmdGen DESTINATION bin)
install(DIRECTORY assets/ DESTINATION share/unturnedCmdGen/assets)
install(DIRECTORY translations/ DESTINATION share/unturnedCmdGen/translations)

# Desktop integration (used by the .deb package).
install(FILES packaging/unturnedcmdgen.desktop
        DESTINATION share/applications)
install(FILES packaging/unturnedcmdgen.svg
        DESTINATION share/icons/hicolor/scalable/apps)

# --- Packaging (deb) --------------------------------------------------------
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "unturnedcmdgen")
set(CPACK_PACKAGE_VENDOR "Ethan Zhou")
set(CPACK_PACKAGE_CONTACT "Ethan Zhou <evil.jellyfish@outlook.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Unturned server command generator")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
    "Generates Unturned server commands in terminal or in-game chat format with ease.")
set(CPACK_DEBIAN_PACKAGE_SECTION "games")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/ether0x20/UnturnedCmdGen")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ethan Zhou <evil.jellyfish@outlook.com>")
# Compute Depends automatically from the binary's shared library requirements,
# falling back to the known Qt6 runtime packages.
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libqt6core6t64, libqt6gui6t64, libqt6widgets6t64, libqt6network6t64")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")

include(CPack)
