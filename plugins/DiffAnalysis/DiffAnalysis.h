#pragma once

#include <QtPlugin>

#include <DrawSurfacePolygon/DrawSurfacePolygon.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

#include <string>
namespace osgGA {
	class CameraManipulator;
}

class DiffAnalysis : public DrawSurfacePolygon
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "DiffAnalysis.json")
	Q_INTERFACES(PluginInterface)

	// View status set
	struct ViewStatus
	{
		osg::ref_ptr<osg::Node> origScene;
		osg::Matrix origViewMatrix;
		osg::Matrix origProjMatrix;
		int origViewWidth;
		int origViewHeight;
		osg::CullSettings::ComputeNearFarMode origNearFarMode;
		osg::ref_ptr<osgGA::CameraManipulator> origManip;
	};

public:
	DiffAnalysis();
	~DiffAnalysis();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

	virtual void onDoubleClick();
	virtual void onLeftButton();

	// Write out as image
	void writeWithGDAL(osg::Image* image, std::string path);
	void testDifference();

protected:
	osg::ref_ptr<osg::Image> analyzeContour(osg::Image* image);
	osg::Vec3 getTranslatedPoint(int x, int y);
	void skeletonize(osg::Image* image);

	// Height computation method
	osg::ref_ptr<osg::Image> initDepthRenderer(osgViewer::View* view);
	void calcuZvaluefromDepth(osg::Image* image, osg::Camera* camera);

	// Setup
	int promptForUserSettings();
	void initParameters();

	// Show or hide the calculation result
	void showContour(osg::Image* image);
	void recoverView(osgViewer::View* view);

protected:
	osg::ref_ptr<osg::Vec3Array> _contour;
	osg::ref_ptr<osg::Group> _captureRoot;

	// Image related
	int _posterWidth;
	int _posterHeight;
	double _pixelPerMeter;
	osg::BoundingBox _boundingBox;
	float _diffThreashold;

	// View related
	unsigned int _viewWidth;
	unsigned int _viewHeight;
	std::map<osgViewer::View*, ViewStatus> _viewStatus;
	double _captureZOffset;

private:
	QAction* _action;
};
