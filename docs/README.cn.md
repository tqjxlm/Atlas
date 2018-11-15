# Atlas

Atlas 是一个三维地图可视化分析项目。Atlas 结合了 [QGIS](https://qgis.org/en/site/) 和 [谷歌地球](https://www.google.com/earth/) 的主要特性，利用 [osgEarth](http://osgearth.org/) 来将二维地图和三维模型展示在同一三维空间内。程序使用 [Qt](https://www.qt.io/) 作为框架，以提供简洁、可定制的 UI，并且实现了一个高扩展性的插件系统。

该项目的主要目的是为地理应用提供更好的可视化解决方案，并在此基础上支持复杂的空间分析、数据处理等，以方便工程应用和学术研究。

该程序在 Windows 10, Fedora 28, Ubuntu 16.04 系统上测试通过。

[English Version](../README.md)

## 主要特性

### 实景三维模型

为大规模三维模型提供渲染性能和交互方面的优化，并实现多种测量和分析工具：

* [倾斜摄影模型](https://www.eagleview.com/product/pictometry-imagery/oblique-imagery/)
* 点云模型
* [OSG](http://www.openscenegraph.org/index.php/documentation/user-guides/61-osgplugins)支持的其他模型格式

### 融合地理数据

支持常见地理数据，并在三维空间中融合渲染：

* 本地数据: GDAL支持的所有[栅格格式](https://www.gdal.org/formats_list.html)和[矢量格式](https://www.gdal.org/ogr_formats.html)
* 在线栅格服务: 通用切片服务, TMS, WMS, WCS, ArcGIS服务
* 在线矢量服务: WFS, ArcGIS服务

### 统一数据接口

为不同数据格式提供统一的高层调用接口，封装数据引擎和驱动的API：

* [osg](http://www.openscenegraph.org/index.php/documentation/user-guides/61-osgplugins) 支持格式
* [osgEarth](http://docs.osgearth.org/en/latest/data.html) 支持格式
* [插件](docs/plugins.md) 扩展格式
  
### 高度可扩展

本程序的大部分功能都由插件实现。使用插件，可以轻易实现新的功能，或是修改现有的功能。

当前所有插件列表：[plugins.md](docs/plugins.md)

## Demo

该项目正在开发中，但我们提供一个 [开发者版本](https://pan.baidu.com/s/17LA7XGeUsZpzTZdLmpRWNw) 以供用户和开发者初步了解它的功能和效果。

地球模式与投影模式

倾斜摄影模型

绘制与测量

数据管理

视域分析

人群仿真

坡度可视化

规划对比（编辑倾斜模型）

正射影像生成（倾斜模型）

## 征集开发者

本项目仍处于*十分早期*的阶段。核心代码还不成熟，目前已有的插件也需要测试和维护。同时，作为一个三维分析软件，我们急切并长期需要开发者和研究人员为程序提供新的功能（插件）。

如果你擅长、或对以下任何话题感兴趣，我们将十分欢迎你成为项目的贡献者：

* 技能：C++，Qt，OSG,，OSGEarth，OpenGL
* 专题：桌面应用，可视化，界面设计，人机交互
* 研究：GIS，CG，CV

### 开发步骤

你可以从以下步骤开始，参与本项目的开发：

1. 试用[开发者版本](https://pan.baidu.com/s/17LA7XGeUsZpzTZdLmpRWNw)
2. [编译源码](docs/build_guide.md)
3. 了解[项目结构](https://www.mindomo.com/mindmap/63290a5a387b4e1a85ad713953be0372)
4. 浏览[任务列表](https://trello.com/b/Z7r1N9yJ)，选择感兴趣的任务或提出新想法
5. Fork本项目，实现功能后提交一个Pull Request

如果你希望编写插件，可以参考 [插件教程](docs/plugin_guide.md)。

### 开发资源

* [项目管理页面(trello)](https://trello.com/b/Z7r1N9yJ)
* [编译教程](docs/build_guide.md)
* [插件教程](docs/plugin_guide.md)
* [项目结构](https://www.mindomo.com/mindmap/63290a5a387b4e1a85ad713953be0372)

## 联系我们

请在 [issues](../../../issues) 页面提出你的意见, 或直接联系 tqjxlm@gmail.com

我们在 [trello](https://trello.com/b/Z7r1N9yJ) 上管理项目进展，你也可以在上面提出需求和反馈。