#include "vmtextfile.h"
#include "vmlibs.h"
#include "heapmanager.h"
#include "fileops.h"

#include <vmNotify/vmnotify.h>

#include <QtCore/QStringList>

static const QString HEADER_ID ( QStringLiteral ( "#!VMFILE" ) );
static const QString CFG_TYPE_LINE ( QStringLiteral ( "@CFG\n" ) );
static const QString DATA_TYPE_LINE ( QStringLiteral ( "@CSV\n" ) );

/*
 *
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

 static void msleep ( unsigned long msecs )
{
	QMutex mutex;
	mutex.lock ();
	QWaitCondition waitCondition;
	waitCondition.wait ( &mutex, msecs );
	mutex.unlock ();
}
*/

//--------------------------------------------TEXT-FILE--------------------------------
textFile::textFile ()
	: m_open ( false ), m_needsaving ( false ), mb_needRechecking ( false ), m_headerSize ( 0 ),
	  m_type ( TF_UNKNOWN ), m_readTime ( VMNT_TIME ), m_readDate ( VMNT_DATE )
{}

textFile::textFile ( const QString& filename )
	: textFile ()
{
	setFileName ( filename );
}

textFile::~textFile ()
{
	close ();
	clearData ();
}

bool textFile::isTextFile ( const QString& filename, const TF_TYPE type )
{
	textFile temp_file ( filename );
	temp_file.open ();
	const TF_TYPE ret_type ( temp_file.type () );
	return ret_type == type;
}

void textFile::remove ()
{
	close ();
	fileOps::removeFile ( m_filename );
}

void textFile::close ()
{
	m_file.flush ();
	m_file.close ();
	m_open = false;
	m_needsaving = false;
	mb_needRechecking = false;
}

bool textFile::open ()
{
	if ( m_open )
		return true;

	if ( m_file.fileName ().isEmpty () )
		m_file.setFileName ( m_filename );
	else
	{
		if ( m_file.fileName () != m_filename )
		{
			close ();
			m_file.setFileName ( QString () );
			return open ();
		}

		if ( m_file.isOpen () )
			return true;
	}

	return open2 ();
}

bool textFile::open2 ()
{
	const bool b_exists ( fileOps::exists ( m_filename ).isOn () );
	QIODevice::OpenModeFlag openflag ( QIODevice::ReadWrite );

	if ( b_exists && !fileOps::canWrite ( m_filename ).isOn () )
		openflag = QIODevice::ReadOnly;

	m_open = m_file.open ( openflag|QIODevice::Text );
	readType ();
	return m_open;
}

void textFile::readType ()
{
	if ( type () == TF_UNKNOWN )
	{
		char buf[20] = { '\0' };
		m_file.seek ( 0 );

		m_headerSize = m_file.readLine ( buf, sizeof ( buf ) );
		if ( m_headerSize > 0 )
		{
			const QString data ( QString::fromUtf8 ( buf, static_cast<int>(strlen ( buf ) - 1) ) );
			if ( data.startsWith ( HEADER_ID ) )
			{
				if ( data.contains ( QStringLiteral ( "@CFG" ) ) )
					m_type = TF_CONFIG;
				else if ( data.contains ( QStringLiteral ( "@CSV" ) ) )
					m_type = TF_DATA;
				else
					m_type = TF_TEXT;
				return;
			}
		}
		m_type = TF_UNKNOWN;
	}
}

triStateType textFile::load ()
{
	if ( m_open && !mb_needRechecking )
		return TRI_ON;

	if ( open () )
	{
		if ( type () == TF_UNKNOWN || isEmpty () )
			return TRI_OFF;

		const triStateType ret ( loadData ( true ) );
		mb_needRechecking = ( ret != TRI_ON );
		if ( ret == TRI_ON )
		{
			fileOps::modifiedDateTime ( m_filename, m_readDate, m_readTime );
			return TRI_ON;
		}
	}
	return TRI_OFF;
}

void textFile::writeHeader ()
{
	if ( m_type != TF_TEXT )
	{
		QString str;
		str = HEADER_ID + ( type () == TF_CONFIG ? CFG_TYPE_LINE : DATA_TYPE_LINE );
		m_headerSize = qstrlen ( str.toLocal8Bit ().data () );
		m_file.write ( str.toLocal8Bit ().data (), m_headerSize );
		m_file.flush ();
	}
}

void textFile::commit ()
{
	if ( !m_needsaving ) return;

	vmNumber modTime ( VMNT_TIME ), modDate ( VMNT_DATE );
	fileOps::modifiedDateTime ( m_filename, modDate, modTime );

	if ( m_readDate != modDate || m_readTime != modTime )
		loadData ( false );

	remove ();
	if ( open () )
	{
		writeHeader ();
		if ( writeData () )
		{
			m_file.flush ();
			m_needsaving = false;
			fileOps::modifiedDateTime ( m_filename, m_readDate, m_readTime );
		}
	}
}

void textFile::setText ( const QString& new_file_text )
{
	m_data = new_file_text;
	m_needsaving = true;
}

bool textFile::loadData ( const bool )
{
	m_data = m_file.readAll ();
	return ( !m_data.isEmpty () );
}

bool textFile::writeData ()
{
	const QByteArray data ( m_data.toLocal8Bit () );
	return ( m_file.write ( data.data (), data.count () ) > 0 );
}

void textFile::clearData ()
{
	m_data.clear ();
}
//--------------------------------------------TEXT-FILE--------------------------------

//--------------------------------------------CONFIG-FILE--------------------------------
struct configFile::configFile_st
{
	QString section_name;
	vmList<QString> fields;
	vmList<QString> values;

	configFile_st () : fields ( QString (), 5 ), values ( QString (), 5 ) {}
};

configFile::configFile ()
	: textFile (), cfgData ( 30 )
{
	m_type = TF_CONFIG;
}

configFile::configFile ( const QString& filename, const QString& object_name )
	: textFile ( filename ), cfgData ( 20 ), m_objectName ( object_name )
{
	m_type = TF_CONFIG;
}

configFile::~configFile ()
{
	commit (); //save pending changes
}

bool configFile::setWorkingSection ( const QString& section_name )
{
	const int i ( findSection ( section_name ) );
	if ( i >= 0 )
	{
		cfgData.setCurrent ( i );
		return true;
	}
	return false;
}

const QString& configFile::fieldValue ( const QString& field_name ) const
{
	const int idx ( fieldIndex ( field_name ) );
	if ( idx != -1 )
		return cfgData.current ()->values.at ( idx );
	return emptyString;
}

int configFile::fieldIndex ( const QString& field_name ) const
{
	int ret ( -1 );
	if ( cfgData.currentIndex () != -1 )
	{
		configFile_st* __restrict section_info ( cfgData.current () );
		const uint n_fields ( section_info->fields.count () );
		for ( int i ( 0 ); i < static_cast<int>(n_fields); ++i )
		{
			if ( section_info->fields.at ( i ) == field_name )
			{
				ret = i;
				break;
			}
		}
	}
	return ret;
}

void configFile::insertNewSection ( const QString& section_name )
{
	if ( !section_name.isEmpty () )
	{
		auto section_info ( new configFile_st );
		section_info->section_name = section_name;
		m_needsaving = true;
		cfgData.append ( section_info );
	}
}

void configFile::deleteSection ( const QString& section_name )
{
	if ( !section_name.isEmpty () )
	{
		for ( uint i ( 0 ); i < cfgData.count (); ++i )
		{
			if ( cfgData.at ( i )->section_name == section_name )
			{
				if ( cfgData.currentIndex () == static_cast<int>(i) )
					cfgData.setCurrent ( -1 );
				cfgData.remove ( static_cast<int>(i), true );
			}
		}
	}
}

void configFile::insertField ( const QString& field_name, const QString& value )
{
	if ( cfgData.currentIndex () != -1 )
	{
		configFile_st* section_info ( cfgData.current () );
		section_info->fields.append ( field_name );
		section_info->values.append ( value );
		m_needsaving = true;
	}
}

void configFile::deleteField ( const QString& field_name )
{
	if ( cfgData.currentIndex () != -1 )
	{
		configFile_st* __restrict section_info ( cfgData.current () );
		const uint n_fields ( section_info->fields.count () );
		for ( uint i ( 0 ); i < n_fields; ++i )
		{
			if ( section_info->fields.at ( i ) == field_name )
			{
				section_info->fields.remove ( static_cast<int>(i) );
				section_info->values.remove ( static_cast<int>(i) );
				m_needsaving = true;
				break;
			}
		}
	}
}

bool configFile::setFieldValue ( const QString& field_name, const QString& value )
{
	if ( cfgData.currentIndex () != -1 )
	{
		configFile_st* __restrict section_info ( cfgData.current () );
		const int idx ( fieldIndex ( field_name ) );
		if ( idx != -1 )
		{
			if ( section_info->values[idx] != value )
			{
				section_info->values[idx] = value;
				m_needsaving = true;
			}
			return true;
		}
	}
	return false;
}

bool configFile::loadData ( const bool b_replaceBuffers )
{
	close ();
	if ( !open () )
		return false;

	auto* __restrict buf ( new char[1024] );
	int64_t n_chars ( -1 );
	int idx ( -1 ), idx2 ( -1 );
	QString line, section_name, fld_name, fld_value;
	bool b_skiplineread ( false );

	do
	{
		if ( !b_skiplineread )
		{
			n_chars = m_file.readLine ( buf, 1024 );
			if ( n_chars <= 2 ) continue;
			line = QString::fromUtf8 ( buf );
		}
		else
			b_skiplineread = false;

		idx = line.indexOf ( CHR_L_BRACKET );
		if ( idx != -1 )
		{
			idx2 = line.indexOf ( CHR_R_BRACKET, ++idx );
			if ( idx2 != -1 )
			{
				section_name = line.mid ( idx, idx2 - idx );

				if ( b_replaceBuffers || !mlst_managedSections.contains ( section_name ) ) // Always b_replaceBuffers = true for non-managed
				{
					if ( !setWorkingSection ( section_name ) ) // managed information is not in memory
						insertNewSection ( section_name );
				}
				else
					continue; //read it and ignore it

				do
				{
					n_chars = m_file.readLine ( buf, 1024 );
					if ( n_chars > 0 )
					{
						line = QString::fromUtf8 ( buf );
						idx = line.indexOf ( CHR_EQUAL );
						if ( idx != -1 )
						{
							fld_name = line.left ( idx );
							fld_value = line.mid ( idx + 1 ).simplified ();
							if ( fieldIndex ( fld_name ) != -1 ) // field read from disk exists in memory
							{
								if ( b_replaceBuffers )
									setFieldValue ( fld_name, fld_value );
							}
							else
							{
								insertField ( fld_name, fld_value );
							}
						}
						else
						{
							if ( line.contains ( CHR_L_BRACKET ) )
							{
								// file was recorded without blank lines between this session
								b_skiplineread = true;
								break;
							}
						}
					}
				} while ( n_chars > 0 ); // whenever there is a blank line, or a line with too few characters, end of session
			}
		}
	} while ( n_chars != -1 );

	delete [] buf;
	cfgData.setCurrent ( 0 );
	return !( cfgData.isEmpty () );
}

bool configFile::writeData ()
{
	QString line;
	configFile_st* __restrict section_info ( nullptr );
	uint n_fields ( 0 );
	qint64 written_total ( 0 );

	for ( uint i ( 0 ); i < cfgData.count (); ++i )
	{
		section_info = cfgData.at ( i );
		line = CHR_NEWLINE + CHR_L_BRACKET + section_info->section_name + CHR_R_BRACKET + CHR_NEWLINE;
		n_fields = section_info->fields.count ();
		for ( uint x ( 0 ); x < n_fields; ++x )
		{
			line += section_info->fields.at ( x ) + CHR_EQUAL + section_info->values.at ( x ) + CHR_NEWLINE;
		}

		written_total += m_file.write ( line.toLocal8Bit () );
	}
	return ( written_total > 0 );
}

void configFile::clearData ()
{
	cfgData.clear ( true );
}

int configFile::findSection ( const QString& section_name ) const
{
	for ( uint i ( 0 ); i < cfgData.count (); ++i )
	{
		if ( cfgData.at ( i )->section_name == section_name )
			return static_cast<int>( i );
	}
	return -1;
}
//--------------------------------------------CONFIG-FILE--------------------------------

//--------------------------------------------DATA-FILE--------------------------------
dataFile::dataFile ()
	: textFile ()
{
	m_type = TF_DATA;
}

dataFile::dataFile ( const QString& filename )
	: textFile ( filename )
{
	m_type = TF_DATA;
}

dataFile::~dataFile ()
{
	commit (); //save pending changes
}

void dataFile::insertRecord ( const int pos, const stringRecord& rec )
{
	if ( pos >= 0 )
	{
		recData.insertRecord ( static_cast<uint>( pos ), rec );
		m_needsaving = true;
	}
}

void dataFile::changeRecord ( const int pos, const stringRecord& rec )
{
	if ( pos >= 0 && pos < static_cast<int>( recData.countRecords () ) )
	{
		recData.changeRecord ( static_cast<uint>( pos ), rec );
		m_needsaving = true;
	}
}

bool dataFile::deleteRecord( const int pos )
{
	if ( pos >= 0 && static_cast<uint>( pos ) < recData.countRecords () )
	{
		recData.removeRecord ( static_cast<uint>(pos) );
		m_needsaving = true;
		return true;
	}
	return false;
}

void dataFile::appendRecord ( const stringRecord& rec )
{
	recData.fastAppendRecord ( rec );
	m_needsaving = true;
}

bool dataFile::getRecord ( stringRecord& rec, const int pos ) const
{

	if ( pos >= 0 && static_cast<uint>( pos ) < recData.countRecords () )
	{
		rec = recData.readRecord ( static_cast<uint>( pos ) );
		return true;
	}
	return false;
}

int dataFile::getRecord ( const QString& value, const uint field ) const
{
	return recData.findRecordRowByFieldValue ( value, field );
}

void dataFile::clearData ()
{
	recData.clear ();
}

bool dataFile::loadData ( const bool )
{
	QString buf ( m_file.readAll () );
	if ( buf.length () > 1 )
	{
		buf.remove ( 0, buf.indexOf ( CHR_NEWLINE, 0 ) + 1 ); //remove header info
		recData.fromString ( buf );
	}
	return recData.isOK ();
}

bool dataFile::writeData ()
{
	qint64 written ( 0 );
	if ( recData.isOK () )
	{
		const QByteArray data ( recData.toString ().toLocal8Bit () );
		written = m_file.write ( data, static_cast<qint64>( data.size () ) );
	}
	return ( written > 0 );
}
