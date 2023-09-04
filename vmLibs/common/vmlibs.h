#ifndef VMLIBS_H
#define VMLIBS_H

#include <QtCore/QString>

#ifdef DEBUG
#include <QDebug>
#undef USE_THREADS
#define DBG_OUT(message,value,b_ShowValueExpression,separate) \
		if ( separate ) \
			qDebug () << QLatin1String ( "-----------------------------------" ); \
		if ( b_ShowValueExpression ) \
			qDebug () << \
			QLatin1String ( "In ") << \
			QLatin1String ( __FILE__ ) << \
			QLatin1String ( "  - at line  " ) << \
			QString::number ( __LINE__  ) << Qt::endl << \
			message << Qt::endl << \
			QLatin1String ( #value ) << \
			QLatin1String ( ": " ) << \
			value << Qt::endl; \
		else \
			qDebug () << \
			QLatin1String ( "In " ) << \
			QLatin1String ( __FILE__ ) << \
			QLatin1String ( "  - at line  " ) << \
			QString::number ( __LINE__  ) << Qt::endl << \
			message << Qt::endl << \
			value << Qt::endl;
#else
#define DBG_OUT(message,value,b_ShowValueExpression,separate)
#define USE_THREADS
#endif

#ifdef TRANSLATION_ENABLED
#define TR_FUNC tr
#define APP_TR_FUNC QApplication::tr
#else
#define TR_FUNC QStringLiteral
#define APP_TR_FUNC QStringLiteral
#endif

/* Don't know what happend here. This is the fix a found for the pŕoblem of Qt misinterpreting all the strings
 * that have non-english characters in them. They display fine on the window, but not on the console. The system-wide 
 * Qt libraries themselves do not read those strings, no matter if write them on the editor and hardcode them into the
 * program or if I read from the system using C libraries or if I read from the Database using Qt's own mechanism for doing
 * it so. 
 */
//#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 1)
//#define QSTRING_ENCODING_FIX(str) QString::fromLocal8Bit ( str.toUtf8 () )
//#else
#define QSTRING_ENCODING_FIX(str) str
//#endif

#define ICON(name) QIcon::fromTheme ( QStringLiteral ( name ), QIcon ( QStringLiteral ( ":resources/" name ) ) )
#define PIXMAP(name) QPixmap ( QStringLiteral ( ":resources/" name ) )

static const QLatin1Char CHR_DOT ( '.' );
static const QLatin1Char CHR_COMMA ( ',' );
static const QLatin1Char CHR_SPACE ( ' ' );
static const QLatin1Char CHR_F_SLASH ( '/' );
static const QLatin1Char CHR_B_SLASH ( '\\' );
static const QLatin1Char CHR_L_PARENTHESIS ( '(' );
static const QLatin1Char CHR_R_PARENTHESIS ( ')' );
static const QLatin1Char CHR_NEWLINE ( '\n' );
static const QLatin1Char CHR_PERCENT ( '%' );
static const QLatin1Char CHR_PIPE ( '|' );
static const QLatin1Char CHR_HYPHEN ( '-' );
static const QLatin1Char CHR_EQUAL ( '=' );
static const QLatin1Char CHR_QUESTION_MARK ( '?' );
static const QLatin1Char CHR_CHRMARK ( '\'' );
static const QLatin1Char CHR_COLON ( ':' );
static const QLatin1Char CHR_SEMICOLON ( ';' );
static const QLatin1Char CHR_QUOTES ( '\"' );
static const QLatin1Char CHR_L_BRACKET ( '[' );
static const QLatin1Char CHR_R_BRACKET ( ']' );
static const QLatin1Char CHR_TILDE ( '~' );

static const QString CHR_ZERO ( QStringLiteral ( "0" ) );
static const QString CHR_ONE ( QStringLiteral ( "1" ) );
static const QString CHR_TWO ( QStringLiteral ( "2" ) );
static const QString sudoCommand ( QStringLiteral ( "echo \"%1\" | sudo -S sh -c \"%2\"" ) );

static const QString emptyString;

#endif // VMLIBS_H
