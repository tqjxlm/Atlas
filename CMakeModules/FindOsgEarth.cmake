# This module defines

# OSGEARTH_LIBRARY
# OSGEARTH_FOUND, if false, do not try to link to osg
# OSGEARTH_INCLUDE_DIRS, where to find the headers
# OSGEARTH_INCLUDE_DIR, where to find the source headers

# to use this module, set variables to point to the osg build
# directory, and source directory, respectively
# OSGEARTHDIR or OSGEARTH_SOURCE_DIR: osg source directory, typically OpenSceneGraph
# OSGEARTH_DIR or OSGEARTH_BUILD_DIR: osg build directory, place in which you've
#    built osg via cmake

# Header files are presumed to be included like
# #include <osg/PositionAttitudeTransform>
# #include <osgUtil/SceneView>

###### headers ######

macro( FIND_OSGEARTH_INCLUDE THIS_OSGEARTH_INCLUDE_DIR THIS_OSGEARTH_INCLUDE_FILE )

find_path( ${THIS_OSGEARTH_INCLUDE_DIR} ${THIS_OSGEARTH_INCLUDE_FILE}
    PATHS
        ${OSGEARTH_DIR}
        $ENV{OSGEARTH_SOURCE_DIR}
        $ENV{OSGEARTHDIR}
        $ENV{OSGEARTH_DIR}
        /usr/local/
        /usr/
        /sw/ # Fink
        /opt/local/ # DarwinPorts
        /opt/csw/ # Blastwave
        /opt/
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OSGEARTH_ROOT]/
        ~/Library/Frameworks
        /Library/Frameworks
    PATH_SUFFIXES
        /include/
)

endmacro( FIND_OSGEARTH_INCLUDE THIS_OSGEARTH_INCLUDE_DIR THIS_OSGEARTH_INCLUDE_FILE )

FIND_OSGEARTH_INCLUDE( OSGEARTH_INCLUDE_DIR       osgEarth/Version )

###### libraries ######

macro( FIND_OSGEARTH_LIBRARY MYLIBRARY MYLIBRARYNAME )

    find_library(${MYLIBRARY}_LIBRARY_RELEASE
        NAMES
            ${MYLIBRARYNAME}
        PATHS
            ${OSGEARTH_DIR}
            $ENV{OSGEARTH_BUILD_DIR}
            $ENV{OSGEARTH_DIR}
            $ENV{OSGEARTHDIR}
            $ENV{OSG_ROOT}
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local
            /usr
            /sw
            /opt/local
            /opt/csw
            /opt
            [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OSGEARTH_ROOT]/lib
            /usr/freeware
        PATH_SUFFIXES
            /lib/
            /lib64/
            /build/lib/
            /build/lib64/
            /Build/lib/
            /Build/lib64/
        )

    find_library(${MYLIBRARY}_LIBRARY_DEBUG
        NAMES
            ${MYLIBRARYNAME}d
        PATHS
            ${OSGEARTH_DIR}
            $ENV{OSGEARTH_BUILD_DIR}
            $ENV{OSGEARTH_DIR}
            $ENV{OSGEARTHDIR}
            $ENV{OSG_ROOT}
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local
            /usr
            /sw
            /opt/local
            /opt/csw
            /opt
            [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OSGEARTH_ROOT]/lib
            /usr/freeware
        PATH_SUFFIXES
            /lib/
            /lib64/
            /build/lib/
            /build/lib64/
            /Build/lib/
            /Build/lib64/
        )

    select_library_configurations(${MYLIBRARY})

endmacro(FIND_OSGEARTH_LIBRARY LIBRARY LIBRARYNAME)

FIND_OSGEARTH_LIBRARY( OSGEARTH osgEarth)
FIND_OSGEARTH_LIBRARY( OSGEARTHFEATURES osgEarthFeatures)
FIND_OSGEARTH_LIBRARY( OSGEARTHUTIL osgEarthUtil )
FIND_OSGEARTH_LIBRARY( OSGEARTHQT osgEarthQt )
FIND_OSGEARTH_LIBRARY( OSGEARTHSYMBOLOGY osgEarthSymbology )
FIND_OSGEARTH_LIBRARY( OSGEARTHANNOTATION osgEarthAnnotation )

set( OSGEARTH_FOUND "NO" )
if( OSGEARTH_LIBRARY AND OSGEARTH_INCLUDE_DIR )
    set( OSGEARTH_FOUND "YES" )
    set( OSGEARTH_INCLUDE_DIRS ${OSGEARTH_INCLUDE_DIR})
    # get_filename_component( OSGEARTH_LIBRARIES_DIR ${OSGEARTH_LIBRARY} DIRECTORY )
endif( OSGEARTH_LIBRARY AND OSGEARTH_INCLUDE_DIR )
