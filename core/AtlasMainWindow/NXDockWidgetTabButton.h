#pragma once

#include <QPushButton>

/*! Button on dockwidget tab to open up dockwidget
*/
class NXDockWidgetTabButton : public QPushButton
{
	Q_OBJECT
public:
	NXDockWidgetTabButton(const QString& text, Qt::DockWidgetArea area);
	~NXDockWidgetTabButton();

public:
	void setAction(QAction* action) { _action = action; }
	QAction* getAction() const { return _action; }

private:
	void setText_(const QString& text);
	QStyleOptionButton getStyleOption() const;

protected:
	virtual void paintEvent(QPaintEvent* event) override;
	virtual void resizeEvent(QResizeEvent* event) override;
	virtual QSize sizeHint() const override;

private:
	QAction* _action;
	Qt::DockWidgetArea _area;
	Qt::Orientation _orientation;
	bool _mirrored;
};

Qt::Orientation areaToOrientation(Qt::DockWidgetArea area);