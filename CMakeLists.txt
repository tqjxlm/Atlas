cmake_minimum_required(VERSION 3.8)

project(Atlas)

# -------------------------------------------------------------
# CMAKE settings
# -------------------------------------------------------------
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INSTALL_PREFIX, "/usr")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_BUILD_TYPE Release)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)
set(QT_MIN_VERSION 5.11.0)

MARK_AS_ADVANCED(MULTITHREADED_BUILD)
SET(MULTITHREADED_BUILD ON CACHE BOOL "Parallel build with as many threads as possible!")

# -------------------------------------------------------------
# Paths and configurations
# -------------------------------------------------------------
set(DIST_PATH ${PROJECT_BINARY_DIR}/dist)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DIST_PATH})
set(DIST_PLUGINS_PATH ${DIST_PATH}/plugins)
set(DIST_RESOURCES_PATH ${DIST_PATH}/resources)

set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/thirdparty CACHE PATH "The root directory for all third party dependencies.")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${THIRD_PARTY_DIR} ${THIRD_PARTY_DIR}/debug)
set(ENV{THIRD_PARTY_DIR} ${THIRD_PARTY_DIR})

# -------------------------------------------------------------
# Dependencies
# -------------------------------------------------------------
if(APPLE AND EXISTS /usr/local/opt/qt5)
  list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt5")
endif()

find_package(
  Qt5
  COMPONENTS Core
             Gui
             Widgets
             OpenGL
             Network
             Xml
  REQUIRED)
find_package(
  OpenSceneGraph
  3.4.0
  REQUIRED
  osgViewer
  osgGA
  osgDB
  osgManipulator
  osgSim
  osgParticle
  osgText
  osgUtil)
find_package(
  OsgEarth
  2.9
  REQUIRED
  osgEarthFeatures
  osgEarthUtil
  osgEarthSymbology
  osgEarthAnnotation)
find_package(OsgQt5 REQUIRED)
find_package(FriBiDi)
find_package(GDAL)

include_directories(
  ${PROJECT_BINARY_DIR}
  ${OSG_INCLUDE_DIR}
  ${GDAL_INCLUDE_DIR}
  ${OSGEARTH_INCLUDE_DIRS}
  ${OSGEARTH_INCLUDE_DIR}
  ${OSGQT5_INCLUDE_DIR}
  core)

# -------------------------------------------------------------
# Find files
# -------------------------------------------------------------
set(${PROJECT_NAME}_HDRS Atlas/Atlas.h Atlas/resource.h)

set(${PROJECT_NAME}_SRCS Atlas/main.cpp Atlas/Atlas.cpp)

set(COMMON_LIBS
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::OpenGL
    Qt5::Network
    Qt5::Xml
    ${OPENSCENEGRAPH_LIBRARIES}
    ${GDAL_LIBRARY}
    ${OSGEARTH_LIBRARY}
    ${OSGEARTHANNOTATION_LIBRARY}
    ${OSGEARTHFEATURES_LIBRARY}
    ${OSGEARTHUTIL_LIBRARY}
    ${OSGEARTHSYMBOLOGY_LIBRARY}
    ${FRIBIDI_LIBRARY}
    ${OSGQT5_LIBRARY})

if (UNIX)
  set(COMMON_LIBS
    ${COMMON_LIBS}
    -lpthread)
endif (UNIX)

macro(SUBDIRLIST result curdir)
  file(
    GLOB children
    RELATIVE ${curdir}
    ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro()

# -------------------------------------------------------------
# Core components
# -------------------------------------------------------------
subdirlist(COMPONENTS ${CMAKE_CURRENT_SOURCE_DIR}/core)

set(CORE_LIBS "")
foreach(componentName ${COMPONENTS})
  add_subdirectory(core/${componentName})
  include_directories(core/${componentName})
  list(APPEND CORE_LIBS ${componentName})
endforeach()

set(PLUGIN_LIBS PluginInterface MousePicker)

# -------------------------------------------------------------
# Plugins
# -------------------------------------------------------------
function(DefinePlugin pluginName)
  option(PLUGIN_${pluginName} "Use ${pluginName}" ON)
  if(PLUGIN_${pluginName})
    message(STATUS "Enable plugin: ${pluginName}")
    add_subdirectory(plugins/${pluginName})
    include_directories(plugins/${pluginName})
  endif()
endfunction()

subdirlist(PLUGINS ${CMAKE_CURRENT_SOURCE_DIR}/plugins)

foreach(p ${PLUGINS})
  defineplugin(${p})
endforeach()

# -------------------------------------------------------------
# Executable and linking
# -------------------------------------------------------------
add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HDRS}
                               resource.qrc)

target_link_libraries(${PROJECT_NAME} ${CORE_LIBS} ${COMMON_LIBS})

if(MULTITHREADED_BUILD)
    if(MSVC)
      target_compile_options(Atlas PUBLIC "/MP")
    endif()
endif()

# -------------------------------------------------------------
# Deployment
# -------------------------------------------------------------
file(REMOVE_RECURSE ${DIST_PATH})
file(MAKE_DIRECTORY ${DIST_RESOURCES_PATH})
add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources
          ${DIST_RESOURCES_PATH})

if (MSVC)
  set(RELEASE_PATHS ${THIRD_PARTY_DIR}/bin ${THIRD_PARTY_DIR}/tools/osgearth ${THIRD_PARTY_DIR}/tools/osg)
  set(DEBUG_PATHS ${THIRD_PARTY_DIR}/debug/bin ${THIRD_PARTY_DIR}/debug/tools/osgearth ${THIRD_PARTY_DIR}/debug/tools/osg)
  set(ALL_PATH "PATH=%PATH%" ${THIRD_PARTY_DIR}/tools ${RELEASE_PATHS} ${DEBUG_PATHS})
  set_target_properties(Atlas PROPERTIES VS_DEBUGGER_ENVIRONMENT "${ALL_PATH}")

  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${THIRD_PARTY_DIR}/plugins ${DIST_PATH}/Debug)

  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${THIRD_PARTY_DIR}/plugins ${DIST_PATH}/Release)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
