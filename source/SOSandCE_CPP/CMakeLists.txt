cmake_minimum_required(VERSION 3.16)
project(sosandce LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Helper to find SDL2 components using either CMake or PkgConfig
find_package(SDL2 REQUIRED)

# For the satellite libraries, we try find_package first, 
# then fallback to PkgConfig (common on Linux)
macro(find_sdl2_component MODULE_NAME TARGET_NAME)
    find_package(${MODULE_NAME} QUIET)
    if(NOT ${MODULE_NAME}_FOUND)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(${MODULE_NAME} REQUIRED IMPORTED_TARGET ${MODULE_NAME})
        add_library(${TARGET_NAME} INTERFACE IMPORTED)
        target_link_libraries(${TARGET_NAME} INTERFACE PkgConfig::${MODULE_NAME})
    endif()
endmacro()

find_sdl2_component(SDL2_image SDL2_image::SDL2_image)
find_sdl2_component(SDL2_mixer SDL2_mixer::SDL2_mixer)
find_sdl2_component(SDL2_ttf SDL2_ttf::SDL2_ttf)
find_package(RmlUi REQUIRED)

# --- Rest of your source setup ---
file(GLOB_RECURSE GAME_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

set(IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui_impl_sdl2.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui_impl_sdlrenderer2.cpp
)

if (WIN32)
    set(APP_ICON_RESOURCE ${CMAKE_SOURCE_DIR}/resources/windows_app.rc)
    add_executable(sosandce WIN32 ${GAME_SOURCES} ${IMGUI_SOURCES} ${APP_ICON_RESOURCE})
else()
    add_executable(sosandce ${GAME_SOURCES} ${IMGUI_SOURCES})
endif()

target_include_directories(sosandce PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
)

# --- Linking ---
target_link_libraries(sosandce PRIVATE
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
    SDL2_ttf::SDL2_ttf
    RmlUi::RmlUi
)

if(WIN32 AND TARGET SDL2::SDL2main)
    target_link_libraries(sosandce PRIVATE SDL2::SDL2main)
endif()

add_custom_command(TARGET sosandce POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying UI asset pack"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets/ui_pack"
        "$<TARGET_FILE_DIR:sosandce>/assets/ui_pack"
)
