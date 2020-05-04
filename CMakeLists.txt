cmake_minimum_required(VERSION 2.8.11)
project (toxsaveparser)

find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Concurrent REQUIRED)

add_executable(${PROJECT_NAME} main.cpp utils.cpp sections.cpp nodeinfo.cpp)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)

target_link_libraries (${PROJECT_NAME} Qt5::Core Qt5::Concurrent)
