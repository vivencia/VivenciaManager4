#ifndef CONTACTSMANAGERWIDGET_H
#define CONTACTSMANAGERWIDGET_H

#include <vmWidgets/vmwidget.h>
#include <vmStringRecord/stringrecord.h>

#include <QtWidgets/QFrame>

class vmComboBox;
class vmLineEdit;
class QToolButton;

class contactsManagerWidget final : public QFrame, public vmWidget
{

public:
	enum CMW_TYPE { CMW_PHONES = 0, CMW_EMAIL = 1 };

	contactsManagerWidget ( QWidget* parent, const CMW_TYPE type = CMW_PHONES );
	virtual ~contactsManagerWidget () final = default;

	inline void setContactType ( const CMW_TYPE type ) { m_contact_type = type; }
	void initInterface ();
	void setEditable ( const bool editable ) override;

	void decodePhones ( const stringRecord& phones, const bool bClear = true );
	void decodeEmails ( const stringRecord& emails, const bool bClear = true );
	void insertItem ();
	bool removeCurrent ( int& removed_idx );

	void clearAll ();

	inline vmComboBox* combo () const { return cboInfoData; }

	inline void setCallbackForInsertion ( const std::function<void ( const QString&, const vmWidget* const )>& func ) {
		insertFunc = func; }
	inline void setCallbackForRemoval ( const std::function<void ( const int idx, const vmWidget* const )>& func ) {
		removeFunc = func; }

	static inline bool isEmailAddress ( const QString& address )
	{
		return address.contains ( QLatin1Char ( '@' ) ) && address.contains ( QLatin1Char ( '.' ) );
	}

private:
	vmComboBox* cboInfoData;
	QToolButton* btnAdd, *btnDel, *btnExtra;

	CMW_TYPE m_contact_type;

	void cbo_textAltered ( const QString& text );
	void keyPressedSelector ( const QKeyEvent* const ke, const vmWidget* const );
	void btnAdd_clicked ( const bool checked );
	void btnDel_clicked ();
	void btnExtra_clicked ();

	std::function<void ( const QString& info, const vmWidget* const widget )> insertFunc;
	std::function<void ( const int idx, const vmWidget* const widget )> removeFunc;
};

#endif // CONTACTSMANAGERWIDGET_H
