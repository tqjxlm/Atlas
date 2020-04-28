#include <QSplashScreen>
#include <QProgressBar>
#include <QPixmap>

#include "AtlasMainWindow_global.h"

class ATLASMAINWINDOW_EXPORT AtlasSplashScreen : public QSplashScreen
{
	Q_OBJECT

public:
	explicit AtlasSplashScreen(QPixmap& pixmap);
	~AtlasSplashScreen();

private:
	QFont splashFont;
	QPixmap rePixmap;
	QProgressBar *progressBar;
	int totalSteps, nowStep;

private slots:
	void setTotalInitSteps(int num);
	void setNowInitName(const QString& name);

};

