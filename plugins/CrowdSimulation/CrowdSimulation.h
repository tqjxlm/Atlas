#pragma once
#include <QtPlugin>
#include <DrawSurfaceLine/DrawSurfaceLine.h>

#include <QMap>
#include <QThread>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
class QDockWidget;
class QLabel;
class QPushButton;
QT_END_NAMESPACE

class CrowdSimRenderer;
class NXDockWidget;

namespace Ped {
	class Cell;
	class AgentGroup;
}

namespace osg {
	class MatrixTransform;
	class Drawable;
}

class CrowdSimulation : public DrawSurfaceLine
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "CrowdSimulation.json")
	Q_INTERFACES(PluginInterface)

	enum Mode {
		BASE,
		OBSTACLE,
		PATH,
		NONE
	};

public:
	CrowdSimulation();
	~CrowdSimulation();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

	virtual void initStage(osg::ref_ptr<osg::Node> baseBound, osg::ref_ptr<osg::Vec3Array> edgePoints, bool addToScene);
	virtual void initSimulator(osg::ref_ptr<osg::Node> model);
	virtual void registerObstacle(osg::ref_ptr<osg::Node> node, osg::ref_ptr<osg::Vec3Array> edgePoints, bool addToScene);
	virtual osg::ref_ptr<osg::Node> loadModel();

	// Inherited event handler
	virtual void onLeftButton();
	virtual void onDoubleClick();
	virtual void onMouseMove();
	virtual void onRightButton();

	// Function to update drawn path
	virtual void drawPath();

	// Left button handler according to the current mode
	void pushPath();
	void pushObstacle();
	void pushAgentSource();

private:
	void initControlPanel();
	// For simulator debuging: generate a cell to draw
	osg::ref_ptr<osg::Geometry> generateCellGeom(const Ped::Cell * cell);

public slots :
	virtual void toggle(bool checked = true) override;

	// Mode swithces
	void setObstacleMode() { _mode = OBSTACLE; }
	void setPathMode() { _mode = PATH; }
	void setNoneMode() { _mode = NONE; }
	void setAreaMode() { _mode = BASE; }
	void setDebugMode(bool enable);

	// Path plan failed alert
	void pathPlanFailed();

	// Simulator re-init, rebuild the whole simulator, including the quad tree
	void resetSimulator(osg::Vec2 upperLeft, osg::Vec2 size);

	// Simulation re-init, while keeping the data structure
	void resetSimulation();

	bool loadSettings();
	void saveSettings();

	// For simulator debuging, update drawn cell to keep up with
	// the underlying cell data
	void updateCellMap();
	void crowdSimDebugMode(bool checked);
	void setSimRate(int value);
	void setSimDensity(int value);
	void setPedDistance(int value);
	void setAgentCount(int value);
	void onResetSimulatingPushButtonClicked();
	void onSimDebugCheckBoxStatusChanged(int status);

signals:
	void pauseSim();
	void startSim();
	void resetSim();
	void addWayPoint(float, float, float);
	void addObstacle(float, float, float, float);
	void addAgentGroup(float, float, float, float, float);
	void updatePath(Ped::AgentGroup*, float, float, float, float);
	void setRenderRoot(osg::MatrixTransform*);

protected:
	osg::ref_ptr<CrowdSimRenderer> _simRenderer;
	osg::ref_ptr<osg::Geometry> _lastLine;
	osg::ref_ptr<osg::Vec3Array> _pickedPoints;
	osg::ref_ptr<osg::Geode> _cellMapRoot;
	osg::ref_ptr<osg::Group> _settingsToSave;

	Mode _mode;
	QThread _renderThread;
	QMap<Ped::Cell*, osg::ref_ptr<osg::Drawable>> _cellMap;

	bool _debugOn = false;

	int _numObstacle;
	int _numAgentGroup;
	int _numPath;

private:
	QAction* _action;
	NXDockWidget *_controlPanel;
	QPushButton *_resetSimulatingPushButton;
	QPushButton *_startSimulatingPushButton;
	QPushButton *_stopSimulatingPushButton;
	QPushButton *_setObstaclePushButton;
	QLabel *_agentCountLabel;
	QLabel *_densityLabel;
	QLabel *_distanceLabel;
	QLabel *_simRateLabel;
	float _simRateRange[2];
	float _simMaxDensity[2];
	float _pedDistRange[2];
};
