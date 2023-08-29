#ifndef CLIENT_H
#define CLIENT_H

#include "dbrecord.h"

class clientListItem;

const uint CLIENT_FIELD_COUNT ( 13 );

enum
{
	FLD_CLIENT_ID = 0, FLD_CLIENT_NAME = 1, FLD_CLIENT_STREET = 2, FLD_CLIENT_NUMBER = 3,
	FLD_CLIENT_DISTRICT = 4, FLD_CLIENT_CITY = 5, FLD_CLIENT_ZIP = 6, FLD_CLIENT_PHONES = 7,
	FLD_CLIENT_EMAIL = 8, FLD_CLIENT_STARTDATE = 9, FLD_CLIENT_ENDDATE = 10, FLD_CLIENT_STATUS = 11,
	FLD_CLIENT_LAST_VIEWED = 12
};

class Client : public DBRecord
{

friend class VivenciaDB;
friend class tableView;

public:
	explicit Client ( const bool connect_helper_funcs = false );
	virtual ~Client () override;

	int searchCategoryTranslate ( const SEARCH_CATEGORIES sc ) const override;
	void copySubRecord ( const uint subrec_field, const stringRecord& subrec ) override;

	static inline const QString clientName ( const uint id ) {
		return ( id >= 1 ) ? clientName ( QString::number ( id ) ) : QString ();
	}
	static QString clientName ( const QString& id );
	static uint clientID ( const QString& name );
	static QString concatenateClientInfo ( const Client& client );

	void setListItem ( clientListItem* client_item );
	clientListItem* clientItem () const;

	static const TABLE_INFO t_info;

protected:
	friend bool updateClientTable ( const unsigned char current_table_version );

	RECORD_FIELD m_RECFIELDS[CLIENT_FIELD_COUNT];
	void ( *helperFunction[CLIENT_FIELD_COUNT] ) ( const DBRecord* );
};

#endif // CLIENT_H
