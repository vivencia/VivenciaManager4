#ifndef SPELLCHECK_H
#define SPELLCHECK_H

#include "vmlibs.h"

#include <vmUtils/tristatetype.h>

#include <QtCore/QObject>
#include <QtCore/QStringList>

#include <functional>
#include <hunspell/hunspell.hxx>

class configOps;
class QMenu;
class QAction;

class spellCheck : public QObject
{

public:
	spellCheck ();
	virtual ~spellCheck ();

	void updateUserDict ();
	bool suggestionsList ( const QString& word, QStringList& wordList );
	void addWord ( const QString& word, const bool b_add );

	// Refer to comment before suggestionsList () in the cpp file
	inline triStateType checkWord ( const QString& word )
	{
		return ( mChecker ? static_cast<TRI_STATE>( mChecker->spell ( word.toLatin1 ().toStdString () ) ) : TRI_UNDEF );
	}

	inline void setCallbackForMenuEntrySelected ( const std::function<void ( const bool )>& func ) { menuEntrySelected_func = func; }

	QMenu* menuAvailableDicts ();

private:
	void menuEntrySelected ( const QAction* action );
	void getDictionariesPath ();
	void setDictionaryLanguage ( const QString& localeString );
	void getDictionaryAff ( QString& dicAff ) const;
	void createDictionaryInterface ();
	bool setUserDictionary ();

	QStringList unknownWordList;
	QString mUserDict;
	QString mDicPath;
	QString mDictionary;

	Hunspell* __restrict mChecker;
	configOps* m_config;
	QMenu* mMenu;

	std::function<void ( const bool )> menuEntrySelected_func;
};

#endif // SPELLCHECK_H
