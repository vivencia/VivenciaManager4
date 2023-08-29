#include "dbrecord.h"
#include "vmlibs.h"
#include "vivenciadb.h"
#include "dblistitem.h"

#include <QtSql/QSqlRecord>

VivenciaDB* DBRecord::VDB ( nullptr );
vmCompleters* DBRecord::completer_manager ( new vmCompleters ( false ) );

static const int podBytes ( sizeof ( RECORD_FIELD::i_field ) + sizeof ( RECORD_FIELD::modified ) + sizeof ( RECORD_FIELD::was_modified ) );

static const TABLE_INFO generic_tinfo = {
	0,
	QString (), QString (), QString (),
	QString (), QString (), QString (),
	nullptr, 'A', 0, 0, nullptr
	#ifdef TRANSITION_PERIOD
	, false
	#endif
};

DBRecord::st_Query& DBRecord::st_Query::operator= ( const st_Query& other )
{
	if ( this != &other )
	{
		search = other.search;
		str_query = other.str_query;
		field = other.field;
		reset = other.reset;
		forward = other.forward;
		std::copy ( other.query, other.query, this->query );
	}
	return *this;
}

static inline int recordFieldsCompare_pod ( const RECORD_FIELD& rec1, const RECORD_FIELD& rec2 )
{
	return ::memcmp ( &rec1, &rec2, podBytes );
}

void db_rec_swap ( DBRecord& dbrec1, DBRecord& dbrec2 )
{
	using std::swap;
	
	swap ( dbrec1.t_info, dbrec2.t_info );
	swap ( dbrec1.fld_count, dbrec2.fld_count );
	swap ( dbrec1.mb_modified, dbrec2.mb_modified );
	swap ( dbrec1.mb_synced, dbrec2.mb_synced );
	swap ( dbrec1.mb_completerUpdated, dbrec2.mb_completerUpdated );
	swap ( dbrec1.mb_refresh, dbrec2.mb_refresh );
	swap ( dbrec1.mListItem, dbrec2.mListItem );
	swap ( dbrec1.stquery, dbrec2.stquery );
	swap ( dbrec1.m_RECFIELDS, dbrec2.m_RECFIELDS );
	swap ( dbrec1.mFieldsTypes, dbrec2.mFieldsTypes );
	swap ( dbrec1.helperFunction, dbrec2.helperFunction );
	dbrec1.setAction ( dbrec2.action () );
	dbrec2.setAction ( dbrec1.prevAction () );
	swap ( dbrec1.m_prevaction, dbrec2.m_prevaction );
}

DBRecord::DBRecord ()
	: t_info ( nullptr ), m_RECFIELDS ( nullptr ), mFieldsTypes ( nullptr ), mListItem ( nullptr ), helperFunction ( nullptr ),
	  fld_count ( 0 ), mb_modified ( false ), mb_synced ( true ), mb_completerUpdated ( false ), mb_refresh ( false ), m_action ( ACTION_NONE )
{
	setAction ( ACTION_READ );
}

DBRecord::DBRecord ( const DBRecord& other )
	: DBRecord ()
{
	t_info = other.t_info;
	fld_count = other.fld_count;
	mb_modified = other.mb_modified;
	mb_synced = other.mb_synced;
	mb_completerUpdated = other.mb_completerUpdated;
	mb_refresh = other.mb_refresh;
	mListItem = other.mListItem;
	stquery = other.stquery;
	mFieldsTypes = other.mFieldsTypes;
	
	std::copy ( other.m_RECFIELDS, other.m_RECFIELDS + fld_count, this->m_RECFIELDS );
	std::copy ( other.helperFunction, other.helperFunction + fld_count, this->helperFunction );
	
	m_action = ACTION_NONE;
	setAction ( other.m_action );
	m_prevaction = other.m_prevaction;
}

bool DBRecord::operator== ( const DBRecord& other ) const
{
	if ( other.t_info == this->t_info )
	{
		if ( other.fld_count == this->fld_count )
		{
			for ( uint i ( 0 ); i < fld_count; ++i )
			{
				if ( recordFieldsCompare_pod ( other.m_RECFIELDS[i], this->m_RECFIELDS[i] ) != 0 )
					return false;
				if ( QString::compare ( other.m_RECFIELDS[i].str_field[0], this->m_RECFIELDS[i].str_field[0] ) != 0 )
					return false;
				if ( QString::compare ( other.m_RECFIELDS[i].str_field[1], this->m_RECFIELDS[i].str_field[1] ) != 0 )
					return false;
			}
		}
	}
	return false;
}

void DBRecord::setHelperFunction ( const uint field, void ( *helperFunc ) ( const DBRecord* ) )
{
	if ( field >= 1 && field < fld_count )
		this->helperFunction[field] = helperFunc;
}

void DBRecord::callHelperFunctions ()
{
	if ( helperFunction != nullptr )
	{
		uint i ( 1 ); // any field but ID can have a helper function
		do
		{
			if ( isModified ( i ) || action () == ACTION_DEL )
			{
				if ( helperFunction[i] )
					( *helperFunction[i] ) ( this );
			}
		} while ( ++i < fld_count );
	}
}

bool DBRecord::readRecord ( const uint id, const bool load_data )
{
	if ( id >= 1 )
	{
		if ( load_data ) // By convention, if this is called with load_data = true, we retrieve the data from the db, possibly again
			setRefreshFromDatabase ( true );

		if ( id != static_cast<uint>( actualRecordInt ( 0 ) ) || mb_refresh )
		{
			setIntValue ( 0, static_cast<int>( id ) );
			setBackupValue ( 0, QString::number ( id ) );
			setRefreshFromDatabase ( false );
			return VDB->getDBRecord ( this, 0, load_data );
		}
	}
	return ( id >= 1 );
}

bool DBRecord::readRecord ( const uint field, const QString& search, const bool load_data )
{
	if ( search.isEmpty () )
		return false;

	if ( field != static_cast<uint>( stquery.field ) || search != stquery.search || mb_refresh )
	{
		stquery.reset = true;
		stquery.field = static_cast<int>( field );
		stquery.search = search;
	}
	else
		return true;

	setRefreshFromDatabase ( false );
	return VDB->getDBRecord ( this, stquery, load_data );
}

void DBRecord::resetQuery ()
{
	stquery.reset = true;
	stquery.field = -1;
	if ( stquery.query )
		stquery.query->finish ();
}

bool DBRecord::readFirstRecord ( const bool load_data )
{
	return readRecord ( VDB->getLowestID ( t_info->table_order ), load_data );
}

bool DBRecord::readFirstRecord ( const int field, const QString& search, const bool load_data )
{
	stquery.forward = true;
	stquery.reset = true;
	stquery.field = field;
	stquery.search = search;
	return VDB->getDBRecord ( this, stquery, load_data );
}

bool DBRecord::readLastRecord ( const bool load_data )
{
	return readRecord ( VDB->getHighestID ( t_info->table_order ), load_data );
}

bool DBRecord::readLastRecord ( const int field, const QString& search, const bool load_data )
{
	stquery.forward = false;
	stquery.reset = true;
	stquery.field = field;
	stquery.search = search;
	return VDB->getDBRecord ( this, stquery, load_data );
}

// We browse by position, but the indexes might not be the same as the positions due to records removal
bool DBRecord::readNextRecord ( const bool follow_search, const bool load_data )
{
	if ( !follow_search )
	{
		const uint last_id ( VDB->getHighestID ( t_info->table_order ) );
		if ( last_id >= 1 && static_cast<uint>( actualRecordInt ( 0 ) ) < last_id )
		{
			do
			{
				if ( readRecord ( static_cast<uint>( actualRecordInt ( 0 ) ) + 1, load_data ) )
					return true;
			} while ( static_cast<uint>( actualRecordInt ( 0 ) ) <= last_id );
		}
	}
	else
	{
		stquery.forward = true;
		return VDB->getDBRecord ( this, stquery, load_data );
	}
	return false;
}

bool DBRecord::readPrevRecord ( const bool follow_search, const bool load_data )
{
	if ( !follow_search )
	{
		const uint first_id ( VDB->getLowestID ( t_info->table_order ) );
		if ( first_id >= 1 && actualRecordInt ( 0 ) > 1 )
		{
			do
			{
				if ( readRecord ( static_cast<uint>(actualRecordInt ( 0 )) - 1, load_data ) )
					return true;
			} while ( actualRecordInt ( 0 ) > 0 );
		}
	}
	else
	{
		stquery.forward = false;
		return VDB->getDBRecord ( this, stquery, load_data );
	}
	return false;
}

/* MySQL removes record with ID=0 when the delete query contains the line WHERE ID=''.
 * When the record is new (ACTION_ADD) there is no actualRecordInt(0), but actualRecordStr(0)
 * is an empty string which will delete ID=0. So we must make sure we do not
 * arrive at VDB->removeRecord (), or there will be trouble
 */
bool DBRecord::deleteRecord ()
{
	if ( actualRecordInt ( 0 ) >= 1 )
	{
		setAction ( ACTION_DEL );
		callHelperFunctions ();
		if ( VDB->removeRecord ( this ) )
		{
			clearAll ();
			setAction ( ACTION_READ );
			return true;
		}
	}
	return false;
}

bool DBRecord::saveRecord ( const bool b_changeAction, const bool b_dbaction )
{
	bool ret ( false );
	if ( b_dbaction )
	{
		if ( m_action == ACTION_ADD )
			ret = VDB->insertDBRecord ( this );
		else
			ret = VDB->updateRecord ( this );
	}
	if ( ret || !b_dbaction )
	{
		callHelperFunctions ();
		if ( b_changeAction )
			setAction ( ACTION_READ );
		setAllModified ( false );

		// The mb_completerUpdated flag is just used to speedup execution by not doing the same thing over for
		// all the fields in the respective record that comprise the product´s completer for it
		setCompleterUpdated ( false );
		return true;
	}
	return ret;
}

int DBRecord::searchCategoryTranslate ( const SEARCH_CATEGORIES ) const
{
	return -1;
}

void DBRecord::setModified ( const uint field, const bool modified )
{
	( m_RECFIELDS+field )->modified = modified;
	if ( modified )
		mb_modified = true;
}

void DBRecord::setAllModified ( const bool modified )
{
	uint i ( 1 );
	do
	{
		m_RECFIELDS[i].was_modified = m_RECFIELDS[i].modified;
		m_RECFIELDS[i].modified = modified;
	} while ( ++i < fld_count );
	mb_modified = modified;
}

void DBRecord::clearAll ( const bool b_preserve_id )
{
	uint i ( b_preserve_id ? 1 : 0 );
	do
	{
		m_RECFIELDS[i].str_field[RECORD_FIELD::IDX_ACTUAL].clear ();
		m_RECFIELDS[i].i_field[RECORD_FIELD::IDX_ACTUAL] = 0;
		m_RECFIELDS[i].str_field[RECORD_FIELD::IDX_TEMP].clear ();
		m_RECFIELDS[i].i_field[RECORD_FIELD::IDX_TEMP] = 0;
		m_RECFIELDS[i].modified = false;
		m_RECFIELDS[i].was_modified = false;
	} while  ( ++i < fld_count );
	mb_modified = false;
	mb_synced = true;
	stquery.search.clear ();
}

void DBRecord::setAction ( const RECORD_ACTION action )
{
	if ( action != m_action )
	{
		m_prevaction = m_action;
		m_action = action;
		if ( m_action != ACTION_DEL )
		{
			switch ( action )
			{
				case ACTION_READ:
				case ACTION_REVERT:
					if ( action == ACTION_READ )
					{
						// copy temp values into actual after a save operation. Canceled edits must not be synced
						if ( !inSync () )
							sync ( RECORD_FIELD::IDX_TEMP, false );
					}
					else
						m_action = ACTION_READ;
					mb_synced = true;
					fptr_change = &DBRecord::setValue;
					fptr_changeInt = &DBRecord::setIntValue;
					fptr_recordStr = &DBRecord::actualRecordStr;
					fptr_recordInt = &DBRecord::actualRecordInt;
					fptr_recordStrAlternate = &DBRecord::backupRecordStr;
				break;
				case ACTION_ADD:
					fptr_change = &DBRecord::addValue;
					fptr_changeInt = &DBRecord::addIntValue;
					fptr_recordStr = &DBRecord::actualRecordStr;
					fptr_recordInt = &DBRecord::actualRecordInt;
					fptr_recordStrAlternate = &DBRecord::backupRecordStr;
					DBRecord::createTemporaryRecord ( this );
				break;
				case ACTION_EDIT:
					sync ( RECORD_FIELD::IDX_ACTUAL, true );
					fptr_change = &DBRecord::editValue;
					fptr_changeInt = &DBRecord::editIntValue;
					fptr_recordStr = &DBRecord::backupRecordStr;
					fptr_recordInt = &DBRecord::backupRecordInt;
					fptr_recordStrAlternate = &DBRecord::actualRecordStr;
				break;
				case ACTION_DEL:
				case ACTION_NONE:
				break;
			}
		}
	}
}


void DBRecord::createTemporaryRecord ( DBRecord* dbrec )
{
	const uint table ( dbrec->t_info->table_order );
	const uint new_id ( VivenciaDB::getNextID ( table ) );
	dbrec->setIntValue ( 0, static_cast<int>( new_id ) ); // this is set so that VivenciaDB::insertDBRecord can use the already evaluated value
	dbrec->setIntBackupValue ( 0, static_cast<int>( new_id ) ); // this is set so that calls using recIntValue in a ACTION_ADD record will retrieve the correct value
	const QString str_id ( QString::number ( new_id ) );
	dbrec->setValue ( 0, str_id );
	dbrec->setBackupValue ( 0, str_id );

	if ( dbrec->mListItem != nullptr )
		dbrec->mListItem->setDBRecID ( new_id );
}

void DBRecord::sync ( const int src_index, const bool b_force )
{
	for ( uint i ( 1 ); i < fld_count; ++i )
	{
		if ( wasModified ( i ) || b_force )
		{
			if ( m_RECFIELDS[i].str_field[!src_index] != m_RECFIELDS[i].str_field[src_index] )
			{
				m_RECFIELDS[i].str_field[!src_index] = m_RECFIELDS[i].str_field[src_index];
				m_RECFIELDS[i].i_field[!src_index] = m_RECFIELDS[i].i_field[src_index];
			}
		}
	}
	mb_synced = true;
}

stringRecord DBRecord::toStringRecord ( const QChar rec_sep ) const
{
	stringRecord str_rec ( rec_sep );
	for ( uint i ( 0 ); i < fld_count; ++i )
		str_rec.fastAppendValue ( recStrValue ( this, i ) );
	return str_rec;
}

void DBRecord::fromStringRecord ( const stringRecord& str_rec, const uint fromField )
{
	bool ok ( false );
	if ( fromField > 0 )
		ok = str_rec.moveToRec ( fromField );
	else
		ok = str_rec.first ();

	if ( ok )
	{
		setRecValue ( this, 0, str_rec.curValue () );
		setRecIntValue ( this, 0, str_rec.curValue ().toInt () );
		uint i ( 1 );
		do
		{
			if ( str_rec.next () )
			{
				//if ( fieldType ( i ) != DBTYPE_SUBRECORD )
					setRecValue ( this, i, str_rec.curValue () );
				//else
				//	copySubRecord ( i, str_rec ); // this func will advance the pointer inside the string record to after the sub string record
				++i;
			}
			else
				break;
		} while ( true );
	}
}

void DBRecord::setCompleterManager ( vmCompleters* const completer )
{
	delete DBRecord::completer_manager;
	DBRecord::completer_manager = completer;
}

void DBRecord::contains ( const QString& value, podList<uint>& fields ) const
{
	fields.clearButKeepMemory ();
	for ( uint i ( 0 ); i < t_info->field_count; ++i )
	{
		if ( recStrValue ( this, i ).contains ( value, Qt::CaseInsensitive ) )
			fields.append ( i );
	}
}
