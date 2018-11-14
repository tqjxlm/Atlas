#pragma once

#include "ViewerWidget_global.h"

QT_BEGIN_NAMESPACE
class QGridLayout;
QT_END_HEADER

namespace osgQt {
	class GraphicsWindowQt;
	class GLWidget;
}

namespace osg {
	class PositionAttitudeTransform;
}

class MapController;

#include <QWidget>
#include <QTimer>
#include <QMap>
#include <osg/Object>
#include <osgViewer/CompositeViewer>

/*
	A QWidget that renders the osg scene with multiple view support
*/
class VIEWERWIDGET_EXPORT ViewerWidget : public QWidget, public osgViewer::CompositeViewer
{
	Q_OBJECT

public:
	 ViewerWidget(osg::Node* mainScene, int x, int y, int w, int h,
		osgViewer::ViewerBase::ThreadingModel threadingModel = osgViewer::CompositeViewer::SingleThreaded);

	 ~ViewerWidget();

	 // Get the main view of the viewer, by default the viewer at position (0, 0)
	 osgViewer::View* getMainView();

	 // Get the main graphics widget of the viewer, by default the viewer at position (0, 0)
	 osgQt::GraphicsWindowQt* getMainContext();

	 // Get camera manipulator of the main view
	 MapController* getManipulator();

	 // Reset camera manipulator to the default position
	 void resetMainManipulator(osg::ref_ptr<osg::Node> centerNode);

	// Add a widget to the viewer layout at specified position
	void setWidgetInLayout(QWidget* widget, int row, int column, bool visible = true);

	void removeView(osgViewer::View* view);

	// Create a qt widget containing the given node
	QWidget* createViewWidget(osgQt::GraphicsWindowQt* gw, osg::Node* scene);

	// Create a qt graphics widget with osg support
	osgQt::GraphicsWindowQt* createGraphicsWindow(
		int x, int y, int w, int h, const std::string& name = "", bool shareMainContext = false, bool windowDecoration = false);	

	// Create a legend that's rendered above the whole view
	static osg::ref_ptr<osg::Camera> createLegendHud(QString titleString, QVector<osg::Vec4> colorVec, QVector<QString> txtVec);

	// Viewer paint event, it is called automatically every UI update
	virtual void paintEvent(QPaintEvent* event) override;

public slots:
	void setMouseStyle(unsigned styleshape);
	void stopRendering();
	void startRendering();
	void setFrameRate(int FPS);

protected:
	// An indicator at the center of the scene
	osg::ref_ptr<osg::PositionAttitudeTransform> createCameraIndicator();

	// A compass that's rendered as an hud
	void initCompass(osg::Group* root);

protected:
	QWidget* _mainWidget;
	QMap<osgViewer::View*, QWidget*> _widgets;
	osgQt::GraphicsWindowQt* _mainContext;
	osg::ref_ptr<MapController> _mainManipulator;
	osgViewer::View* _mainView;
	QTimer _timer;
	QGridLayout* _grid;
	int _frameRate;
};
