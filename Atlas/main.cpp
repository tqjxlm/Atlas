#include "Atlas.h"
#include "AtlasSplashScreen.h"

#include <QApplication>
#include <QFile>
#include <QSurfaceFormat>

int  main(int argc, char *argv[])
{
  // A trick to get higher fps than 30
  QSurfaceFormat format = QSurfaceFormat::defaultFormat();
  format.setSwapInterval(0);
  QSurfaceFormat::setDefaultFormat(format);

  QApplication  app(argc, argv);

  // Load an application style
  QFile  styleFile("resources/styles/Atlas.qss");

  if (styleFile.open(QFile::ReadOnly))
  {
    QString  style(styleFile.readAll());
    app.setStyleSheet(style);
  }

	// Show splash screen
  QPixmap            a("./resources/images/atlas_big.png");
  AtlasSplashScreen *splash = new AtlasSplashScreen(a);
  Atlas              w;
	QObject::connect(&w, SIGNAL(sendTotalInitSteps(int)), splash, SLOT(setTotalInitSteps(int)));
	QObject::connect(&w, SIGNAL(sendNowInitName(const QString&)), splash, SLOT(setNowInitName(const QString&)));

	splash->show();
  w.initAll();

	// Begin application
	w.showMaximized();
	splash->finish(&w);
	delete splash;

	return app.exec();
}
