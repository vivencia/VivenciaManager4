#ifndef DOCUMENTEDITORWINDOW_H
#define DOCUMENTEDITORWINDOW_H

#include <QtCore/QFileInfo>
#include <QtWidgets/QWidget>

#include <functional>

class QVBoxLayout;
class documentEditor;

class documentEditorWindow : public QWidget
{

public:
	explicit documentEditorWindow ( documentEditor* parent );
	virtual ~documentEditorWindow () override;

	inline uint editorType () const { return m_type; }
	inline void setEditorType ( const uint type ) { m_type = type; }

	inline bool isUntitled () const { return mb_untitled; }
	inline void setIsUntitled ( const bool untitled ) { mb_untitled = untitled; }

	inline bool isModified () const { return mb_modified; }
	inline void setIsModified ( const bool modified ) { mb_modified = modified; }

	bool canClose ();
	void newFile ();
	bool save ( const QString& filename );
	bool load ( const QString& filename, const bool add_to_recent = false );
	bool saveas ( const QString& filename );

	inline documentEditor* parentEditor () const {
		return m_parentEditor;
	}

	static inline QString filter () {
		return tr ( "All files (*.*)" );
	}

	virtual void cut ();
	virtual void copy ();
	virtual void paste ();
	virtual void buildMailMessage ( QString&  /*address*/, QString&  /*subject*/, QString&  /*attachment*/, QString&  /*body*/ );
	virtual const QString initialDir () const;

	inline QString currentFile () const {
		return curFile;
	}
	void setCurrentFile ( const QString& fileName );

	inline QString title () const {
		return strippedName ( curFile ) + QLatin1String ( mb_modified ? "[*]" : "" ) ;
	}

	inline void setCallbackForDocumentModified ( const std::function<void ( documentEditorWindow* )>& func ) {
		documentModified_func = func;
	}


protected:
	QVBoxLayout* mainLayout;

	void documentWasModified ();
	void documentWasModifiedByUndo ( const bool );
	void documentWasModifiedByRedo ( const bool );
	
	//void closeEvent ( QCloseEvent* event );

	inline QString strippedName ( const QString& fullFileName ) const
	{
		return QFileInfo ( fullFileName ).fileName ();
	}

	virtual void createNew () = 0;
	virtual bool loadFile ( const QString& filename ) = 0;
	virtual bool saveAs ( const QString& filename ) = 0;
	virtual bool saveFile ( const QString& fileName ) = 0;
	virtual bool displayText ( const QString& text ) = 0;

	inline void setPreview ( const bool preview ) { mb_inPreview = preview; }
	inline bool inPreview () const { return mb_inPreview; }

private:
	uint m_type;
	QString curFile;
	QString mstr_Filter;
	bool mb_untitled;
	bool mb_programModification;
	bool mb_modified;
	bool mb_HasUndo, mb_HasRedo;
	bool mb_inPreview;
	documentEditor* m_parentEditor;
	
	std::function<void ( documentEditorWindow* )> documentModified_func;
};

#endif // DOCUMENTEDITORWINDOW_H
