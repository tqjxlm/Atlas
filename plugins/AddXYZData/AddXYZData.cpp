#include "AddXYZData.h"

#include <QToolBar>
#include <QAction>
#include <QMenu>

#include <osgEarth/Map>
#include <osgEarth/ImageLayer>
#include <osgEarthDrivers/xyz/XYZOptions>
using namespace osgEarth::Drivers;

#include "urlDialog.hpp"

AddXYZData::AddXYZData()
{
    _pluginCategory = "Data";
    _pluginName = tr("XYZ Data");
}

AddXYZData::~AddXYZData()
{

}

void AddXYZData::setupUi(QToolBar *toolBar, QMenu *menu)
{
	QAction* addImageAction = new QAction(_mainWindow);
	addImageAction->setObjectName(QStringLiteral("addXYZImgAction"));
	addImageAction->setText(tr("Online images (XYZ)"));
	addImageAction->setToolTip(tr("Load online image with standard XYZ organization pattern"));

	menu = getOrAddMenu(IMAGE_LAYER);
	menu->addAction(addImageAction);
	connect(addImageAction, SIGNAL(triggered()), this, SLOT(addImage()));
}

void AddXYZData::addImage()
{
	QMap<QString, QString> examples;
	examples[tr("Open Street Map")] = "http://[abc].tile.openstreetmap.org/{z}/{x}/{y}.png";
	examples[tr("Gaode")] = "http://wprd0[1234].is.autonavi.com/appmaptile?lang=zh_cn&size=1&style=7&x={x}&y={y}&z={z}";
	urlDialog dialog(examples, _mainWindow);

	int accepted = dialog.exec();
	if (accepted == QDialog::Accepted)
	{
		QString url = dialog.getUrl();
		if (url.isEmpty())
			return;

		std::string nodeName = url.toLocal8Bit().toStdString();
		XYZOptions opt;
		opt.url() = nodeName;
        opt.profile() = { "spherical-mercator" };

		auto layer = new osgEarth::ImageLayer(osgEarth::ImageLayerOptions(nodeName, opt));

		QVector<attrib> attribute;

		addLayerToMap(layer, IMAGE_LAYER, url, attribute);
	}
}
