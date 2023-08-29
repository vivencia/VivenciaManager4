#ifndef SIMPLECALCULATOR_H
#define SIMPLECALCULATOR_H

#include <QtWidgets/QDialog>

class vmCalculator;
class vmLineEdit;

class QPushButton;
class QPlainTextEdit;

class simpleCalculator final : public QDialog
{

public:
	explicit simpleCalculator ( QWidget* parent = nullptr );
	virtual ~simpleCalculator () final;
	void showCalc ( const QPoint&, vmLineEdit* line = nullptr, QWidget* parentWindow = nullptr );
	void showCalc ( const QString& input, const QPoint &, vmLineEdit* line = nullptr, QWidget* parentWindow = nullptr );
	inline const QString getResult () const { return mStrResult; }

private:
	void setupUi ();
	void calculate ();
	void relevantKeyPressed ( const QKeyEvent* qe );
	void btnCopyResultClicked ();

	QString mStrResult;
	vmCalculator* m_calc;

	QPlainTextEdit* txtResult;
	QPushButton* btnCalc;
	QPushButton* btnCopyResult;
	QPushButton* btnClose;
	vmLineEdit* txtInput;
	vmLineEdit* txtReceiveResult;
};

#endif // SIMPLECALCULATOR_H
