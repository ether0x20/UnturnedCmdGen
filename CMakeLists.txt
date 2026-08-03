cmake_minimum_required(VERSION 3.16)
project(unturnedCmdGen VERSION 1.1.0 LANGUAGES CXX)

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

# --- Windows resource file (application icon + version info) ---------------
# The .rc is generated so the icon path can be made absolute (rc.exe and
# windres resolve resource paths relative to the .rc location otherwise).
set(WINDOWS_RC)
if(WIN32)
    set(RC_VERSION "${PROJECT_VERSION}")
    string(REPLACE "." "," RC_VERSION "${RC_VERSION}")
    set(RC_VERSION "${RC_VERSION},0") # VERSIONINFO needs major,minor,revision,build
    set(WIN_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/icons/unturned.ico")
    string(REPLACE "/" "\\\\" WIN_ICON_PATH "${WIN_ICON_PATH}")
    configure_file(windows/app.rc.in "${CMAKE_CURRENT_BINARY_DIR}/app.rc" @ONLY)
    set(WINDOWS_RC "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
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

add_executable(unturnedCmdGen WIN32 ${SOURCES} ${HEADERS} ${WINDOWS_RC}
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

# Install layout. The app probes for assets/ and translations/ next to the
# executable first, so Windows uses a flat layout under the install root;
# Linux keeps the FHS share/ layout used by the .deb package.
if(WIN32)
    install(TARGETS unturnedCmdGen DESTINATION .)
    install(DIRECTORY assets/ DESTINATION assets)
    install(DIRECTORY translations/ DESTINATION translations)

    # Bundle the Qt runtime next to the installed exe. Runs during the CPack
    # staging install, so the DLLs/plugins land in the same place the installer
    # packages. Skipped when cross-compiling (windeployqt is a Windows tool).
    if(NOT CMAKE_CROSSCOMPILING)
        get_filename_component(_qt_root
            "${Qt6_DIR}/../../.." ABSOLUTE)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt
            HINTS "${_qt_root}/bin" "${_qt_root}/lib/qt6/bin")
        if(WINDEPLOYQT_EXECUTABLE)
            install(CODE "
                message(STATUS \"Running windeployqt on installed executable\")
                execute_process(
                    COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                            --release
                            --no-translations
                            --no-system-d3d-compiler
                            --no-opengl-sw
                            \"\${CMAKE_INSTALL_PREFIX}/UnturnedCmdGen.exe\"
                    RESULT_VARIABLE _wdq_result
                    ERROR_VARIABLE _wdq_err
                )
                if(NOT _wdq_result EQUAL 0)
                    message(WARNING \"windeployqt failed: \${_wdq_err}\")
                endif()
            ")
        else()
            message(WARNING "windeployqt not found; Qt runtime DLLs will not be "
                            "bundled and the installer may not run without Qt.")
        endif()
    endif()
else()
    install(TARGETS unturnedCmdGen DESTINATION bin)
    install(DIRECTORY assets/ DESTINATION share/unturnedCmdGen/assets)
    install(DIRECTORY translations/ DESTINATION share/unturnedCmdGen/translations)

    # Desktop integration (used by the .deb package).
    install(FILES packaging/unturnedcmdgen.desktop
            DESTINATION share/applications)
    install(FILES packaging/unturnedcmdgen.svg
            DESTINATION share/icons/hicolor/scalable/apps)
endif()

# --- Packaging --------------------------------------------------------------
set(CPACK_PACKAGE_NAME "unturnedcmdgen")
set(CPACK_PACKAGE_VENDOR "Ethan Zhou")
set(CPACK_PACKAGE_CONTACT "Ethan Zhou <evil.jellyfish@outlook.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Unturned server command generator")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "UnturnedCmdGen")
    set(CPACK_NSIS_DISPLAY_NAME "Unturned Command Generator")
    set(CPACK_NSIS_PACKAGE_NAME "Unturned Command Generator ${PROJECT_VERSION}")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/unturned.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/unturned.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "UnturnedCmdGen.exe")
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
    set(CPACK_NSIS_HELP_LINK "https://github.com/ether0x20/UnturnedCmdGen")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/ether0x20/UnturnedCmdGen")
    set(CPACK_NSIS_CONTACT "Ethan Zhou <evil.jellyfish@outlook.com>")
    # Manual shortcut creation — CPack 4.4 auto-generates "$INSTDIR\bin\..."
    # and ignores DESTINATION ".", so we override with our own commands.
    set(CPACK_PACKAGE_EXECUTABLES) # disable auto-generated shortcuts
    set(CPACK_CREATE_DESKTOP_LINKS) # also handled manually below
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateDirectory '$SMPROGRAMS/$STARTMENU_FOLDER'
CreateShortCut '$SMPROGRAMS/$STARTMENU_FOLDER/Unturned Command Generator.lnk' '$INSTDIR/UnturnedCmdGen.exe'
StrCmp '$INSTALL_DESKTOP' '1' 0 +2
CreateShortCut '$DESKTOP/Unturned Command Generator.lnk' '$INSTDIR/UnturnedCmdGen.exe'
")
    set(CPACK_PACKAGE_FILE_NAME "unturnedcmdgen-${PROJECT_VERSION}-win64")
    # Version resource for the installer itself. Without these the file
    # properties show Publisher/Product as "(Unknown)".
    set(_nsis_ver_num "${PROJECT_VERSION}.0") # NSIS wants dotted X.X.X.X
    set(CPACK_NSIS_DEFINES "VIProductVersion \\\"${_nsis_ver_num}\\\"
VIAddVersionKey \\\"ProductName\\\" \\\"Unturned Command Generator\\\"
VIAddVersionKey \\\"ProductVersion\\\" \\\"${PROJECT_VERSION}\\\"
VIAddVersionKey \\\"FileDescription\\\" \\\"Unturned server command generator\\\"
VIAddVersionKey \\\"FileVersion\\\" \\\"${PROJECT_VERSION}\\\"
VIAddVersionKey \\\"CompanyName\\\" \\\"Ethan Zhou\\\"
VIAddVersionKey \\\"LegalCopyright\\\" \\\"Copyright (c) Ethan Zhou. MIT License.\\\"
VIAddVersionKey \\\"OriginalFilename\\\" \\\"${CPACK_PACKAGE_FILE_NAME}.exe\\\"")
else()
    set(CPACK_GENERATOR "DEB")
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
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_PACKAGE_FILE_NAME
        "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
endif()

include(CPack)
