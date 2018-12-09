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
	void setMapNode(osg::Node* mapNode) { _mapNode = mapNode; }

protected:
  void doMosaic();
  void doTranslate();
  void buildOverview();
  void processTile(osg::MatrixTransform* subTile, std::string nodeName);

signals:
	void updateWaitMessage(const QString&);
	void updateTime();

public slots:
	void advanceCapturing(const QString& msg);
	void startCapturing();
	void writeWithGDAL(std::string fileName, std::string saveName, const unsigned char *data, osg::BoundingBox bounding, int width, int height);
	void captureFailed(const QString& msg);
  void finish();

private:
	Ui_SaveOrthoProjDialog _ui;
	ProjectionMode _mode;

	WaitProgressDialog* _waitDialog;

	osg::PositionAttitudeTransform* _scene;
	osg::ref_ptr<osg::Node> _mapNode;
	ViewerWidget& _vw;
	osgViewer::View& _view;

	// Original view states
	osg::ref_ptr<osg::Node> _origScene;
	osg::ref_ptr<osg::Camera> _orthoCamera;
	osg::ref_ptr<osgGA::CameraManipulator> _origManip;
	osg::Matrix _origViewMatrix;
	osg::Matrix _origProjMatrix;
  osg::Vec4 _origColor;
	int _origViewWidth;
	int _origViewHeight;
	int _origMaxLOD;
	osg::CullSettings::ComputeNearFarMode _origNearFarMode;

  QString _extension = "tiff";
  int _numTiles;
	double _pixelPerMeter;
	std::string _srsWKT;

	QString _path;
	bool _successed;
	bool _finished;
	QString _msg;
	QStringList _fileList;
};

#endif // SAVEORTHOPROJDIALOG_H
