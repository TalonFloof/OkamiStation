cmake_minimum_required(VERSION 3.15)
project(emberwolf)

find_package(raylib 3.2 REQUIRED) # Requires at least version 3.2

set(CMAKE_C_STANDARD 99) # Requires C99 standard

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lm")

file(GLOB sources src/*.c)
add_executable(${PROJECT_NAME} ${sources})

target_include_directories(${PROJECT_NAME} PRIVATE src/include/)

target_link_libraries(${PROJECT_NAME} raylib)

# Checks if OSX and links appropriate frameworks (only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()

install(TARGETS ${PROJECT_NAME}
        CONFIGURATIONS Debug
        RUNTIME DESTINATION Debug/bin)
install(TARGETS ${PROJECT_NAME}
        CONFIGURATIONS Release
        RUNTIME DESTINATION Release/bin)