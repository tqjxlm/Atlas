# List of plugins

Here lists all the current plugins that pass the compilation test, including core plugins and third party plugins. We don't make guarantee about the quality of any third party plugin.

According to their usage, plugins are divided into 6 catagories: __Data, Draw, Measure, Effect, Analysis, Edit__.

## Data

Data plugins load and stylize external data.

| Project Name | Description | Maintainer | Remark | Test |
| --- | --- | :---: | :---: | :---: |
| EarthDataInterface | Common interface for other data plugins that load osgEarth layers. | Atlas | core | ✓ |
| AddXYZData | Load image map from tiles services that match standard XYZ url pattern. | Atlas | core | ✓ |
| AddGDALData | Load local files (image, elevation, shape file) using GDAL. | Atlas | core | ✓ |
| AddTMSData | Load image or elevation map from Tile Map Service. | Atlas | core |
| AddOGCData | Load data from WMS, WFS or WTS services. | Atlas | core |
| AddArcGISData | Load data from an ArcGIS server. | Atlas | core |
| AddModel | Load common formats of models from file. | Atlas | core | ✓ |
| AddObliqueModel | Load oblique imagery models organized in *.osgb LOD tiles and annotated by a *.xml file. | tqjxlm |  | ✓ |
| AddPointCloud | Load point cloud data from a *.txt or *.las file. | TJoe | 3rdparty | |

## Draw

Draw plugins provide interactive drawing tools. They often act as base class for other plugins.

| Project Name | Description | Maintainer | Remark | Test |
| --- | --- | :---: | :---: | :---: |
| DrawLine | Draw straight lines on the scene. | Atlas | core | ✓ |
| DrawSurfaceLine | Draw lines with elevations of the underlying terrain, eg. projected straight lines. | Atlas | core | ✓ |
| DrawCircle | Draw filled circles on the scene. | Atlas | core | ✓ |
| DrawPolygon | Draw filled polygons on the scene. | Atlas | core | ✓ |
| DrawSurfacePolygon | Project filled polygons to the underlying terrain. | Atlas | core | ✓ |

## Measure

Measure plugins provide interactive measure tools.

| Project Name | Description | Maintainer | Remark | Test |
| --- | --- | :---: | :---: | :---: |
| MeasureLine | Measure distance over straight lines. | Atlas | core | ✓ |
| MeasureHeight | Measure heights. | Atlas | core | ✓ |
| MeasureArea | Measure area of a 2D polygon. | Atlas | core | |
| MeasureTerrainArea | Measure area of all surfaces in a given region. | Atlas | core | ✓ |
| MeasureTerrainVolume | Measure volume between the model surface and a reference plane in a given region. | Atlas | core | |
| SetRefPlane | Set a reference plane for volume measurements. | Atlas | core | |

## Effect

Effect plugins provide visualization tools or view control.

| Project Name | Description | Maintainer | Remark | Test |
| --- | --- | :---: | :---: | :---: |
| Locator | Navigate between coords by clicking or lat-lon location. | Atlas | core | ✓ |
| ModelPath | Add a model moving in a specified path. | Atlas | core | |
| ShowWeather | Add several weather effect to the scene. | Atlas | core | ✓ |
| MultiView | Open multiple views that share the same scene and camera but show different contents. | Atlas | core | ✓ |
| ScreenShot | Export the current rendered scene as image. | Atlas | core | ✓ |
| PathRoaming | Record a camera moving path and play it for demonstration use. | Atlas | core | |
| MeshMode | Switch render mode between Fill, Line or Point. | tqjxlm | | ✓ |
| TileSelect | Select tiles of oblique imagery models. | TJoe | | ✓ |
| VRMode | Provide 3D view with VR devices supported by OpenVR SDK. | tqjxlm, fwd | 3rdparty | |

## Analysis

Analysis plugins provide various GIS analysis tools.

| Project Name | Description | Maintainer | Remark | Test |
| --- | --- | :---: | :---: | :---: |
| DiffAnalysis | In MultiView mode, find the difference between two views. | Atlas | core | |
| SlopAnalysis | Color the scene according to local slop. | Atlas | core | ✓ |
| VisibilityTest | Visibility test along a line segment. | tqjxlm | | ✓ |
| VisibilityTestArea | Visibility test of one observation spot over an area. | tqjxlm | | ✓ |
| ElevationSlice | Draw a line and generate an elevation curve along it. | TJoe | | |
| InsolationAnalysis | Calculate the insolation rate in a time range of the selected region. | TJoe | | |
| CrowdSimulation | Crowd simulation. | tqjxlm | 3rdparty | |

## Edit

Edit plugins provide tools to edit the scene and generate data products.

| Project Name | Description | Maintainer | Remark | Test |
| --- | --- | :---: | :---: | :---: |
| OrthoMap | Generate an orthographic image or height map of the selected model or region. | tqjxlm | | |
| ModelFlatten | Edit a model and make the selected area flatten to a specified height. | tqjxlm | | |
| ContourPlot | Generate contour plots based on elevation. | TJoe | | |
