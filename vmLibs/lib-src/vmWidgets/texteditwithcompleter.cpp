#include "texteditwithcompleter.h"
#include "heapmanager.h"

#include "vmwidgets.h"
#include "spellcheck.h"

#include <vmNumbers/vmnumberformats.h>

#include <QtGui/QTextCursor>
#include <QtGui/QKeyEvent>
#include <QtGui/QFocusEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCompleter>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
//------------------------------------------------TEXT-EDIT-COMPLETER--------------------------------------------

//static const QString eow                ( "~!@#$%^&*() _+{}|:\"<>?,./;'[]\\-=" ); // end of word
static const QString eow ( QStringLiteral ( "~!@#$%^&*() _+{}|:\"<>?,./;'[]\\=" ) ); // end of word

textEditWithCompleter::textEditWithCompleter ( QWidget* parent )
	: QTextEdit ( parent ), vmWidget ( WT_TEXTEDIT ),
	  mFirstInsertedActionPos ( 0 ), mCursorPos ( -1 ), mbDocumentModified ( false ),
	  mCompleter ( nullptr ), m_highlighter ( nullptr ), m_searchPanel ( nullptr ), mContextMenu ( nullptr )
{
	setWidgetPtr ( static_cast<QWidget*> ( this ) );
	setAcceptRichText ( true );

	/* When print previewing, Qt makes a copy of QTextEdit (not textEditWithCompleter) and certain features of this class are, therefore,
	   lost. The object's properties, though, are carried with the copy and, if we deference the QTextEdit pointer Qt uses, we can extract all of them,
	   and get a value that will have a special meaning. Sometimes we need to check the Highligher's property as well change won't have any effect
	   maybe because Qt copies the pointer before we have the chance to alter its name.
	 */
	document ()->setProperty ( PROPERTY_PRINT_PREVIEW, false );

	mSpellChecker = new spellCheck;
	m_highlighter = new wordHighlighter ( this->document (), mSpellChecker );
	m_highlighter->setInPreview ( false );
	m_highlighter->enableSpellChecking ( true );
	
	misspelledWordsActs[0] = new QAction ( this );
	misspelledWordsActs[0]->setVisible ( false );
	misspelledWordsActs[0]->setSeparator ( true );
	misspelledWordsActs[1] = new QAction ( this );
	static_cast<void>( connect ( misspelledWordsActs[1], &QAction::triggered, this, [&] () { addWord (); } ));
	misspelledWordsActs[1]->setVisible ( false );
	misspelledWordsActs[2] = new QAction ( this );
	static_cast<void>( connect ( misspelledWordsActs[2], &QAction::triggered, this, [&] () { ignoreWord (); } ));
	misspelledWordsActs[2]->setVisible ( false );
	QAction* action ( nullptr );
	for  ( uint i ( 3 ); i < WRONG_WORDS_MENUS; ++i )
	{
		misspelledWordsActs[i] = new QAction ( this );
		misspelledWordsActs[i]->setVisible ( false );
		action = misspelledWordsActs[i];
		static_cast<void>( connect ( action, &QAction::triggered, this, [&, action] ( const bool ) { correctWord ( action ); } ));
	}

	createContextMenu ();
	newest_edited_text.reserve ( 20000 );
	static_cast<void>( connect ( document (), &QTextDocument::modificationChanged, this, [&] ( const bool bChanged ) { mbDocumentModified = bChanged; } ) );
}

textEditWithCompleter::~textEditWithCompleter ()
{
	for ( QAction* act : misspelledWordsActs )
		heap_del ( act );
	//for  ( uint i = 0; i < WRONG_WORDS_MENUS; ++i )
	//	heap_del ( misspelledWordsActs[i] );
	heap_del ( mContextMenu );
	heap_del ( m_searchPanel );
	heap_del ( m_highlighter );
	heap_del ( mSpellChecker );
}

void textEditWithCompleter::setEditable ( const bool editable )
{
	m_highlighter->enableSpellChecking ( editable );
	setReadOnly ( !editable );
	vmWidget::setEditable ( editable );
}

void textEditWithCompleter::setPreview ( const bool preview )
{
	document ()->setProperty ( PROPERTY_PRINT_PREVIEW, preview );
	m_highlighter->setInPreview ( preview );
	m_highlighter->enableSpellChecking ( !preview );
	m_highlighter->enableHighlighting ( !preview );
	// printing and print previewing set document () state to modified by default. We need to overrule
	// this behaviour. This func must be called before asking Qt to print or preview
	if ( preview )
		document ()->setProperty ( PROPERTY_DOC_MODIFIED, document ()->isModified () );
	else // return state to non-modified only if it was not modfied before starting the process
		document ()->setModified ( document ()->property ( PROPERTY_DOC_MODIFIED ).toBool () );
}

void textEditWithCompleter::showhideUtilityPanel ()
{
	if ( !m_searchPanel )
	{
		m_searchPanel = new searchWordPanel ( this );
		addUtilityPanel ( m_searchPanel );
	}
	static_cast<void>( toggleUtilityPanel ( 0 ) );
}

void textEditWithCompleter::createContextMenu ()
{
	mContextMenu = createStandardContextMenu ();
	mContextMenu->addSeparator ();
	mContextMenu->addMenu ( mSpellChecker->menuAvailableDicts () );
	mSpellChecker->setCallbackForMenuEntrySelected ( [&] ( const bool b_spell_enabled ) {
		m_highlighter->enableSpellChecking ( b_spell_enabled ); } );
	mContextMenu->addSeparator ();
	auto findAction ( new QAction ( TR_FUNC ( "Find... (CTRL+F)" ), nullptr ) );
	static_cast<void>( connect ( findAction, &QAction::triggered, this, [&] ( const bool ) { showhideUtilityPanel (); } ) );
	mContextMenu->addAction ( findAction );
	mContextMenu->addSeparator ();
}

void textEditWithCompleter::insertCustomActionToContextMenu ( QAction* action )
{
	if ( mFirstInsertedActionPos == 0 )
		mFirstInsertedActionPos = mContextMenu->actions ().count ();
	mContextMenu->addAction ( action );
}

void textEditWithCompleter::saveContents ( const bool b_force, const bool b_notify )
{
	if ( mbDocumentModified || b_force )
	{
		newest_edited_text = document ()->toPlainText ();
		mbDocumentModified = false;
		if ( contentsAltered_func && b_notify )
			contentsAltered_func ( this );
	}
}

QString textEditWithCompleter::paragraphText () const
{
	return textCursor ().block ().text ();
}

void textEditWithCompleter::setText ( const QString& text, const bool b_notify )
{
	QTextEdit::setPlainText ( newest_edited_text = text );
	mbDocumentModified = false;
	if ( b_notify && contentsAltered_func )
		contentsAltered_func ( this );
}

void textEditWithCompleter::replaceWord ( const int cursor_pos, const QString& word )
{
	QTextCursor cursor ( textCursor () );
	cursor.setPosition ( cursor_pos, QTextCursor::MoveAnchor );
	cursor.movePosition ( QTextCursor::NextCharacter, QTextCursor::KeepAnchor, mSearchTerm.count () );
	cursor.insertText ( word );
	setTextCursor ( cursor );
}

bool textEditWithCompleter::selectFound ( const int cursor_pos )
{
	if ( cursor_pos >= 0 )
	{
		highlight ( vmBlue, mSearchTerm );
		QTextCursor cursor = textCursor ();
		cursor.setPosition ( mCursorPos = cursor_pos, QTextCursor::MoveAnchor );
		cursor.movePosition ( QTextCursor::NextCharacter, QTextCursor::KeepAnchor, mSearchTerm.count () );
		setTextCursor ( cursor );
		return true;
	}
	return false;
}

bool textEditWithCompleter::searchStart ( const QString& searchTerm )
{
	if ( searchTerm.count () >= 2 && searchTerm != mSearchTerm )
	{
		searchCancel ();
		mSearchTerm = searchTerm;
		return searchFirst ();
	}
	return false;
}

void textEditWithCompleter::searchCancel ()
{
	mCursorPos = -1;
	highlight ( vmDefault_Color );
}

bool textEditWithCompleter::searchFirst ()
{
	//QTextCursor cursor ( textCursor () );
	//cursor.setPosition ( 0 );
	//cursor.movePosition ( QTextCursor::Start, QTextCursor::KeepAnchor, mSearchTerm.count () );
	//setTextCursor ( cursor );
	return selectFound ( newest_edited_text.indexOf ( mSearchTerm, 0, Qt::CaseInsensitive ) );
}

bool textEditWithCompleter::searchNext ()
{
	return selectFound ( newest_edited_text.indexOf ( mSearchTerm, mCursorPos + 1, Qt::CaseInsensitive ) );
}

bool textEditWithCompleter::searchPrev ()
{
	return selectFound ( newest_edited_text.lastIndexOf ( mSearchTerm, mCursorPos - 1, Qt::CaseInsensitive ) );
}

bool textEditWithCompleter::searchLast ()
{
	mCursorPos = newest_edited_text.length () - 1;
	QTextCursor cursor ( textCursor () );
	cursor.setPosition ( mCursorPos );
	setTextCursor ( cursor );
	return searchPrev ();
}

QString textEditWithCompleter::defaultStyleSheet () const
{
	QString colorstr;
	if ( !parentWidget () )
		colorstr = QStringLiteral ( " ( 255, 255, 255 ) }" );
	else
	{
		const auto edt ( new QTextEdit ( parentWidget () ) );
		colorstr = QLatin1String ( " ( " ) + edt->palette ().color ( edt->backgroundRole () ).name () + QLatin1String ( " ) }" );
		delete edt;
	}
	return ( QLatin1String ( "QTextEdit { background-color: hex" ) + colorstr );
}

void textEditWithCompleter::highlight ( const VMColors wm_color, const QString& str )
{
	//vmWidget::highlight ( wm_color ); // maybe not needed and/or a stupid idea. Test to see if should be removed
	if ( !str.isEmpty () && wm_color != vmDefault_Color )
	{
		mHighlightedWord = str;
		m_highlighter->enableHighlighting ( true );
		m_highlighter->highlightWord ( str );
	}
	else
	{
		m_highlighter->unHighlightWord ( mHighlightedWord );
		mHighlightedWord.clear ();
		m_highlighter->enableHighlighting ( false );
	}
}

void textEditWithCompleter::keyPressEvent ( QKeyEvent* e )
{
	if ( !isEditable () )
	{
		e->accept (); // do not propagate
		if ( e->modifiers () & Qt::ControlModifier )
		{
			if ( e->key () == Qt::Key_F )
				showhideUtilityPanel ();
		}
		return;
	}

	if ( e->modifiers () & Qt::ControlModifier )
	{
		switch ( e->key () )
		{
			default:
				e->ignore ();
				QTextEdit::keyPressEvent ( e );
			return;
			case Qt::Key_Left:
				moveCursor ( QTextCursor::StartOfWord );
			break;
			case Qt::Key_Right:
				moveCursor ( QTextCursor::EndOfWord );
			break;
			case Qt::Key_Enter:
			case Qt::Key_Return:
				if ( keypressed_func )
					keypressed_func ( e, this );
			break;
			case Qt::Key_F:
				showhideUtilityPanel ();
			break;
			case Qt::Key_S:
				// Commit the latest text to this control's buffers. Qt might not be doing it right when we want it.
				saveContents ( false );
				if ( keypressed_func )
					keypressed_func ( e, this );
			break;
		}
		e->accept ();
	}
	else
	{
		if ( mCompleter->popup ()->isVisible () )
		{
			switch ( e->key () )
			{
				default:
					if ( e->key () >= Qt::Key_Dead_Grave && e->key () <= Qt::Key_Dead_Horn )
					{
						mCompleter->popup ()->hide ();
						e->accept ();
						return;
					}
				break;
						// The following keys are forwarded by the completer to the widget
				case Qt::Key_Enter:
				case Qt::Key_Return:
				case Qt::Key_Escape:
				case Qt::Key_Tab:
				case Qt::Key_Backtab:
					e->ignore ();
				return; // let the completer do its default behavior
				case Qt::Key_Shift:
				case Qt::Key_Control:
				case Qt::Key_Alt:
				case Qt::Key_Meta:
					mCompleter->popup ()->hide ();
					e->accept ();
				return;
			}
		}
	}

	QTextEdit::keyPressEvent ( e );

	const QString completionPrefix ( textUnderCursor () );
	const bool hasModifier ( e->modifiers () != Qt::NoModifier );
	if  ( hasModifier || e->text().isEmpty () || completionPrefix.length () < 3 || eow.contains ( e->text ().right ( 1 ) ) )
	{
		mCompleter->popup ()->hide ();
		return;
	}

	if  ( completionPrefix != mCompleter->completionPrefix () )
	{
		mCompleter->setCompletionPrefix ( completionPrefix );
		mCompleter->popup ()->setCurrentIndex ( mCompleter->completionModel ()->index ( 0, 0 ) );
	}

	QRect cr ( cursorRect () );
	cr.setWidth ( mCompleter->popup ()->sizeHintForColumn ( 0 ) + mCompleter->popup ()->verticalScrollBar ()->sizeHint ().width () );
	mCompleter->complete ( cr );
}

void textEditWithCompleter::focusInEvent ( QFocusEvent* e )
{
	if ( !isReadOnly () )
	{
		if  ( mCompleter )
			mCompleter->setWidget ( this );
		e->setAccepted ( true ); // do not propagate
		QTextEdit::focusInEvent ( e );
	}
	else
		e->setAccepted ( false );
}

void textEditWithCompleter::focusOutEvent ( QFocusEvent* e )
{
	if ( !isReadOnly () )
	{
		saveContents ( false );
		e->setAccepted ( true ); // do not propagate
		QTextEdit::focusOutEvent ( e );
	}
	else
		e->setAccepted ( false );
}

void textEditWithCompleter::mousePressEvent ( QMouseEvent* e )
{
	if ( !isReadOnly () )
	{
		// Allow for right clicks to change this->textCursor to the current word so that when the mouseReleaseEvent
		// it emits the signal contextMenuEvent with the word under the cursor being the word the user intended to get some information about
		// ( most likely a misspelled word ). If I don't do this, and right click will not reflect the context under the menu (cursor) but the
		// context of the document ()'s caret
		if ( e->button () == Qt::RightButton )
		{
			auto e_press ( new QMouseEvent ( e->type (), e->pos (), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier ) );
			e_press->setAccepted ( true );
			QTextEdit::mousePressEvent ( e_press );

			auto e_release ( new QMouseEvent ( QMouseEvent::MouseButtonRelease, e->pos (), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier ) );
			e_release->setAccepted ( true );
			QTextEdit::mouseReleaseEvent ( e_release );

			delete e_press;
			delete e_release;
		}
		e->setAccepted ( true );
	}
	QTextEdit::mousePressEvent ( e );
}

void textEditWithCompleter::contextMenuEvent ( QContextMenuEvent* e )
{
	if ( isReadOnly () )
	{
		QTextEdit::contextMenuEvent ( e );
		return;
	}

	if ( mFirstInsertedActionPos != 0 )
	{
		int n ( mContextMenu->actions ().count () - 1 );
		vmAction* action ( nullptr );
		bool enableWhenReadonly ( false );

		do
		{
			action = dynamic_cast<vmAction*>( mContextMenu->actions ().at ( n ) );
			if ( action != nullptr )
			{
				enableWhenReadonly = action->data ().toBool ();
				action->setEnabled ( enableWhenReadonly || !isReadOnly () );
			}
		} while ( --n >= mFirstInsertedActionPos );
	}
	lastPos = e->pos ();

	QTextCursor cursor ( textCursor () );
	cursor.select ( QTextCursor::WordUnderCursor );
	m_wordUnderCursor = cursor.selectedText ();

	QStringList list_words;
	int i ( 0 );
	if ( mSpellChecker->suggestionsList ( m_wordUnderCursor, list_words ) )
	{
		misspelledWordsActs[1]->setText ( QString ( TR_FUNC ( "Add  ( %1 ) " ) ).arg ( m_wordUnderCursor ) );
		misspelledWordsActs[2]->setText ( QString ( TR_FUNC ( "Ignore  ( %1 ) " ) ).arg ( m_wordUnderCursor ) );
		for ( i = 0; i < qMin ( static_cast<int>(WRONG_WORDS_MENUS), list_words.size () + 3 ); ++i )
		{
			if ( i >= 3 )
				misspelledWordsActs[i]->setText ( list_words.at ( i - 3 ).trimmed () );
			misspelledWordsActs[i]->setVisible ( true );
			mContextMenu->addAction ( misspelledWordsActs[i] );
		}
	} // if  misspelled
	mContextMenu->exec ( e->globalPos () );

	// Remove misspelled words menus because the next context menu might be over a empty string or a known word, so that
	// these menus will not be needed
	for ( i = 0; i < qMin ( static_cast<int>(WRONG_WORDS_MENUS), list_words.size () + 3 ); ++i )
		mContextMenu->removeAction ( misspelledWordsActs[i] );
	e->setAccepted ( true );
}

QString textEditWithCompleter::textUnderCursor () const
{
	QTextCursor tc ( textCursor () );
	tc.select ( QTextCursor::WordUnderCursor );
	return tc.selectedText ();
}

void textEditWithCompleter::setCompleter ( QCompleter* const completer )
{
	mCompleter = completer;
	if ( mCompleter != nullptr )
	{
		mCompleter->setWidget ( this );
		static_cast<void>( connect ( mCompleter, static_cast<void (QCompleter::*)( const QString& )>( &QCompleter::activated ),
			this, [&, this] ( const QString& text ) { insertCompletion ( text ); } ) );
	}
}

// complete word, capitilized or not, depending on the category to which it pertains and
// its position in the paragraph
void textEditWithCompleter::insertCompletion ( const QString& completion )
{
	QTextCursor tc ( textCursor () );
	tc.movePosition ( QTextCursor::StartOfWord );

	QString completion_copy ( completion );
	/*switch ( APP_COMPLETERS ()->completerType ( completer, completion ) )
	{
		default:
		break;
		case vmCompleters::DELIVERY_METHOD:
		case vmCompleters::ACCOUNT:
		case vmCompleters::JOB_TYPE:
		case vmCompleters::STOCK_TYPE:
		case vmCompleters::STOCK_PLACE:
		case vmCompleters::PAYMENT_METHOD:
		case vmCompleters::ITEM_NAMES:
		{*/
			bool b_can_change ( true );
			bool b_is_space ( false );
			const QString str ( tc.block ().text () );
			int pos ( tc.positionInBlock () - 1 );

			if ( pos >= 0 && pos < str.count () )
			{
				QChar chr;
				const QChar chrExclamationPoint ( QLatin1Char ( '!' ) );
				do
				{
					chr = str.at ( pos );
					if ( chr == chrExclamationPoint || chr == CHR_DOT || chr == CHR_QUESTION_MARK )
					{
						b_can_change = false;
						break;
					}
					if ( chr.isSpace () )
						b_is_space = true;
					else
					{
						// reached another word; now we know we have not completed the first word in a sentence
						if ( b_is_space )
							break;
					}
					--pos;
				} while ( pos > 0 );
			}
			else
				b_can_change = false;

			if ( b_can_change )
			{
				completion_copy = completion_copy.toLower ();
			}
		//}
	//}
	tc.movePosition ( QTextCursor::EndOfWord, QTextCursor::KeepAnchor );
	tc.insertText ( completion_copy );
	setTextCursor ( tc );
}

void textEditWithCompleter::correctWord ( const QAction* action )
{
	if ( action )
	{
		QTextCursor cursor ( cursorForPosition ( lastPos ) );
		cursor.select ( QTextCursor::WordUnderCursor );
		cursor.deleteChar ();
		cursor.insertText ( action->text () );
	}
}

void textEditWithCompleter::addWord ()
{
	mSpellChecker->addWord ( m_wordUnderCursor, true );
	m_highlighter->rehighlight ();
}

void textEditWithCompleter::ignoreWord ()
{
	mSpellChecker->addWord ( m_wordUnderCursor, false );
	m_highlighter->rehighlight ();
}
//------------------------------------------------TEXT-EDIT-COMPLETER--------------------------------------------

//------------------------------------------------FIND-WORD-PANEL--------------------------------------------
#define HTML_BOLD_ITALIC_UNDERLINE_11 "<span style=\" font-size:11pt;text-decoration:underline;font-style:italic;font-weight:600;\">%</span>"

searchWordPanel::searchWordPanel ( textEditWithCompleter* tewc )
	: QFrame ( nullptr ), m_texteditor ( tewc ),
	  mLayout ( nullptr ), btnSearchNext ( nullptr ), btnSearchPrev ( nullptr ),
	  btnRplcWord ( nullptr ), btnRplcAll ( nullptr ), btnClose ( nullptr ),
	  searchField ( nullptr ), replaceField ( nullptr )
{
	setFrameStyle ( QFrame::Raised | QFrame::StyledPanel );

	searchField = new vmLineEdit;
	searchField->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		searchField_textAltered ( sender->text () ); } );
	searchField->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
		searchField_keyPressed ( ke ); } );
	searchField->setEditable ( true );

	btnSearchStart = new QToolButton;
	btnSearchStart->setEnabled ( false );
	btnSearchStart->setIcon ( ICON ( "search" ) );
	static_cast<void>( connect ( btnSearchStart, &QToolButton::clicked, this, [&] () { searchStart (); } ) );

	btnSearchPrev = new QToolButton;
	btnSearchPrev->setEnabled ( false );
	btnSearchPrev->setToolTip ( QString ( tr ( "Find previous word" ) + QLatin1String ( HTML_BOLD_ITALIC_UNDERLINE_11 ) ).replace ( CHR_PERCENT, QLatin1String ( "F2" ) ) );
	btnSearchPrev->setIcon ( ICON ( "arrow-left" ) );
	static_cast<void>( connect ( btnSearchPrev, &QToolButton::clicked, this, [&] () { return searchPrev (); } ) );

	btnSearchNext = new QToolButton;
	btnSearchNext->setEnabled ( false );
	btnSearchNext->setToolTip ( QString ( tr ( "Find next word" ) + QLatin1String ( HTML_BOLD_ITALIC_UNDERLINE_11 ) ).replace ( CHR_PERCENT, QLatin1String ( "F3" ) ) );
	btnSearchNext->setIcon ( ICON ( "arrow-right" ) );
	static_cast<void>( connect ( btnSearchNext, &QToolButton::clicked, this, [&] () { return searchNext (); } ) );

	btnClose = new QToolButton;
	btnClose->setIcon ( ICON ( "cancel" ) );
	static_cast<void>( connect ( btnClose, &QToolButton::clicked, this, [&] () { searchCancel ( false ); } ) );

	replaceField = new vmLineEdit;
	replaceField->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		replaceField_textAltered ( sender->text () ); } );
	replaceField->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
		replaceField_keyPressed ( ke ); } );

	btnRplcWord = new QToolButton;
	btnRplcWord->setEnabled ( false );
	btnRplcWord->setIcon ( ICON ( "find-service" ) );
	btnRplcWord->setToolTip ( QString ( tr ( "Replace currently selected occurence" ) + QLatin1String ( HTML_BOLD_ITALIC_UNDERLINE_11 ) ).replace ( CHR_PERCENT, QLatin1String ( "F4" ) ) );
	static_cast<void>( connect ( btnRplcWord, &QToolButton::clicked, this, [&] () { replaceWord (); } ) );

	btnRplcAll = new QToolButton;
	btnRplcAll->setEnabled ( false );
	btnRplcAll->setIcon ( ICON ( "replace-all" ) );
	btnRplcAll->setToolTip ( QString ( tr ( "Replace all highlighted occurences" )  + QLatin1String ( HTML_BOLD_ITALIC_UNDERLINE_11 ) ).replace ( CHR_PERCENT, QLatin1String ( "F5" ) ) );
	static_cast<void>( connect ( btnRplcAll, &QToolButton::clicked, this, [&] () { return rplcAll (); } ) );

	mLayout = new QGridLayout;
	mLayout->addWidget ( new QLabel ( tr ( "Search: " ) ), 0, 0 );
	mLayout->addWidget ( searchField, 0, 1 );
	mLayout->addWidget ( btnSearchStart, 0, 3 );
	mLayout->addWidget ( btnSearchPrev, 0, 4 );
	mLayout->addWidget ( btnSearchNext, 0, 5 );
	mLayout->addWidget ( new QWidget, 0, 6 );
	mLayout->addWidget ( btnClose, 0, 7 );

	mLayout->addWidget ( new QLabel ( tr ( "Replace: " ) ), 1, 0 );
	mLayout->addWidget ( replaceField, 1, 1 );
	mLayout->addWidget ( btnRplcWord, 1, 3 );
	mLayout->addWidget ( btnRplcAll, 1, 4 );

	mLayout->setColumnStretch ( 1, 1 );
	mLayout->setVerticalSpacing ( 3 );
	mLayout->setHorizontalSpacing ( 3 );

	setLayout ( mLayout );

	setMinimumSize ( 250, 60 );
	setMaximumHeight ( sizeHint ().height () );
	if ( tewc && tewc->layout () != nullptr )
	{
		auto bLayout ( dynamic_cast<QVBoxLayout*>( m_texteditor->layout () ) );
		bLayout->insertWidget ( 0, this );
	}
}

void searchWordPanel::showPanel ()
{
	show ();
	searchField->setFocus ();
}

bool searchWordPanel::searchStart ()
{
	const bool b_result ( m_texteditor->searchStart ( searchField->text () ) );
	if ( b_result )
	{
		searchField->setCurrentText ( searchField->text () );
		btnSearchStart->setEnabled ( !b_result );
		btnSearchNext->setEnabled ( b_result );
		btnSearchPrev->setEnabled ( false );
		replaceField->setEnabled ( b_result );
		btnRplcWord->setEnabled ( b_result & !m_texteditor->isReadOnly () );
		btnRplcAll->setEnabled ( btnRplcWord->isEnabled () );
	}
	return b_result;
}

void searchWordPanel::searchCancel ( const bool b_reset_search )
{
	m_texteditor->searchCancel ();
	btnSearchNext->setEnabled ( false );
	btnSearchPrev->setEnabled ( false );
	btnRplcWord->setEnabled ( false );
	btnRplcAll->setEnabled ( false );
	if ( !b_reset_search )
	{
		searchField->clear ();
		hide ();
		m_texteditor->setFocus ();
	}
}

bool searchWordPanel::searchNext ()
{
	const bool b_result ( m_texteditor->searchNext () );
	btnSearchNext->setEnabled ( b_result );
	btnSearchPrev->setEnabled ( btnSearchPrev->isEnabled () | b_result );
	btnRplcWord->setEnabled ( b_result );
	btnRplcAll->setEnabled ( b_result );
	return b_result;
}

bool searchWordPanel::searchPrev ()
{
	const bool b_result ( m_texteditor->searchPrev () );
	btnSearchNext->setEnabled ( btnSearchNext->isEnabled () | b_result );
	btnSearchPrev->setEnabled ( b_result );
	btnRplcWord->setEnabled ( b_result );
	btnRplcAll->setEnabled ( b_result );
	return b_result;
}

void searchWordPanel::replaceWord ()
{
	if ( btnRplcWord->isEnabled () )
		m_texteditor->replaceWord ( m_texteditor->searchTerm ().toInt (), replaceField->text () );
}

void searchWordPanel::rplcAll ()
{
	do {
		replaceWord ();
	} while ( searchNext () );
	btnRplcWord->setEnabled ( false );
	btnRplcAll->setEnabled ( false );
}

void searchWordPanel::searchField_textAltered ( const QString& text )
{
	if ( text.length () >= 2 )
	{
		btnSearchStart->setEnabled ( text != m_texteditor->searchTerm () );
		btnSearchPrev->setEnabled ( false );
		btnSearchNext->setEnabled ( false );
	}
	else
	{
		if ( text.isEmpty () )
			searchCancel ( true );
	}
}

void searchWordPanel::searchField_keyPressed ( const QKeyEvent *ke )
{
	switch  ( ke->key () )
	{
		case Qt::Key_Return:
		case Qt::Key_Enter:
			searchStart ();
		break;
		case Qt::Key_Escape:
			if ( searchField->text () != m_texteditor->searchTerm () )
			{
				searchField->setText ( m_texteditor->searchTerm () );
				btnSearchStart->setEnabled ( !searchField->text ().isEmpty () );
			}
			else
				searchCancel ( false );
		break;
		case Qt::Key_F3:
			searchNext ();
		break;
		case Qt::Key_F2:
			searchPrev ();
		break;
		default:
		break;
	}
}

void searchWordPanel::replaceField_textAltered ( const QString& )
{
	btnRplcWord->setEnabled ( true );
	btnSearchPrev->setEnabled ( false );
	btnSearchNext->setEnabled ( false );
}

void searchWordPanel::replaceField_keyPressed ( const QKeyEvent *ke )
{
	switch  ( ke->key () )
	{
		case Qt::Key_Return:
		case Qt::Key_Enter:
			replaceWord ();
		break;
		case Qt::Key_F4:
			rplcAll ();
		break;
		default:
		break;
	}
}
//------------------------------------------------FIND-WORD-PANEL--------------------------------------------
