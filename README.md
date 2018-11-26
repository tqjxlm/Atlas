# Atlas

Atlas is a GIS visualization project that aims to combine some basic features of [QGIS](https://qgis.org/en/site/) and [Google Earth](https://www.google.com/earth/). It utilizes [OpenSceneGraph (OSG)](http://www.openscenegraph.org/) and [OSGEarth](http://osgearth.org/) to handle 2D maps and 3D models in the same space. It depends on [Qt](https://www.qt.io/) to provide a friendly and easy-to-extend UI and provides a convenient plugin system so that all kinds of extensions can be implemented easily.

The main purpose of the project is to provide fancier visualization of geographic applications, while supporting some complicated analysis tasks and more advanced research purposes.

It currently supports Windows 7 x64 and above systems.

[中文版说明](docs/README.cn.md)

## Demos

The project is still under development, but a [development release](https://drive.google.com/open?id=10lZnGS43kjEyBusYYlvUROLRjjqIqwvH) is provided so you can have a taste of it. If you are interested to contribute, please see [Call for Contributors](#call-for-contributors) section below.

A simple video demonstration:

## Main Features

1. __Large Scale 3D Models__
    Atlas handles large scale real world models like [Oblique Imagery](https://www.eagleview.com/product/pictometry-imagery/oblique-imagery/), dense point clouds and city-scale model group. Using LOD technology, models with great sizes and high resolutions can be viewed with better user experience. Useful GIS oriented tools like measurements, visibility analysis, ortho imagery generation and model editing are shipped with friendly UI.

2. __Mixing Geographic Data__
    Most formats of traditional data are supported and fused together, such like local and online maps, terrains and geo-features. They can be managed in a uniform way, shown in the same space and interact with each other. Complicated scene structure, rendering strategy and data analysis can be achieved though our various plugins.

3. __Multiple Formats__
    Theoretically, all data formats supported by OSG and OSGEarth can be utilized in the program. Other data formats can also be supported by means of plugins. Typical formats include: geographic images, map services and 3D models. A full list can be found [here](http://www.openscenegraph.org/index.php/documentation/user-guides/61-osgplugins) and [here](http://docs.osgearth.org/en/latest/data.html).

4. __Fully Pluggable__
    Thanks to Qt, a plugin system is implemented as a core component of Atlas. Plugins compiled under the same environment as the main program can just drop-and-run. Plugins are given  quite a lot of freedom, so there is little limit on what you can do with plugins.

## Plugin List

All current plugins are listed at [plugins.md](docs/plugins.md).

## Call for Contributors

This project is at a very early stage. The core code is far from being mature or stable, and the plugins need testing and maintenance. Also, since the project aims to mimic QGIS and Google Earth, more functionalities (plugins) are urgently and consistently needed.

If you are interested in or skilled at any of the following topics, we are most delightful if you can become a contributor:

* skills: C++, Qt, OSG, OSGEarth
* development topics: desktop app, visualization, UI
* research interest: GIS, CG, CV

You can get started by:

* try out the [dev release](https://drive.google.com/open?id=10lZnGS43kjEyBusYYlvUROLRjjqIqwvH)
* [building the project](docs/build_guide.md)
* read the [project structure](https://www.mindomo.com/mindmap/63290a5a387b4e1a85ad713953be0372)
* find your interested task in [ToDo List](https://trello.com/b/Z7r1N9yJ) or put up a new idea
* fork the project, finish coding and commit a PR

If you want to write a new plugin, see [plugin_guide.md](docs/plugin_guide.md).

You are welcome to give comments on issues page, or just contact me via tqjxlm@gmail.com. We manage the project progress on [trello](https://trello.com/b/Z7r1N9yJ) where you can also request for features and report bugs.

## Todo List

See __"To Do"__ card on [trello board](https://trello.com/b/Z7r1N9yJ).

# On Linux 
This Fork work on linux with cmake

mkdir build
cd build
cmake-gui ../ #Select Plugins ...

make

## Development

See [build_guide.md](docs/build_guide.md), [plugin_guide.md](docs/plugin_guide.md) and [project structure](https://www.mindomo.com/mindmap/63290a5a387b4e1a85ad713953be0372).
