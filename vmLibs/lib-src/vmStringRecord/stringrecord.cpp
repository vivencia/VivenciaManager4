#include "stringrecord.h"

static const stringRecord emptyStringRecord;
static const stringTable emptyStringTable;

static void s_appendEmptyFields ( QString& data, const uint field, uint& nfields, const QChar* __restrict separator )
{
	if ( field >= nfields )
	{
		uint i_field ( field - nfields + 1 );
		nfields += i_field;
		while ( i_field-- > 0 )
		{
			data += *separator;
		}
	}
}

static void s_insertFieldValue ( QString& data, const uint field, const QString& value, uint& __restrict nfields, 
								 const QChar* __restrict const separator, const QStringMatcher* __restrict const sep_matcher )
{
	s_appendEmptyFields ( data, field, nfields, separator );
	if ( field == 0 )
	{
		if ( data.at ( 0 ) == *separator )
		{
			data.insert ( 0, value );
		}
		else
		{
			data.insert ( 0, value + *separator );
			++nfields;
		}
		return;
	}

	int idx ( -1 );
	int idx2 ( 0 );
	auto i_field ( static_cast<int>( field ) );

	do
	{
		idx2 = sep_matcher->indexIn ( data, idx + 1 );
		if ( idx2 >= 0 )
		{
			if ( i_field == 0 )
			{
				if ( data.at ( idx2 - 1 ) != *separator )
				{
					break;
				}
			}
			idx = idx2;
		}
		--i_field;
	} while ( i_field >= 0 );
	if ( !value.isEmpty () )
	{
		if ( idx >= 0 )
		{
			if ( data.at ( idx - 1 ) == *separator )  // empty field
			{
				data.insert ( idx, value );
			}
			else
			{
				data.insert ( idx, *separator + value );
				++nfields;
			}
		}
	}
}

static void s_changeFieldValue ( QString& data, const uint field, const QString& value, uint& __restrict nfields,
								 const QChar* __restrict const separator, const QStringMatcher* __restrict const sep_matcher )
{
	s_appendEmptyFields ( data, field, nfields, separator );
	int idx ( 0 );
	int idx2 ( 0 );
	auto i_field ( static_cast<int>( field ) );

	do
	{
		idx2 = sep_matcher->indexIn ( data, idx );
		if ( idx2 >= 0 )
		{
			if ( i_field > 0 )
			{
				idx = idx2 + 1;
			}
		}
		--i_field;
	} while ( i_field >= 0 );
	const QStringRef cur_value ( data.midRef ( idx, idx2 - idx ) );

	if ( cur_value.isEmpty () )
	{
		data.insert ( idx, value );
	}
	else
	{
		data.replace ( idx, cur_value.length (), value );
	}
}

static bool s_removeField ( QString& data, const uint field, uint& nfields, const QStringMatcher* __restrict const sep_matcher  )
{
	if ( field < nfields )
	{
		auto i_field ( static_cast<int>( field ) );
		int idx ( 0 );
		int idx2 ( 0 );
		do
		{
			idx2 = sep_matcher->indexIn ( data, idx );
			if ( idx2 >= 0 )
			{
				if ( i_field > 0 )
				{
					idx = idx2 + 1;
				}
			}
			else
			{
				break;
			}
			--i_field;
		} while ( i_field >= 0 );
		if ( idx >= 0 )
		{
			data.remove ( idx, ( idx2 - idx ) + 1 );
			--nfields;
			return true;
		}

	}
	return false;
}

void strrec_swap ( stringRecord& s_rec1, stringRecord& s_rec2 )
{
	using std::swap;
	swap ( s_rec1.mData, s_rec2.mData );
	swap ( s_rec1.mFields, s_rec2.mFields );
	swap ( s_rec1.mFastIdx, s_rec2.mFastIdx );
	swap ( s_rec1.mState, s_rec2.mState );
	swap ( s_rec1.mCurValue, s_rec2.mCurValue );
	swap ( s_rec1.record_sep, s_rec2.record_sep );
}

void strtable_swap ( stringTable& s_table1, stringTable& s_table2 )
{
	using std::swap;
	swap ( s_table1.mRecords, s_table2.mRecords );
	swap ( s_table1.nRecords, s_table2.nRecords );
	swap ( s_table1.mFastIdx, s_table2.mFastIdx );
	swap ( s_table1.mCurIdx, s_table2.mCurIdx );
	swap ( s_table1.table_sep, s_table2.table_sep );
	swap ( s_table1.tablesep_matcher, s_table2.tablesep_matcher );
	strrec_swap ( s_table1.mRecord, s_table2.mRecord );
	swap ( s_table1.mCurRecord, s_table2.mCurRecord );
}

//------------------------------------------------------RECORD-----------------------------------------------
stringRecord::stringRecord ( const stringRecord& other )
{
	mData = other.mData;
	mFields = other.mFields;
	mFastIdx = other.mFastIdx;
	mState = other.mState;
	setFieldSeparationChar ( other.record_sep );
}

stringRecord::stringRecord (const QString& str, const QChar sep )
	: stringRecord ()
{
	setFieldSeparationChar ( sep );
	fromString ( str );
}

bool stringRecord::isOK () const
{
	switch ( mState.state () )
	{
		case TRI_ON:	return true;
		case TRI_OFF:	return mData.endsWith ( record_sep );
		case TRI_UNDEF:	return false;
	}
	return false;
}

void stringRecord::fromString ( const QString& str )
{
	// making the stringRecord load an empty string or a unformatted string, sets mState
	// to Undefined, a means for callers to check the returned value and see that it is an invalid result.
	// Off is not an invalid result, only marks the stringRecord as being empty
	if ( str.isEmpty () )
	{
		clear ();
		mState.setUndefined (); // is this a good idea? is it really needed? is it useful?
	}
	else
	{
		mData = str;
		if ( !mData.endsWith ( record_sep ) )
		{
			mData += record_sep;
		}
		mState.setOn ();
		mFields = static_cast<uint>( mData.count ( record_sep ) );
	}
	mFastIdx = -1;
}

void stringRecord::clear ()
{
	mData.clear ();
	mFields = 0;
	mFastIdx = -1;
	mState.setOff ();
}

void stringRecord::insertField ( const uint field, const QString& value )
{
	s_insertFieldValue ( mData, field, value, mFields, &record_sep, &recsep_matcher );
}

void stringRecord::changeValue ( const uint field, const QString& value )
{
	s_changeFieldValue ( mData, field, value, mFields, &record_sep, &recsep_matcher );
}

void stringRecord::removeField ( const uint field )
{
	s_removeField ( mData, field, mFields, &recsep_matcher );
}

bool stringRecord::removeFieldByValue ( const QString& value, const bool b_allmatches )
{
	if ( nFields () > 0 )
	{
		uint field ( 0 );
		/* Save variables state so that this function can be used while this instance
		 * is being iterated elsewhere, without interfering with what is being done with it
		 */
		const QString mCurValue_backup ( mCurValue );
		const int mFastIdx_backup ( mFastIdx );

		if ( first () )
		{
			bool found ( false );
			do
			{
				if ( curValue () == value )
				{
					removeField ( field );
					found = true;
					if ( !b_allmatches )
					{
						break;
					}
				}
				++field;
			} while ( next () );
			mCurValue = mCurValue_backup;
			mFastIdx = mFastIdx_backup;
			return found;
		}
	}
	return false;
}

void stringRecord::insertStrRecord ( const int field, const QString& inserting_rec )
{
	if ( field >= 0 )
	{
		insertField ( static_cast<uint>( field ), inserting_rec.left ( inserting_rec.count () - 1 ) );
	}
}

void stringRecord::appendStrRecord ( const QString& inserting_rec )
{
	fastAppendValue ( inserting_rec.left ( inserting_rec.count () - 1 ) );
}

const QString stringRecord::section ( const uint start_field, int end_field ) const
{
	const auto last_field ( static_cast<int>( nFields () ) );
	if ( ( last_field >= 1 ) && ( start_field < static_cast<uint>( last_field ) ) )
	{
		if ( (end_field == -1) || (end_field > last_field) )
		{
			end_field = last_field;
		}

		int field ( 0 );
		int idx ( 0 );
		int idx2 ( 0 );
		do
		{
			idx2 = recsep_matcher.indexIn ( mData, idx );
			if ( idx2 >= 0 )
			{
				if ( static_cast<uint>( field ) < start_field )
				{
					idx = idx2 + 1;
				}
				else
				{
					break;
				}
			}
			++field;
		} while ( true );
		
		const int left ( idx );
		--field;
		++idx;
		if ( start_field == 0 )
		{
			--field;
		}

		do
		{
			idx2 = recsep_matcher.indexIn ( mData, idx );
			if ( idx2 >= 0 )
			{
				if ( field < end_field )
				{
					idx = idx2 + 1;
				}
				else
				{
					break;
				}
			}
			++field;
		} while ( true );
		const int right ( idx );
		return mData.mid ( left, right - left );
	}
	return QString ();
}


const QString stringRecord::fieldValue ( uint field ) const
{
	const uint fields ( nFields () );
	if ( field < fields )
	{
		int idx ( 0 );
		int idx2 ( 0 );
		do
		{
			idx2 = recsep_matcher.indexIn ( mData, idx );
			if ( idx2 >= 0 )
			{
				if ( field > 0 )
				{
					idx = idx2 + 1;
				}
			}
			if ( field == 0 )
			{
				break;
			}
			--field;
		} while ( true );
		return mData.mid ( idx, idx2 - idx );
	}
	return QString ();
}

void stringRecord::fastAppendValue ( const QString& value )
{
	mData += value + record_sep;
	mFields++;
}

int stringRecord::field ( const QString& value, const int init_idx, const bool bprecise ) const
{
	if ( isOK () )
	{
		int idx ( init_idx + 1 );
		int idx2 ( 0 );
		int field ( 0 );
		do
		{
			idx2 = recsep_matcher.indexIn ( mData, idx );
			if ( idx2 >= 0 )
			{
				if ( bprecise )
				{
					if ( value == mData.midRef ( idx, idx2 - idx ) )
					{
						return field;
					}
				}
				else
				{
					if ( mData.midRef ( idx, idx2 - idx ).contains ( value ) )
					{
						return field;
					}
				}
				idx = idx2 + 1;
			}
		} while ( static_cast<uint>(++field) < nFields () );
	}
	return -1;
}

const QString stringRecord::fieldValue ( const QString& str_record, const uint field, const QChar rec_sep )
{
	int idx ( 0 );
	int idx2 ( 0 );
	auto fld ( static_cast<int>( field ) );
	QStringMatcher recsep_matcher ( rec_sep );

	do
	{
		idx2 = recsep_matcher.indexIn ( str_record, idx );
		if ( idx2 >= 0 )
		{
			if ( fld > 0 )
			{
				idx = idx2 + 1;
			}
		}
		else
		{
			return QString ();
		}
	} while ( --fld >= 0 );
	return str_record.mid ( idx, idx2 - idx );
}

bool stringRecord::first () const
{
	const int idx2 ( mData.indexOf ( record_sep, 0 ) );
	if ( idx2 != -1 )
	{
		mCurValue = mData.mid ( 0, idx2 );
		mFastIdx = idx2 + 1;
		return true;
	}
	return false;
}

bool stringRecord::next () const
{
	const int idx2 ( mData.indexOf ( record_sep, mFastIdx ) );
	if ( idx2 != -1 && idx2 < mData.count () )
	{
		mCurValue = mData.mid ( mFastIdx, idx2 - mFastIdx );
		mFastIdx = idx2 + 1;
		return true;
	}
	return false;
}

bool stringRecord::prev () const
{
	const int idx2 ( mData.lastIndexOf ( record_sep, mFastIdx - mData.count () - 2 ) );
	if ( idx2 != -1 || mFastIdx > 0 )
	{
		mCurValue = mData.mid ( idx2 + 1, mFastIdx - idx2 - 2 );
		mFastIdx = idx2 + 1;
		return true;
	}
	return false;
}

bool stringRecord::last () const
{
	const int idx2 ( mData.lastIndexOf ( record_sep, -2 ) );
	if ( idx2 != -1 )
	{
		mCurValue = mData.mid ( idx2 + 1, mData.count () - idx2 - 2 );
		mFastIdx = idx2 + 1;
		return true;
	}
	return false;
}

bool stringRecord::moveToRec ( const uint rec ) const
{
	int idx ( 0 );
	uint counter ( 0 );
	const int length ( mData.count () );
	while ( idx < length )
	{
		mFastIdx = mData.indexOf ( record_sep, idx ) + 1;
		if ( mFastIdx > 0 )
		{
			if ( counter == rec )
			{
				mCurValue = mData.mid ( idx, mFastIdx - idx - 1 );
				return true;
			}
			++counter;
			idx = mFastIdx;
		}
	}
	return false;
}

void stringRecord::fixFieldsAndSeparators ( const QLatin1Char correct_sep, const QLatin1Char wrong_sep )
{
	mData.remove ( wrong_sep );
	if ( !mData.endsWith ( correct_sep ) )
	{
		mData += correct_sep;
	}
	mState.setOn ();
	mFields = static_cast<uint>( mData.count ( correct_sep ) );
}
//------------------------------------------------------RECORD-----------------------------------------------

//------------------------------------------------------TABLE------------------------------------------------
stringTable::stringTable ( const stringTable& other )
{
	mRecords = other.mRecords;
	nRecords = other.nRecords;
	mFastIdx = other.mFastIdx;
	mCurIdx = other.mCurIdx;
	mRecord = other.mRecord;
	mCurRecord = other.mCurRecord;
	setRecordSeparationChar ( other.table_sep );
}

stringTable::stringTable ( const QString& str, const QChar sep )
	: stringTable ()
{
	setRecordSeparationChar ( sep );
	fromString ( str );
}

bool stringTable::isOK () const
{
	return mRecords.endsWith ( table_sep );
}

void stringTable::fromString ( const QString& str )
{
	if ( !str.isEmpty () )
	{
		mRecords = str;
		if ( !mRecords.endsWith ( table_sep ) )
		{
			mRecords += table_sep;
		}
		nRecords = static_cast<uint>( mRecords.count ( table_sep ) );
		mFastIdx = -1;
		mCurIdx = -1;
	}
}

void stringTable::clear ()
{
	mCurRecord.clear ();
	mRecords.clear ();
	mRecord.clear ();
	nRecords = 0;
	mFastIdx = -1;
	mCurIdx = -1;
}

void stringTable::insertRecord ( const uint row, const QString& record )
{
	s_insertFieldValue ( mRecords, row, record, nRecords, &table_sep, &tablesep_matcher );
	mCurIdx = static_cast<int>( row );
}

void stringTable::changeRecord ( const uint row, const QString& record )
{
	s_changeFieldValue ( mRecords, row, record, nRecords, &table_sep, &tablesep_matcher );
}

void stringTable::changeRecord ( const uint row, const uint field, const QString& new_value )
{
	readRecord ( row );
	mRecord.changeValue ( field, new_value );
	changeRecord ( row, mRecord );
}

void stringTable::removeRecord ( const uint row )
{
	s_removeField ( mRecords, row, nRecords, &tablesep_matcher );
	if ( mCurIdx >= static_cast<int>( row ) )
	{
		mCurIdx--;
	}
}

bool stringTable::removeRecordByValue ( const QString& record, const bool b_allmatches )
{
	bool found ( false );
	if ( nRecords > 0 )
	{
		uint row ( 0 );
		do
		{
			if ( readRecord ( row ).toString () == record )
			{
				removeRecord ( row );
				found = true;
				if ( !b_allmatches )
				{
					break;
				}
			}
		} while ( ++row < nRecords );
	}
	return found;
}

const stringRecord& stringTable::readRecord ( uint row ) const
{
	const uint rows ( countRecords () );
	if ( row < rows )
	{
		int idx ( 0 );
		int idx2 ( 0 );
		mCurIdx = static_cast<int>( row );
		do
		{
			idx2 = tablesep_matcher.indexIn ( mRecords, idx );
			if ( idx2 >= 0 )
			{
				if ( row > 0 )
				{
					idx = idx2 + 1;
				}
			}
			if ( row == 0 )
			{
				break;
			}
			row--;
		} while ( true );
		mRecord.fromString ( mRecords.mid ( idx, idx2 - idx ) );
	}
	else
	{
		mRecord.clear ();
	}
	return mRecord;
}

int stringTable::findRecordRowByFieldValue ( const QString& value, const uint field, const uint nth_occurrence ) const
{
	const int fast_idx_saved ( mFastIdx );
	int row ( -1 );
	bool found ( false );

	if ( first ().isOK () )
	{
		uint n_occurrences ( 0 );
		row++;
		do
		{
			if ( mRecord.fieldValue ( field ) == value )
			{
				if ( n_occurrences == nth_occurrence )
				{
					found = true;
					break;
				}
				++n_occurrences;
			}
			row++;
		} while ( next ().isOK () );
	}
	mFastIdx = fast_idx_saved;
	return found ? mCurIdx = row : -1;
}

int stringTable::findRecordRowThatContainsWord ( const QString& word, podList<uint>* dayAndFieldList, const int field, const uint nth_occurrence ) const
{
	const int fast_idx_saved ( mFastIdx );
	int row ( -1 );

	if ( first ().isOK () )
	{
		uint n_occurrences ( 0 );
		int foundField ( -1 );
		row++;
		do
		{
			if ( field != -1 )
			{
				if ( mRecord.fieldValue ( static_cast<uint>( field ) ).contains ( word ) )
				{
					foundField = field;
				}
			}
			else
			{
				foundField = mRecord.field ( word, -1, false );
			}

			if ( foundField >= 0 )
			{
				if ( dayAndFieldList != nullptr )
				{
					dayAndFieldList->operator [] ( row ) = static_cast<uint>( foundField );
				}
				if ( n_occurrences == nth_occurrence )
				{
					break;
				}

				foundField = -1;
				++n_occurrences;
			}
			row++;
		} while ( next ().isOK () );
	}
	mFastIdx = fast_idx_saved;
	return (dayAndFieldList != nullptr ? (dayAndFieldList->isEmpty () ? -1 : mCurIdx = row) : -1);
}

int stringTable::matchRecord ( const stringRecord& record )
{
	const int fast_idx_saved ( mFastIdx );
	bool b_match ( true );
	if ( first ().isOK () )
	{
		do
		{
			if ( record.first () )
			{
				static_cast<void>( mRecord.first () );
				do
				{
					if ( mRecord.curValue () == record.curValue () )
					{
						continue;
					}
					b_match = false;
					break;
				} while ( record.next () );
			}
		} while ( next ().isOK () );
	}
	mFastIdx = fast_idx_saved;
	return b_match ? currentIndex () : -1;
}

void stringTable::fastAppendRecord ( const QString& record )
{
	mRecords += record + table_sep;
	nRecords++;
}

void stringTable::appendTable ( const QString& inserting_table )
{
	if ( inserting_table.endsWith ( table_sep ) )
	{
		mRecords += inserting_table.left ( inserting_table.count () - 1 ) + table_sep;
		nRecords += static_cast<uint>( inserting_table.count ( table_sep ) );
	}
}

bool stringTable::firstStr () const
{
	const int idx2 ( mRecords.indexOf ( table_sep, 0 ) );
	if ( idx2 != -1 )
	{
		mCurRecord = mRecords.mid ( 0, idx2 );
		mFastIdx = idx2 + 1;
		mCurIdx = 0;
		return true;
	}
	return false;
}

bool stringTable::nextStr () const
{
	const int idx2 ( mRecords.indexOf ( table_sep, mFastIdx ) );
	if ( idx2 != -1 && idx2 < mRecords.count () )
	{
		mCurRecord = mRecords.mid ( mFastIdx, idx2 - mFastIdx );
		mFastIdx = idx2 + 1;
		mCurIdx++;
		return true;
	}
	return false;
}

bool stringTable::prevStr () const
{
	const int idx2 ( mRecords.lastIndexOf ( table_sep, mFastIdx - mRecords.count () - 2 ) );
	if ( idx2 != -1 || mFastIdx > 0 )
	{
		mCurRecord = mRecords.mid ( idx2 + 1, mFastIdx - idx2 - 2 );
		mFastIdx = idx2 + 1;
		mCurIdx--;
		return true;
	}
	return false;
}

bool stringTable::lastStr () const
{
	const int idx2 ( mRecords.indexOf ( table_sep, -2 ) );
	if ( idx2 != -1 )
	{
		mCurRecord = mRecords.mid ( idx2 + 1, mRecords.count () - idx2 - 2 );
		mFastIdx = idx2 + 1;
		mCurIdx = static_cast<int>( nRecords );
		return true;
	}
	return false;
}

const stringRecord& stringTable::first () const
{
	if ( firstStr () )
	{
		mRecord.fromString ( mCurRecord );
		return mRecord;
	}
	return emptyStringRecord;
}

const stringRecord& stringTable::next () const
{
	if ( nextStr () )
	{
		mRecord.fromString ( mCurRecord );
		return mRecord;
	}
	return emptyStringRecord;
}

const stringRecord& stringTable::prev () const
{
	if ( prevStr () )
	{
		mRecord.fromString ( mCurRecord );
		return mRecord;
	}
	return emptyStringRecord;
}

const stringRecord& stringTable::last () const
{
	if ( lastStr () )
	{
		mRecord.fromString ( mCurRecord );
		return mRecord;
	}
	return emptyStringRecord;
}
//------------------------------------------------------TABLE------------------------------------------------
