#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include "documenteditorwindow.h"

#include <QtCore/QMap>
#include <QtGui/QTextCursor>
#include <QtGui/QTextObjectInterface>
#include <QtWidgets/QDockWidget>

class textEditWithCompleter;
class vmComboBox;

class QPushButton;
class QSpinBox;
class QToolButton;
class QFontComboBox;
class QRadioButton;
class QDockWidget;
class QPrinter;

static const uint TEXT_EDITOR_SUB_WINDOW ( 2 );
static const uint RICH_TEXT_EDITOR_SUB_WINDOW ( 4 );

class textEditor : public documentEditorWindow
{

friend class textEditorToolBar;
friend class dockQP;
friend class dockBJ;

public:
	explicit textEditor ( documentEditor* mdiParent );
	virtual ~textEditor () override;

	static inline QString filter () {
		return tr ( "Text files (*.txt);;Richtext files (*.rtf)" );
	}

	void setEditable ( const bool b_editable );

	void cut () override;
	void copy () override;
	void paste () override;

	void createNew () override;
	bool loadFile ( const QString& filename ) override;
	bool displayText ( const QString& text ) override;
	bool saveAs ( const QString& filename ) override;
	bool saveFile ( const QString& filename ) override;

	inline bool useHtml () const { return mb_UseHtml; }
	inline void setUseHtml ( const bool html ) { mb_UseHtml = html; }
	inline bool ignoreCursorPos () const { return mb_IgnoreCursorPos; }
	inline void setIgnoreCursorPos ( const bool ignore ) { mb_IgnoreCursorPos = ignore; }
	const QString initialDir () const override;

protected:
	QTextCursor mCursor;
	textEditWithCompleter* mDocument;
	QString mailAddress, mailSubject;

private:
	QString mPDFName;
	bool mb_UseHtml;
	bool mb_IgnoreCursorPos;
	uint m_ImageNumber;

	QMap<QString,QString> mapImages;
};

class textEditorToolBar : public QDockWidget
{

friend class textEditor;
friend class reportGenerator;

public:
	textEditorToolBar ();
	virtual ~textEditorToolBar () override;
	void setCurrentWindow ( textEditor *ed );
	inline textEditor *currentWindow () const {
		return mEditorWindow;
	}

	void show ( documentEditor* mdiParent );

	void updateControls ();
	void checkAlignment ( const Qt::Alignment align );

	void btnCreateTable_clicked ();
	void btnRemoveTable_clicked ();
	void btnInsertTableRow_clicked ();
	void btnRemoveTableRow_clicked ();
	void btnInsertTableCol_clicked ();
	void btnRemoveTableCol_clicked ();
	void btnInsertBulletList_cliked ();
	void btnInsertNumberedList_clicked ();
	void btnInsertImage_clicked ();
	void btnBold_clicked ( const bool );
	void btnItalic_clicked ( const bool );
	void btnUnderline_clicked ( const bool );
	void btnStrikethrough_clicked ( const bool );
	void btnAlignLeft_clicked  ();
	void btnAlignRight_clicked  ();
	void btnAlignCenter_clicked  ();
	void btnAlignJustify_clicked  ();
	void btnTextColor_clicked ();
	void btnHighlightColor_clicked ();
	void btnPrint_clicked ();
	void btnPrintPreview_clicked ();
	void btnExportToPDF_clicked ();
	void previewPrint ( QPrinter* const printer );
	void preparePrinterDevice ( QPrinter* const printer );

	void setFontType ( const QString& type );
	void setFontSize ( const int size );
	void setFontSizeFromComboBox ( const int index );
	void insertImage ( const QString& imageFile, QTextFrameFormat::Position pos = QTextFrameFormat::InFlow );

	void removeList ( const int indent = 0 );
	
private:
	friend void initToolbarInstace ();
	static void initToolbarInstace ();
	void setFontColor ( const QColor& color );
	void setHighlight  ( const QColor& color );
	void createList ( const QTextListFormat::Style = QTextListFormat::ListDisc );
	void alignText ( const Qt::Alignment align = Qt::AlignLeft );

	QPushButton* btnCreateTable, *btnRemoveTable;
	QToolButton* btnInsertTableRow, *btnRemoveTableRow;
	QToolButton* btnInsertTableCol, *btnRemoveTableCol;
	QPushButton* btnInsertBulletList, *btnInsertNumberedList;
	QPushButton* btnInsertImage;

	QSpinBox *spinNRows, *spinNCols;

	QToolButton* btnBold, *btnItalic, *btnUnderline, *btnStrikethrough;
	QToolButton* btnAlignLeft, *btnAlignRight, *btnAlignCenter, *btnAlignJustify;
	QToolButton* btnTextColor, *btnHighlightColor;
	QToolButton* btnPrint, *btnPrintPreview, *btnExportToPDF;

	QFontComboBox *cboFontType;
	vmComboBox* cboFontSizes;

	void mergeFormatOnWordOrSelection ( const QTextCharFormat& format );
	static textEditorToolBar *s_instance;
	friend textEditorToolBar *TEXT_EDITOR_TOOLBAR ();
	textEditor *mEditorWindow;
};

class imageTextObject : public QObject, public QTextObjectInterface
{

Q_OBJECT
Q_INTERFACES ( QTextObjectInterface )

public:
	virtual ~imageTextObject () override;
	QSizeF intrinsicSize ( QTextDocument* /*doc*/, int posInDocument /*posInDocument*/, const QTextFormat& format ) override;
	void drawObject ( QPainter* painter, const QRectF &rect, QTextDocument* doc, int posInDocument, const QTextFormat &format ) override;
};

inline textEditorToolBar *TEXT_EDITOR_TOOLBAR ()
{
	return textEditorToolBar::s_instance;
}

#endif // TEXTEDITOR_H
