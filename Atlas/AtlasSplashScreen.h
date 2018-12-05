#include <QSplashScreen>
#include <QProgressBar>
#include <QPixmap>

class AtlasSplashScreen : public QSplashScreen
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

