#include "imageviewer.h"

#include <vmWidgets/vmwidgets.h>

#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QScrollArea>

/*inline static bool operator> ( const QSize& size1, const QSize& size2 )
{
	return ( size1.width () > size2.width () ) | ( size2.height () > size1.height () );
}*/

static const QStringList name_filters ( QStringList () << QStringLiteral ( ".jpg" ) << QStringLiteral ( ".png" ) );

imageViewer::imageViewer ( QWidget* parent, QGridLayout* parentLayout )
	: QLabel (), m_parent ( parent ), mb_maximized ( false ), images_array ( 10 ), funcImageRequested ( nullptr ), funcShowMaximized ( nullptr )
{
	images_array.setAutoDeleteItem ( true );

	mScrollArea = new QScrollArea ( parent );
	mScrollArea->setAutoFillBackground ( true );
	mScrollArea->setWidgetResizable ( false );
	mScrollArea->setBackgroundRole ( QPalette::Dark );
	mScrollArea->setAlignment ( Qt::AlignHCenter | Qt::AlignVCenter );
	mScrollArea->setWidget ( this );
	parentLayout->addWidget ( mScrollArea, 0, 0, 1, 1 );

	setFocusPolicy ( Qt::WheelFocus );
}

void imageViewer::prepareToShowImages ( const int rec_id, const QString& path )
{
	bool b_exists ( false );
	if ( rec_id != -1 )
	{
		if ( ( b_exists = findImagesByID ( rec_id ) ) )
		{
			if ( images_array.current ()->path != path )
				removeDir (); // in editing, the image path changed
		}
	}
	else if ( !path.isEmpty () )
		b_exists = findImagesByPath ( path );
	if ( !b_exists )
	{
		if ( !hookUpDir ( rec_id, path ) )
			images_array.setCurrent ( -1 );
	}
	mScrollArea->installEventFilter ( this ); //Display images in accordance to the available size for the viewer
}

const QString imageViewer::imageFileName () const
{
	RECORD_IMAGES* ri ( images_array.current () );
	if ( ri != nullptr )
	{
		if ( ri->files.currentIndex () >= 0 )
			return fileOps::filePathWithoutExtension ( ri->files.current ()->filename );
	}
	return emptyString;
}

const QString imageViewer::imageCompletePath () const
{
	RECORD_IMAGES* ri ( images_array.current () );
	if ( ri != nullptr )
	{
		fileOps::st_fileInfo* fi ( ri->files.current () );
		if ( fi != nullptr )
			return fi->fullpath;
	}
	return emptyString;
}

bool imageViewer::findImagesByID ( const int rec_id )
{
	for ( uint i ( 0 ); i < images_array.count (); ++i )
	{
		if ( images_array.at ( i )->rec_id == rec_id )
		{
			images_array.setCurrent ( static_cast<int>( i ) );
			return true;
		}
	}
	return false;
}

bool imageViewer::findImagesByPath ( const QString& path )
{
	for ( uint i ( 0 ); i < images_array.count (); ++i )
	{
		if ( images_array.at ( i )->path == path )
		{
			images_array.setCurrent ( static_cast<int>( i ) );
			return true;
		}
	}
	return false;
}

inline void imageViewer::reloadInternal ( RECORD_IMAGES* ri, const QString& path )
{
	const int list_size ( fileOps::fileCount ( path ) );
	if ( list_size <= 0 )
		return;
	ri->files.reserve (	static_cast<uint>( list_size ) );
	fileOps::lsDir ( ri->files, path, name_filters );
}

bool imageViewer::hookUpDir ( const int rec_id, const QString& path )
{
	if ( !fileOps::exists ( path ).isOn () )
		return false;

	auto ri ( new RECORD_IMAGES );
	ri->rec_id = ( rec_id != -1 ) ? rec_id : -2;
	ri->path = path;
	reloadInternal ( ri, path );
	images_array.setCurrent ( static_cast<int>( images_array.count () ) - 1 );
	images_array.append ( ri );
	return true;
}

void imageViewer::reload ( const QString& new_path )
{
	if ( images_array.current () )
	{
		images_array.current ()->files.clearButKeepMemory ();
		if ( !new_path.isEmpty () && new_path != images_array.current ()->path )
			hookUpDir ( -1, new_path );
		else
		{
			reloadInternal ( static_cast<RECORD_IMAGES*>( images_array.current () ),
						 static_cast<RECORD_IMAGES*>( images_array.current () )->path );
		}
		showFirstImage ();
	}
	else
		hookUpDir ( -1, new_path );
}

// called when deleting a record, when changing the dir path of a record or when changing a dir path for a single image
void imageViewer::removeDir ( const bool b_deleteview )
{
	if ( images_array.current () )
	{
		images_array.current ()->files.clear ();
		if ( b_deleteview )
			images_array.remove ( images_array.currentIndex (), true );
	}
}

void imageViewer::addImagesList ( vmComboBox* combo ) const
{
	if ( images_array.currentIndex () >= 0 )
	{
		const pointersList<fileOps::st_fileInfo*> &files ( images_array.current ()->files );
		for ( uint i ( 0 ); i < files.count (); ++i )
			combo->insertItem ( files.at ( i )->filename, -1, false );
	}
}

void imageViewer::loadImage ( const QString& picture_path )
{
	QPixmap pixture;
	pixture.load ( fileOps::canRead ( picture_path ).isOn () ? picture_path : QStringLiteral ( ":/resources/no_image.jpg" ) );
	loadImage ( pixture );
}

void imageViewer::loadImage ( QPixmap& picture )
{
	double factor ( 1.0 );
	const QSizeF imgSize ( picture.size () );
	const QSizeF maxSize ( m_parent->size () );

	if ( imgSize.width () > maxSize.width () )
		factor = maxSize.width () / imgSize.width ();

	if ( imgSize.height () > maxSize.height () )
	{
		const double factor2 ( maxSize.height () / imgSize.height () );
		if ( factor2 < factor )
			factor = factor2;
	}
	QSizeF new_imgSize ( factor * imgSize );
	setPixmap ( picture.scaled ( new_imgSize.toSize (), Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
	adjustSize ();
}

void imageViewer::showSpecificImage ( const int index )
{
	if ( index >= 0 && static_cast<uint>( index ) < images_array.current ()->files.count () )
	{
		images_array.current ()->files.setCurrent ( index );
		loadImage ( images_array.current ()->files.current ()->fullpath );
		funcImageRequested ( index );
		return;
	}
	loadImage ( emptyString );
}

int imageViewer::showFirstImage ()
{
	if ( images_array.currentIndex () >= 0 )
	{
		RECORD_IMAGES* ri ( images_array.current () );
		if ( !ri->files.isEmpty () )
		{
			loadImage ( ri->files.first ()->fullpath );
			funcImageRequested ( 0 );
			return 0;
		}
	}
	loadImage ( emptyString );
	return -1;
}

int imageViewer::showLastImage ()
{
	if ( images_array.currentIndex () >= 0 )
	{
		RECORD_IMAGES* ri ( images_array.current () );
		if ( !ri->files.isEmpty () )
		{
			loadImage ( ri->files.last ()->fullpath );
			funcImageRequested ( ri->files.currentIndex () );
			return ri->files.currentIndex ();
		}
	}
	loadImage ( emptyString );
	return -1;
}

int imageViewer::showPrevImage ()
{
	if ( images_array.currentIndex () >= 0 )
	{
		RECORD_IMAGES* ri ( images_array.current () );
		if ( ri->files.currentIndex () > 0 )
		{
			loadImage ( ri->files.prev ()->fullpath );
			funcImageRequested ( ri->files.currentIndex () );
			return ri->files.currentIndex ();
		}
	}
	loadImage ( emptyString );
	return -1;
}

int imageViewer::showNextImage ()
{
	if ( images_array.currentIndex () >= 0 )
	{
		RECORD_IMAGES* ri ( images_array.current () );
		if ( ri->files.currentIndex () < static_cast<int>( ri->files.count () ) - 1 )
		{
			loadImage ( ri->files.next ()->fullpath );
			funcImageRequested ( ri->files.currentIndex () );
			return ri->files.currentIndex ();
		}
	}
	loadImage ( emptyString );
	return -1;
}

int imageViewer::rename ( const QString& newName )
{
	if ( images_array.currentIndex () >= 0 )
	{
		RECORD_IMAGES* ri ( images_array.current () );
		const QString currentImageName ( ri->files.current ()->filename );
		if ( newName != currentImageName )
		{
			QString new_fileName ( ri->path + newName );
			const QString ext ( CHR_DOT + fileOps::fileExtension ( currentImageName ) );
			if ( !newName.endsWith ( ext ) )
				new_fileName.append ( ext );

			if ( fileOps::rename ( currentImageName, new_fileName ).isOn () )
			{
				ri->files.current ()->fullpath = new_fileName;
				ri->files.current ()->filename = fileOps::fileNameWithoutPath ( new_fileName );
				return ri->files.currentIndex ();
			}
		}
	}
	return -1;
}

bool imageViewer::eventFilter ( QObject* o, QEvent* e )
{
	if ( o == mScrollArea )
	{
		if ( e->type () == QEvent::Resize )
		{
			loadImage ( images_array.current ()->files.current ()->fullpath );
			e->accept ();
			return true;
		}
		return false;
	}
	switch ( e->type () )
	{
		default:
			return false;
		case QEvent::KeyPress:
		{
			const auto k ( static_cast<QKeyEvent*>( e ) );
			switch  ( k->key () )
			{
				default:
					return false;
				case Qt::Key_Left:
				case Qt::Key_Down:
					showPrevImage ();
				break;
				case Qt::Key_Right:
				case Qt::Key_Up:
					showNextImage ();
				break;
				case Qt::Key_Home:
					showFirstImage ();
				break;
				case Qt::Key_End:
					showLastImage ();
				break;
				case Qt::Key_F:
					funcShowMaximized ( mb_maximized = !mb_maximized );
				break;
			}
		}
		break;
		case QEvent::MouseButtonDblClick:
			funcShowMaximized ( mb_maximized = !mb_maximized );
		break;
	}
	e->accept ();
	return true;
}
