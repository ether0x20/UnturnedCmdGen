cmake_minimum_required(VERSION 3.16)
project(unturnedCmdGen VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

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
    src/util/JsonHelper.h
    src/util/Translator.h
)

add_executable(unturnedCmdGen ${SOURCES} ${HEADERS})

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
