#include "NXDockWidgetTitle.h"
#include "NXDockWidget.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QCoreApplication>
#include <QPushButton>
#include <QFrame>

static const QString s_autoHideDisabledStyle =
R"(QPushButton
{
	border: 0px;
	width: 15px; height: 15px;
    image: url("resources/widgets/pin_dockwidget_normal.png");
}
QPushButton:hover {
    image: url("resources/widgets/pin_dockwidget_hover.png");
}
QPushButton:pressed:hover {
    image: url("resources/widgets/pin_dockwidget_pressed.png");
})";

static const QString s_autoHideEnabledStyle =
R"(QPushButton 
{
	border: 0px;
	width: 15px; height: 15px;
    image: url("resources/widgets/unpin_dockwidget_normal.png");
}
QPushButton:hover {
    image: url("resources/widgets/unpin_dockwidget_hover.png");
}
QPushButton:pressed:hover {
    image: url("resources/widgets/unpin_dockwidget_pressed.png");
})";

static const QString s_closeButtonStyle =
R"(QPushButton 
{
	border: 0px;
	width: 15px; height: 15px;
    image: url("resources/widgets/close_dockwidget_normal.png");
}
QPushButton:hover {
    image: url("resources/widgets/close_dockwidget_hover.png");
}
QPushButton:pressed:hover {
    image: url("resources/widgets/close_dockwidget_pressed.png");
})";

static const QString s_menuButtonStyle =
R"(QPushButton
{
	border: 0px;
	width: 15px; height: 15px;
    image: url("resources/widgets/menu_dockwidget_normal.png");
}
QPushButton:hover {
    image: url("resources/widgets/menu_dockwidget_hover.png");
}
QPushButton:pressed:hover {
    image: url("resources/widgets/menu_dockwidget_pressed.png");
})";

NXDockWidgetTitle::NXDockWidgetTitle()
	: QFrame(nullptr)
	, _LMPressed(false)
	, _autoHideEnabled(false)
	, _textLabel(nullptr)
{
	setObjectName("DockWidgetTitle");

	QHBoxLayout* layout = new QHBoxLayout();
	setLayout(layout);

	layout->setContentsMargins(3, 2, 3, 2);
	layout->setSpacing(1);

	_textLabel = new QLabel();

	layout->addWidget(_textLabel);

	layout->addStretch(1);

	_menuButton = new QPushButton();
	_menuButton->setStyleSheet(s_menuButtonStyle);
	_menuButton->setToolTip(tr("Menu"));
	layout->addWidget(_menuButton);

	_autoHideButton = new QPushButton();
	_autoHideButton->setStyleSheet(s_autoHideDisabledStyle);
	_autoHideButton->setToolTip(tr("Auto Hide"));
	_autoHideEnabled = true;
	layout->addWidget(_autoHideButton);

	_closeButton = new QPushButton();
	_closeButton->setStyleSheet(s_closeButtonStyle);
	_closeButton->setToolTip(tr("Close"));
	layout->addWidget(_closeButton);

	connect(_menuButton, &QPushButton::clicked, this, &NXDockWidgetTitle::menuButton_pressed);
	connect(_autoHideButton, &QPushButton::clicked, this, &NXDockWidgetTitle::autoHideButton_pressed);
	connect(_closeButton, &QPushButton::clicked, this, &NXDockWidgetTitle::closeButton_pressed);
}

NXDockWidgetTitle::~NXDockWidgetTitle()
{
}

void NXDockWidgetTitle::mousePressEvent(QMouseEvent* event)
{
	if((event->button() == Qt::LeftButton) && _autoHideEnabled) {
		_LMPressed = true;
	}

	QFrame::mousePressEvent(event);
}

void NXDockWidgetTitle::mouseReleaseEvent(QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		_LMPressed = false;
	}

	QFrame::mouseReleaseEvent(event);
}

void NXDockWidgetTitle::mouseMoveEvent(QMouseEvent* event)
{
	if(_LMPressed)
	{
		NXDockWidget* dockWidget = static_cast<NXDockWidget*>(parentWidget());
		if(dockWidget != nullptr)
		{
			_LMPressed = false;

			dockWidget->setFloating(true);

			event = new QMouseEvent(QEvent::MouseButtonPress,
									event->pos(), 
									mapToGlobal(event->pos()), 
									Qt::LeftButton, 
									Qt::LeftButton, 
									Qt::NoModifier);

			QCoreApplication::postEvent(this, event);
		}
	}

	QFrame::mouseMoveEvent(event);
}

void NXDockWidgetTitle::setAutoHideEnadled(bool enabled)
{
	_autoHideEnabled = enabled;

	if(enabled) {
		_autoHideButton->setStyleSheet(s_autoHideEnabledStyle);
	}
	else {	
		_autoHideButton->setStyleSheet(s_autoHideDisabledStyle);
	}
}

QPoint NXDockWidgetTitle::menuPos() const
{
	QPoint p = _menuButton->pos();
	p.ry() += _menuButton->height();

	return QPoint(mapToGlobal(p));
}

void NXDockWidgetTitle::setFloating(bool state)
{
	_autoHideButton->setVisible(state);
}
