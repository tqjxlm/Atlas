#ifndef SAVEORTHOPROJDIALOG_H
#define SAVEORTHOPROJDIALOG_H

#include <QDialog>
#include <QProcess>

#include "PosterPrinter.h"
#include <ViewerWidget/ViewerWidget.h>
#include "ui_SaveOrthoProjDialog.h"

class CameraSyncCallback;
class PrintPosterHandler;
class StaticPrintHandler;
class CustomRenderer;
class CaptureThread;
class WaitProgressDialog;

namespace osg {
	class Camera;
}

namespace osgGA {
	class CameraManipulator;
}

class SaveOrthoProjDialog : public QDialog
{
	Q_OBJECT

public:
	enum ProjectionMode {
		DOM,
		DSM
	};
	SaveOrthoProjDialog(ViewerWidget& vw, osg::Node* scene, std::string srs, ProjectionMode mode, QWidget* parent);
	~SaveOrthoProjDialog();
	void setup();
	void finish();
	void setMapNode(osg::Node* mapNode) { _mapNode = mapNode; }

signals:
	void updateWaitMessage(QString);
	void updateTime();

public slots:
	void waitForCapturing(bool started);
	void advanceCapturing(QString msg);
	void startCapturing();
	void writeWithGDAL();
	void captureFailed(QString msg);

private:
	Ui_SaveOrthoProjDialog _ui;
	ProjectionMode _mode;

	osg::ref_ptr<osg::Switch> _captureRoot;
	osg::ref_ptr<PosterPrinter> _printer;
	WaitProgressDialog* _waitDialog;

	osg::Node* _scene;
	osg::ref_ptr<osg::Node> _mapNode;
	osg::ref_ptr<osg::Node> _subTile;
	ViewerWidget& _vw;
	osgViewer::View& _view;

	// 原视图状态
	osg::ref_ptr<osg::Node> _origScene;
	osg::ref_ptr<osg::Camera> _orthoCamera;
	osg::ref_ptr<osgGA::CameraManipulator> _origManip;
	osg::Matrix _origViewMatrix;
	osg::Matrix _origProjMatrix;
	int _origViewWidth;
	int _origViewHeight;
	int _origMaxLOD;
	osg::CullSettings::ComputeNearFarMode _origNearFarMode;

	osg::Vec4 _bgColor;
	osg::BoundingBox _boundingBox;
	float _boundingWidth;
	float _boundingHeight;
	int _numTiles;
	double _pixelPerMeter;
	int _tileWidth;
	int _tileHeight;
	int _posterWidth;
	int _posterHeight;
	bool _activeMode;
	std::string _srsWKT;

	//QTime _processTime;
	QString _path;
	bool _successed;
	bool _finished;
	QString _msg;
	QStringList _fileList;
};

#endif // SAVEORTHOPROJDIALOG_H
