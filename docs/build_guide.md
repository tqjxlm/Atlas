# Build Guide

## Install Tools

The following environment has been tested. If you build successfully with other versions of any of these environments, or get in to problems, please inform us.

1. Visual Studio 2015 update 3
2. [Qt 5.11](https://download.qt.io/official_releases/qt/5.11/) and above (Visual Studio 2015 x64 verison) with [qt-vsaddin](https://download.qt.io/official_releases/vsaddin/)
3. CMake 3.6 and above

## Ready-to-use dev pack

Prebuilt pack with full plugins (only for __VS2015 + Win10 + Qt5.11.x_x64__):

* OpenSceneGraph.rar ([Google](https://drive.google.com/open?id=1WDazxtmwctlUJS7kPhkJQ3aO-6tyIVG6) / [Baidu](https://pan.baidu.com/s/17LA7XGeUsZpzTZdLmpRWNw))
* thirdparty.rar ([Google](https://drive.google.com/open?id=1_mEG45tPvaDolTiJlHWhsCWOfC2M_MVK) / [Baidu](https://pan.baidu.com/s/17LA7XGeUsZpzTZdLmpRWNw))

Extract them to separate dirs, and add them as *__OSG_ROOT__* and *__THIRD_PARTY_DIR__* to system environment variables, respectively. Then you can jump to [Build Atlas](#build-atlas) directly.

If there's no special needs, it is recommended to install VS2015 + Qt5.11.x_x64 and use our prebuilt pack to save your life.

If your want to prepare by yourself or use a different version of MSVC, read on. The whole process will cost half to a whole day, depending on your hardware and your familiarity of the tools.

## Prepare Third Party Libs

Dependency list (indirect or redundant dependencies not listed):

* gdal
* libjpeg
* libpng
* libtiff
* libgeotiff
* giflib
* freetype
* optional: geos
* optional: sqlite3
* optional: openvr
* optional: ffmpeg
* optional: liblas (requiring boost)
* optional: python3-gdal
* optional: [FBX](https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2019-0) (it required 1GB of space to install)

### Using prebuilt pack

For Visual Studio 2015, we provide a thirdparty pack ([Google](https://drive.google.com/open?id=1_mEG45tPvaDolTiJlHWhsCWOfC2M_MVK) / [Baidu](https://pan.baidu.com/s/17LA7XGeUsZpzTZdLmpRWNw)) with full dependencies.
For other version of Visual Studio, refer to [OSG dependencies](http://www.openscenegraph.org/index.php/download-section/dependencies). You may need to build some additional libs to support optional functions.

If any part of the prebuilt pack does not work, you have to prepare dependencies by yourself. The two great tools are __osgeo4w__ (for prebuilt but maybe outdated libs) and __vcpkg__ (for automatic compiling from source using native tools).

__Important__: No matter how you prepare the dependencies, please copy all your dependencies to a separate folder, and add it as *__THIRD_PARTY_DIR__* to system environment variables. It is ugly but it is the only way, until we finish CMake integration.

### Using osgeo4w

1. Download [osgeo4w x64 version](https://trac.osgeo.org/osgeo4w/), and open the installer
2. Choose advance installation -> Install from Internet
3. Choose a proper position as installation directory (it may take some storage)
4. Next all the way (If you are behind GFW, VPN may be required instead of direct connections)
5. At the Select Packages page, search and select desired packages to install by typing key words in the search bar
6. Find the package you want to install and click on the "Skip" text until a desired version number is shown.
7. Confirm and wait for installation

Whenever you find a missing lib in the following sections, you can always come back and do 1-7 again, picking up what you have left.

After installing, copy the /bin /include /lib dirs to [THIRD_PARTY_DIR]

### Using vcpkg

For any missing package, use [vcpkg](https://github.com/Microsoft/vcpkg) to compile them from source. Remember to compile the x64 version. For example

``` batch
.\vcpkg install gdal:x64-windows
```

After packages are installed, you can find them at *VCPKG_ROOT*\installed\x64-windows. copy the /bin /include /lib dirs to [THIRD_PARTY_DIR]

## Build OSG

The build process may cost a long time. Put the folder on an SSD may help.

1. Get OpenSceneGraph [source code](https://github.com/openscenegraph/OpenSceneGraph/releases), extract it to an arbitrary folder (referred to as *OSG_SRC* in the following)
2. Open CMake, choose the source position as [*OSG_SRC*], and the build position as [*OSG_SRC*]/build
3. Choose Grouped and Advanced, click Configure, choose Visual Studio 2015 x64, confirm
4. Search and set ACTUAL_3RDPARTY_DIR as [*THIRD_PARTY_DIR*]
5. Click Configure again
6. For every "Could Not find" warning, check if the missing lib is required. If so, go to the corresponding section in CMake and specify it manually, or install it if you haven't. (Don't mind those "Debug version not found" warnings. Don't mind when the lib is not listed in the previous section)
7. If you have any other need of dependency, just get them by yourself and specify the path in CMake
8. Configure until not error is shown (leave alone warnings)
9. Optional: search and choose WIN32_USE_MP, BUILD_OSG_EXAMPLES
10. Search and set CMAKE_INSTALL_PREFIX as your target library path. It should be __seperated__ from [*OSG_SRC*]
11. Configure & Generate. Open the generated solution (OpenSceneGraph.sln)
12. Build ALL_BUILD and INSTALL projects for both Debug and Release
13. Add the output directory as *__OSG_ROOT__* in your system environment variables

## Build OSGQt

1. Download OSGQt [source code](https://github.com/openscenegraph/osgQt/releases), choose the newest release even though it is named "Qt4"
2. Open CMake, set source dir as the OSGQt extracted folder, set build dir as /build folder under OSGQt (it may not exist), click Configure
3. Set OSG_INCLUDE_DIR to [*OSG_ROOT*]/include. click Configure
4. Set QT5_DIR to [*QT_DIR*]/msvc2015_64/lib/cmake/Qt5. click Configure
5. Set CMAKE_INSTALL_PREFIX to [*OSG_ROOT*]. Set EXECUTABLE_OUTPUT_PATH to [*OSG_ROOT*]/bin. Set LIBRARY_OUTPUT_PATH to [*OSG_ROOT*]/lib
6. Configure & Generate. Open the generated solution (OSGQt.sln)
7. Remove optimized.lib and debug.lib from the link input of the Release version project OsgQt
8. Build ALL_BUILD and INSTALL projects for Debug ad Release

## Build OSGEarth

1. Download OSGEarth [source code](https://github.com/gwaldron/osgearth/releases). Extract. And use CMake to configure it just as the previous sections
2. Set OSG_DIR to [*OSG_ROOT*]
3. Set THIRD_PARTY_DIR to [*THIRD_PARTY_DIR*]
4. Configure. And set other dependencies that are not detected automatically. The missing ones are:
    * OsgQt
    * GDAL
    * zlib
    * sqlite3
    * geos
5. Choose INSTALL_TO_OSG_DIR, OSGEARTH_QT_BUILD. You may have to specify some Qt directories (found in [*QTDIR*]/msvc2015_64/lib/cmake)
6. Set CMAKE_INSTALL_PREFIX to *OSG_ROOT*
7. Optional: choose WIN32_USE_MP, BUILD_OSGEARTH_EXAMPLES
8. Configure & Generate. Open generated solution
9. Build ALL_BUILD and INSTALL projects for Debug and Release

## Build Atlas

1. Clone this project
2. Make sure you have added *__OSG_ROOT__* and *__THIRD_PARTY_DIR__* to system environment variables
3. Download __resources pack__ ([Google](https://drive.google.com/open?id=1TQhXl1HS5B7jR9sQ9bpaye_PVkqjkxda) / [Baidu](https://pan.baidu.com/s/17LA7XGeUsZpzTZdLmpRWNw)) and extract to [ProjectRoot]/Atlas/resources
4. Open [ProjectRoot]/Atlas.sln
5. Build ALL_BUILD project, or simply build the solution

## Run program

### Debug

1. Open Atlas project properties
2. Add \$(*OSG_ROOT*)\bin and \$(*THIRD_PARTY_DIR*)\bin to Debugging -> Environment
3. Press F5 to start debugging

### Deploy

In cmd, run

``` batch
[ProjectRoot]/deploy.bat [output dir] [release dir] [qt dir]
```

For example:

``` batch
.\deploy.bat .\deploy .\x64\Release C:\Qt\Qt5.11.1\5.11.1\msvc2015_64
```

If you have any dependency problem that shows up after deployment (or running Release build), try to find out which dll is missing, and add it to deploy.bat. The batch is quite self-explaining. You can use [depends.exe](http://www.dependencywalker.com/) to help.