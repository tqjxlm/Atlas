#include "MeshMode.h"

#include <QAction>
#include <QMenu>
#include <QToolBar>

#include <osg/PolygonMode>

#include <DataManager/FindNode.hpp>

MeshMode::MeshMode()
	: _mode(0)
{
	_pluginName = tr("Mesh Mode");
	_pluginCategory = "Effect";
}

MeshMode::~MeshMode()
{
}

void MeshMode::setupUi(QToolBar * toolBar, QMenu * menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("meshModeAction"));
	QIcon icon8;
	icon8.addFile(QStringLiteral("resources/icons/triangulation.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon8);

	_action->setText(tr("Mesh Mode"));
	_action->setToolTip(tr("Change Mesh Rendering Mode"));

	connect(_action, SIGNAL(triggered()), this, SLOT(trigger()));
	registerMutexAction(_action);

	toolBar->addAction(_action);
	menu->addAction(_action);
}

void MeshMode::trigger()
{
	osg::PolygonMode * polygonMode = new osg::PolygonMode;

	switch (_mode)
	{
	case(0):
    {
        auto skyNode = findNodeInNode("Sky", _root);
        if (skyNode)
            skyNode->setNodeMask(0);
        polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
        _mode++;
        break;
    }
	case(1):
    {
        polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::POINT);
        _mode++;
        break;
    }
	case(2):
    {
        auto skyNode = findNodeInNode("Sky", _root);
        if (skyNode)
            skyNode->setNodeMask(1);
        polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
        _mode = 0;
        break;
    }
    default:
        return;
	}
	_root->getStateSet()->setAttributeAndModes(polygonMode, osg::StateAttribute::ON);
}
