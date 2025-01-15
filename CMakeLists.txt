cmake_minimum_required(VERSION 3.16)
project(SpriteEditor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Find the QtWidgets library
find_package(Qt6 REQUIRED COMPONENTS 
    Core
    Gui
    Widgets
)

# Tell CMake to create the SpriteEditor executable
add_executable(SpriteEditor
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/pakreader.cpp
    src/pakreader.h
    src/spritemodel.cpp
    src/spritemodel.h
)

# Link against Qt libraries
target_link_libraries(SpriteEditor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Specify include directories
target_include_directories(SpriteEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(WIN32)
    set_target_properties(SpriteEditor PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif() 