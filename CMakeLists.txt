cmake_minimum_required(VERSION 3.10)
project(vk-watchdog)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Crow REQUIRED)
find_package(Threads REQUIRED)
find_package(eCAL REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC Crow::Crow Threads::Threads eCAL::core)

add_custom_command(OUTPUT ws.html
    COMMAND ${CMAKE_COMMAND} -E
    copy ${PROJECT_SOURCE_DIR}/templates/ws.html ${CMAKE_CURRENT_BINARY_DIR}/templates/ws.html
    DEPENDS ${PROJECT_SOURCE_DIR}/templates/ws.html
)
add_custom_target(ws_copy ALL DEPENDS ws.html)