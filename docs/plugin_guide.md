# Plugin guide

## Getting started

### How do I create a plugin project for Visual Studio?

1. Copy ./templates/PluginTemplate.rar to C:/Users/($your_user_name)/Documents/Visual Studio 2015/Templates/ProjectTemplate/
2. In VS2015, open Atlas.sln, go to Solution Explorer, right click on "Plugins", and choose Add->New Project
3. In the new project dialog, choose from the left side, Installed->Visual C++->AtlasPlugin
4. Set the name as your plugin name, and choose Location as ($project_root)/plugins
5. Click OK. You'll see the project in Solution Explorer

### How do I create a plugin project for Linux?

1. Make a new directory under plugins directory
2. Copy a CMakeList file from another plugin directory
3. Change the plugin name, dependencies, etc. as you need
4. Create $(project_name).cpp and $(project_name).h files
5. Run CMake as mentioned in [build_guild](build_guide.md)

### How to add plugin dependencies?

1. If the dependency is another plugin, specify it in the ($project_name).json file
2. No matter what the dependency is, add it to the link targets in CMakeList and VS project properties
3. Put any third-party dependency in the output directory (./x64/debug or ./x64/release)

### Where to put resources?

1. The recommended way is to use a .qrc file to embed the resource files into your output library (.dll or .a). Please refer to Qt resource management docs.
2. An alternative way is to put your resources under Atlas/resources, and use it in your program with "resources/CATEGORY/FILE.EXTENSION"

### How to load my plugin?

1. You should specify _pluginName and _pluginCategory in constructor
2. You should make sure all dependencies and resources are available
3. Then your plugin will be loaded on program start
4. You can go to C:/Users/$(user_name)/AppData/Roaming/Atlas/Atlas/AtlasLog.txt or /tmp/Atlas/Atlas/AtlasLog.txt for more loading details

## Plugin functionality

### How do I learn the basics (examples)?

1. The best way should be learning from an existing plugin, e.g. DrawLine or AddXYZ
2. Inherit another plugin as a quick start

### What should I implement?

1. Read through what is provided in PluginInterface and MousePicker
2. setupUi() should always be implemented to provide entrance for the plugin
3. Event callbacks like onLeftButton() will be called automatically when user input. So implement your interaction logic in those event callbacks

### How to get the coordinate under cursor?

1. Access _currentGeoPos for geographic coordinates
2. Access _currentLocalPos and _currentWorldPos for renderer coordinates
3. Access _anchoredWorldPos for anti-jittering coordinates (to put under _currentAnchor after calling updateAnchorPoint())
4. Don't write to these coordinates because they are shared across all plugins

## Data manipulation

### Where to add nodes?

There are several options for you, the priority would be:

1. _mainMap[ ] or _mapNode[ ]: any osgEarth supported data
2. _pluginRoot: general osg nodes
3. _currentAnchor: osg nodes that suffer from jittering (use _anchoredOffset as coordinate after calling updateAnchorPoint())

### How to register data with DataManager?

1. Emit recordNode() after adding the data to the scene
2. Emit removeData() when you want to remove it (no need to remove it from the scene by yourself, DataManager will do it)
3. Emit switchData() to toggle a node
4. Change a node's mask to make it visible on left or write sub-view

## User interface

### What is the recommended interaction procedure?

1. QToolButton or Menu entry: activate plugin (toggle)
2. Left button: start operation
3. Mouse move: perform dragging
4. Right button: cancel operation
5. Double click: confirm operation
6. QToolButton or Menu entry: deactivate plugin (toggle)

Of course, for those one-time functionalities, the procedure can be as short as:

1. QToolButton or Menu entry: trigger the function

### How to implement the UI?

A basic Ui setup would be:

1. QAction
   * One-time operation: checkable = false. Connect trigger() with your functional slots.
   * Procedural operation: checkable = true. Connect toggle() with PluginInterface::toggle(). And use _activated to check plugin status.
2. QToolButton to wrap the action. Add it to *toolBar.
3. Add the QAction to *menu.