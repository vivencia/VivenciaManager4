#include "client.h"

#include <vmUtils/configops.h>
#include <vmUtils/fileops.h>

#include "dblistitem.h"
#include "vivenciadb.h"

const unsigned char TABLE_VERSION ( 'B' );

constexpr DB_FIELD_TYPE CLIENTS_FIELDS_TYPE[CLIENT_FIELD_COUNT] = {
	DBTYPE_ID, DBTYPE_LIST, DBTYPE_LIST, DBTYPE_NUMBER, DBTYPE_LIST, DBTYPE_LIST,
	DBTYPE_NUMBER, DBTYPE_SUBRECORD, DBTYPE_SUBRECORD, DBTYPE_DATE, DBTYPE_DATE,
	DBTYPE_YESNO, DBTYPE_YESNO
};

bool updateClientTable ( const unsigned char /*current_table_version*/ )
{
	return false;
	Client client;
	if ( client.readFirstRecord () )
	{
		QString name, city;
		do
		{
			name = recStrValue ( &client, FLD_CLIENT_NAME );
			if ( !name.isEmpty () )
			{
				client.setAction ( ACTION_EDIT );
				if ( name.startsWith ( "Viv" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Vivência" ) );
				}
				if ( name.contains ( "Anast" ) )
				{
					if ( name.contains ( "Marcelo" ) )
						setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Marcelo da Anastácia" ) );
					else
						setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Anastácia" ) );
				}
				else if ( name.contains ( "nio Carlos" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Antônio Carlos" ) );
				}
				else if ( name.contains ( "rbara" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Bárbara" ) );
				}
				else if ( name.contains ( "udia" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Cláudia" ) );
				}
				else if ( name.contains ( "Maria L" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "D. Maria Lúcia" ) );
				}
				else if ( name.contains ( "Delc" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Delcílio Azevedo" ) );
				}
				else if ( name.contains ( "Esth" ) )
				{
					if ( name.contains ( "rcio" ) )
						setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Laércio e Esthér" ) );
					else if ( name.contains ( "Ant" ) )
						setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Esthér e Antônia" ) );
				}
				else if ( name.contains ( "Expor" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Exporádicos" ) );
				}
				else if ( name.contains ( "Crepaldi" ) )
				{
					if ( name.contains ( "Arnaldo" ) )
						setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Inês e Arnaldo Crepaldi" ) );
					else
						setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Inês Crepaldi" ) );
				}
				else if ( name.contains ( "e Carlos" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Inês e Carlos" ) );
				}
				else if ( name.contains ( "Jos" ) )
				{
					if ( name.contains ( "Cabana" ) )
						setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "José Cabana" ) );
					else if ( name.contains ( "Roseli" ) )
						setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "José Carlos Fontanelli e Roseli" ) );
					else if ( name.contains ( "Gomes" ) )
						setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "José Gomes Verrengia - Escritório São Pedro" ) );
				}
				else if ( name.contains ( "Luis Ant" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Luis Antônio" ) );
				}
				else if ( name.contains ( "gia Cabana" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Lígia Cabana" ) );
				}
				else if ( name.contains ( "Buffab" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Lígia Maria Buffab" ) );
				}
				else if ( name.endsWith ( "dia" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Lídia" ) );
				}
				else if ( name.contains ( "Marta" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Marta Lúcia" ) );
				}
				else if ( name.contains ( "Prefeitura" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Prefeitura de São Pedro" ) );
				}
				else if ( name.contains ( "ngela" ) )
				{
					if ( name.contains ( "Barbosa" ) )
						setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Rozângela Barbosa" ) );
					else if ( name.contains ( "Rotiroti" ) )
						setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Rubens e Rosângela Rotiroti" ) );
					else if ( name.endsWith ( "ngela" ) )
						setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Ângela" ) );
				}
				else if ( name.contains ( "Toninho" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Sr. Antònio \"Toninho\"" ) );
				}
				else if ( name.endsWith ( "vio" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Sr. Otávio" ) );
				}
				else if ( name.contains ( "lvia" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Sílvia Liberali" ) );
				}
				else if ( name.endsWith ( "nia" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Sônia" ) );
				}
				else if ( name.contains ( "Tide" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Tide e Sérgio" ) );
				}
				else if ( name.contains ( "Meneghini" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Wálter Meneghini" ) );
				}
				else if ( name.contains ( "nia Amaral" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Vânia Amaral" ) );
				}
				else if ( name.contains ( "Galo" ) )
				{
					setRecValue ( &client, FLD_CLIENT_NAME, QStringLiteral ( "Ângelo Galo" ) );
				}
			}
			city = recStrValue ( &client, FLD_CLIENT_CITY );
			if ( city.endsWith ( "Pedro" ) )
			{
				setRecValue ( &client, FLD_CLIENT_CITY, QStringLiteral ( "São Pedro" ) );
			}
			else if ( city.endsWith ( "Paulo" ) )
			{
				setRecValue ( &client, FLD_CLIENT_CITY, QStringLiteral ( "São Paulo" ) );
			}
			client.saveRecord ();
		} while ( client.readNextRecord () );
		VivenciaDB::optimizeTable ( &Client::t_info );
		return true;
	}

	/*if ( current_table_version == 'A')
	{
		if ( DBRecord::databaseManager ()->insertColumn ( FLD_CLIENT_LAST_VIEWED, &Client::t_info ) )
		{
			VivenciaDB::optimizeTable ( &Client::t_info );
			return true;
		}
	}*/
	return false;
}

const TABLE_INFO Client::t_info =
{
	CLIENT_TABLE,
	QStringLiteral ( "CLIENTS" ),
	QStringLiteral ( " ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci" ),
	QStringLiteral ( " PRIMARY KEY ( `ID` ) , UNIQUE KEY `id` ( `ID` ) " ),
	QStringLiteral ( "`ID`|`NAME`|`STREET`|`NUMBER`|`DISTRICT`|`CITY`|`ZIPCODE`|`PHONES`|`EMAIL`|`BEGINDATE`|`ENDDATE`|`STATUS`|`LAST_VIEWED`|" ),
	QStringLiteral ( " int ( 9 ) NOT NULL, | varchar ( 60 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 60 ) COLLATE utf8_unicode_ci DEFAULT NULL, |"
	" varchar ( 6 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 30 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 30 ) COLLATE utf8_unicode_ci DEFAULT NULL, |"
	" varchar ( 20 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 200 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 200 ) COLLATE utf8_unicode_ci DEFAULT NULL, | "
	" varchar ( 60 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 60 ) COLLATE utf8_unicode_ci DEFAULT NULL, | int ( 2 ) DEFAULT NULL, | int ( 2 ) DEFAULT 0, |" ),
	QStringLiteral ( "ID|Name|Street|Number|District|City|Zip code|Phones|E-mail|Client since|Client to|Active|Last Viewed|" ),
	CLIENTS_FIELDS_TYPE,
	TABLE_VERSION, CLIENT_FIELD_COUNT, TABLE_CLIENT_ORDER,
	&updateClientTable
	#ifdef TRANSITION_PERIOD
	// it is actually false, but the update routine in generaltable.cpp checks for it, and is the only place in all of the code.
	// Since the code there must not call updateIDs for the client table, this false information here is actually harmless and makes for one less conditional statement there
	, true
	#endif
};

static void clientNameChangeActions ( const DBRecord* db_rec )
{
	configOps config;
	if ( db_rec->action () == ACTION_ADD )
	{
		const QString baseClientDir ( config.projectsBaseDir () + recStrValue ( db_rec, FLD_CLIENT_NAME ) + CHR_F_SLASH );
		fileOps::createDir ( baseClientDir );
		fileOps::createDir ( baseClientDir + QLatin1String ( "Pictures/" ) + QString::number ( vmNumber::currentDate ().year () ) );
	}
	else if ( db_rec->action () == ACTION_EDIT )
	{
		fileOps::rename ( config.projectsBaseDir () + db_rec->actualRecordStr ( FLD_CLIENT_NAME ),
						  config.projectsBaseDir () + db_rec->backupRecordStr ( FLD_CLIENT_NAME ) );
	}
	
	db_rec->completerManager ()->updateCompleter ( db_rec, FLD_CLIENT_NAME, CC_CLIENT_NAME );
}

static void updateClientStreetCompleter ( const DBRecord* db_rec )
{
	db_rec->completerManager ()->updateCompleter ( db_rec, FLD_CLIENT_STREET, CC_ADDRESS );
}

static void updateClientDistrictCompleter ( const DBRecord* db_rec )
{
	db_rec->completerManager ()->updateCompleter ( db_rec, FLD_CLIENT_DISTRICT, CC_ADDRESS );
}

static void updateClientCityCompleter ( const DBRecord* db_rec )
{
	db_rec->completerManager ()->updateCompleter ( db_rec, FLD_CLIENT_CITY, CC_ADDRESS );
}

Client::Client ( const bool connect_helper_funcs )
	: DBRecord ( CLIENT_FIELD_COUNT )
{
	::memset ( this->helperFunction, 0, sizeof ( this->helperFunction ) );
	DBRecord::t_info = &( this->t_info );
	DBRecord::m_RECFIELDS = this->m_RECFIELDS;
	DBRecord::mFieldsTypes = CLIENTS_FIELDS_TYPE;
	
	if ( connect_helper_funcs )
	{
		DBRecord::helperFunction = this->helperFunction;
		setHelperFunction ( FLD_CLIENT_NAME, &clientNameChangeActions );
		setHelperFunction ( FLD_CLIENT_STREET, &updateClientStreetCompleter );
		setHelperFunction ( FLD_CLIENT_DISTRICT, &updateClientDistrictCompleter );
		setHelperFunction ( FLD_CLIENT_CITY, &updateClientCityCompleter );
	}
}

Client::~Client () {}

int Client::searchCategoryTranslate ( const SEARCH_CATEGORIES sc ) const
{
	switch ( sc )
	{
		case SC_ID:			return FLD_CLIENT_ID;
		case SC_ADDRESS_1:	return FLD_CLIENT_STREET;
		case SC_ADDRESS_2:	return FLD_CLIENT_NUMBER;
		case SC_ADDRESS_3:	return FLD_CLIENT_DISTRICT;
		case SC_ADDRESS_4:	return FLD_CLIENT_CITY;
		case SC_ADDRESS_5:	return FLD_CLIENT_ZIP;
		case SC_DATE_1:		return FLD_CLIENT_STARTDATE;
		case SC_DATE_2:		return FLD_CLIENT_ENDDATE;
		case SC_TYPE:		return FLD_CLIENT_NAME;
		case SC_EXTRA_1:	return FLD_CLIENT_EMAIL;
		default:			return -1;
	}
}

void Client::copySubRecord ( const uint subrec_field, const stringRecord& subrec )
{
	if ( subrec_field == FLD_CLIENT_PHONES || subrec_field == FLD_CLIENT_EMAIL )
	{
		if ( subrec.curIndex () == -1 )
			subrec.first ();
		stringRecord contact_info;
		
		// These sub record fields can contain any number of entries, so we keep on adding until we find a string that is not of the expected type
		if ( subrec_field == FLD_CLIENT_PHONES )
		{
			vmNumber phone;
			do
			{
				if ( phone.fromTrustedStrPhone ( subrec.curValue (), false ).isPhone () )
					contact_info.fastAppendValue ( phone.toString () );
				else
					break;
			} while ( subrec.next () );
		}
		else
		{
			vmNumber date;	// FLD_CLIENT_STARTDATE is the next field after the emails sub records
			QString email;
			do
			{
				email = subrec.curValue ();
				if ( !email.isEmpty () )
				{
					if ( !date.fromTrustedStrDate ( email, vmNumber::VDF_DB_DATE, false ).isDate () )
						contact_info.fastAppendValue ( email );
					else
						break;
				}
			} while ( subrec.next () );
		}
		
		setRecValue ( this, subrec_field, contact_info.toString () );
	}
}

QString Client::clientName ( const QString& id )
{
	QSqlQuery query;
	if ( VivenciaDB::runSelectLikeQuery ( QLatin1String ( "SELECT NAME FROM CLIENTS WHERE ID='" ) + id + CHR_CHRMARK, query ) )
		return query.value ( 0 ).toString ();
	return QString ();
}

uint Client::clientID ( const QString& name )
{
	QSqlQuery query;
	if ( VivenciaDB::runSelectLikeQuery ( QLatin1String (	"SELECT ID FROM CLIENTS WHERE NAME='" ) + name + CHR_CHRMARK, query ) )
		return query.value ( 0 ).toUInt ();
	return 0;
}

QString Client::concatenateClientInfo ( const Client& client )
{
	QString info;
	info = recStrValue ( &client, FLD_CLIENT_NAME );
	if ( !info.isEmpty () )
	{
		info += QLatin1String ( client.opt ( FLD_CLIENT_STATUS ) ? "(*)\n" : "\n" );
		if ( !recStrValue ( &client, FLD_CLIENT_STREET ).isEmpty () )
			info += recStrValue ( &client, FLD_CLIENT_STREET ) + QLatin1String ( ", " );

		if ( !recStrValue ( &client, FLD_CLIENT_NUMBER ).isEmpty () )
			info += recStrValue ( &client, FLD_CLIENT_NUMBER );
		else
			info += QLatin1String ( "S/N" );

		info += QLatin1String ( " - " );

		if ( !recStrValue ( &client, FLD_CLIENT_DISTRICT ).isEmpty () )
			info += recStrValue ( &client, FLD_CLIENT_DISTRICT ) + QLatin1String ( " - " );

		if ( !recStrValue ( &client, FLD_CLIENT_CITY ).isEmpty () )
			info += recStrValue ( &client, FLD_CLIENT_CITY ) + QLatin1String ( "/SP" );

		if ( !recStrValue ( &client, FLD_CLIENT_PHONES ).isEmpty () )
		{
			info += CHR_NEWLINE;
			const stringRecord phones_rec ( recStrValue ( &client, FLD_CLIENT_PHONES ) );
			if ( phones_rec.first () )
			{
				info += QLatin1String ( "Telefone(s): " ) + phones_rec.curValue ();
				while ( phones_rec.next () )
					info += QLatin1String ( " / " ) + phones_rec.curValue ();
			}
		}
		if ( !recStrValue ( &client, FLD_CLIENT_EMAIL ).isEmpty () )
		{
			info += CHR_NEWLINE;
			const stringRecord emails_rec ( recStrValue ( &client, FLD_CLIENT_EMAIL ) );
			if ( emails_rec.first () )
			{
				info += QLatin1String ( "email(s)/site(s): " ) + emails_rec.curValue ();
				while ( emails_rec.next () )
					info += QLatin1String ( " / " ) + emails_rec.curValue ();
			}
		}
	}
	return info;
}

void Client::setListItem ( clientListItem* client_item )
{
	DBRecord::mListItem = static_cast<dbListItem*>( client_item );
}

clientListItem* Client::clientItem () const
{
	return dynamic_cast<clientListItem*>( DBRecord::mListItem );
}
