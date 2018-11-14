#pragma once
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>
#include <QVector>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
class QDockWidget;
class QSpinBox;
class QPushButton;
QT_END_NAMESPACE

namespace osg {
    class PositionAttitudeTransform;
    class TextureCubeMap;
}

class VisibilityTestArea : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "VisibilityTestArea.json")
	Q_INTERFACES(PluginInterface)

public:
	VisibilityTestArea();
	~VisibilityTestArea();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

	virtual void onLeftButton() override;
    virtual void onRightButton() override;
    virtual void onMouseMove() override;
    virtual void onDoubleClick() override;

public slots:
	void heightSliderChanged(int value);
	void radiusSliderChanged(int value);
	void setAttributes();
	void updateAttributes();

protected:
    void showControlPanel();

    void generateTestSphere(osg::ref_ptr<osg::TextureCubeMap> depthMap, osg::ref_ptr<osg::TextureCubeMap> colorMap);
    void generateCubeCamera(osg::ref_ptr<osg::TextureCubeMap> cubeMap, unsigned face, osg::Camera::BufferComponent component);

private:
	QAction* _action;
    bool _movingMode = false;

	osg::ref_ptr<osg::Group> _shadowedScene;
	osg::ref_ptr<osg::Group> _parentScene;
    osg::ref_ptr<osg::PositionAttitudeTransform> debugNode;

	osg::ref_ptr<osg::Program> _renderProgram;
	QVector<osg::Camera*> _depthCameras;
    QVector<osg::Camera*> _colorCameras;

	int _nodeMask;
	int _userRadius;
	int _userHeight;
	osg::Vec3 _pickPos;

	QWidget* _attributePanel;
	QDockWidget* _attributeDock;
	QSpinBox* _heightBox;
	QSpinBox* _radiusBox;
	QPushButton* _okButton;
};
