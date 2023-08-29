#ifndef VMNOTIFY_H
#define VMNOTIFY_H

#include "vmlist.h"

#include <QtCore/QSize>
#include <QtCore/QPointer>
#include <QtCore/QEventLoop>
#include <QtWidgets/QDialog>

#include <functional>

class vmTaskPanel;
class vmActionGroup;
class vmNotify;

class QTimer;
class QSystemTrayIcon;
class QPixmap;
class QPushButton;
class QVBoxLayout;
class QTimerEvent;
class QCompleter;

enum { MESSAGE_BTN_OK = 0, MESSAGE_BTN_CANCEL = 1 };

class Message
{

friend class vmNotify;

public:

	struct st_widgets
	{
		QWidget* widget;
		uint row;
		QSize size;
		Qt::Alignment alignment;
		bool isButton;
	};

	explicit Message ( vmNotify* parent = nullptr );
	~Message ();

	void addWidget ( QWidget* widget, const uint row,
					 const Qt::Alignment alignment = Qt::AlignLeft,
					 const bool is_button = false );

	inline void setMessageFinishedCallback ( const std::function<void ( Message* )>& func ) {
		messageFinishedFunc = func; }

	inline void setButtonClickedFunc ( const std::function<void ( const int )>& func ) {
		buttonClickedfunc = func; }

	void inputFormKeyPressed ( const QKeyEvent* ke );

	int timeout; // ms
	QString title, bodyText, iconName;
	bool isModal, mbClosable, mbAutoRemove;
	pointersList<st_widgets*> widgets;
	vmNotify* m_parent;
	int mBtnID;
	QPixmap* icon;
	vmActionGroup* mGroup;
	QTimer* timer;

	std::function<void ( Message* )> messageFinishedFunc;
	std::function<void ( const int btn_idx )> buttonClickedfunc;
};

class vmNotify : public QDialog
{

friend class Message;

public:
	enum MESSAGE_BOX_ICON { QUESTION = 1, WARNING = 2, CRITICAL = 3 };

	vmNotify ( const QString& position = QString (), QWidget* const parent = nullptr );
	virtual ~vmNotify ();

	void notifyMessage ( const QString& title, const QString& msg, const int msecs = 3000, const bool b_critical = false );
	static void notifyMessage ( QWidget* referenceWidget, const QString& title, const QString& msg, const int msecs = 3000, const bool b_critical = false );

	int messageBox ( const QString& title, const QString& msg, const MESSAGE_BOX_ICON icon,
					 const QStringList& btnsText, const int m_sec = -1, const std::function<void ( const int btn_idx )>& messageFinished_func = nullptr );
	static int messageBox ( QWidget* const referenceWidget, const QString& title, const QString& msg,
					const MESSAGE_BOX_ICON icon, const QStringList& btnsText, const int m_sec = -1, const std::function<void ( const int btn_idx )>& messageFinished_func = nullptr );

	bool questionBox ( const QString& title, const QString& msg, const int m_sec = -1, const std::function<void ( const int btn_idx )>& messageFinished_func = nullptr );
	static bool questionBox ( QWidget* const referenceWidget, const QString& title, const QString& msg, const int m_sec = -1, const std::function<void ( const int btn_idx )>& messageFinished_func = nullptr );

	int criticalBox ( const QString& title, const QString& msg, const int m_sec = -1, const std::function<void ( const int btn_idx )>& messageFinished_func = nullptr );
	static int criticalBox ( QWidget* const referenceWidget, const QString& title, const QString& msg, const int m_sec = -1, const std::function<void ( const int btn_idx )>& messageFinished_func = nullptr );

	static bool inputBox ( QString& result, QWidget* const referenceWidget, const QString& title, const QString& label_text,
					const QString& initial_text = QString (), const QString& icon = QString (), QCompleter* completer = nullptr );

	static bool passwordBox ( QString& result, QWidget* const referenceWidget, const QString& title,
					const QString& label_text, const QString& icon = QString () );

	static vmNotify* progressBox ( vmNotify* box = nullptr, QWidget* parent = nullptr, const uint max_value = 10, uint next_value = 0,
								   const QString& title = QString (), const QString& label = QString () );

public slots:
	virtual void accept () override;
	virtual void reject () override;

private:
	void buttonClicked ( QPushButton* btn, Message* const message );
	void setupWidgets ( Message* const message );
	void startMessageTimer ( Message* const message );
	void fadeWidget ();
	void showMenu ();
	void enterEventLoop ();
	void addMessage ( Message* message );
	void removeMessage ( Message* message );
	void setupWidgets ( const Message* message );
	void adjustSizeAndPosition ();
	QPoint displayPosition ( const QSize& widgetSize );

	bool mbDeleteWhenStackIsEmpty, mbButtonClicked;
	vmTaskPanel* mPanel;
	QTimer *fadeTimer;
	QWidget* m_parent;
	QString mPos;
	QPointer<QEventLoop> mEventLoop;
	pointersList<Message*> messageStack;
};
#endif // VMNOTIFY_H
