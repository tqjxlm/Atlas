#pragma once

#include <QFrame>
#include <QLabel>

class QPushButton;

class NXDockWidgetTitle : public QFrame
{
	Q_OBJECT
public:
	NXDockWidgetTitle();
	~NXDockWidgetTitle();

public:
	QString getText() const { return _textLabel->text(); }
	void setText(const QString& text) { _textLabel->setText(text); }

	void setFloating(bool state);
	void setAutoHideEnadled(bool state);

	QPoint menuPos() const;

protected:
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;

signals:
	void menuButton_pressed();
	void autoHideButton_pressed();
	void closeButton_pressed();

private:
	QPushButton* _menuButton;
	QPushButton* _autoHideButton;
	QPushButton* _closeButton;

	bool _LMPressed;
	QLabel* _textLabel;
	bool _autoHideEnabled;
};