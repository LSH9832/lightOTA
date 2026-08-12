cmake_minimum_required(VERSION 3.10)

project(lightOTA LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3 -fPIC -pthread -w ${CMAKE_CXX_FLAGS}")
set(CMAKE_BUILD_TYPE Release) # or Debug

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/install/apps/lightOTA/bin/${CMAKE_SYSTEM_PROCESSOR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/install/apps/lightOTA/lib/${CMAKE_SYSTEM_PROCESSOR}")

include_directories(
    3rd_party/flaskcpp/include
    include
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/flaskcpp)

add_executable(ota_server
    src/ota_test.cpp
    src/ota_server.cpp
    src/lightota.cpp
)

target_link_libraries(ota_server
    zip
    pthread
    FlaskCpp
    dl
)