#include "calendarviewui.h"
#include "ui_calendarviewui.h"

#include <dbRecords/dbcalendar.h>
#include <dbRecords/job.h>
#include <dbRecords/client.h>
#include <dbRecords/dblistitem.h>
#include <vmWidgets/vmlistwidget.h>
#include <vmWidgets/vmlistitem.h>
#include <mainwindow.h>

#include <QtWidgets/QTabWidget>

calendarViewUI::calendarViewUI ( QTabWidget* parent, const uint tab_index, MainWindow* mainwindow, dbCalendar* db_cal )
	: QWidget ( parent ), ui ( new Ui::calendarViewUI ), mMainWindow ( mainwindow ), mCal ( db_cal )
{
	ui->setupUi ( this );
	parent->insertTab ( tab_index, this, ICON ( "x-office-calendar" ), APP_TR_FUNC ( "Calendar" ) );
	setupCalendarMethods ();
}

calendarViewUI::~calendarViewUI ()
{
	ui->lstCalJobsDay->setIgnoreChanges ( true );
	ui->lstCalJobsWeek->setIgnoreChanges ( true );
	ui->lstCalJobsMonth->setIgnoreChanges ( true );
	ui->lstCalPaysDay->setIgnoreChanges ( true );
	ui->lstCalPaysWeek->setIgnoreChanges ( true );
	ui->lstCalPaysMonth->setIgnoreChanges ( true );
	ui->lstCalBuysDay->setIgnoreChanges ( true );
	ui->lstCalBuysWeek->setIgnoreChanges ( true );
	ui->lstCalBuysMonth->setIgnoreChanges ( true );
	delete ui;
}

void calendarViewUI::activate ()
{
	if ( mCalendarDate.isNull () )
		mCalendarDate = vmNumber::currentDate ();
	updateCalendarView ( mCalendarDate.year (), mCalendarDate.month (), false );
	// prevent unecessary execution of code by letting the function decide whether to proceed as if a user called it
	// via UI signal. The programmatically argument bUserAction = false, would make calMain_activated use the
	// same date the calendar has at this moment, triggering an unecessary code path
	calMain_activated ( ui->calMain->selectedDate (), true );
}

void calendarViewUI::setupCalendarMethods ()
{
	static_cast<void>( connect ( ui->calMain, &QCalendarWidget::activated, this, [&] ( const QDate& date ) {
				return calMain_activated ( date ); } ) );
	static_cast<void>( connect ( ui->calMain, &QCalendarWidget::clicked, this, [&] ( const QDate& date ) {
				return calMain_activated ( date ); } ) );
	static_cast<void>( connect ( ui->calMain, &QCalendarWidget::currentPageChanged, this, [&] ( const int year, const int month ) {
				return updateCalendarView ( static_cast<uint>( year ), static_cast<uint>( month ) ); } ) );
	static_cast<void>( connect ( ui->tboxCalJobs, &QToolBox::currentChanged, this, [&] ( const int index ) {
				return tboxCalJobs_currentChanged ( index ); } ) );
	static_cast<void>( connect ( ui->tboxCalPays, &QToolBox::currentChanged, this, [&] ( const int index ) {
				return tboxCalPays_currentChanged ( index ); } ) );
	static_cast<void>( connect ( ui->tboxCalBuys, &QToolBox::currentChanged, this, [&] ( const int index ) {
				return tboxCalBuys_currentChanged ( index ); } ) );

	ui->lstCalJobsDay->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item )
			{ return mMainWindow->displayJobFromCalendar ( static_cast<dbListItem*>( item ) ); } );
	ui->lstCalJobsWeek->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item )
			{ return mMainWindow->displayJobFromCalendar ( static_cast<dbListItem*>( item ) ); } );
	ui->lstCalJobsMonth->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item )
			{ return mMainWindow->displayJobFromCalendar ( static_cast<dbListItem*>( item ) ); } );
	ui->lstCalPaysDay->setCallbackForCurrentItemChanged( [&] ( vmListItem* item )
			{ return mMainWindow->displayPayFromCalendar ( static_cast<dbListItem*>( item ) ); } );
	ui->lstCalPaysWeek->setCallbackForCurrentItemChanged( [&] ( vmListItem* item )
			{ return mMainWindow->displayPayFromCalendar ( static_cast<dbListItem*>( item ) ); } );
	ui->lstCalPaysMonth->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item )
			{ return mMainWindow->displayPayFromCalendar ( static_cast<dbListItem*>( item ) ); } );
	ui->lstCalBuysDay->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item )
			{ return mMainWindow->displayBuyFromCalendar ( static_cast<dbListItem*>( item ) ); } );
	ui->lstCalBuysWeek->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item )
			{ return mMainWindow->displayBuyFromCalendar ( static_cast<dbListItem*>( item ) ); } );
	ui->lstCalBuysMonth->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item )
			{ return mMainWindow->displayBuyFromCalendar ( static_cast<dbListItem*>( item ) ); } );
}

void calendarViewUI::calMain_activated ( const QDate& date, const bool bUserAction )
{
	if ( date != mCalendarDate.toQDate () && bUserAction )
	{
		changeCalendarToolBoxesText ( vmNumber ( date ) );

		switch ( ui->tboxCalJobs->currentIndex () )
		{
			case 0:	 ui->lstCalJobsDay->clear ();	break;
			case 1:	 ui->lstCalJobsWeek->clear ();   break;
			case 2:	 ui->lstCalJobsMonth->clear ();  break;
		}
		switch ( ui->tboxCalPays->currentIndex () )
		{
			case 0:	 ui->lstCalPaysDay->clear ();	break;
			case 1:	 ui->lstCalPaysWeek->clear ();   break;
			case 2:	 ui->lstCalPaysMonth->clear ();  break;
		}
		switch ( ui->tboxCalBuys->currentIndex () )
		{
			case 0:	 ui->lstCalBuysDay->clear ();	break;
			case 1:	 ui->lstCalBuysWeek->clear ();   break;
			case 2:	 ui->lstCalBuysMonth->clear ();  break;
		}

		mCalendarDate = date;
		tboxCalJobs_currentChanged ( ui->tboxCalJobs->currentIndex () );
		tboxCalPays_currentChanged ( ui->tboxCalPays->currentIndex () );
		tboxCalBuys_currentChanged ( ui->tboxCalBuys->currentIndex () );
	}
}

void calendarViewUI::updateCalendarView ( const uint year, const uint month, const bool bUserAction )
{
	QString price;
	vmNumber date;
	date.setDate ( 1, static_cast<int>( month ), static_cast<int>( year ) );
	const stringTable jobsPerDateList ( mCal->dateLog ( date, FLD_CALENDAR_MONTH,
				FLD_CALENDAR_JOBS, price, FLD_CALENDAR_TOTAL_JOBPRICE_SCHEDULED, true ) );

	if ( jobsPerDateList.countRecords () > 0 )
	{
		const stringRecord* str_rec ( &jobsPerDateList.first () );
		if ( str_rec->isOK () )
		{
			uint jobid ( 0 );
			Job job;
			QString tooltip;
			QTextCharFormat dateChrFormat;

			do
			{
				jobid =  str_rec->fieldValue ( 0 ).toUInt ();
				if ( job.readRecord ( jobid ) )
				{
					date.fromTrustedStrDate ( str_rec->fieldValue ( 3 ), vmNumber::VDF_DB_DATE );
					dateChrFormat = ui->calMain->dateTextFormat ( date.toQDate () );
					tooltip = dateChrFormat.toolTip ();
					if ( !tooltip.contains ( QStringLiteral ( "Job ID: " ) + recStrValue ( &job, FLD_JOB_ID ) ) )
					{
						tooltip +=  QStringLiteral ( "<br>Job ID: " ) + recStrValue ( &job, FLD_JOB_ID ) +
								QStringLiteral ( "<br>Client: " ) + Client::clientName ( recStrValue ( &job, FLD_JOB_CLIENTID ) ) +
								QStringLiteral ( "<br>Job type: " ) + recStrValue ( &job, FLD_JOB_TYPE ) +
								QStringLiteral ( " (Day " ) + str_rec->fieldValue ( 2 ) + CHR_R_PARENTHESIS + QStringLiteral ( "<br>" );

						dateChrFormat.setProperty ( QTextFormat::FontItalic, 1 );
						dateChrFormat.setForeground ( Qt::darkRed );
						dateChrFormat.setBackground ( QBrush ( Qt::transparent ) );
						dateChrFormat.setToolTip ( tooltip );
						ui->calMain->setDateTextFormat ( date.toQDate (), dateChrFormat );
					}
				}
				str_rec = &jobsPerDateList.next ();
			} while ( str_rec->isOK () );
		}
	}
	//Update lists for the different month when the user clicks on the calendar page changing button
	if ( bUserAction )
	{
		//call the function below only if user clicked on the calendar (bUserAction = true). But we call it
		//programmatically, so its bUserAction argument must be false.
		calMain_activated ( QDate ( static_cast<int>( year ), static_cast<int>( month ), 1 ), false );
	}
}

void calendarViewUI::fillCalendarJobsList ( const stringTable& jobids, vmListWidget* list )
{
	if ( jobids.countRecords () > 0 )
	{
		const stringRecord* str_rec ( &jobids.first () );
		if ( str_rec->isOK () )
		{
			uint jobid ( 0 );
			const QString dayStr ( QStringLiteral ( " (Day(s) " ) );
			jobListItem* job_item ( nullptr ), *job_parent ( nullptr );
			clientListItem* client_parent ( nullptr );
			vmListWidget* old_list ( nullptr );
			bool b_old_is_ignoring_change ( false );
			Job job;
			QString day, curLabel;
			list->setIgnoreChanges ( true );
			do
			{
				jobid =  str_rec->fieldValue ( 0 ).toUInt () ;
				if ( job.readRecord ( jobid ) )
				{
					client_parent = mMainWindow->getClientItem ( static_cast<uint>( recIntValue ( &job, FLD_JOB_CLIENTID ) ) );
					job_parent = mMainWindow->getJobItem ( client_parent, jobid );
					if ( !job_parent )
					{
						job_parent = static_cast<jobListItem*>( mMainWindow->loadItem ( client_parent, jobid, JOB_TABLE ) );
					}

					if ( job_parent )
					{
						day = str_rec->fieldValue ( 2 );
						job_item = static_cast<jobListItem*>( job_parent->relatedItem ( RLI_CALENDARITEM ) );
						if ( !job_item )
						{
							job_item = new jobListItem;
							job_item->setRelation ( RLI_CALENDARITEM );
							job_parent->syncSiblingWithThis ( job_item );
						}

						job_item->setData ( Qt::UserRole, day.toInt () );
						if ( job_item->listWidget () != list )
						{
							old_list = job_item->listWidget ();
							if ( old_list )
							{
								b_old_is_ignoring_change = old_list->isIgnoringChanges ();
								old_list->setIgnoreChanges ( true );
							}
							job_item->setText ( recStrValue ( client_parent->clientRecord (), FLD_CLIENT_NAME ) +
								CHR_SPACE + CHR_HYPHEN + CHR_SPACE + recStrValue ( &job, FLD_JOB_TYPE ) +
								dayStr + day + CHR_R_PARENTHESIS, false, false, false );
							job_item->addToList ( list, false );
							if ( old_list )
								old_list->setIgnoreChanges ( b_old_is_ignoring_change );
						}
						else
						{
							curLabel = job_item->text ();
							curLabel.insert ( curLabel.count () - 1, CHR_COMMA + day );
							job_item->setText ( curLabel, false, false, false );
						}
						job_item->setToolTip ( job_item->text () );
					}
				}
				str_rec = &jobids.next ();
			} while ( str_rec->isOK () );
			list->setIgnoreChanges ( false );
		}
	}
}

void calendarViewUI::tboxCalJobs_currentChanged ( const int index )
{
	QString price;
	switch ( index )
	{
		case 0:
			if ( ui->lstCalJobsDay->count () == 0 )
			{
				fillCalendarJobsList (
					mCal->dateLog ( mCalendarDate, FLD_CALENDAR_DAY_DATE,
								FLD_CALENDAR_JOBS, price, FLD_CALENDAR_TOTAL_JOBPRICE_SCHEDULED ), ui->lstCalJobsDay );
				ui->txtCalPriceJobDay->setText ( price );
			}
		break;
		case 1:
			if ( ui->lstCalJobsWeek->count () == 0 )
			{
				fillCalendarJobsList (
					mCal->dateLog ( mCalendarDate, FLD_CALENDAR_WEEK_NUMBER,
								FLD_CALENDAR_JOBS, price, FLD_CALENDAR_TOTAL_JOBPRICE_SCHEDULED ), ui->lstCalJobsWeek );
				ui->txtCalPriceJobWeek->setText ( price );
			}
		break;
		case 2:
			if ( ui->lstCalJobsMonth->count () == 0 )
			{
				fillCalendarJobsList (
					mCal->dateLog ( mCalendarDate, FLD_CALENDAR_MONTH,
								FLD_CALENDAR_JOBS, price, FLD_CALENDAR_TOTAL_JOBPRICE_SCHEDULED ), ui->lstCalJobsMonth );
				ui->txtCalPriceJobMonth->setText ( price );
			}
		break;
		default: return;
	}
}

void calendarViewUI::fillCalendarPaysList ( const stringTable& payids, vmListWidget* list, const bool use_date )
{
	if ( payids.countRecords () > 0 )
	{
		const stringRecord* str_rec ( &payids.first () );
		if ( str_rec->isOK () )
		{
			uint payid ( 0 );
			payListItem* pay_item ( nullptr ), *pay_parent ( nullptr );
			clientListItem* client_parent ( nullptr );
			vmListWidget* old_list ( nullptr );
			bool b_old_is_ignoring_change ( false );
			Payment pay;
			QString paynumber;
			list->setIgnoreChanges ( true );
			do
			{
				payid =  str_rec->fieldValue ( 0 ).toUInt ();
				if ( pay.readRecord ( payid ) )
				{
					client_parent = mMainWindow->getClientItem ( static_cast<uint>( recIntValue ( &pay, FLD_PAY_CLIENTID ) ) );
					pay_parent = mMainWindow->getPayItem ( client_parent, payid );
					if ( !pay_parent )
					{
						pay_parent = static_cast<payListItem*>( mMainWindow->loadItem ( client_parent, payid, PAYMENT_TABLE ) );
					}

					if ( pay_parent )
					{
						paynumber = str_rec->fieldValue ( 2 );
						if ( paynumber.isEmpty () )
							paynumber = CHR_ONE;
						pay_item = static_cast<payListItem*>( pay_parent->relatedItem ( RLI_CALENDARITEM ) );
						if ( !pay_item )
						{
							pay_item = new payListItem;
							pay_item->setRelation ( RLI_CALENDARITEM );
							pay_parent->syncSiblingWithThis ( pay_item );
						}

						const int role ( use_date ? Qt::UserRole + 1 : Qt::UserRole );
						QString role_str ( pay_item->data ( role ).toString () );

						if ( !role_str.isEmpty () )
						{
							/* Do not add the same pay number info. This might happen in case of an error in the database or,
							 * more likely, in the ui->lstCalPaysDay when the pay day date differs from the pay use date and the
							 * user clicks on those dates alternately. In such cases, the same pay number would be accreting to the role string
							 */
							const stringRecord pays_roles ( role_str, CHR_COMMA );
							if ( pays_roles.field ( paynumber, -1, true ) == -1 )
								role_str += CHR_COMMA + paynumber;
						}
						else
							role_str = paynumber;

						if ( pay_item->listWidget () != list )
						{
							old_list = pay_item->listWidget ();
							if ( old_list )
							{
								b_old_is_ignoring_change = old_list->isIgnoringChanges ();
								old_list->setIgnoreChanges ( true );
							}
							//pay_item->setData ( role, emptyString );
							pay_item->addToList ( list );
							if ( old_list )
								old_list->setIgnoreChanges ( b_old_is_ignoring_change );
						}
						if ( list == ui->lstCalPaysDay )
						{
							/* This is the list for the one day only. An item in it cannot accumulate information from other days.
							 * If on that day there were a pay day and a pay use, then, display both information. If there was either of them,
							 * clear the other role info so that info is not displayed. For the week and month lists, this is not done because
							 * we need all the information we can get
							 */
							const stringRecord pay_info ( stringTable ( recStrValue ( &pay, FLD_PAY_INFO ) ).readRecord ( paynumber.toUInt () ) );
							if ( pay_info.fieldValue ( PHR_DATE ) == pay_info.fieldValue ( PHR_USE_DATE ) )
								pay_item->setData ( use_date ? Qt::UserRole : Qt::UserRole + 1, emptyString );
						}
						pay_item->setData ( role, role_str );
						pay_item->update ();
					}
				}
				str_rec = &payids.next ();
			} while ( str_rec->isOK () );
			list->setIgnoreChanges ( false );
		}
	}
}

void calendarViewUI::tboxCalPays_currentChanged ( const int index )
{
	bool bFillList ( false );
	vmListWidget* list ( nullptr );
	vmLineEdit* line ( nullptr );

	switch ( index )
	{
		case 0:
			bFillList = ( ui->lstCalPaysDay->count () == 0 );
			list = ui->lstCalPaysDay;
			line = ui->txtCalPricePayDay;
		break;
		case 1:
			bFillList = ( ui->lstCalPaysWeek->count () == 0 );
			list = ui->lstCalPaysWeek;
			line = ui->txtCalPricePayWeek;
		break;
		case 2:
			bFillList = ( ui->lstCalPaysMonth->count () == 0 );
			list = ui->lstCalPaysMonth;
			line = ui->txtCalPricePayMonth;
		break;
		default: return;
	}
	if ( bFillList )
	{
		QString price;
		const uint search_field ( FLD_CALENDAR_DAY_DATE + static_cast<uint>(index) );
		fillCalendarPaysList ( mCal->dateLog ( mCalendarDate, search_field,
				FLD_CALENDAR_PAYS, price, FLD_CALENDAR_TOTAL_PAY_RECEIVED ), list );
		line->setText ( price );
		fillCalendarPaysList ( mCal->dateLog ( mCalendarDate, search_field,
				FLD_CALENDAR_PAYS_USE, price, FLD_CALENDAR_TOTAL_PAY_RECEIVED ), list, true );
	}
}

void calendarViewUI::fillCalendarBuysList ( const stringTable& buyids, vmListWidget* list, const bool pay_date )
{
	if ( buyids.countRecords () > 0 )
	{
		const stringRecord* str_rec ( &buyids.first () );
		if ( str_rec->isOK () )
		{
			buyListItem* buy_item ( nullptr ), *buy_parent ( nullptr );
			clientListItem* client_parent ( nullptr );
			vmListWidget* old_list ( nullptr );
			bool b_old_is_ignoring_change ( false );
			uint buyid ( 0 );
			list->setIgnoreChanges ( true );
			do
			{
				buyid = str_rec->fieldValue ( 0 ).toUInt ();
				client_parent = mMainWindow->getClientItem ( str_rec->fieldValue ( 1 ).toUInt () );
				buy_parent = mMainWindow->getBuyItem ( client_parent, buyid );

				if ( !buy_parent )
				{
					buy_parent = static_cast<buyListItem*>( mMainWindow->loadItem ( client_parent, buyid, PURCHASE_TABLE ) );
				}

				if ( buy_parent )
				{
					buy_item = static_cast<buyListItem*>( buy_parent->relatedItem ( RLI_CALENDARITEM ) );
					if ( !buy_item )
					{
						buy_item = new buyListItem;
						buy_item->setRelation ( RLI_CALENDARITEM );
						buy_parent->syncSiblingWithThis ( buy_item );
						buy_item->setData ( Qt::UserRole, 0 ); // payment number
						buy_item->setData ( Qt::UserRole + 1, false ); // purchase date
						buy_item->setData ( Qt::UserRole + 2, false ); // payment date
					}

					if ( pay_date )
					{
						buy_item->setData ( Qt::UserRole + 2, true );
						buy_item->setData ( Qt::UserRole, str_rec->fieldValue ( 2 ).toInt () );
					}
					else
					{
						buy_item->setData ( Qt::UserRole + 1, true );
					}
					if ( buy_item->listWidget () != list )
					{
						old_list = buy_item->listWidget ();
						if ( old_list )
						{
							b_old_is_ignoring_change = old_list->isIgnoringChanges ();
							old_list->setIgnoreChanges ( true );
						}
						buy_item->addToList ( list );
						if ( old_list )
							old_list->setIgnoreChanges ( b_old_is_ignoring_change );
					}
					buy_item->update ();
				}
				str_rec = &buyids.next ();
			} while ( str_rec->isOK () );
			list->setIgnoreChanges ( false );
		}
	}
}

void calendarViewUI::tboxCalBuys_currentChanged ( const int index )
{
	bool bFillList ( false );
	vmListWidget* list ( nullptr );
	vmLineEdit* line ( nullptr );

	switch ( index )
	{
		case 0:
			bFillList = ( ui->lstCalBuysDay->count () == 0 );
			list = ui->lstCalBuysDay;
			line = ui->txtCalPriceBuyDay;
		break;
		case 1:
			bFillList = ( ui->lstCalBuysWeek->count () == 0 );
			list = ui->lstCalBuysWeek;
			line = ui->txtCalPriceBuyWeek;
		break;
		case 2:
			bFillList = ( ui->lstCalBuysMonth->count () == 0 );
			list = ui->lstCalBuysMonth;
			line = ui->txtCalPriceBuyMonth;
		break;
		default: return;
	}
	if ( bFillList )
	{
		QString price;
		const uint search_field ( FLD_CALENDAR_DAY_DATE + static_cast<uint>(index) );
		fillCalendarBuysList ( mCal->dateLog ( mCalendarDate, search_field,
							FLD_CALENDAR_BUYS, price, FLD_CALENDAR_TOTAL_BUY_BOUGHT ), list );
		line->setText ( price );
		fillCalendarBuysList ( mCal->dateLog ( mCalendarDate, search_field,
							FLD_CALENDAR_BUYS_PAY, price, FLD_CALENDAR_TOTAL_BUY_PAID ), list, true );
		if ( !price.isEmpty () )
			line->setText ( line->text () + QStringLiteral ( " (" ) + price + CHR_R_PARENTHESIS );
	}
}

void calendarViewUI::changeCalendarToolBoxesText ( const vmNumber& date )
{
	vmNumber firstDayOfWeek ( date );
	firstDayOfWeek.setDate ( static_cast<int>(1 - date.dayOfWeek ()), 0, 0, true );
	const QString str_firstDayOfWeek ( firstDayOfWeek.toString () );
	vmNumber lastDayOfWeek ( firstDayOfWeek );
	lastDayOfWeek.setDate ( 6, 0, 0, true );
	const QString str_lastDayOfWeek ( lastDayOfWeek.toString () );
	const QString date_str ( date.toString () );

	const QLocale locale;
	const QString monthName ( locale.monthName ( static_cast<int>(date.month ()), QLocale::LongFormat ) );

	ui->tboxCalJobs->setItemText ( 0, TR_FUNC ( "Jobs at " ) + date_str );
	ui->tboxCalJobs->setItemText ( 1, TR_FUNC ( "Jobs from " ) + str_firstDayOfWeek + TR_FUNC ( " through " ) + str_lastDayOfWeek );
	ui->tboxCalJobs->setItemText ( 2, TR_FUNC ( "Jobs in " ) + monthName );
	ui->tboxCalPays->setItemText ( 0, TR_FUNC ( "Payments at " ) + date_str );
	ui->tboxCalPays->setItemText ( 1, TR_FUNC ( "Payments from " ) + str_firstDayOfWeek + TR_FUNC ( " through " ) + str_lastDayOfWeek );
	ui->tboxCalPays->setItemText ( 2, TR_FUNC ( "Payments in " ) + monthName );
	ui->tboxCalBuys->setItemText ( 0, TR_FUNC ( "Purchases at " ) + date_str );
	ui->tboxCalBuys->setItemText ( 1, TR_FUNC ( "Purchases from " ) + str_firstDayOfWeek + TR_FUNC ( " through " ) + str_lastDayOfWeek );
	ui->tboxCalBuys->setItemText ( 2, TR_FUNC ( "Purchases in " ) + monthName );
}
