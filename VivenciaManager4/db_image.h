#ifndef DB_IMAGE_H
#define DB_IMAGE_H

#include <vmlist.h>
#include <vmUtils/fileops.h>

#include <QtWidgets/QFrame>

#include <functional>

class vmComboBox;

class QLabel;
class QScrollArea;

class DB_Image final : public QFrame
{

public:
	explicit DB_Image ( QWidget* parent = nullptr );
	virtual ~DB_Image () final = default;

	void showImage ( const int rec_id = -1, const QString& path = QString () );

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

	void showSpecificImage ( const int index );
	int showPrevImage ();
	int showNextImage ();
	int showFirstImage ();
	int showLastImage ();
	int rename ( const QString& newName );

	void zoomIn ();
	void zoomOut ();
	void normalSize ();
	void fitToWindow ();
	inline void setMaximized ( const bool maximized ) {
		mb_maximized = maximized;
	}

	void reload ( const QString& new_path = QString () );
	void removeDir ( const bool b_deleteview = false );

protected:
	bool eventFilter ( QObject* o, QEvent* e ) override;

private:
	QLabel* imageViewer;
	QScrollArea* scrollArea;

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

	void loadImage ( const QPixmap& );
	void loadImage ( const QString& );
	void scrollBy ( const int x, const int y );
	void adjustScrollBar ( const double factor );
	void scaleImage ( const double factor );

	QStringList name_filters;
	int mouse_ex, mouse_ey;
	bool mb_fitToWindow, mb_maximized;
	double scaleFactor;
	pointersList <RECORD_IMAGES*> images_array;

	std::function<void ( const int )> funcImageRequested;
	std::function<void ( const bool )> funcShowMaximized;
};

#endif // DB_IMAGE_H
