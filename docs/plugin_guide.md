# Plugin guide

## Steps to create a plugin project

1. Establish a working dev environment as explained in the [Build Guide](#build-guide) section first
2. Copy ./templates/PluginTemplate.rar to C:/Users/($your_user_name)/Documents/Visual Studio 2015/Templates/ProjectTemplate/
3. In VS2015, open Atlas.sln, go to Solution Explorer, right click on "Plugins", and choose Add->New Project
4. In the new project dialog, choose from the left side, Installed->Visual C++->AtlasPlugin
5. Set the name as your plugin name, and choose Location as ($project_root)/plugins
6. Click OK

## Steps to write your plugin

1. The best way to learn is to modify an existing plugin.
2. Read through what is provided in PluginInterface and MousePicker
3. Override functions or callbacks or slots according to your need. Declare what you want to override in .h file, and create their implementations in the .cpp file.
4. addToUI() is required be override to provide entrance for your plugin. All other functions are optional.
5. Create your QAction in addToUI() and connect it to toggle(). Then add it to toolBar and menu. The action acts as the trigger or switch for the plugin.
6. If your plugin depend on any other plugins, specify the dependencies in the ($project_name).json file.
7. Build your project. Put any third-party dependency in the output directory (./x64/debug or ./x64/release) and any resources under output resource directory (./x64/debug/resources or ./x64/release/resources)

## Tips and rules

1. PluginInterface provides interfaces, plugin roots and drawing attributes. MousePicker provides global roots and coordinates. Be careful to overwrite any class member of them.
2. Use _anchoredWorldPos as the current coordinate under mouse. Add any data nodes that you create to _pluginRoot.
3. Use _activated as the plugin switch.
4. Be creative in addToUI(). You can add an action, a menu, a dockWidget, or a tabPage to the main window.
5. The "onXXX()" functions are event callbacks that will be called automatically by handle() when such an event happens. You don't have to call them explicitly. Also, you don't have to override handle(). (unless you have a reason to)
6. See the official plugins as examples. DrawLine may be the simplest.

## 创建插件项目

1. 按照 [编译](#编译) 小节描述的步骤，搭建开发环境
2. 将 ./templates/PluginTemplate.rar 复制到 C:/Users/($your_user_name)/Documents/Visual Studio 2015/Templates/ProjectTemplate/
3. 使用 VS2015 打开 Atlas.sln, 在解决方案视图中，右键 "Plugins" 文件夹，然后选择“添加”->“新项目”
4. 在新项目对话框中，在左侧选择“已安装”->“Visual C++”->“Atlas Plugin”
5. 项目名称填写你的插件名称，目标地址选择 ($project_root)/plugins
6. 点击完成

## 实现插件功能

1. 要想快速开始，最好的办法是修改现有的插件
2. 浏览 PluginInterface 和 MousePicker 提供的接口和成员
3. 根据你的需要，重载函数。在 .h 文件中声明你想要重载的函数，在 .cpp 文件中实现它们
4. 必须重载 addToUI()，因为你需要在此提供插件的入口或是开关。其他的函数都是可选重载的
5. 在 addToUI() 中初始化一个 QAction，把它的信号连接到 toggle() 槽函数上。然后把它添加到 toolBar 和 menu，这样用户就可以在工具栏和菜单栏中触发插件
6. 如果你的插件依赖其他插件，需要在 ($project_name).json 文件中声明
7. 构建项目。将其他第三方依赖放在输出文件夹的根目录 (./x64/debug 或 ./x64/release)，插件用到的资源放在输出文件夹的资源目录下 (./x64/debug/resources 或 ./x64/release/resources)

## 一些建议

1. PluginInterface 主要提供函数接口、插件根节点和绘制相关的属性。MousePicker 主要提供全局根节点和鼠标坐标。请谨慎重写这两个父类的成员
2. 使用 _anchoredWorldPos 作为当前的鼠标坐标。在插件中产生的数据节点应当挂载在 _pluginRoot 下
3. 使用 _activated 作为插件开关
4. 在 addToUI() 中可以做很多事情。比如向主窗口添加动作、工具栏按钮、菜单、Dock组件、Tab组件等等，基本上可以向主窗口添加任何东西
5. 形如 "onXXX()" 的函数是事件回调，它会在事件发生时被 handle() 函数自动调用。通常情况下，你只需要重载他们，但不需要主动调用这些函数，也不需要重写 handle()
6. 多多参考官方插件。DrawLine可能是比较简单的一个。