#ifndef GLOBAL_H
#define GLOBAL_H

#include <vmlibs.h>

#include <QtCore/QString>
const QString PROGRAM_NAME ( QStringLiteral ( "VivenciaManager4" ) );

#define VERSION_MAJOR 3
#define VERSION_MINOR 10
#define VERSION_REVISION 0
const QString VERSION_APPEND ( QStringLiteral ( "Reborn" ) );
const QString VERSION_DATE ( QStringLiteral ( "2023-08-18") );
const QString APP_ICON ( QStringLiteral ( "vm-logo-new" ) );

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)

static const QString PROGRAM_VERSION (
	QUOTEME(VERSION_MAJOR) + CHR_DOT +
	QUOTEME(VERSION_MINOR) + CHR_DOT +
	QUOTEME(VERSION_REVISION ) + QStringLiteral ( " - " ) +
	VERSION_APPEND + CHR_SPACE + CHR_L_PARENTHESIS + VERSION_DATE + CHR_R_PARENTHESIS
);

#endif // GLOBAL_H
