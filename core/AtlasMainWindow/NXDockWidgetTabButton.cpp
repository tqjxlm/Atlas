#include "NXDockWidgetTabButton.h"

#include <QStylePainter>
#include <QStyleOptionButton>
#include <QMenu>

NXDockWidgetTabButton::NXDockWidgetTabButton(const QString& text, Qt::DockWidgetArea area) 
	: QPushButton(text, nullptr)
	, _mirrored(false)
	, _area(area)
	, _action(nullptr)
{
	setCheckable(true);
	setToolTip(text);

	_orientation = areaToOrientation(area);

	int fw = fontMetrics().width(text) + 25;

	fw = (fw < 15) ? 15 : fw;
	fw = (fw > 120) ? 121 : fw;

	if(_orientation == Qt::Vertical) {
		setFixedSize(30, fw);
	}
	else if(_orientation == Qt::Horizontal) {
		setFixedSize(fw, 30);
	}
}

NXDockWidgetTabButton::~NXDockWidgetTabButton()
{
}

void NXDockWidgetTabButton::setText_(const QString& text)
{
	int aw = (_orientation == Qt::Horizontal) ? width() - 4 : height() - 4;

	QFontMetrics fm = fontMetrics();
	if(aw < fm.width(text))
	{
		QString str;

		// Need to cut the text
		for(int i = 0; i < text.size(); i++)
		{
			str += text.at(i);

			if(fm.width(str + ".......") > aw)
				break;
		}

		setText(str + "...");
	}
	else
	{
		setText(text);
	}
}

QSize NXDockWidgetTabButton::sizeHint() const
{
	QSize size = QPushButton::sizeHint();
	if(_orientation == Qt::Vertical) {
		size.transpose();
	}
	return size;
}

void NXDockWidgetTabButton::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QStylePainter painter(this);

	switch(_orientation)
	{
	case Qt::Vertical:
		if(_mirrored)
		{
			painter.rotate(-90);
			painter.translate(-height(), 0);
		}
		else
		{
			painter.rotate(90);
			painter.translate(0, -width());
		}
		break;
	}

	painter.drawControl(QStyle::CE_PushButton, getStyleOption());
}

QStyleOptionButton NXDockWidgetTabButton::getStyleOption() const
{
	QStyleOptionButton opt;
	opt.initFrom(this);

	if(_orientation == Qt::Vertical)
	{
		QSize size = opt.rect.size();
		size.transpose();
		opt.rect.setSize(size);
	}

	opt.features = QStyleOptionButton::None;

	if(isFlat()) {
		opt.features |= QStyleOptionButton::Flat;
	}
	if(menu()) {
		opt.features |= QStyleOptionButton::HasMenu;
	}

	if(autoDefault() || isDefault()) {
		opt.features |= QStyleOptionButton::AutoDefaultButton;
	}

	if(isDefault()) {
		opt.features |= QStyleOptionButton::DefaultButton;
	}

	if(isDown() || (menu() && menu()->isVisible())) {
		opt.state |= QStyle::State_Sunken;
	}

	if(isChecked()) {
		opt.state |= QStyle::State_On;
	}

	if(!isFlat() && !isDown()) {
		opt.state |= QStyle::State_Raised;
	}

	opt.text = text();
	opt.icon = icon();
	opt.iconSize = iconSize();
	return opt;
}

void NXDockWidgetTabButton::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event)
	setText_(text());
}

Qt::Orientation areaToOrientation(Qt::DockWidgetArea area)
{
	assert((area == Qt::LeftDockWidgetArea) || (area == Qt::RightDockWidgetArea) ||
		(area == Qt::TopDockWidgetArea) || (area == Qt::BottomDockWidgetArea));

	switch (area)
	{
	case Qt::LeftDockWidgetArea:
	case Qt::RightDockWidgetArea:
		return Qt::Vertical;
	case Qt::TopDockWidgetArea:
	case Qt::BottomDockWidgetArea:
		return Qt::Horizontal;
	default:
		return Qt::Orientation(0);
	}
}
