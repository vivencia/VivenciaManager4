#include "wordhighlighter.h"
#include "heapmanager.h"
#include "spellcheck.h"

#undef QT_NO_CAST_FROM_ASCII
#undef QT_NO_CAST_TO_ASCII

#include <QtCore/QStringMatcher>
#include <QtGui/QTextDocument>
#include <QtGui/QTextCharFormat>

#ifdef USING_QT6
#include <QtCore/QRegularExpression>
static const QRegularExpression word_split_syntax ( QStringLiteral ( "([^\\w,^\\\\]|(?=\\\\))+" ) );
#else
#include <QtCore/QRegExp>
static const QRegExp word_split_syntax ( QStringLiteral ( "([^\\w,^\\\\]|(?=\\\\))+" ) );
#endif

wordHighlighter::wordHighlighter ( QTextDocument* parent, spellCheck* const spellchecker )
	: QSyntaxHighlighter ( parent ), mb_spellCheckEnabled ( false ), mb_HighlightEnabled ( false ),
	  mSpellChecker ( spellchecker ), m_spellCheckFormat ( nullptr ), m_HighlightFormat ( nullptr )
{
	m_spellCheckFormat = new QTextCharFormat;
	m_spellCheckFormat->setForeground ( Qt::red );
	m_spellCheckFormat->setUnderlineColor ( QColor ( Qt::red ) );
	m_spellCheckFormat->setUnderlineStyle ( QTextCharFormat::SpellCheckUnderline );
}

wordHighlighter::~wordHighlighter ()
{
	heap_del ( m_spellCheckFormat );
	heap_del ( m_HighlightFormat );
}

void wordHighlighter::enableHighlighting ( const bool enable )
{
	if ( enable )
	{
		if ( !m_HighlightFormat )
		{
			m_HighlightFormat = new QTextCharFormat;
			m_HighlightFormat->setBackground ( Qt::yellow );
			m_HighlightFormat->setUnderlineStyle ( QTextCharFormat::WaveUnderline );
		}
	}
	mb_HighlightEnabled = enable;
	rehighlight ();
}

void wordHighlighter::enableSpellChecking ( const bool enable )
{
	if ( m_spellCheckFormat )
	{
		mb_spellCheckEnabled = enable;
		rehighlight ();
	}
}

void wordHighlighter::unHighlightWord ( const QString& word )
{
	const int i ( highlightedWordsList.indexOf ( word ) );
	if ( i != -1 )
	{
		highlightedWordsList.removeAt ( i );
		rehighlight ();
	}
}

void wordHighlighter::highlightWord ( const QString& word )
{
	if ( !word.isEmpty () )
	{
		highlightedWordsList.clear ();
		highlightedWordsList.append ( word );
		rehighlight ();
	}
}

void wordHighlighter::highlightWords ( const QStringList &words )
{
	if ( !words.isEmpty () )
	{
		highlightedWordsList = words;
		rehighlight ();
	}
}

void wordHighlighter::highlightBlock ( const QString& text )
{
	if ( inPreview () ) return; // In preview mode. Disable any highlight

	if ( spellCheckingEnbled () || highlightingEnabled () )
	{
		// split text into words
		QString str ( text.simplified () );
		if ( !str.isEmpty () )
		{
			QStringMatcher str_match;
			str_match.setCaseSensitivity ( Qt::CaseInsensitive );
			const QStringList wordsInBlock ( str.split ( word_split_syntax, Qt::SkipEmptyParts ) );
			//const QStringList wordsInBlock ( str.split ( CHR_SPACE, QString::SkipEmptyParts ) );

			int l ( -1 ), number ( -1 );
			int i ( 0 ), j ( 0 ), x ( 0 );
			bool proceed_highlight ( false );

			// check all words
			for ( ; i < wordsInBlock.size (); ++i )
			{
				str = wordsInBlock.at ( i );
				if ( str.length () > 1 )
				{
					if ( m_spellCheckFormat != nullptr )
					{
						if ( !mSpellChecker->checkWord ( str ) )
						{
#ifdef USING_QT6
                            number = text.count ( QRegularExpression ( QLatin1String ("\\b" ) + str + QLatin1String ("\\b" ) ) );
#else
                            number = text.count ( QRegExp ( QLatin1String ("\\b" ) + str + QLatin1String ("\\b" ) ) );
#endif
							// underline all incorrect occurences of misspelled word
							str_match.setPattern ( str );
							for ( j = 0; j < number; ++j )
							{
								l = str_match.indexIn ( text, l + 1 );
								if ( l >= 0 )
									setFormat ( l, str.length (), *m_spellCheckFormat );
							}
						}
					}
					if ( m_HighlightFormat != nullptr )
					{
						for ( x = 0; x < highlightedWordsList.count (); ++x )
						{
							// The highlighting is not done by a word-by-word basis, instead, it's by
							// characters. We might want do find only a part of a word, or even a letter
							number = highlightedWordsList.at ( x ).count ();
							str_match.setPattern ( highlightedWordsList.at ( x ) );
							if ( str.count () >= number )
								proceed_highlight = str_match.indexIn ( text );
							else // The word in the text cannot be smaller than the word ( or partial word ) we a looking for
								proceed_highlight = false;

							if ( proceed_highlight )
							{
								for ( j = 0; j < number; ++j )
								{
									l = str_match.indexIn ( text, l + 1 );
									if ( l >= 0 )
										setFormat ( l, number, *m_HighlightFormat );
								}
							}
						}
					}
				}
			}
		}
	}
}
