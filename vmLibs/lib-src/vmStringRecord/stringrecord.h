#ifndef STRINGRECORD_H
#define STRINGRECORD_H

#include "vmlist.h"

#include <vmUtils/tristatetype.h>

#include <QtCore/QString>
#include <QtCore/QChar>
#include <QtCore/QStringMatcher>

static const QLatin1Char record_separator ( 30 );
static const QLatin1Char table_separator ( 29 );
static const QLatin1Char public_rec_sep ( 31 );
static const QLatin1Char public_table_sep ( 28 );
//------------------------------------------------------RECORD-----------------------------------------------
class stringRecord
{

public:
	friend void strrec_swap ( stringRecord& s_rec1, stringRecord& s_rec2 );
	
	inline stringRecord ()
		: mFields ( 0 ), mFastIdx ( -1 ), mState ( TRI_UNDEF )
	{
		setFieldSeparationChar ( record_separator );
	}
	
	stringRecord ( const stringRecord& other );
	stringRecord ( const QString& str, const QChar sep = record_separator );
	
	inline const stringRecord& operator= ( stringRecord other )
	{
		strrec_swap ( *this, other );
		return *this;
	}

	inline const stringRecord& operator= ( stringRecord&& other )
	{
		strrec_swap ( *this, other );
		return *this;
	}
	
	inline stringRecord ( stringRecord&& other ) : stringRecord ()
	{
		strrec_swap ( *this, other );
	}
	
	inline void setFieldSeparationChar ( const QChar& chr )
	{
		record_sep = chr;
		recsep_matcher.setPattern ( record_sep );
	}
	
	inline uint countFields ( const QString& str ) const
	{
		return static_cast<uint>(str.count ( record_sep ));
	}

	bool isOK () const;
	void fromString ( const QString& str );
	inline uint nFields () const {
		return mFields;
	}
	void clear ();
	void insertField ( const uint field, const QString& value = QString () );
	void changeValue ( const uint field, const QString& value );
	void removeField ( const uint field );
	bool removeFieldByValue ( const QString& value, const bool b_allmatches = false );
	void insertStrRecord ( const int field, const QString& inserting_rec );
	inline void insertStrRecord ( const int field, const stringRecord& inserting_rec ) {
		insertStrRecord ( field, inserting_rec.toString () );
	}
	void appendStrRecord ( const QString& inserting_rec );
	inline void appendStrRecord ( const stringRecord& inserting_rec ) { appendStrRecord ( inserting_rec.toString () ); }

	const QString section ( const uint start_field, int end_field = -1 ) const;
	const QString fieldValue ( uint field ) const;
	void fastAppendValue ( const QString& value );
	int field ( const QString& value, const int init_idx = -1, const bool bprecise = true ) const;
	inline const QString& toString () const {
		return mData;
	}

	static const QString fieldValue ( const QString& str_record, const uint field, const QChar rec_sep = record_separator );

	/* Do not rely on curValue () alone after calling any of these functions, rather rely on
	 * their return value. In order to speed things up I will not clear mCurValue if a result is
	 * not found so it will remain with its last valid value. Also, do not call next () and then prev ()
	 * or vice-versa, for the index pointer will be swapped when changing directions. Actually, only after the
	 * second call will it be at the right position, i.e. next () -> prev () -> prev () will yield the expected result
	 */
	bool first () const;
	bool next () const;
	bool prev () const;
	bool last () const;
	bool moveToRec ( const uint rec ) const;

	inline int curIndex () const {
		return mFastIdx;
	}
	
	inline const QString& curValue () const {
		return mCurValue;
	}

	static inline QString joinStringRecords ( const QString& rec1, const QString& rec2, const QChar& sep = record_separator )
	{
		return rec1 + rec2 + sep;
	}

	// a Null stringRecord is not to be used, unless when returning an invalid result. This is so the caller
	// knows that it should not use that returned object, and discard it
	struct Null {};
	static constexpr Null null = {};
	inline explicit stringRecord ( const Null& ) : mFields ( 0 ), mFastIdx ( -1 ), mState ( TRI_UNDEF ) {}
	inline void setNull () {
		mState.setUndefined ();
	}
	inline bool isNull () const {
		return mState.isUndefined ();
	}

	/* This function corrects the misbehavior of stringRecords that are read from a dataFile. Because that class
	 * uses stringRecords to transfer information and because those stringRecords may contain sub-stringRecords, it changes
	 * the separator of its internal stringRecord object. When a stringRecord is retrieved from dataFile it contains the
	 * original structure plus a terminating separator that was introduced by dataFile. We remove this separator, and correct other
	 * information that were skewed. The arguments are provided for further flexibility, but anything different from the defaults
	 * was not tested
	 */
	void fixFieldsAndSeparators ( const QLatin1Char correct_sep, const QLatin1Char wrong_sep );

private:
	QChar record_sep;
	QStringMatcher recsep_matcher;
	QString mData;
	mutable QString mCurValue;
	uint mFields;
	mutable int mFastIdx;
	triStateType mState;
};

static const stringRecord emptyStrRecord;
//------------------------------------------------------RECORD-----------------------------------------------

//------------------------------------------------------TABLE------------------------------------------------
class stringTable
{

public:
	friend void strtable_swap ( stringTable& s_table1, stringTable& s_table2 );
	
	inline stringTable ()
		: nRecords ( 0 ), mFastIdx ( -1 ), mCurIdx ( -1 )
	{
		setRecordSeparationChar ( table_separator );
	}
	
	inline explicit stringTable ( const QChar& sep )
		: stringTable ()
	{
		setRecordSeparationChar ( sep );
	}
	
	stringTable ( const stringTable& other );
	stringTable ( const QString& str, const QChar sep = table_separator );
	
	inline stringTable ( stringTable&& other ) : stringTable ()
	{
		strtable_swap ( *this, other );
	}

	inline const stringTable& operator= ( stringTable other )
	{
		strtable_swap ( *this, other );
		return *this;
	}
	
	inline void setRecordSeparationChar ( const QChar& chr )
	{
		table_sep = chr;
		tablesep_matcher.setPattern ( table_sep );
	}

	inline void setRecordFieldSeparationChar ( const QChar& chr )
	{
		mRecord.setFieldSeparationChar ( chr );
	}

	bool isOK () const;
	void fromString ( const QString& str );
	
	inline uint countRecords () const { return nRecords; }
	void clear ();

	void insertRecord ( const uint row, const QString& record );
	inline void insertRecord ( const uint row, const stringRecord& record )
	{
		insertRecord ( row, record.toString () );
	}
	void changeRecord ( const uint row, const QString& record );
	inline void changeRecord ( const uint row, const stringRecord& record ) {
		changeRecord ( row, record.toString () );
	}
	void changeRecord ( const uint row, const uint field, const QString& new_value );
	bool removeRecordByValue ( const QString& record, const bool b_allmatches = false );
	inline bool removeRecordByValue ( const stringRecord& record, const bool b_allmatches = false ) {
		return removeRecordByValue ( record.toString (), b_allmatches );
	}
	void removeRecord ( const uint row );

	const stringRecord& readRecord ( uint row ) const;
	int findRecordRowByFieldValue ( const QString& value, const uint field, const uint nth_occurrence = 0 ) const;
	int findRecordRowThatContainsWord ( const QString& word, podList<uint>* dayAndFieldList = nullptr, const int field = -1, const uint nth_occurrence = 0 ) const;
	int matchRecord ( const stringRecord& record );
	void fastAppendRecord ( const QString& record );
	inline void fastAppendRecord ( const stringRecord& record ) {
		fastAppendRecord ( record.toString () );
	}
	void appendTable ( const QString& inserting_table );

	inline const QString& toString () const {
		return mRecords;
	}

	const stringRecord& first () const;
	const stringRecord& next () const;
	const stringRecord& prev () const;
	const stringRecord& last () const;

	bool firstStr () const;
	bool nextStr () const;
	bool prevStr () const;
	bool lastStr () const;

	inline const stringRecord& curRecord () const {
		return mRecord;
	}
	inline const QString& curRecordStr () const {
		return mCurRecord;
	}
	
	inline int currentIndex () const {
		return mCurIdx;
	}

private:
	QChar table_sep;
	QStringMatcher tablesep_matcher;
	QString mRecords;
	mutable stringRecord mRecord;
	mutable QString mCurRecord;
	uint nRecords;
	mutable int mFastIdx;
	mutable int mCurIdx;
};
//------------------------------------------------------TABLE------------------------------------------------

#endif // STRINGRECORD_H
