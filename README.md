# Atlas

Atlas is a 3D GIS visualization project that aims to combine some basic features of [QGIS](https://qgis.org/en/site/) and [Google Earth](https://www.google.com/earth/). It utilizes [osgEarth](http://osgearth.org/) to handle 2D maps and 3D models in the same space. It depends on [Qt 5](https://www.qt.io/) to provide an customizable UI and provides a strong plugin system.

The main purpose of the project is to provide fancier and easier visualization of geographic applications, while supporting complicated analysis tasks and more advanced research purposes.

It has been tested on Windows 10, Fedora 28 and ubuntu 16.04.

[中文版说明](docs/README.cn.md)

## Features

### Large Scale 3D Models

Improved performance, interaction, and various analysis and measurement tools for:

* [Oblique Imagery](https://www.eagleview.com/product/pictometry-imagery/oblique-imagery/) model
* dense point cloud
* other models supported by [OSG](http://www.openscenegraph.org/index.php/documentation/user-guides/61-osgplugins)

### Geographic Data

Common geographic data formats and services are supported and fused together in the 3D space.

* local files: see [GDAL raster](https://www.gdal.org/formats_list.html) and [GDAL ogr](https://www.gdal.org/ogr_formats.html)
* raster services: XYZ tiles, TMS, WMS, ArcGIS services
* vector services: WFS, ArcGIS services

### Coherent Data Api

Atlas provides a high level and uniform api for different data formats supported by OSG, osgEarth and custom plugins. For supported data formats please see:

* [OpenSceneGraph](http://www.openscenegraph.org/index.php/documentation/user-guides/61-osgplugins) formats
* [osgEarth](http://docs.osgearth.org/en/latest/data.html) formats
* [plugin extensions](docs/plugins.md)

### Highly Extensible

Atlas is built upon plugins. It is easy to add new features to the main program or modify the existing behaviours with the help of plugins.

A full list of current plugins can be found at [plugins.md](docs/plugins.md).

## Demos

The project is still under development, but a [development release](https://drive.google.com/open?id=10lZnGS43kjEyBusYYlvUROLRjjqIqwvH) is provided so you can have a taste of it.

Geodetic & projected modes

![image not available](https://s1.ax1x.com/2018/12/25/FcBSMt.gif)

![image not available](https://s1.ax1x.com/2018/12/25/FcBns0.gif)

Online images & terrains

![image not available](https://s1.ax1x.com/2018/12/25/FcB9qf.gif)

ShapeFile or Geo-features

![image not available](https://s1.ax1x.com/2018/12/25/FcBAiQ.gif)

Oblique imagery models

![image not available](https://s1.ax1x.com/2018/12/25/FcBFIg.gif)

![image not available](https://s1.ax1x.com/2018/12/25/FcBidS.gif)

Draw & measure

![image not available](https://s1.ax1x.com/2018/12/25/FgEpeU.gif)

Data management

![image not available](https://s1.ax1x.com/2018/12/25/Fc0xxI.gif)

Visibility test

![image not available](https://s1.ax1x.com/2018/12/25/FcBmMq.gif)

Slope grade visualization

![image not available](https://s1.ax1x.com/2018/12/25/FcBZzn.gif)

Plan comparison
(by editing oblique model and insert new models)

![image not available](https://s1.ax1x.com/2018/12/25/FcBpsP.gif)

![image not available](https://s1.ax1x.com/2018/12/25/FcBVRs.gif)

Orthographic generation
(generate orthographic DOM and DSM for oblique model)

![image not available](https://s1.ax1x.com/2018/12/25/FcBEGj.gif)

## Contribute

This project is at a *very early* stage. The core code is far from being mature or stable, and the plugins need testing and maintenance. Also, since the project aims to mimic QGIS and Google Earth, more functionalities (plugins) are urgently and consistently needed.

If you are interested in or skilled at any of the following topics, we are most delightful if you can become a contributor:

* skills: C++, Qt, OSG, OSGEarth, OpenGL
* topics: desktop app, visualization, UI
* research: GIS, CG, CV

### Getting started

You can get started following these steps:

1. try out the [dev release](https://drive.google.com/open?id=10lZnGS43kjEyBusYYlvUROLRjjqIqwvH)
2. [build from source](docs/build_guide.md)
3. read the [project structure](https://www.mindomo.com/mindmap/63290a5a387b4e1a85ad713953be0372)
4. find your interested task on [trello board](https://trello.com/b/Z7r1N9yJ) or put up a new idea
5. fork the project, finish coding and commit a PR

If you want to write a new plugin, see [plugin_guide.md](docs/plugin_guide.md).

### Dev Resources

* [trello board](https://trello.com/b/Z7r1N9yJ)
* [build guide](docs/build_guide.md)
* [plugin guide](docs/plugin_guide.md)
* [project structure](https://www.mindomo.com/mindmap/63290a5a387b4e1a85ad713953be0372)

## Contact

You are welcome to give feedbacks on [issues](../../issues) page, or just contact us via tqjxlm@gmail.com.

We manage the project progress on [trello](https://trello.com/b/Z7r1N9yJ) where you can also request for features and report bugs.
