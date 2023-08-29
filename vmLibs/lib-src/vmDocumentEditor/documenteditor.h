#ifndef DOCUMENTEDITOR_H
#define DOCUMENTEDITOR_H

#include "documenteditorwindow.h"

#include <vmStringRecord/stringrecord.h>
#include <QtWidgets/QMainWindow>

class textEditor;
class reportGenerator;
class vmAction;
class vmCompleters;
class configOps;

class QToolBar;
class QTabWidget;
class QDockWidget;

class documentEditor : public QMainWindow
{

public:
	explicit documentEditor ( QWidget* parent = nullptr );
	virtual ~documentEditor () override;
	textEditor* startNewTextEditor ( textEditor* editor = nullptr );
	reportGenerator* startNewReport ( const bool b_windowless = false );
											
	void addDockWindow ( Qt::DockWidgetArea area, QDockWidget* dockwidget );
	void removeDockWindow ( QDockWidget* dockwidget );

	void updateMenus ( const int tab_index );
	void updateWindowMenu ();
	void openDocument ();
	void saveDocument ();
	void saveAsDocument ();
	void addToRecentFiles ( const QString& filename, const bool b_AddToList = true );
	void openRecentFile ( QAction* action );
	void makeWindowActive ( QAction* action );
	void changeTabText ( documentEditorWindow* window );
	void closeTab ( int tab_index = -1 );
	void closeAllTabs ();
	void activateNextTab ();
	void activatePreviousTab ();

	static void setCompleterManager ( vmCompleters* const completer );
	inline static vmCompleters* completerManager () { return documentEditor::completer_manager; }

	void setCallbackForEditorWindowClosed ( const std::function<void()>& func ) {
		editorWindowClosed_func = func;
	}

protected:
	void closeEvent ( QCloseEvent* ) override;
	
private:
	void openDocument ( const QString& filename );
	void createActions ();
	void createMenus ();
	void createToolBars ();
	void createStatusBar ();

	inline documentEditorWindow* activeDocumentWindow () const
	{
		return static_cast<documentEditorWindow*>( tabDocuments->currentWidget () );
	}
	
	void resizeViewPort ( documentEditorWindow* window );
	uint openByFileType ( const QString& filename );

	QTabWidget* tabDocuments;
	QMenu* fileMenu;
	QMenu* editMenu;
	QMenu* windowMenu;
	QMenu* recentFilesSubMenu;
	QToolBar* fileToolBar;
	QToolBar* editToolBar;
	vmAction* newAct, *newReportAct;
	vmAction* openAct;
	vmAction* saveAct;
	vmAction* saveAsAct;
	vmAction* cutAct;
	vmAction* copyAct;
	vmAction* pasteAct;
	vmAction* closeAct;
	vmAction* closeAllAct;
	vmAction* hideAct;
	vmAction* nextAct;
	vmAction* previousAct;
	vmAction* separatorAct;
	vmAction* calcAct;

	bool mb_ClosingAllTabs;
	stringRecord recentFilesList;

	configOps* m_config;

	static vmCompleters* completer_manager;
	std::function<void()> editorWindowClosed_func;
};

#endif // DOCUMENTEDITOR_H
