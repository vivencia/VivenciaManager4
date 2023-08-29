#ifndef VMCHECKEDTABLEITEM_H
#define VMCHECKEDTABLEITEM_H

#include <QHeaderView>
#include <QRect>

#include <functional>

class QPainter;
class QMouseEvent;

class vmCheckedTableItem final : public QHeaderView
{

friend class vmTableWidget;
friend class vmTableSearchPanel;

public:
	explicit vmCheckedTableItem ( const Qt::Orientation orientation, QWidget* parent = nullptr );
	virtual ~vmCheckedTableItem () final = default;

protected:
	void paintSection ( QPainter* painter, const QRect& rect, const int logicalIndex ) const;
	void mousePressEvent ( QMouseEvent* event );

private:
	void setCheckable ( const bool checkable );
	bool isChecked ( const uint column ) const;
	void setChecked ( const uint column, const bool checked, const bool b_notify = false );
	inline void setCallbackForCheckStateChange ( const std::function<void ( const uint col, const bool checked )>& func ) {
				checkChange_func = func; }
	QRect checkBoxRect ( const QRect& sourceRect ) const;

	std::function<void ( const uint col, const bool checked )> checkChange_func;

	uint mColumnState;
	bool mbIsCheckable;
};

#endif // VMCHECKEDTABLEITEM_H
