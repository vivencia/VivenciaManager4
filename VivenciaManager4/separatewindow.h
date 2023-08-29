#ifndef SEPARATEWINDOW_H
#define SEPARATEWINDOW_H

#include <QtWidgets/QDialog>

#include <functional>

class QCloseEvent;
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;

class separateWindow : public QDialog
{

public:
	explicit separateWindow ( QWidget* w_child );
	void setCallbackForReturningToParent ( const std::function<void (QWidget*)>& func ) { w_funcReturnToParent = func; }
	void showSeparate ( const QString& window_title, const bool b_exec = false, const Qt::WindowStates w_state = Qt::WindowActive );
	void returnToParent ();

protected:
	void closeEvent ( QCloseEvent* e ) override;
	bool eventFilter ( QObject* o, QEvent* e ) override;

	int exec () override;
	void open () override;
	void show ();
	void accept () override;
	void reject () override;
	void done ( int ) override;
	void hide ();

	void childCloseRequested ( const int ); // intercept a child'd close request and handle and accept it
	void childHideRequested ();
	void childShowRequested ();

private:
	QWidget* m_child;
	QVBoxLayout* mainLayout;
	QPushButton* btnReturn;
	bool mb_Active, mb_Visible;

	std::function<void ( QWidget*)> w_funcReturnToParent;
};

#endif // SEPARATEWINDOW_H
