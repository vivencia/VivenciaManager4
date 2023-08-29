#ifndef CALENDARVIEWUI_H
#define CALENDARVIEWUI_H

#include <vmNumbers/vmnumberformats.h>
#include <vmStringRecord/stringrecord.h>

#include <QtWidgets/QWidget>

class MainWindow;
class dbCalendar;
class vmListWidget;

class QTabWidget;

namespace Ui
{
	class calendarViewUI;
}

class calendarViewUI : public QWidget
{

public:
	explicit calendarViewUI ( QTabWidget* parent, const uint tab_index, MainWindow* mainwindow );
	virtual ~calendarViewUI ();

	void activate ();
	void setupCalendarMethods ();
	void calMain_activated ( const QDate& date, const bool bUserAction = true );
	void updateCalendarView ( const uint year, const uint month, const bool bUserAction = true );

	void fillCalendarJobsList ( const stringTable& jobids, vmListWidget* list );
	void tboxCalJobs_currentChanged ( const int index );

	void fillCalendarPaysList ( const stringTable& payids, vmListWidget* list, const bool use_date = false );
	void tboxCalPays_currentChanged ( const int index );

	void fillCalendarBuysList ( const stringTable& buyids, vmListWidget* list, const bool pay_date = false );
	void tboxCalBuys_currentChanged ( const int index );

	void changeCalendarToolBoxesText ( const vmNumber& date );

private:
	Ui::calendarViewUI* ui;
	MainWindow* mMainWindow;
	vmNumber mCalendarDate;
	dbCalendar* mCal;
};

#endif // CALENDARVIEWUI_H
