cmake_minimum_required(VERSION 3.1)

project(Atlas)

option(PRELOAD_MAP "Preload default map." OFF)

if (PRELOAD_MAP)
  add_definitions(-DPRELOAD_MAP)
endif()

set(DIST_PATH ${PROJECT_BINARY_DIR}/dist)
set(DIST_PLUGINS_PATH ${DIST_PATH}/plugins)
set(DIST_LIB_PATH ${DIST_PATH}/lib)
set(DIST_RESOURCES_PATH ${DIST_PATH}/resources)

set(CMAKE_MODULE_PATH "${Atlas_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INSTALL_PREFIX, "/usr")
set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DIST_PATH})
set(QT_MIN_VERSION 5.11.0)

find_package(Qt5 COMPONENTS Core Gui Widgets Svg OpenGL Network Xml REQUIRED)
find_package(OpenSceneGraph 3.4.0 REQUIRED osgViewer osgGA osgQt osgDB osgManipulator osgSim osgParticle osgText osgUtil)
find_package(OsgEarth 2.9 REQUIRED osgEarthFeatures osgEarthUtil osgEarthSymbology osgEarthAnnotation)
find_package(FriBiDi)
find_package(GDAL)

include_directories(
    ${PROJECT_BINARY_DIR}
    ${OSG_INCLUDE_DIR}
    ${GDAL_INCLUDE_DIR}
    ${OSGEARTH_INCLUDE_DIRS}
    ${OSGEARTH_INCLUDE_DIR}
    core
)

#file(GLOB_RECURSE HFiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
#file(GLOB_RECURSE CppFiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
#file(GLOB_RECURSE UiFiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui)

set(${PROJECT_NAME}_HDRS
    Atlas/Atlas.h
    Atlas/AtlasSplashScreen.h
    Atlas/resource.h
    core/AtlasMainWindow/atlasmainwindow_global.h
    core/AtlasMainWindow/AtlasMainWindow.h
    core/AtlasMainWindow/NXDockWidget.h
    core/AtlasMainWindow/NXDockWidgetTabBar.h
    core/AtlasMainWindow/NXDockWidgetTabButton.h
    core/AtlasMainWindow/NXDockWidgetTitle.h
    core/SettingsManager/SettingsManager_global.h
    core/SettingsManager/SettingsManager.h
    core/DataManager/FontVisitor.h
    core/DataManager/DataFormats.h
    core/DataManager/DataManager_global.h
    core/DataManager/DataManager.h
    core/DataManager/DataRecord.h
    core/DataManager/DataTree.h
    core/DataManager/FeatureStyleSettingDlg.h
    core/DataManager/FindNode.hpp
    core/MapController/MapController_global.h
    core/MapController/MapController.h
    core/PluginInterface/PluginInterface_global.h
    core/PluginInterface/PluginInterface.h
    core/PluginManager/PluginManager_global.h
    core/PluginManager/PluginManager.h
    core/MousePicker/MousePicker_global.h
    core/MousePicker/MousePicker.h
    core/ViewerWidget/Compass.h
    core/ViewerWidget/ViewerWidget_global.h
    core/ViewerWidget/ViewerWidget.h
    )

set(${PROJECT_NAME}_SRCS
    Atlas/main.cpp
    Atlas/Atlas.cpp
    Atlas/AtlasSplashScreen.cpp
    core/AtlasMainWindow/AtlasMainWindow.cpp
    core/AtlasMainWindow/NXDockWidget.cpp
    core/AtlasMainWindow/NXDockWidgetTabBar.cpp
    core/AtlasMainWindow/NXDockWidgetTabButton.cpp
    core/AtlasMainWindow/NXDockWidgetTitle.cpp
    core/SettingsManager/SettingsManager.cpp
    core/DataManager/FontVisitor.cpp
    core/DataManager/DataManager.cpp
    core/DataManager/DataRecord.cpp
    core/DataManager/DataTree.cpp
    core/DataManager/FeatureStyleSettingDlg.cpp
    core/MapController/MapController.cpp
    core/PluginInterface/PluginInterface.cpp
    core/PluginManager/PluginManager.cpp
    core/MousePicker/MousePicker.cpp
    core/ViewerWidget/Compass.cpp
    core/ViewerWidget/ViewerWidget.cpp

#    core/AtlasMainWindow/AtlasMainWindow.ui
#    core/DataManager/StyleTab.ui
#    core/DataManager/FeatureStyleSettingDlg.ui

    # Resources:
    resource.qrc
)

set (${PROJECT_NAME}_UIS
    core/AtlasMainWindow/AtlasMainWindow.ui
    core/DataManager/StyleTab.ui
    core/DataManager/FeatureStyleSettingDlg.ui
    )

#QT5_ADD_RESOURCES(RSCS ${enzarmonitor_RSCS})
QT5_WRAP_UI(UIS ${${PROJECT_NAME}_UIS})

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HDRS} ${UIS})

target_link_libraries(${PROJECT_NAME}
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
)

# Defining plugins.
function(DefinePlugin pluginName)
  message("Defining plugin: ${pluginName}")
  option (PLUGIN_${pluginName} "Use ${pluginName}")
  if (PLUGIN_${pluginName})
    add_subdirectory(plugins/${pluginName})
    include_directories(plugins/${pluginName}/include)
    string(TOUPPER ${pluginName} upperName)
    add_definitions(-D${upperName})
  endif ()
endfunction()

macro(SUBDIRLIST result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro()

SUBDIRLIST(PLUGINS ${CMAKE_CURRENT_SOURCE_DIR}/plugins)

foreach(p ${PLUGINS})
  DefinePlugin(${p})
endforeach()

# Cleaning all previously built plugins.
file(REMOVE_RECURSE ${DIST_PLUGINS_PATH})

# Copying resources.
file(GLOB resdata resources/*)
file(COPY ${resdata} DESTINATION ${DIST_RESOURCES_PATH})


add_subdirectory(libs)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
