#ifndef textFile_H
#define textFile_H

#include "vmlist.h"
#include "tristatetype.h"

#include <vmStringRecord/stringrecord.h>
#include <vmNumbers/vmnumberformats.h>

#include <QtCore/QFile>

class vmFileMonitor;

//--------------------------------------------TEXT-FILE--------------------------------
class textFile
{

public:

	enum TF_TYPE { TF_UNKNOWN, TF_TEXT, TF_CONFIG, TF_DATA };

	explicit textFile ();
	textFile ( const QString& filename );
	virtual ~textFile ();

	inline void setFileName ( const QString& filename ) { m_filename = filename; }
	inline const QString& fileName () { return m_filename; }

	static bool isTextFile ( const QString& filename, const TF_TYPE type );
	inline TF_TYPE type () const { return m_type; }
	void remove ();

	inline bool isOpen () const { return m_open; }
	inline bool isEmpty () const { return m_file.size () <= 0; }

	void close ();
	triStateType load ();
	void commit ();

	void setText ( const QString& new_file_text );
	inline QString text () const { return m_data; }

	void setIgnoreEvents ( const bool b_ignore );

protected:
	bool open ();
	bool open2 ();
	void readType ();
	bool write ();
	void writeHeader ();
	void fileExternallyAltered ( const QString&, const uint event );

	virtual bool loadData ( const bool b_replaceBuffers = true );
	virtual bool writeData();
	virtual void clearData ();

	bool m_open;
	bool m_needsaving;
	bool mb_needRechecking;
	int64_t m_headerSize;
	TF_TYPE m_type;
	QString m_filename;
	QString m_data;
	QFile m_file;
	vmFileMonitor* m_filemonitor;
	bool mb_IgnoreEvents;
	vmNumber m_readTime;
	vmNumber m_readDate;
};
//--------------------------------------------TEXT-FILE--------------------------------

//--------------------------------------------CONFIG-FILE--------------------------------
class configFile : public textFile
{

public:
	explicit configFile ();
	configFile ( const QString& filename, const QString& object_name );
	virtual ~configFile ();

	//Mandatory call, so that we alter only the indicated sections in a config file and not mess with others
	inline void addManagedSection ( const QString& section_name )
	{
		mlst_managedSections.append ( section_name );
	}

	bool setWorkingSection ( const QString& section_name );
	const QString& fieldValue ( const QString& field_name ) const;
	int fieldIndex ( const QString& field_name ) const;

	void insertNewSection ( const QString& section_name );
	void deleteSection ( const QString& section_name );
	void insertField ( const QString& field_name, const QString& value );
	void deleteField ( const QString& field_name );

	bool setFieldValue ( const QString& field_name, const QString& value );

	inline uint sectionCount () const { return cfgData.count (); }

protected:
	bool loadData ( const bool b_replaceBuffers = true );
	bool writeData ();
	void clearData ();

	int findSection ( const QString& section_name ) const;
	bool parseConfigFile ( const bool b_reload = false, const bool b_load_non_managed = false );

private:
	struct configFile_st;
	pointersList<configFile_st*> cfgData;
	QString m_objectName;
	QStringList mlst_managedSections;
};
//--------------------------------------------CONFIG-FILE--------------------------------

//--------------------------------------------DATA-FILE--------------------------------
class dataFile : public textFile
{
public:
	explicit dataFile ();
	dataFile ( const QString& filename );
	virtual ~dataFile ();

	inline void setRecordSeparationChar ( const QChar& table_sep = table_separator, const QChar& rec_sep = record_separator )
	{
		recData.setRecordSeparationChar ( table_sep );
		recData.setRecordFieldSeparationChar ( rec_sep );
	}

	void insertRecord ( const int pos, const stringRecord& rec );
	void changeRecord ( const int pos, const stringRecord& rec );
	bool deleteRecord ( const int pos );
	void appendRecord ( const stringRecord& rec );
	bool getRecord ( stringRecord& rec, const int pos ) const;
	int getRecord ( const QString& value, const uint field ) const; //returns the first record index whose row contains "value" in any column

	inline uint recCount () const { return recData.countRecords (); }

protected:
	void clearData ();
	bool loadData ( const bool b_replaceBuffers = true );
	bool writeData ();

private:
	stringTable recData;
};
//--------------------------------------------DATA-FILE--------------------------------

#endif // textFile_H
