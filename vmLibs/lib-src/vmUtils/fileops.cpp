#include "vmlibs.h"

#include <vmNotify/vmnotify.h>

#include <vmUtils/fileops.h>
#include <vmUtils/configops.h>
#include <vmUtils/fast_library_functions.h>

#include <QtWidgets/QFileDialog>
#include <QtCore/QFile>
#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>

#include <cstdio>
#include <cstdlib>
#include <ctime>

extern "C"
{
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#include <dirent.h>
}

const QString fileOps::appPath ( const QString& appname )
{
	const QString strPath ( QString::fromUtf8 ( ::getenv ( "PATH" ) ) );

	if ( !strPath.isEmpty () )
	{
		QString path;
		int idx1 = 0;
		int idx2 ( strPath.indexOf ( CHR_COLON ) );
		while ( idx2 != -1 )
		{
			path = strPath.mid ( idx1, idx2 - idx1 );
			path += CHR_F_SLASH + appname;
			if ( exists ( path ).isOn () )
				return path;

			idx1 = idx2 + 1;
			idx2 = strPath.indexOf ( CHR_COLON, idx2 + 1 );
		}
		path = strPath.mid ( idx1, strPath.length () - 1 );
		path += CHR_F_SLASH + appname;
		if ( exists ( path ).isOn () )
			return path;
	}
	return emptyString;
}

triStateType fileOps::exists ( const QString& file )
{
	if ( !file.isEmpty () )
	{
		struct stat stFileInfo {};
		return ( ::stat ( file.toLocal8Bit ().constData (), &stFileInfo ) == 0 ) ? TRI_ON : TRI_OFF;
	}
	return TRI_UNDEF;
}

const QString fileOps::currentUser ()
{
	return QString::fromUtf8  ( ::getenv ( "USER" ) );
}

triStateType fileOps::isDir ( const QString& param )
{
	if ( !param.isEmpty () )
	{
		struct stat stFileInfo {};
		if ( ::stat ( param.toLocal8Bit ().constData (), &stFileInfo ) == 0 )
			return ( static_cast<bool> ( S_ISDIR ( stFileInfo.st_mode ) ) ) ? TRI_ON : TRI_OFF;
		return TRI_OFF;
	}
	return TRI_UNDEF;
}

triStateType fileOps::isLink ( const QString& param )
{
	if ( !param.isEmpty () )
	{
		struct stat stFileInfo {};
		if ( ::stat ( param.toLocal8Bit ().constData (), &stFileInfo ) == 0 )
			return ( static_cast<bool> ( S_ISLNK ( stFileInfo.st_mode ) ) ) ? TRI_ON : TRI_OFF;
	}
	return TRI_UNDEF;
}

triStateType fileOps::isFile ( const QString& param )
{
	if ( !param.isEmpty () )
	{
		struct stat stFileInfo {};
		if ( ::stat ( param.toLocal8Bit ().constData (), &stFileInfo ) == 0 )
			return ( static_cast<bool> ( S_ISREG ( stFileInfo.st_mode ) ) ) ? TRI_ON : TRI_OFF;
	}
	return TRI_UNDEF;
}

long int fileOps::fileSize ( const QString& filepath )
{
	if ( isFile ( filepath ).isOn () )
	{
		struct stat stFileInfo {};
		if ( ::stat ( filepath.toLocal8Bit ().constData (), &stFileInfo ) == 0 )
			return stFileInfo.st_size;
	}
	return -1;
}

int fileOps::fileCount ( const QString& path, const QStringList& name_filters )
{
	DIR* __restrict dir ( nullptr );
	if ( ( dir = ::opendir ( path.toLocal8Bit ().constData () ) ) != nullptr )
	{
		struct dirent* __restrict dir_ent ( nullptr );
		int n_files ( 0 );
		if ( name_filters.isEmpty () )
		{
			while ( ( dir_ent = ::readdir ( dir ) ) != nullptr )
			{
				if ( dir_ent->d_type == DT_REG )
					n_files++;
			}
		}
		else
		{
			const int filter_n ( name_filters.count () );
			int i ( 0 );
			bool b_match ( false );
			while ( ( dir_ent = ::readdir ( dir ) ) != nullptr )
			{
				if ( dir_ent->d_type == DT_REG )
				{
					const QString& filename ( QString::fromUtf8 ( dir_ent->d_name ) );
					for ( i = 0; i < filter_n; ++i )
					{
						if ( filename.contains ( name_filters.at ( i ), Qt::CaseInsensitive ) )
						{
							b_match = true;
							break;
						}
					}
					if ( !b_match )
						n_files++;
					b_match = false;
				}
			}
		}
		::closedir ( dir );
		return n_files;
	}
	return -1;
}

bool fileOps::modifiedDateTime ( const QString& path, vmNumber& modDate, vmNumber& modTime )
{
	if ( !path.isEmpty () )
	{
		struct stat stFileInfo {};
		if ( ::stat ( path.toLocal8Bit ().constData (), &stFileInfo ) == 0 )
		{
			time_t filetime ( stFileInfo.st_mtime );
			struct tm* __restrict date ( ::localtime ( &filetime ) );
			modDate.setDate ( static_cast<int> ( date->tm_mday ),
							static_cast<int> ( date->tm_mon ) + 1,
							static_cast<int> ( date->tm_year ) + 1900 );

			modTime.setTime ( static_cast<int> ( date->tm_hour ),
							static_cast<int> ( date->tm_min ),
							static_cast<int> ( date->tm_sec ),
							false );
			return true;
		}
	}
	return false;
}

triStateType fileOps::canRead ( const QString& path )
{
	struct stat stFileInfo {};
	if ( ::stat ( path.toLocal8Bit ().constData (), &stFileInfo ) == 0 )
	{
		bool r_ok ( false );
		const QString username ( QString::fromUtf8 ( ::getenv ( "USER" ) ) );
		auto pwd ( new struct passwd );
		const size_t buffer_len ( static_cast<size_t>(::sysconf ( _SC_GETPW_R_SIZE_MAX )) * sizeof ( char ) );
		auto* __restrict buffer ( new char[buffer_len] );

		::getpwnam_r ( username.toLocal8Bit ().constData (), pwd, buffer, buffer_len, &pwd );
		if ( ( stFileInfo.st_mode & S_IRUSR ) == S_IRUSR ) /* Read by owner.*/
			r_ok = ( stFileInfo.st_uid == pwd->pw_uid ); // file can be read by username

		if ( !r_ok )
		{ // not read by owner or uids don't match
			if ( ( stFileInfo.st_mode & S_IRGRP ) == S_IRGRP ) /* Read by group.*/
				r_ok = ( stFileInfo.st_gid == pwd->pw_gid ); // file cannot be read by username directly. But can indirectly because of group permissions;
			if ( !r_ok )
			{ // cannot be read by group or gids don't match
				if ( ( stFileInfo.st_mode & S_IROTH ) == S_IROTH )
				{ /* Read by others.*/
					// file cannot be read by username directly. But can indirectly because of other permissions
					r_ok = true;
				}
			}
		}
		delete pwd;
		delete [] buffer;
		return { r_ok };
	}
	return TRI_UNDEF;
}

triStateType fileOps::canWrite ( const QString& path )
{
	struct stat stFileInfo {};
	if ( ::stat ( path.toLocal8Bit ().constData (), &stFileInfo ) == 0 )
	{
		bool w_ok ( false );
		const QString username ( QString::fromUtf8 ( ::getenv ( "USER" ) ) );
		auto* pwd ( new struct passwd );
		const size_t buffer_len ( static_cast<size_t>(::sysconf ( _SC_GETPW_R_SIZE_MAX )) * sizeof ( char ) );
		auto* __restrict buffer ( new char[buffer_len] );

		::getpwnam_r ( username.toLocal8Bit ().constData (), pwd, buffer, buffer_len, &pwd );
		if ( ( stFileInfo.st_mode & S_IWUSR ) == S_IWUSR ) /* Write by owner.*/
			w_ok = ( stFileInfo.st_uid == pwd->pw_uid ); // file can be written by username

		if ( !w_ok )
		{ // not written by owner or uids don't match
			if ( ( stFileInfo.st_mode & S_IWGRP ) == S_IWGRP ) /* Read by group.*/
				w_ok = ( stFileInfo.st_gid == pwd->pw_gid ); // file cannot be written by username directly. But can indirectly because of group permissions;
			if ( !w_ok )
			{ // cannot be written by group or gids don't match
				if ( ( stFileInfo.st_mode & S_IWOTH ) == S_IWOTH )
				{ /* Read by others.*/
					// file cannot be written by username directly. But can indirectly because of other permissions
					w_ok = true;
				}
			}
		}
		delete pwd;
		delete [] buffer;
		return { w_ok };
	}
	return TRI_UNDEF;
}

triStateType fileOps::canExecute ( const QString& path )
{
	struct stat stFileInfo {};
	if ( ::stat ( path.toLocal8Bit ().constData (), &stFileInfo ) == 0 )
	{
		bool x_ok ( false );
		const QString username ( QString::fromUtf8 ( ::getenv ( "USER" ) ) );
		auto pwd ( new struct passwd );
		const size_t buffer_len ( static_cast<size_t>(::sysconf ( _SC_GETPW_R_SIZE_MAX )) * sizeof (char) );
		auto* __restrict buffer ( new char[buffer_len] );

		::getpwnam_r ( username.toLocal8Bit ().constData (), pwd, buffer, buffer_len, &pwd );
		if ( ( stFileInfo.st_mode & S_IXUSR ) == S_IXUSR ) /* Execute by owner.*/
			x_ok = ( stFileInfo.st_uid == pwd->pw_uid ); // file can be executed by username
		if ( !x_ok )
		{
			// not executed by owner or uids don't match
			if ( ( stFileInfo.st_mode & S_IXGRP ) == S_IXGRP ) /* Execute by group.*/
				x_ok = ( stFileInfo.st_gid == pwd->pw_gid ); // file cannot be executed by username directly. But can indirectly because of group permissions;
			if ( !x_ok )
			{
				// cannot be executed by group or gids don't match
				if ( ( stFileInfo.st_mode & S_IXOTH ) == S_IXOTH ) /* Executed by others. */
				{
					// file cannot be executed by username directly. But can indirectly because of other permissions
					x_ok = true;
				}
			}
		}
		delete pwd;
		delete [] buffer;
		return { x_ok };
	}
	return TRI_UNDEF;
}

triStateType fileOps::createDir ( const QString& path )
{
	triStateType ret ( isDir ( path ) );
	if ( ret == TRI_OFF ) // path does not exits
	{
		const int idx ( path.lastIndexOf ( CHR_F_SLASH, ( path.at ( path.length () - 1 ) == CHR_F_SLASH ) ? -2 : -1 ) );
		if ( idx != -1 )
		{
			ret = createDir ( path.left ( idx ) );
			if ( ret == TRI_ON )
			{
				QString new_path ( path );
				if ( new_path.at ( new_path.length () - 1 ) != CHR_F_SLASH )
					new_path.append ( CHR_F_SLASH );
				struct stat stFileInfo {};
#ifdef USING_QT6
                ::stat ( new_path.left ( idx ).toLocal8Bit ().constData (), &stFileInfo );
#else
				::stat ( new_path.leftRef ( idx ).toLocal8Bit ().constData (), &stFileInfo );
#endif
				return ( ::mkdir ( new_path.toLocal8Bit ().constData (), stFileInfo.st_mode ) == 0 ) ? TRI_ON : TRI_OFF;
			}
		}
	}
	return ret;
}

bool fileOps::copyFile ( const QString& dst, const QString& src )
{
	QString mutable_dst ( dst );

	bool ret ( false );
	if ( exists ( src ).isOn () )
	{
		if ( isFile ( dst ).isOn () )
		{
			if ( canWrite ( dst ).isOn () )
			{
				if ( vmNotify::questionBox ( nullptr, APP_TR_FUNC ( "Same filename" ), QString ( APP_TR_FUNC (
													 "Destination file %1 already exists. Overwrite it?" ) ).arg ( dst ) ) )
					ret = ::remove ( dst.toLocal8Bit ().constData () ); // if remove succeeds there is not need to check if dst's path is writable
			}
		}
		else if ( isDir ( dst ).isOn () )
		{
			if ( canWrite ( dst ).isOn () )
			{
				ret = true;
				if ( mutable_dst.at ( mutable_dst.length () - 1 ) != CHR_F_SLASH )
					mutable_dst.append ( CHR_F_SLASH );
				mutable_dst.append ( fileNameWithoutPath ( src ) );
			}
		}
		else //dst does not exist
			ret = !createDir ( mutable_dst.left ( mutable_dst.lastIndexOf ( CHR_F_SLASH ) ) ).isOff ();

		if ( ret )
			ret = QFile::copy ( src, mutable_dst );
	}
	return ret;
}

triStateType fileOps::rename ( const QString& old_name, const QString& new_name )
{
	return ( ::rename ( old_name.toLocal8Bit ().constData (), new_name.toLocal8Bit ().constData () ) == 0 ) ? TRI_ON : TRI_OFF;
}

triStateType fileOps::removeFile ( const QString& filename )
{
	if ( isFile ( filename ) == TRI_ON )
		return ( ::remove ( filename.toLocal8Bit ().constData () ) == 0 ) ? TRI_ON : TRI_OFF;
	return TRI_UNDEF;
}

const QString fileOps::nthDirFromPath ( const QString& c_path, const int n )
{
	QString dir;
	if ( !c_path.isEmpty () )
	{
		QString path ( c_path );
		if ( isDir ( path ).isOn () )
		{
			if ( path.at ( path.length () -1 ) != CHR_F_SLASH )
				path.append ( CHR_F_SLASH );
		}
		int idx ( -1 ), idx2 ( 0 );
		if ( n == -1 ) // last dir
		{
			idx = path.lastIndexOf ( CHR_F_SLASH, -2 );
			if ( idx != -1 )
			{
				dir = path.mid ( idx + 1, path.length () - idx - 1 );
#ifdef USING_QT6
                if ( !isDir ( c_path.left ( idx + 1 ) + dir ).isOn () )
#else
				if ( !isDir ( c_path.leftRef ( idx + 1 ) + dir ).isOn () )
#endif
				{
					idx2 = path.lastIndexOf ( CHR_F_SLASH, 0 - ( path.length () - idx ) - 1 );
					dir = path.mid ( idx2 + 1, idx - idx2 );
				}
			}
		}
		else
		{
			int i = 0;
			while ( ( idx = path.indexOf ( CHR_F_SLASH, idx2 + 1 ) ) != -1 )
			{
				if ( i == 0 ) //include leading '/' only for the top level dir
					dir = path.mid ( idx2, idx - idx2 + 1 );
				else
					dir = path.mid ( idx2 + 1, idx - idx2 );
				if ( i == n )
					return dir;
				idx2 = idx;
				++i;
			}
			dir.clear (); // n is bigger than the number of dirs and subdir
		}
	}
	return dir;
}

// Returns dir name from path. The filename does not need to exists. If path is null/empty or a dir, returns itself
const QString fileOps::dirFromPath ( const QString& path )
{
	if ( !isDir ( path ).isOn () )
	{
		const int idx ( path.lastIndexOf ( CHR_F_SLASH ) );
		if ( idx != -1 )
			return path.left ( idx + 1 );
	}
	return path;
}

const QString fileOps::fileExtension ( const QString& filepath )
{
	const int idx ( filepath.indexOf ( CHR_DOT ) );
	if ( idx != -1 )
		return filepath.right ( filepath.length () - idx - 1 );
	return emptyString;
}

const QString fileOps::fileNameWithoutPath ( const QString& filepath )
{
	const int idx ( filepath.lastIndexOf ( CHR_F_SLASH ) );
	if ( idx != -1 )
		return filepath.right ( filepath.length () - idx - 1 );
	return filepath;
}

const QString fileOps::filePathWithoutExtension ( const QString& filepath )
{
	const int idx ( filepath.indexOf ( CHR_DOT ) );
	if ( idx != -1 )
		return filepath.left ( idx );
	return filepath;
}

const QString fileOps::replaceFileExtension ( const QString& filepath, const QString& new_ext )
{
	return QString ( filePathWithoutExtension ( filepath ) + CHR_DOT + new_ext );
}

void fileOps::lsDir ( pointersList<st_fileInfo*>& result, const QString& baseDir,
					  const QStringList& name_filters, const QStringList& exclude_filter,
					  const int filter, const int follow_into, const bool b_insert_sorted )
{
	DIR* __restrict dir ( nullptr );
	if ( ( dir = ::opendir ( baseDir.toLocal8Bit ().constData () ) ) != nullptr )
	{
		struct dirent* __restrict dir_ent ( nullptr );
		QString filename, pathname;
		static const QString dot ( CHR_DOT );
		static const QString doubleDot ( QStringLiteral ( ".." ) );
		bool ok ( false );

		while ( ( dir_ent = ::readdir ( dir ) ) != nullptr )
		{
			filename = QString::fromUtf8 ( dir_ent->d_name );
			if ( filename == dot || filename == doubleDot )
				continue;
			if ( !( filter & LS_HIDDEN_FILES ) )
			{
				if ( filename.startsWith ( CHR_DOT ) || filename.startsWith ( CHR_TILDE ) )
					continue;
			}
			
			ok = true;
			for ( int i ( 0 ); i < exclude_filter.count (); ++i )
			{
				if ( filename == exclude_filter.at ( i ) )
				{
					ok = false;
					break;
				}
			}
			if ( !ok )
				continue;
			
			pathname = baseDir + CHR_F_SLASH + filename;
			
			ok = ( filter & LS_FILES ) && ( dir_ent->d_type == DT_REG );
			if ( !ok )
				ok = ( filter & LS_DIRS ) && ( dir_ent->d_type == DT_DIR );
			else
			{
				const int filter_n ( name_filters.count () );
				if ( filter_n > 0 )
				{
					int i ( 0 );
					for ( ; i < filter_n; ++i )
					{
						if ( filename.contains ( name_filters.at ( i ), Qt::CaseInsensitive ) )
							break;
					}
					if ( i >= filter_n )
						ok = false;
				}
			}

			if ( ok )
			{
				auto fi ( new st_fileInfo );
				fi->filename = filename;
				fi->fullpath = pathname;
				fi->is_file = dir_ent->d_type == DT_REG;
				fi->is_dir = !fi->is_file;
				if ( b_insert_sorted )
				{
					insertStringIntoContainer ( result, filename,
											[&] ( const int idx ) -> QString { return result.at ( idx )->filename; },
											[&,fi] ( const int idx, const QString& ) { result.insert ( static_cast<uint>( idx ), fi ); },
											false );
				}
				else
					result.append ( fi );

				if ( fi->is_dir )
				{
					if ( follow_into > 0 )
						lsDir ( result, pathname, name_filters, exclude_filter, filter, follow_into - 1, b_insert_sorted );
				}

			}
		}
		::closedir ( dir );
	}
}

bool fileOps::rmDir ( const QString& baseDir,
					  const QStringList& name_filters, const int filter, const int follow_into )
{
	bool ok ( false );
	static int level ( 0 );
	++level;
	DIR* __restrict dir ( nullptr );
	if ( ( dir = ::opendir ( baseDir.toLocal8Bit ().constData () ) ) != nullptr )
	{	
		struct dirent* __restrict dir_ent ( nullptr );
		QString filename, pathname;

		while ( ( dir_ent = ::readdir ( dir ) ) != nullptr )
		{
			filename = QString::fromUtf8 ( dir_ent->d_name );
			if ( filename == QStringLiteral ( "." ) || filename == QStringLiteral ( ".." ) )
				continue;

			pathname = baseDir + CHR_F_SLASH + filename;
			ok = ( filter & LS_FILES ) && ( dir_ent->d_type == DT_REG );
			if ( !ok )
				ok = ( filter & LS_DIRS ) && ( dir_ent->d_type == DT_DIR );
			else
			{
				const int filter_n ( name_filters.count () );
				if ( filter_n > 0 )
				{
					int i ( 0 );
					for ( ; i < filter_n; ++i )
					{
						if ( filename.contains ( name_filters.at ( i ), Qt::CaseInsensitive ) )
							break;
					}
					if ( i >= filter_n )
						ok = false;
				}
			}

			if ( ok )
			{
				if ( dir_ent->d_type == DT_REG )
					::remove ( pathname.toLocal8Bit ().constData () );
				else
				{
					if ( follow_into != 0 )
						(void) rmDir ( pathname, name_filters, filter, follow_into == -1 ? -1 : follow_into - 1 );
					ok = ( ::rmdir ( pathname.toLocal8Bit ().constData () ) == 0 );
				}
			}
		}
		if ( --level == 0 )
			ok = ( ::rmdir ( baseDir.toLocal8Bit ().constData () ) == 0 );
		::closedir ( dir );
	}
	return ok;
}

int fileOps::sysExec ( const QStringList &command_line )
{
	return sysExec ( command_line.join ( emptyString ) );
}

int fileOps::sysExec ( const QString& command_line )
{
	const QString cmdLine ( configOps::pkexec () + CHR_SPACE + QSTRING_ENCODING_FIX(command_line) );
	MSG_OUT(cmdLine);
	return ::system ( cmdLine.toLocal8Bit ().constData () );
}

/*bool fileOps::executeWait ( const QString& arguments, const QString& program,
							int* __restrict exitCode, const QString& as_root_message )
{
	auto* __restrict proc ( new QProcess () );
	QString args;
	if ( !arguments.isEmpty () )
	{
		args += CHR_SPACE;
		args += CHR_QUOTES + QSTRING_ENCODING_FIX ( arguments ) + CHR_QUOTES;
	}
	if ( !as_root_message.isEmpty () )
		args = suProgram ( as_root_message, program );

	proc->start ( program, args );
	const bool ret ( proc->waitForFinished ( -1 ) );
	if ( exitCode != nullptr )
	{
		*exitCode = proc->exitCode ();
	}
	delete proc;
	return ret;
}*/

bool fileOps::executeWait ( QStringList& arguments, const QString& program, const bool b_asRoot, int* __restrict exitCode )
{
	auto* __restrict proc ( new QProcess () );
	QString prog;

	if ( b_asRoot )
	{
		arguments.prepend ( program );
		prog = configOps::pkexec ();
	}
	else
		prog = program;

	MSG_OUT (prog + CHR_SPACE + arguments.join ( CHR_SPACE ) );
	proc->start ( prog, arguments );
	const bool ret ( proc->waitForFinished ( -1 ) );
	if ( exitCode != nullptr )
	{
		*exitCode = proc->exitCode ();
	}

	if ( b_asRoot )
		arguments.removeAt ( 0 );
	delete proc;
	return ret;
}

const QString fileOps::executeAndCaptureOutput ( QStringList& arguments, const QString& program, const bool b_asRoot,
												 int* __restrict exitCode )
{
	auto* __restrict proc ( new QProcess () );
	QString prog;

	if ( b_asRoot )
	{
		arguments.prepend ( program );
		prog = configOps::pkexec ();
	}
	else
		prog = program;

	proc->start ( prog, arguments );
	proc->waitForFinished ( -1 );
	QString strOutput ( QString::fromUtf8 ( proc->readAllStandardOutput ().constData () ) );

	if ( strOutput.isEmpty () )
		strOutput = QString::fromUtf8 ( proc->readAllStandardError ().constData () );
	if ( exitCode != nullptr )
	{
		*exitCode = proc->exitCode ();
	}

	if ( b_asRoot )
		arguments.removeAt ( 0 );
	delete proc;
	return strOutput;
}

bool fileOps::executeWithFeedFile ( QStringList& arguments, const QString& program,  const QString& filename, const bool b_input,
									const bool b_asRoot, int* __restrict exitCode )
{
	auto* __restrict proc ( new QProcess () );
	QString prog;

	if ( b_asRoot )
	{
		arguments.prepend ( program );
		prog = configOps::pkexec ();
	}
	else
		prog = program;

	if ( b_input )
	{
		proc->setStandardInputFile ( filename );
		proc->waitForBytesWritten ();
	}
	else
	{
		proc->setStandardOutputFile ( filename );
		proc->waitForReadyRead ();
	}

	MSG_OUT (prog + CHR_SPACE + arguments.join ( CHR_SPACE ) );
	proc->waitForStarted ();
	proc->start ( prog, arguments );

	const bool ret ( proc->waitForFinished () );
	if ( exitCode != nullptr )
		*exitCode = proc->exitCode ();

	if ( b_asRoot )
		arguments.removeAt ( 0 );
	delete proc;
	return ret;
}

bool fileOps::executeFork ( QStringList& arguments, const QString& program )
{
	bool ret ( false );
	auto* __restrict proc ( new QProcess () );
	ret = proc->startDetached ( program, arguments );
	delete proc;
	return ret;
}

void fileOps::openAddress ( const QString& address )
{
	QStringList args ( address );
	executeFork ( args, XDG_OPEN );
}

QString fileOps::getExistingDir ( const QString& dir )
{
	return QFileDialog::getExistingDirectory ( nullptr, APP_TR_FUNC ( "Choose dir " ), QSTRING_ENCODING_FIX( dir ) );
}

QString fileOps::getOpenFileName ( const QString& dir, const QString& filter )
{
	return QFileDialog::getOpenFileName ( nullptr, APP_TR_FUNC ( "Choose file" ), QSTRING_ENCODING_FIX( dir ), filter );
}

QString fileOps::getSaveFileName ( const QString& default_name, const QString& filter )
{
	return QFileDialog::getSaveFileName ( nullptr, APP_TR_FUNC ( "Save as ..." ), default_name, filter );
}

fileOps::DesktopEnvironment fileOps::detectDesktopEnvironment ()
{
	// session/desktop env variables: XDG_SESSION_DESKTOP or XDG_CURRENT_DESKTOP
	const QString xdgCurrentDesktop ( QString::fromUtf8 ( ::getenv ( "XDG_CURRENT_DESKTOP" ) ) );
	if ( xdgCurrentDesktop == QStringLiteral ( "KDE" ) )
		return getKDEVersion ();
	if ( xdgCurrentDesktop == QStringLiteral ( "GNOME" ) )
		return DesktopEnv_Gnome;
	if ( xdgCurrentDesktop == QStringLiteral ( "Unity" ) )
		return DesktopEnv_Unity;
	if ( xdgCurrentDesktop.contains ( QStringLiteral ( "xfce" ), Qt::CaseInsensitive )
			|| xdgCurrentDesktop.contains ( QStringLiteral ( "xubuntu" ), Qt::CaseInsensitive ) )
		return DesktopEnv_Xfce;

	return DesktopEnv_Other;
}

// the following detection algorithm is derived from chromium,
// licensed under BSD, see base/nix/xdg_util.cc
fileOps::DesktopEnvironment fileOps::getKDEVersion ()
{
	const QString value ( QString::fromUtf8 ( ::getenv ( "KDE_SESSION_VERSION" ) ) );
	if ( value == QStringLiteral ( "5" ) )
		return DesktopEnv_Plasma5;
	if ( value == QStringLiteral ( "4" ) )
		return DesktopEnv_Kde4;
	// most likely KDE3
	return DesktopEnv_Other;
}
