#include "system_init.h"

#include <QtWidgets/QApplication>

#ifdef TRANSLATION_ENABLED
#include <QTranslator>
#include <QLocale>
#endif

/*#include <time.h>
clock_t start, finish;
start = clock();

// code here
finish = clock();
qDebug() << ( (finish - start) );
*/

int main ( int argc, char *argv[] )
{
	QApplication app ( argc, argv );
	
#ifdef TRANSLATION_ENABLED
	const QString locale ( QLocale::system ().name () );
	QTranslator translator;
	translator.load ( QStringLiteral ( ":/i18n/VivenciaManager_" ) + locale );
	app.installTranslator ( &translator );
#endif

	Sys_Init::init ( QString ( argv[0] ) );
	return app.exec ();
}
