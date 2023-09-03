#ifndef FILEOPS_H
#define FILEOPS_H

#include "vmlist.h"
#include <vmUtils/tristatetype.h>
#include <vmNumbers/vmnumberformats.h>

#include <QtCore/QString>

class fileOps
{

public:

	enum LS_FILTER
	{
		LS_DIRS			= 0x001,
		LS_FILES		= 0x002,
		LS_HIDDEN_FILES	= 0x004 | LS_FILES,
		LS_ALL			= LS_DIRS | LS_FILES | LS_HIDDEN_FILES
	};

	struct st_fileInfo
	{
		QString filename;
		QString fullpath;
		bool is_dir;
		bool is_file;

		st_fileInfo () : is_dir ( false ), is_file ( true ) {}

		const st_fileInfo& operator= ( const st_fileInfo& fi2 ) noexcept;
	};

	enum DesktopEnvironment
	{
		DesktopEnv_Plasma5,
		DesktopEnv_Gnome,
		DesktopEnv_Kde4,
		DesktopEnv_Unity,
		DesktopEnv_Xfce,
		DesktopEnv_Other
	};

	constexpr explicit inline fileOps () {}

	static const QString appPath ( const QString& appname );
	static triStateType exists ( const QString& file );

	static const QString currentUser ();

	static triStateType isDir ( const QString& param );
	static triStateType isLink ( const QString& param );
	static triStateType isFile ( const QString& param );
	static long int fileSize ( const QString& filepath );
	static int fileCount ( const QString& path, const QStringList& name_filters = QStringList () );
	static bool modifiedDateTime ( const QString& path, vmNumber& modDate, vmNumber& modTime );

	static triStateType canRead ( const QString& path );
	static triStateType canWrite ( const QString& path );
	static triStateType canExecute ( const QString& path );

	static triStateType createDir ( const QString& path );
	static bool copyFile ( const QString& dst, const QString& src );

	static triStateType removeFile ( const QString& filename );
	static triStateType rename ( const QString& old_name, const QString& new_name );

	static const QString nthDirFromPath ( const QString& path, const int n = -1 );
	static const QString dirFromPath ( const QString& path );
	static const QString fileExtension ( const QString& filepath );
	static const QString fileNameWithoutPath ( const QString& filepath );
	static const QString filePathWithoutExtension ( const QString& filepath );
	static const QString replaceFileExtension ( const QString& filepath, const QString& new_ext );

	static void lsDir ( pointersList<st_fileInfo*>& result, const QString& baseDir,
						const QStringList& name_filters = QStringList (), const QStringList& exclude_filter = QStringList (),
						const int filter = LS_FILES, const int follow_into = 0, const bool b_insert_sorted = true );

	static bool rmDir ( const QString& baseDir, const QStringList& name_filters,
						const int filter = LS_FILES, const int follow_into = 0 );

	static int sysExec ( const QStringList &command_line );
	static int sysExec ( const QString& command_line );
	//static bool executeWait ( const QString& arguments, const QString& program, int* exitCode = nullptr, const QString& as_root_message = QString () );
	static bool executeWait ( QStringList& arguments, const QString& program, const bool b_asRoot = false, int* __restrict exitCode = nullptr );
	static const QString executeAndCaptureOutput ( QStringList& arguments, const QString& program, const bool b_asRoot = false, int* __restrict exitCode = nullptr );
	static bool executeWithFeedFile ( QStringList& arguments, const QString& program,
									  const QString& filename, const bool b_input = true,
									  const bool b_asRoot = false, int* exitCode = nullptr );
	static bool executeFork ( QStringList& arguments, const QString& program );
	static void openAddress ( const QString& address );

	static QString getExistingDir ( const QString& dir );
	static QString getOpenFileName ( const QString& dir, const QString& filter );
	static QString getSaveFileName ( const QString& dir, const QString& filter );

	static DesktopEnvironment detectDesktopEnvironment ();
	static DesktopEnvironment getKDEVersion ();
};

#endif // FILEOPS_H

extern bool operator!= ( fileOps::st_fileInfo fi1, fileOps::st_fileInfo fi2 );
