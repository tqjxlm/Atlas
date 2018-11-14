# Atlas

Atlas 是一个三维地图可视化分析项目。Atlas 结合了 [QGIS](https://qgis.org/en/site/) 和 [谷歌地球](https://www.google.com/earth/) 的主要特性，利用 [OpenSceneGraph (OSG)](http://www.openscenegraph.org/) 引擎和 [OSGEarth](http://osgearth.org/) 来将二维地图和三维模型展示在同一三维空间内。程序使用 [Qt](https://www.qt.io/) 作为框架，以提供简洁、可定制的 UI，并且实现了一个高扩展性的插件系统，允许用户在不需要了解核心源码的情况下，轻松开发各种新功能，或是定制和修改现有的特性。

该项目的主要目的是为地理应用提供更好的可视化解决方案，并在此基础上支持复杂的空间分析、数据处理等，以方便工程应用和学术研究。

该程序目前支持Windows 7 x64及以上的系统。

[English Version](../README.md)

## Demo

该项目正在开发中，但我们提供一个 [开发者版本](https://pan.baidu.com/s/17LA7XGeUsZpzTZdLmpRWNw) 以供用户和开发者初步了解它的功能和效果。如果你有兴趣成为项目贡献者，请参见下面的 [征集开发者](#征集开发者) 小节。

一个简单的视频 demo:

## 主要特性

1. __大规模三维模型__
    Atlas 可以处理大规模三维模型，如[倾斜摄影模型](https://www.eagleview.com/product/pictometry-imagery/oblique-imagery/)，密集点云和城市级模型规划群。通过使用LOD技术，可以为大尺寸高清晰度的模型提供更流畅的交互体验。同时，基于实际地理坐标，提供了包括测量、视域分析、正射影像生成、模型编辑等实用工具。

2. __融合地理数据__
    Atlas 支持主流的地理数据格式，并将融合，其包括地理影像，地图切片，高程图，矢量特征，以及它们所对应的在线服务，如 WMS,WTS,WFS,TMS 等等。程序将这些数据准确合理地渲染在同一个三维空间中，并且为不同数据提供了统一的接口，从而允许用户自由叠加复杂的图层结构，实现分析或展示的需要。

3. __广泛格式支持__
    理论上，OSG 和 OSGEarth 支持的所有格式的数据，本项目都能够支持。此外，其他的数据格式也可以通过插件的方式获得支持。本项目支持的数据类型涵盖：地理数据，倾斜摄影模型，常见三维模型等。在 [这里](http://www.openscenegraph.org/index.php/documentation/user-guides/61-osgplugins) 和 [这里](http://docs.osgearth.org/en/latest/data.html) 可以看到完整的格式列表。

4. __高度可扩展__
    本程序通过高兼容性和扩展性的插件系统，实现了完全的插件化。在互相兼容的环境下编译的插件，放在程序的指定目录下就可以自动被加载。同时，插件被赋予了很高的权限，可以访问大部分的数据和UI，因此程序员可以不受限制地开发复杂的功能。

## 插件列表

[当前所有插件列表](plugins.md)

## 征集开发者

本项目仍处于十分早期的阶段。核心代码还不成熟，目前已有的插件也需要测试和维护。同时，作为一个三维分析软件，我们急切并长期需要开发者和研究人员为程序提供新的功能（插件）。

如果你擅长、或对以下任何话题感兴趣，我们将十分欢迎你成为项目的贡献者：

* 技能：C++，Qt，OSG,，OSGEarth，OpenGL
* 专题：桌面应用，可视化，界面设计，人机交互
* 研究：GIS，CG，CV

你可以从以下步骤开始，参与本项目的开发：

* 试用[开发者版本](https://pan.baidu.com/s/17LA7XGeUsZpzTZdLmpRWNw)
* [编译源码](docs/build_guide.md)
* 了解[项目结构](https://www.mindomo.com/mindmap/63290a5a387b4e1a85ad713953be0372)
* 浏览[任务列表](https://trello.com/b/Z7r1N9yJ)，选择感兴趣的任务或提出新想法
* Fork本项目，实现代码，并提交一个PR

如果你希望编写插件，可以参考 [插件教程](docs/plugin_guide.md)。

请在 issues 页面提出你的意见, 或直接联系我 tqjxlm@gmail.com。我们在 [trello](https://trello.com/b/Z7r1N9yJ) 上管理项目进展，你也可以在上面提出需求和反馈。

## 任务列表

请查看 [trello 面板](https://trello.com/b/Z7r1N9yJ) 中的 To Do 卡片

## 开发教程

请见 [编译](docs/build_guide.md)，[开发插件](docs/plugin_guide.md)，[项目结构](https://www.mindomo.com/mindmap/63290a5a387b4e1a85ad713953be0372)