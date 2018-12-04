cmake_minimum_required(VERSION 3.1)

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${DIST_PLUGINS_PATH})

find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)

include_directories(
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ../
)

file(GLOB_RECURSE HFiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE CppFiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE UiFiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.ui)
file(GLOB_RECURSE JsonFiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.json)

set(${PROJECT_NAME}_HDRS
  ${HFiles}
)

set(${PROJECT_NAME}_SRCS
  ${CppFiles}
  ${UiFiles}
)

set(${PROJECT_NAME}_JSONS
  ${JsonFiles}
)

add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}_JSONS} ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HDRS})

target_link_libraries(${PROJECT_NAME}
    DrawSurfaceLine
)
