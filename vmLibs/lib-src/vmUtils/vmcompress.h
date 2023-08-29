#ifndef VMCOMPRESS_H
#define VMCOMPRESS_H

#include <QtCore/QString>

class VMCompress
{

public:
	explicit inline VMCompress () {}

	/*
	 * These functions do not overwrite out_filename if it points to an existing file.
	 * These functions do not create directories and will only create a (un)compressed file in existing dirs.
	 * These functions do not escalate the caller's privileges. If it cannot read/write files, it will return false.
	 * These functions do not add/remove a .bz2 extension to the output file.
	 * To sum up: this function only reads in_filename and writes out_filename if it can. Only that
	 */
	static bool compress ( const QString& in_filename, const QString& out_filename );
	static bool decompress ( const QString& in_filename, const QString& out_filename );

	static bool isCompressed ( const QString& filename );

	static bool createTar ( const QString& input_dir, const QString& out_filename, const bool overwrite = false );
	static bool addToTar ( const QString& input, const QString& tar_file, const bool create_if_not_exists = true );
};

#endif // VMCOMPRESS_H

