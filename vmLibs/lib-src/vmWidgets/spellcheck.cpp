#include "spellcheck.h"
#include "heapmanager.h"
#include "vmwidgets.h"

#include <vmUtils/configops.h>
#include <vmUtils/fileops.h>

#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QLocale>
#include <QtCore/QByteArray>
#include <QtWidgets/QMenu>

#ifdef USING_QT6
#include <QtCore/QStringConverter>
#include <QtGui/QActionGroup>
auto toUtf8 = QStringDecoder ( QStringDecoder::Utf8 );
#else
#include <QtCore/QTextCodec>
#endif

const QString cfgSectionName ( QStringLiteral ( "SPELLCHECK" ) );
const QString cfgFieldLanguage ( QStringLiteral ( "LANG" ) );

spellCheck::spellCheck ()
    : mChecker ( nullptr ), m_config ( new configOps ( configOps::appConfigFile (), "SpellCheckConfig" ) ),
	  mMenu ( nullptr ), menuEntrySelected_func ( nullptr )
#ifndef USING_QT6
      ,mCodec ( nullptr )
#endif
{
	getDictionariesPath ();
	m_config->addManagedSectionName ( cfgSectionName );
	setDictionaryLanguage ( m_config->getValue ( cfgSectionName, cfgFieldLanguage, false ) );
	createDictionaryInterface ();
	setUserDictionary ();
}

spellCheck::~spellCheck ()
{
	heap_del ( mChecker );
	heap_del ( mMenu );
	heap_del ( m_config );
}

void spellCheck::updateUserDict ()
{
	QFile file ( mUserDict );
	if ( file.open ( QIODevice::WriteOnly | QIODevice::Text ) )
	{
		bool ok ( false );
		unknownWordList.at ( 0 ).toInt ( &ok );
		if ( !ok )
			unknownWordList.prepend ( QString::number ( unknownWordList.count () + 2 ) );
		else
			unknownWordList.replace ( 0, QString::number ( unknownWordList.count () + 1 ) );

		QTextStream out ( &file );
		QStringList::const_iterator itr ( unknownWordList.constBegin () );
		const QStringList::const_iterator& itr_end ( unknownWordList.constEnd () );
		while ( itr != itr_end )
		{
			out << static_cast<QString>( *itr ) << CHR_NEWLINE;
			++itr;
		}
		file.close ();
	}
}

bool spellCheck::checkWord ( const QString& word )
{
	// Encode from Unicode to the encoding used by current dictionary
#ifdef USING_QT6
    QString wordToDic ( toUtf8 ( word.toLocal8Bit () ) );
    return ( mChecker ? mChecker->spell ( wordToDic.toStdString () ) != 0 : false );
#else
	return ( mChecker ? mChecker->spell ( mCodec->fromUnicode ( word ).toStdString () ) != 0 : false );
#endif
}

/* The dictionaries files (.dic and .aff) are encoded in Western Europe's ISO-8859-15. Using
 * QString's UTF-8, local8Bit and to or from StdString(which assumes std::string::data () to be in UTF-8 format)
 * results in misinterpreted characters. Again, this behavior of QString started showing after the upgrade to Qt
 * 5.9.1, the same version that prompeted errors elsewhere and had me create the QSTRING_ENCODING_FIX to overcome
 * those errors
 */
bool spellCheck::suggestionsList ( const QString& word, QStringList& wordList )
{
	if ( !checkWord ( word ) )
	{
#ifdef USING_QT6
        QString wordToDic ( toUtf8 ( word.toLocal8Bit () ) );
        std::vector<std::string> suggestList ( mChecker->suggest ( wordToDic.toStdString () ) );
#else
		std::vector<std::string> suggestList ( mChecker->suggest ( mCodec->fromUnicode ( word ).constData () ) );
#endif
		if ( !suggestList.empty () )
		{
            for ( const std::string& suggestion : suggestList )
            {
#ifdef USING_QT6
                wordList.append ( toUtf8 ( suggestion.data () ) );
#else
                wordList.append ( mCodec->toUnicode ( suggestion.data () ) );
#endif
			}
			suggestList.clear ();
			return true;
		}
	}
	return false;
}

void spellCheck::addWord ( const QString& word, const bool b_add )
{
	if ( mChecker )
	{
#ifdef USING_QT6
        QString wordToDic ( toUtf8 ( word.toLocal8Bit () ) );
        mChecker->add ( wordToDic.toStdString () );
#else
        mChecker->add ( mCodec->fromUnicode ( word ).constData () );
#endif
		if ( b_add )
		{
			unknownWordList.append ( word );
			updateUserDict ();
		}
	}
}

QMenu* spellCheck::menuAvailableDicts ()
{
	if ( mMenu == nullptr )
	{
		// QMenu::addMenu () for some reason crashes if we pass a null pointer. So, regardless of having
		// any dictionary on the system, we must create a menu to pass to it. A bug, in my opinion.
		mMenu = new QMenu ( APP_TR_FUNC ( "Choose spell language" ) );

		pointersList<fileOps::st_fileInfo*> dics;
		fileOps::lsDir ( dics, mDicPath, QStringList () << QStringLiteral ( ".dic" ), QStringList (), fileOps::LS_FILES );
		if ( !dics.isEmpty () )
		{
			QAction* qaction ( nullptr );
			QString menuText;
			mMenu->connect ( mMenu, &QMenu::triggered, this, [&] ( QAction* action ) {
				return menuEntrySelected ( action ); } );
			qaction = new vmAction ( -1, APP_TR_FUNC ( "Disable spell checking" ) );
			mMenu->addAction ( qaction );
			mMenu->addSeparator ();
			auto menuDicts ( new QActionGroup ( mMenu ) );
			auto curLang ( m_config->getValue ( cfgSectionName, cfgFieldLanguage, false ) );
			for ( uint i ( 0 ); i < dics.count (); ++i )
			{
				menuText = dics.at ( i )->filename;
				menuText.chop ( 4 ); // remove ".dic"
				qaction = new vmAction ( static_cast<int>(i), menuText, menuDicts );
				qaction->setCheckable ( true );
				if ( curLang == menuText )
					qaction->setChecked ( true );
				mMenu->addAction ( qaction );
			}
		}
	}
	return mMenu;
}

void spellCheck::menuEntrySelected ( const QAction* __restrict__ action )
{
	if ( action->text ().startsWith ( QStringLiteral ( "Disab" ) ) )
		heap_del ( mChecker );
	else
	{
		setDictionaryLanguage ( action->text () );
		createDictionaryInterface ();
	}
	if ( menuEntrySelected_func )
		menuEntrySelected_func ( mChecker != nullptr );
}

void spellCheck::getDictionariesPath ()
{
	if ( fileOps::exists ( QStringLiteral ( "/etc/lsb-release" ) ).isOn () ) // *buntu derivatives
		mDicPath = QStringLiteral ( "/usr/share/hunspell/" );
	else
		mDicPath = QStringLiteral ( "/usr/share/myspell/dicts/" );
}

inline void spellCheck::setDictionaryLanguage ( const QString& localeString )
{
	const QString locale ( localeString.isEmpty() ? QLocale::system ().name () : localeString );
	mDictionary = mDicPath + locale + QStringLiteral ( ".dic" );
	m_config->setValue ( cfgSectionName, cfgFieldLanguage, locale );
}

#define AFF_STR_LEN 4
inline void spellCheck::getDictionaryAff ( QString& dicAff ) const
{
	dicAff = mDictionary.left ( mDictionary.length () - AFF_STR_LEN ) + QStringLiteral ( ".aff" );
}

void spellCheck::createDictionaryInterface ()
{
	if ( fileOps::canRead ( mDictionary ).isOn () )
	{
		heap_del ( mChecker );
		QString dicAff;
		getDictionaryAff ( dicAff );
		mChecker = new Hunspell ( dicAff.toLocal8Bit ().constData (), mDictionary.toLocal8Bit ().constData () );
#ifndef USING_QT6
		mCodec = QTextCodec::codecForName ( mChecker->get_dic_encoding () );
#endif
	}
}

bool spellCheck::setUserDictionary ()
{
	mUserDict = configOps::appDataDir () + QStringLiteral ( "User_" ) + fileOps::fileNameWithoutPath ( mDictionary );
	QFile file ( mUserDict );
	if ( file.open ( QIODevice::ReadOnly | QIODevice::Text ) )
	{
		QTextStream in ( &file );
		in.readLine (); //skip word count
		while ( !in.atEnd () )
			unknownWordList.append ( in.readLine () );
		file.close ();
		return ( mChecker->add_dic ( mUserDict.toLocal8Bit ().constData () ) == 0 );
	}
	return false;
}
