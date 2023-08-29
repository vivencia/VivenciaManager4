#include "vmcompress.h"
#include "fileops.h"

#include <QtCore/QFile>

#include <cstdio>
#include <cstring>

extern "C"
{
#include <bzlib.h>
}

static bool checkSomeThingsFirst ( const QString& in_filename, const QString& out_filename )
{
	if ( !fileOps::isFile ( in_filename ).isOn () )
		return false;
	return fileOps::canWrite ( fileOps::dirFromPath ( out_filename ) ).isOn ();
}

bool VMCompress::compress ( const QString& in_filename, const QString& out_filename )
{
	if ( !checkSomeThingsFirst ( in_filename, out_filename ) )
		return false;

	QFile in_file ( in_filename );
	if ( !in_file.open ( QIODevice::ReadOnly|QIODevice::Text ) )
		return false;

	FILE* f_out ( nullptr );
	f_out = ::fopen ( out_filename.toUtf8 ().constData (), "wb" );
	if ( f_out == nullptr )
	{
		in_file.close ();
		return false;
	}

	int bzerror ( 0 );
	BZFILE* bzf ( nullptr );
	bzf = ::BZ2_bzWriteOpen ( &bzerror, f_out, 9, 0, 30 );
	if ( bzerror != BZ_OK )
	{
		::BZ2_bzWriteClose ( &bzerror, bzf, 1, nullptr, nullptr );
		::fclose ( f_out );
		in_file.close ();
		return false;
	}

	const uint max_len ( 50000 );
	int bytes_read = 0;
	bool error ( false );
	char buf[max_len] = { '\0' };

	do
	{
		bytes_read = static_cast<int>(in_file.read ( buf, max_len ));
		if ( bytes_read <= 0 )
		{
			error = true;
			break;
		}
		::BZ2_bzWrite ( &bzerror, bzf, static_cast<void*> ( buf ), bytes_read );
		if ( bzerror != BZ_OK )
		{
			error = true;
			break;
		}
	} while ( !in_file.atEnd () );

	::BZ2_bzWriteClose ( &bzerror, bzf, 0, nullptr, nullptr );
	in_file.close ();
	::fclose ( f_out );

	if ( bytes_read == 0 )
		error = false;
	return ( !error && ( bzerror == BZ_OK ) );
}

bool VMCompress::isCompressed ( const QString& filename )
{
	FILE* f = nullptr;
	f = ::fopen ( filename.toUtf8 ().constData (), "rb" );
	if ( f == nullptr )
		return false;

	int bzerror ( 0 );
	BZFILE* bzf = nullptr;
	bzf = ::BZ2_bzReadOpen ( &bzerror, f, 0, 0, nullptr, 0 );

	const uint max_len ( 100 );
	char buf[max_len] = { '\0' };
	::BZ2_bzRead ( &bzerror, bzf, static_cast<void*> ( buf ), max_len );
	const bool ret ( bzerror == BZ_OK );
	::BZ2_bzReadClose ( &bzerror, bzf );
	::fclose ( f );
	return ret;
}

bool VMCompress::decompress ( const QString& in_filename, const QString& out_filename )
{
	if ( !checkSomeThingsFirst ( in_filename, out_filename ) )
		return false;

	FILE* f_in = nullptr;
	f_in = ::fopen ( in_filename.toUtf8 ().constData (), "rb" );
	if ( f_in == nullptr )
		return false;

	QFile out_file ( out_filename );
	if ( !out_file.open ( QIODevice::WriteOnly|QIODevice::Text ) )
	{
		::fclose ( f_in );
		return false;
	}

	int bzerror ( 0 );
	BZFILE* bzf = nullptr;
	bzf = ::BZ2_bzReadOpen ( &bzerror, f_in, 0, 0, nullptr, 0 );
	if ( bzerror != BZ_OK )
	{
		::BZ2_bzReadClose ( &bzerror, bzf );
		::fclose ( f_in );
		out_file.close ();
		return false;
	}

	static const uint max_len ( 50000 );
	int bytes_read ( 0 );
	uint bytes_written ( 0 );
	bool error ( false );
	char buf[max_len] = { '\0' };

	do
	{
		bytes_read = ::BZ2_bzRead ( &bzerror, bzf, static_cast<void*> ( buf ), max_len );
		if ( bzerror != BZ_OK )
		{
			if ( bzerror != BZ_STREAM_END )
			{
				error = true;
				break;
			}
		}
		bytes_written = static_cast<uint> ( out_file.write ( buf, bytes_read ) );
		if ( bytes_written <= 0 )
		{
			error = true;
			break;
		}
	} while ( bzerror != BZ_STREAM_END );

	::BZ2_bzReadClose ( &bzerror, bzf );
	out_file.close ();
	::fclose ( f_in );

	return ( !error && ( bzerror == BZ_OK ) );
}

bool VMCompress::createTar ( const QString& input_dir, const QString& out_filename, const bool overwrite )
{
	if ( fileOps::isFile ( out_filename ).isOn () && !overwrite )
		return true;

	const QString cmd ( QStringLiteral ( "tar -C %1 -cf %2 %3" ) );
	return ( ::system ( cmd.arg ( fileOps::nthDirFromPath ( input_dir, 0 ), out_filename, fileOps::nthDirFromPath ( input_dir ) ).toUtf8 ().constData () ) == 0 );
}

bool VMCompress::addToTar ( const QString& input, const QString& tar_file, const bool create_if_not_exists )
{
	if ( !fileOps::isFile ( tar_file ).isOn () )
	{
		if ( create_if_not_exists )
		{
			if ( !createTar ( input, tar_file ) )
				return false;
		}
		else
			return false;
	}
	const QString cmd ( QStringLiteral ( "tar -C %1 -rf %2 %3" ) );
	const QString topDir ( fileOps::nthDirFromPath ( input, 0 ) );
	return ( ::system ( cmd.arg ( topDir, tar_file,	QString ( input ).remove ( topDir ) ).toUtf8 ().constData () ) == 0 );
}
