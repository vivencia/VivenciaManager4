#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <vmlist.h>
#include "fileops.h"

#include <QtWidgets/QLabel>
#include <QtGui/QPixmap>

#include <functional>

class vmComboBox;
class QScrollArea;
class QGridLayout;

class imageViewer final : public QLabel
{

public:
	explicit imageViewer ( QWidget* parent, QGridLayout* parentLayout );
	virtual ~imageViewer () final = default;

	void prepareToShowImages ( const int rec_id = -1, const QString& path = QString () );

	const QString imageFileName () const;
	const QString imageCompletePath () const;

	inline const QString currentPath () const {
		return ( images_array.current () ? images_array.current ()->path : QString () );
	}
	inline void setID ( const int id ) {
		if ( images_array.current () ) images_array.current ()->rec_id = id;
	}

	inline void setCallbackForshowImageRequested ( const std::function<void ( const int )>& func ) {
		funcImageRequested = func;
	}

	inline void setCallbackForshowMaximized ( const std::function<void ( const bool )>& func ) {
		funcShowMaximized = func;
	}

	void addImagesList ( vmComboBox* combo ) const;

	void loadImage ( const QString& picture_path );
	void loadImage ( QPixmap& picture );
	void showSpecificImage ( const int index );
	int showPrevImage ();
	int showNextImage ();
	int showFirstImage ();
	int showLastImage ();
	int rename ( const QString& newName );

	inline void setMaximized ( const bool maximized ) {
		mb_maximized = maximized;
	}

	void reload ( const QString& new_path = QString () );
	void removeDir ( const bool b_deleteview = false );

protected:
	bool eventFilter ( QObject* o, QEvent* e ) override;

private:
	struct RECORD_IMAGES
	{
		pointersList<fileOps::st_fileInfo*> files;
		QString path;
		int rec_id;

		RECORD_IMAGES () : rec_id ( -1 ) { files.setAutoDeleteItem ( true ); }
	};

	bool findImagesByID ( const int rec_id );
	bool findImagesByPath ( const QString& path );
	void reloadInternal ( RECORD_IMAGES* ri, const QString& path );
	bool hookUpDir ( const int rec_id, const QString& path );

	QScrollArea* mScrollArea;
	QWidget* m_parent;
	bool mb_maximized;
	pointersList <RECORD_IMAGES*> images_array;

	std::function<void ( const int )> funcImageRequested;
	std::function<void ( const bool )> funcShowMaximized;
};

#endif // IMAGEVIEWER_H
