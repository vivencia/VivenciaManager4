#include "documenteditorwindow.h"
#include "vmlibs.h"
#include "documenteditor.h"
#include "heapmanager.h"

#include <vmNotify/vmnotify.h>
#include <vmWidgets/texteditwithcompleter.h>

#include <QtGui/QCloseEvent>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QVBoxLayout>

documentEditorWindow::documentEditorWindow ( documentEditor *parent )
	: QWidget (), mainLayout ( nullptr ), m_type ( 0 ), mb_untitled ( true ),
	  mb_programModification ( false ), mb_modified ( false ), mb_HasUndo ( false ),
	  mb_HasRedo ( false ), mb_inPreview ( false ), m_parentEditor ( parent ), documentModified_func ( nullptr )
{
	setAttribute ( Qt::WA_DeleteOnClose );

	mainLayout = new QVBoxLayout;
	mainLayout->setContentsMargins ( 1, 1, 1, 1 );
	mainLayout->setSpacing ( 1 );
	setLayout ( mainLayout );
}

documentEditorWindow::~documentEditorWindow ()
{
	heap_del ( mainLayout );
}

void documentEditorWindow::newFile ()
{
	canClose ();
	static uint sequenceNumber ( 0 );

	mb_untitled = true;
	mb_modified = false;

	curFile = tr ( "document%1" ).arg ( ++sequenceNumber );

	mb_programModification = true;
	createNew ();
	mb_programModification = false;
}

bool documentEditorWindow::load ( const QString& filename, const bool add_to_recent )
{
	if ( canClose () )
	{
		//TEST mb_programModification = true;
		mb_programModification = false;
		if ( loadFile ( filename ) )
		{
			//mb_programModification = false;
			setCurrentFile ( filename );
			if ( add_to_recent )
				parentEditor ()->addToRecentFiles ( filename );
			return true;
		}
		//mb_programModification = false;
	}
	return false;
}

bool documentEditorWindow::save ( const QString& filename )
{
	bool ret ( false );
	if ( mb_modified )
	{
		if ( !mb_untitled )
			ret = saveFile ( filename );
		else
		{
			parentEditor ()->saveAsDocument ();
			ret = !mb_untitled;
		}
		if ( ret )
			setCurrentFile ( filename );
	}
	return ret;
}

bool documentEditorWindow::saveas ( const QString& filename )
{
	const bool ret ( saveAs ( filename ) );
	if ( ret )
		setCurrentFile ( filename );
	return ret;
}

void documentEditorWindow::documentWasModified ( textEditWithCompleter* tec )
{
	if ( !mb_programModification )
	{
		/* When printing and previewing, we want to ignore the modified status. But a QTextEdit emits
		the documentModified signal more than once when previewing (don't know how many when printing only)
		and more than once it does it again when the preview dialog is closed. We need to count those calls
		and ignore them selectively. In my opinion Qt should quit with those annoying habits (set the modified flag to
		true when printing and calling it so many times).
		 */
		static int wasInPreview ( 0 );
		if ( inPreview () )
		{
			++wasInPreview;
			return;
		}
		if ( wasInPreview > 0 )
		{
			--wasInPreview;
			return;
		}
		if ( !mb_modified )
		{
			mb_modified = true;
			if ( documentModified_func )
				documentModified_func ( this );
		}

		/* Rationale for this piece of code: spellchecking highlight was not working in textEditor::textEditWithCompleter. It was, though, in other parts
		 * of the application. Maybe it is something in the program's code or maybe it has to do with some signals being intercepted by other objects in the chain
		 * QTextDocument->textEditWithCompleter->QWidget->documentEditorWindow->textEditor. If I tried to connect textEditWithCompleter::document ()::&QTextDocument::contentsChanged()
		 * within textEditWithCompleter itself I would not get any response if textEditWithCompleter was within a documentEditorWindow, even when disabling the connection to this slot.
		 * But anywhere else in the app it worked. Also, I did not need to connect that signal in order for the highlighting to work elsewhere.
		 * Finally, in order for it to work here I must block all signals from being emitted by textEditWithCompleter and QTextDocument.
		 * Otherwise, when QSyntaxHighlighter from wordHighlighter called rehighlight () it would trigger QTextDocument::contentsChanged() signal to be emitted and an infinite loop would ensue
		 * (the signal connect to this slot, mind you, not any slot from within textEditWithCompleter itself, which as explained above, never gets called)
		 */
		tec->blockSignals ( true );
		tec->document ()->blockSignals ( true );
		tec->enableSpellChecking ( true );
		tec->document ()->blockSignals ( false );
		tec->blockSignals( false );
	}
}

void documentEditorWindow::documentWasModifiedByUndo ( const bool undo )
{
	if ( !mb_programModification )
		mb_HasUndo = undo;
}

void documentEditorWindow::documentWasModifiedByRedo ( const bool redo )
{
	if ( !mb_programModification )
	{
		mb_HasRedo = redo;
		if ( redo && !mb_HasUndo )
		{
			mb_modified = false;
			if ( documentModified_func )
				documentModified_func ( this );
		}
	}
}

void documentEditorWindow::cut () {}
void documentEditorWindow::copy () {}
void documentEditorWindow::paste () {}
void documentEditorWindow::buildMailMessage ( QString&  /*address*/, QString&  /*subject*/, QString&  /*attachment*/, QString&  /*body*/ ) {}

const QString documentEditorWindow::initialDir () const
{
	return QString ();
}

void documentEditorWindow::setCurrentFile ( const QString& fileName )
{
	curFile = QFileInfo ( fileName ).canonicalFilePath ();
	mb_untitled = false;
	mb_modified = false;
	if ( documentModified_func )
		documentModified_func ( this );
}

bool documentEditorWindow::canClose ()
{
	bool b_can_close ( true );
	if ( mb_modified )
	{
		const QStringList btns ( QStringList () << APP_TR_FUNC ( "Save" ) << APP_TR_FUNC ( "Cancel" ) << APP_TR_FUNC ( "Don't Save" ) );
		const int btn (
			vmNotify::messageBox ( nullptr, strippedName ( curFile ) + tr ( " was modified" ),  tr ( "Do you want to save your changes?" ),
								  vmNotify::QUESTION, btns ) );

		switch ( btn )
		{
			case MESSAGE_BTN_OK: //0
				b_can_close = save ( curFile );
			break;
			case MESSAGE_BTN_CANCEL: //1
				b_can_close = false;
			break;
			case 2: // don't save
				b_can_close = true;
			break;
		}
	}
	if ( b_can_close )
		this->disconnect ();
	return b_can_close;
}
