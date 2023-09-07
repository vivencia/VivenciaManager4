#ifndef TEXTEDITWITHCOMPLETER_H
#define TEXTEDITWITHCOMPLETER_H

#include "vmwidget.h"
#include "wordhighlighter.h"

#include <QtWidgets/QTextEdit>

class searchWordPanel;
class wordHighlighter;
class spellCheck;
class vmLineEdit;

class QCompleter;
class QGridLayout;
class QToolButton;
class QMenu;
class QAction;

//------------------------------------------------TEXT-EDIT-COMPLETER--------------------------------------------

const uint WRONG_WORDS_MENUS ( 8 );

class textEditWithCompleter : public QTextEdit, public vmWidget
{

friend class searchWordPanel;

public:
	explicit textEditWithCompleter ( QWidget* parent = nullptr );
	virtual ~textEditWithCompleter ();

	void setEditable ( const bool editable ) override;
	void setPreview ( const bool preview );
	inline bool inPreview () const
	{
		return document ()->property ( PROPERTY_PRINT_PREVIEW ).toBool ();
	}

	inline void enableSpellChecking ( const bool b_enable ) { m_highlighter->enableSpellChecking ( b_enable ); }

	void setCompleter ( QCompleter* const completer );
	inline spellCheck* spellChecker () const { return mSpellChecker; }

	void showhideUtilityPanel ();
	void createContextMenu ();
	void insertCustomActionToContextMenu ( QAction* action );

	void saveContents ( const bool b_force = false, const bool b_notify = true );
	QString paragraphText () const;
	void setText ( const QString& text, const bool b_notify = false ) override;
	inline const QString& currentText () const { return newest_edited_text; }
	inline const QString& searchTerm () const { return mSearchTerm; }
	
	void replaceWord ( const int cursor_pos, const QString& word );
	bool selectFound ( const int cursor_pos );
	bool searchStart ( const QString& searchTerm );
	void searchCancel ();
	bool searchFirst ();
	bool searchNext ();
	bool searchPrev ();
	bool searchLast ();

	inline QLatin1String qtClassName () const override { return QLatin1String ( "QTextEdit" ); }
	QString defaultStyleSheet () const override;
	void highlight ( const VMColors wm_color, const QString& str = QString () ) override;

protected:
	void keyPressEvent ( QKeyEvent* e ) override;
	void focusInEvent ( QFocusEvent* e ) override;
	void focusOutEvent ( QFocusEvent* e ) override;
	void mousePressEvent ( QMouseEvent* e ) override;
	void contextMenuEvent ( QContextMenuEvent* e ) override;

private:
	friend class documentEditorWindow;

	QString spell_dic;
	QString m_wordUnderCursor;
	QString mHighlightedWord;
	QString newest_edited_text;
	QPoint lastPos;
	QString mSearchTerm;
	int mFirstInsertedActionPos;
	int mCursorPos;
	bool mbDocumentModified;

	QAction* misspelledWordsActs[WRONG_WORDS_MENUS];
	QCompleter* mCompleter;
	spellCheck* mSpellChecker;
	wordHighlighter* m_highlighter;
	searchWordPanel* m_searchPanel;
	QMenu* mContextMenu;

	QString textUnderCursor () const;
	void insertCompletion ( const QString& completion );
	void correctWord ( const QAction* action );
	void addWord ();
	void ignoreWord ();
};
//------------------------------------------------TEXT-EDIT-COMPLETER--------------------------------------------

//------------------------------------------------FIND-WORD-PANEL--------------------------------------------

class searchWordPanel : public QFrame
{

friend class textEditWithCompleter;

public:
	explicit searchWordPanel ( textEditWithCompleter* );
	void showPanel ();

private:
	textEditWithCompleter *m_texteditor;
	QGridLayout* mLayout;

	QToolButton* btnSearchStart, *btnSearchNext, *btnSearchPrev, *btnRplcWord, *btnRplcAll;
	QToolButton* btnClose;
	vmLineEdit* searchField, *replaceField;

	bool searchStart ();
	void searchCancel ( const bool b_reset_search );
	bool searchNext ();
	bool searchPrev ();

	void replaceWord ();
	void rplcAll ();
	void searchField_textAltered ( const QString& text );
	void searchField_keyPressed ( const QKeyEvent* ke );
	void replaceField_textAltered (const QString& );
	void replaceField_keyPressed ( const QKeyEvent* ke );
};

//------------------------------------------------FIND-WORD-PANEL--------------------------------------------

#endif // TEXTEDITWITHCOMPLETER_H
