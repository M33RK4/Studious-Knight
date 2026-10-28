cmake_minimum_required(VERSION 3.16)
project(StudiousKnight CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# raylib 5.5 gets downloaded from GitHub on the first build
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(StudiousKnight
    src/main.cpp
    src/tekst.cpp
    src/pitanja.cpp
    src/statistika.cpp
    src/ulaz.cpp
    src/grafika.cpp
    src/zvuk.cpp
)
target_link_libraries(StudiousKnight PRIVATE raylib)

if(WIN32)
    target_sources(StudiousKnight PRIVATE assets/ikona.rc)       # ikona .exe datoteke
endif()

if(MSVC)
    target_compile_options(StudiousKnight PRIVATE /utf-8)          # bosanska slova u kodu
    set_target_properties(StudiousKnight PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(StudiousKnight PRIVATE /ENTRY:mainCRTStartup)
elseif(WIN32)
    set_target_properties(StudiousKnight PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# copies the pitanja folder (one file per level) next to the .exe
add_custom_command(TARGET StudiousKnight POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/pitanja $<TARGET_FILE_DIR:StudiousKnight>/pitanja)
