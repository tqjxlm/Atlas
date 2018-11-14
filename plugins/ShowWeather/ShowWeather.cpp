#include "ShowWeather.h"

#include <QDockWidget>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QLayout>
#include <QTabWidget>
#include <QSpinBox>
#include <QLabel>
#include <QSlider>
#include <QGroupBox>
#include <QAction>
#include <QToolBar>
#include <QMenu>

#include <osg/MatrixTransform>
#include <osgParticle/PrecipitationEffect>

ShowWeather::ShowWeather()
{
	_pluginName = tr("Weather Effect");
	_pluginCategory = "Analysis";

	_controlPanel = _mainWindow->findChild<QDockWidget*>(QStringLiteral("controlPanel"));
	_tabWidget = _controlPanel->findChild<QTabWidget*>(QStringLiteral("tabWidget"));

	if (_controlPanel->maximumHeight() < 200)
		_controlPanel->setMaximumHeight(200);

    _maxRain = getOrAddPluginSettings("Max rain size", 10).toInt();
    _maxSnow = getOrAddPluginSettings("Max snow size", 10).toInt();
}

ShowWeather::~ShowWeather()
{
}

void ShowWeather::setupUi(QToolBar * toolBar, QMenu * menu)
{
	initPanelTabPage();
}

void ShowWeather::toggle(bool checked)
{
	if (checked)
	{
		_controlPanel->setHidden(false);
		_tabWidget->setCurrentIndex(_tabIndex);
	}
	else
	{
		_controlPanel->setHidden(true);
	}
}

void ShowWeather::initPanelTabPage()
{
	QWidget *weatherStyleTab;
	QVBoxLayout *mainLayout;
	QSpacerItem *verticalSpacer;

    // Panel
	weatherStyleTab = new QWidget();
	weatherStyleTab->setObjectName(QStringLiteral("weatherStyleTab"));
	weatherStyleTab->setWindowTitle(tr("Weather"));
	weatherStyleTab->setMaximumHeight(400);
	mainLayout = new QVBoxLayout(weatherStyleTab);
	mainLayout->setSpacing(6);
	mainLayout->setContentsMargins(11, 11, 11, 11);
	mainLayout->setObjectName(QStringLiteral("mainLayout"));

    // Rain
    QGroupBox *rainGroup;
    QGridLayout *rainGrid;
    QSlider *rainSizeSlider;
    QLabel *rainLabel;
    QSpinBox *rainSizeSpinBox;
    rainGroup = new QGroupBox(weatherStyleTab);
	rainGroup->setObjectName(QStringLiteral("rainGroup"));
	rainGrid = new QGridLayout(rainGroup);
	rainGrid->setSpacing(6);
	rainGrid->setContentsMargins(11, 11, 11, 11);
	rainGrid->setObjectName(QStringLiteral("rainGrid"));
	rainSizeSlider = new QSlider(rainGroup);
	rainSizeSlider->setObjectName(QStringLiteral("rainSizeSlider"));
	QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Minimum);
	sizePolicy3.setHorizontalStretch(0);
	sizePolicy3.setVerticalStretch(0);
	sizePolicy3.setHeightForWidth(rainSizeSlider->sizePolicy().hasHeightForWidth());
	rainSizeSlider->setSizePolicy(sizePolicy3);
	rainSizeSlider->setMinimumSize(QSize(0, 0));
	rainSizeSlider->setMaximumSize(QSize(16777215, 25));
	rainSizeSlider->setMinimum(0);
	rainSizeSlider->setMaximum(10);
	rainSizeSlider->setSingleStep(1);
	rainSizeSlider->setPageStep(1);
	rainSizeSlider->setValue(0);
	rainSizeSlider->setOrientation(Qt::Horizontal);

	rainGrid->addWidget(rainSizeSlider, 0, 1, 1, 1);

	rainSizeSpinBox = new QSpinBox(rainGroup);
	rainSizeSpinBox->setObjectName(QStringLiteral("rainSizeSpinBox"));
	rainSizeSpinBox->setMinimumSize(QSize(42, 0));
	rainSizeSpinBox->setMaximumSize(QSize(16777215, 16777215));
	rainSizeSpinBox->setFrame(true);
	rainSizeSpinBox->setAlignment(Qt::AlignCenter);
	rainSizeSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
	rainSizeSpinBox->setMinimum(0);
	rainSizeSpinBox->setMaximum(_maxRain);
	rainSizeSpinBox->setValue(0);

	rainGrid->addWidget(rainSizeSpinBox, 0, 2, 1, 1);

	rainLabel = new QLabel(rainGroup);
	rainLabel->setObjectName(QStringLiteral("rainLabel"));
	QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Minimum);
	sizePolicy2.setHorizontalStretch(0);
	sizePolicy2.setVerticalStretch(0);
	sizePolicy2.setHeightForWidth(rainLabel->sizePolicy().hasHeightForWidth());
	rainLabel->setSizePolicy(sizePolicy2);
	rainLabel->setMinimumSize(QSize(30, 0));
	rainLabel->setAlignment(Qt::AlignCenter);
	rainLabel->setText(tr("Rain"));

	rainGrid->addWidget(rainLabel, 0, 0, 1, 1);

	mainLayout->addWidget(rainGroup);

    // Snow
    QGroupBox *snowGroup;
    QGridLayout *snowGrid;
    QSlider *snowSizeSlider;
    QLabel *snowLabel;
    QSpinBox *snowSizeSpinBox;
    snowGroup = new QGroupBox(weatherStyleTab);
	snowGroup->setObjectName(QStringLiteral("snowGroup"));
	snowGrid = new QGridLayout(snowGroup);
	snowGrid->setSpacing(6);
	snowGrid->setContentsMargins(11, 11, 11, 11);
	snowGrid->setObjectName(QStringLiteral("snowGrid"));
	snowSizeSlider = new QSlider(snowGroup);
	snowSizeSlider->setObjectName(QStringLiteral("snowSizeSlider"));
	sizePolicy3.setHeightForWidth(snowSizeSlider->sizePolicy().hasHeightForWidth());
	snowSizeSlider->setSizePolicy(sizePolicy3);
	snowSizeSlider->setMinimumSize(QSize(0, 0));
	snowSizeSlider->setMaximumSize(QSize(16777215, 25));
	snowSizeSlider->setMinimum(0);
	snowSizeSlider->setMaximum(10);
	snowSizeSlider->setSingleStep(1);
	snowSizeSlider->setPageStep(1);
	snowSizeSlider->setValue(0);
	snowSizeSlider->setOrientation(Qt::Horizontal);

	snowGrid->addWidget(snowSizeSlider, 0, 1, 1, 1);

	snowSizeSpinBox = new QSpinBox(snowGroup);
	snowSizeSpinBox->setObjectName(QStringLiteral("snowSizeSpinBox"));
	snowSizeSpinBox->setMinimumSize(QSize(42, 0));
	snowSizeSpinBox->setMaximumSize(QSize(16777215, 16777215));
	snowSizeSpinBox->setFrame(true);
	snowSizeSpinBox->setAlignment(Qt::AlignCenter);
	snowSizeSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
	snowSizeSpinBox->setMinimum(0);
	snowSizeSpinBox->setMaximum(_maxSnow);
	snowSizeSpinBox->setValue(0);

	snowGrid->addWidget(snowSizeSpinBox, 0, 2, 1, 1);

	snowLabel = new QLabel(snowGroup);
	snowLabel->setObjectName(QStringLiteral("snowLabel"));
	sizePolicy2.setHeightForWidth(snowLabel->sizePolicy().hasHeightForWidth());
	snowLabel->setSizePolicy(sizePolicy2);
	snowLabel->setMinimumSize(QSize(30, 0));
	snowLabel->setAlignment(Qt::AlignCenter);
	snowLabel->setText(tr("Snow"));

	snowGrid->addWidget(snowLabel, 0, 0, 1, 1);

	mainLayout->addWidget(snowGroup);

    // Setup
	verticalSpacer = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
	mainLayout->addItem(verticalSpacer);

	_tabWidget->addTab(weatherStyleTab, tr("Weather"));
	_tabIndex = _tabWidget->indexOf(weatherStyleTab);

	connect(rainSizeSlider, SIGNAL(sliderMoved(int)), rainSizeSpinBox, SLOT(setValue(int)));
	connect(rainSizeSpinBox, SIGNAL(valueChanged(int)), rainSizeSlider, SLOT(setValue(int)));
	connect(rainSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(showRainSimActionSlot(int)));

	connect(snowSizeSlider, SIGNAL(sliderMoved(int)), snowSizeSpinBox, SLOT(setValue(int)));
	connect(snowSizeSpinBox, SIGNAL(valueChanged(int)), snowSizeSlider, SLOT(setValue(int)));
	connect(snowSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(showSnowSimActionSlot(int)));
}

void ShowWeather::showRainSimActionSlot(int rainSize)
{
	if (_rainEffect.valid())
	{
        _root->removeChild(_rainEffect);   
    }

    _rainEffect = new osgParticle::PrecipitationEffect;
    _rainEffect->setName("rain");
    _rainEffect->rain(2.0f * rainSize / _maxRain);

    _root->addChild(_rainEffect);
}


void ShowWeather::showSnowSimActionSlot(int snowSize)
{
	if (_snowEffect.valid())
	{
        _root->removeChild(_snowEffect);
    }

    _snowEffect = new osgParticle::PrecipitationEffect;
    _snowEffect->setName("snow");
    _snowEffect->snow(2.0f * snowSize / _maxSnow);
    _root->addChild(_snowEffect);
}