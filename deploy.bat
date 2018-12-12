@ECHO off

set OUT_DIR=%~1
IF "OUT_DIR"=="" GOTO INVALID_ARGS
IF %OUT_DIR:~-1%==\ SET OUT_DIR=%OUT_DIR:~0,-1%

set REL_DIR=%~2
IF "REL_DIR"=="" GOTO INVALID_ARGS
IF %REL_DIR:~-1%==\ SET REL_DIR=%REL_DIR:~0,-1%

IF NOT "%~3".==. set QTDIR=%~3
IF "%QTDIR%"=="" GOTO INVALID_ARGS

ECHO =================Deploying Atlas...=================
IF NOT "%REL_DIR%"=="%OUT_DIR%" (
    xcopy "%REL_DIR%\*.exe" "%OUT_DIR%\" /Y /D
    xcopy "%REL_DIR%\*.dll" "%OUT_DIR%\" /Y /D
    xcopy "%REL_DIR%\plugins\*.dll" "%OUT_DIR%\plugins" /Y /D /i
)
xcopy ".\Atlas\resources" "%OUT_DIR%\resources" /E /Y /D /i

ECHO =================Deploying Qt...=================
ECHO %QTDIR%
"%QTDIR%\bin\windeployqt.exe" "%OUT_DIR%" -xml -opengl

ECHO =================Copying Third Party DLLs...=================
IF "%THIRD_PARTY_DIR%"=="" GOTO NO_THIRD_PARTY

xcopy "%THIRD_PARTY_DIR%\bin\*.dll" "%OUT_DIR%\" /Y /D

ECHO =================Copying OSG Core DLLs...=================
IF "%OSG_ROOT%"=="" GOTO NO_OSG_ROOT

xcopy "%OSG_ROOT%\bin\osg*-osgQt5.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osg*-osg.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osg*-osgDB.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osg*-osgGA.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osg*-osgManipulator.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osg*-osgParticle.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osg*-osgShadow.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osg*-osgSim.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osg*-osgTerrain.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osg*-osgText.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osg*-osgUtil.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osg*-osgViewer.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osgEarth.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osgEarthAnnotation.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osgEarthFeatures.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osgEarthSplat.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osgEarthUtil.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\osgEarthSymbology.dll" "%OUT_DIR%\" /Y /D
xcopy "%OSG_ROOT%\bin\ot*-OpenThreads.dll" "%OUT_DIR%\" /Y /D

ECHO =================Copying OSG Plugins DLLs...=================
for /f %%i in ('dir %OSG_ROOT%\bin\osgPlugins-* /A:D /B') do set PLUGIN_DIR=%%i
IF "%PLUGIN_DIR%" == "File Not Found" GOTO PLUGIN_NOT_FOUND

dir /b "%OSG_ROOT%\bin\%PLUGIN_DIR%\*d.dll" > "%OSG_ROOT%\bin\%PLUGIN_DIR%\debug_files.txt"
xcopy "%OSG_ROOT%\bin\%PLUGIN_DIR%\*.dll" "%OUT_DIR%\%PLUGIN_DIR%\" /Y /D /EXCLUDE:%OSG_ROOT%\bin\%PLUGIN_DIR%\debug_files.txt
del /f "%OSG_ROOT%\bin\%PLUGIN_DIR%\debug_files.txt"

ECHO =================Deploy OK!=================
GOTO END

:INVALID_ARGS
    ECHO Usage: deploy.bat [output dir] [release dir] [qt dir]
    GOTO END
:PLUGIN_NOT_FOUND
    ECHO ERROR: osgPlugins directory NOT found
    GOTO END
:NO_THIRD_PARTY
    ECHO ERROR: THIRD_PARTY_DIR is NOT defined
    GOTO END
:NO_OSG_ROOT
    ECHO ERROR: OSG_ROOT is NOT defined
    GOTO END
:NO_RSC
    ECHO ERROR: Resource files NOT found
    GOTO END

:END
