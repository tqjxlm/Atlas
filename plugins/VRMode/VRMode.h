#pragma once
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace osg
{
class GraphicsContext;
}

namespace osgQt
{
class GraphicsWindowQt;
class GLWidget;
}

namespace osgViewer
{
class View;
class Viewer;
class GraphicsWindow;
}

class OpenVRDevice;
class VRControlCallback;

class VRMode: public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "VRMode.json")
	Q_INTERFACES(PluginInterface)

public:
	VRMode();

	~VRMode();

	virtual void  setupUi(QToolBar *toolBar, QMenu *menu) override;

	bool          isVRRunning();

	bool          isVRReady();

public slots:
	virtual void  toggle(bool checked = false);

	void          removeVRView();

	void          addVRView();

	void          controlEvent();

signals:
	void          aborted();

private:
	QAction                               *_action;
	double                                 _fake_position_x;
	double                                 _fake_position_y;
	bool                                   _trigger;
	bool                                   _clear;
	bool                                   _VRReady;
	osg::ref_ptr<osg::GraphicsContext>     _VRContext;
	osg::ref_ptr<osgQt::GraphicsWindowQt>  _VRGraphicsWindow;
	osg::ref_ptr<OpenVRDevice>             _openVRDevice;
	osg::ref_ptr<osgViewer::View>          _VRView;
	QWidget                               *_VRWidget;
	osg::ref_ptr<VRControlCallback>        _controlCallback;
};
