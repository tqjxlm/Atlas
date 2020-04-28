cmake_minimum_required(VERSION 3.1)

project(Atlas)

# -------------------------------------------------------------
# CMAKE settings
# -------------------------------------------------------------
set(DIST_PATH ${PROJECT_BINARY_DIR}/dist)
set(DIST_PLUGINS_PATH ${DIST_PATH}/plugins)
set(DIST_RESOURCES_PATH ${DIST_PATH}/resources)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INSTALL_PREFIX, "/usr")
set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DIST_PATH})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${DIST_PATH})
set(QT_MIN_VERSION 5.11.0)

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
             Svg
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
find_package(FriBiDi)
find_package(GDAL)
find_package(OsgQt5)

include_directories(
  ${PROJECT_BINARY_DIR} ${OSG_INCLUDE_DIR} ${GDAL_INCLUDE_DIR}
  ${OSGEARTH_INCLUDE_DIRS} ${OSGEARTH_INCLUDE_DIR} ${OSGQT5_INCLUDE_DIR} core)

# -------------------------------------------------------------
# Find files
# -------------------------------------------------------------
set(${PROJECT_NAME}_HDRS
    Atlas/Atlas.h
        core/AtlasMainWindow/AtlasSplashScreen.h
    Atlas/resource.h)

set(${PROJECT_NAME}_SRCS
    Atlas/main.cpp
    Atlas/Atlas.cpp)

set(COMMON_LIBS
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::OpenGL
    Qt5::Network
    Qt5::Xml
    -lpthread
    ${OPENSCENEGRAPH_LIBRARIES}
    ${GDAL_LIBRARY}
    ${OSGEARTH_LIBRARY}
    ${OSGEARTHANNOTATION_LIBRARY}
    ${OSGEARTHFEATURES_LIBRARY}
    ${OSGEARTHUTIL_LIBRARY}
    ${OSGEARTHSYMBOLOGY_LIBRARY}
    ${FRIBIDI_LIBRARY}
        ${OSGQT5_LIBRARY})

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
    string(TOUPPER ${componentName} upperName)
    add_definitions(-D${upperName})
endforeach()

# -------------------------------------------------------------
# Plugins
# -------------------------------------------------------------
function(DefinePlugin pluginName)
  option(PLUGIN_${pluginName} "Use ${pluginName}" OFF)
  if(PLUGIN_${pluginName})
    message("Enable plugin: ${pluginName}")
    add_subdirectory(plugins/${pluginName})
    include_directories(plugins/${pluginName})
    string(TOUPPER ${pluginName} upperName)
    add_definitions(-D${upperName})
  endif()
endfunction()

subdirlist(PLUGINS ${CMAKE_CURRENT_SOURCE_DIR}/plugins)

foreach(p ${PLUGINS})
  defineplugin(${p})
endforeach()

# -------------------------------------------------------------
# Executable and linking
# -------------------------------------------------------------
add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HDRS} resource.qrc)

message("Core components ${CORE_LIBS}")
target_link_libraries(${PROJECT_NAME} ${CORE_LIBS} ${COMMON_LIBS})

# -------------------------------------------------------------
# File management
# -------------------------------------------------------------
file(REMOVE_RECURSE ${DIST_PLUGINS_PATH})
file(MAKE_DIRECTORY ${DIST_RESOURCES_PATH})
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        ${DIST_RESOURCES_PATH})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
