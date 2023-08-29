#ifndef WORDHIGHLIGHTER_H
#define WORDHIGHLIGHTER_H

#undef QT_NO_CAST_FROM_ASCII
#undef QT_NO_CAST_TO_ASCII

#include "vmlibs.h"

#include <QtCore/QStringList>
#include <QtGui/QSyntaxHighlighter>

const char* const PROPERTY_PRINT_PREVIEW ( "ppp" );
const char* const PROPERTY_DOC_MODIFIED ( "pdm" );

class spellCheck;

class QTextCharFormat;
class QTextDocument;

class wordHighlighter : public QSyntaxHighlighter
{

public:
	explicit wordHighlighter (QTextDocument* parent, spellCheck* const spellchecker );
	virtual ~wordHighlighter ();

	inline bool spellCheckingEnbled () const { return mb_spellCheckEnabled; }
	void enableSpellChecking ( const bool enable );

	inline bool highlightingEnabled () const { return mb_HighlightEnabled; }
	void enableHighlighting ( const bool );

	void unHighlightWord ( const QString& );
	void highlightWord ( const QString& );
	void highlightWords ( const QStringList& );

	inline bool inPreview () const { return property ( PROPERTY_PRINT_PREVIEW ).toBool (); }
	inline void setInPreview ( const bool b_preview ) { setProperty ( PROPERTY_PRINT_PREVIEW, b_preview ); }
	
protected:
	void highlightBlock ( const QString& ) override;

private:
	QStringList highlightedWordsList;
	bool mb_spellCheckEnabled, mb_HighlightEnabled;
	spellCheck* mSpellChecker;

	static QTextCharFormat* __restrict m_spellCheckFormat, * __restrict m_HighlightFormat;
};

#endif // WORDHIGHLIGHTER_H
