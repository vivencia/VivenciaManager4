#include "fast_library_functions.h"

/*#include <QApplication>
#include <QClipboard>

void VM_LIBRARY_FUNCS::copyToClipboard ( const QString& str )
{
	QApplication::clipboard ()->setText ( str, QClipboard::Clipboard );
}

int VM_LIBRARY_FUNCS::insertStringListItem ( QStringList& list, const QString& text )
{
	if ( text.isEmpty () )
		return -1;

	QString str;
	int x ( 0 );
	int i ( 0 );

	for ( ; i < list.count (); ++i )
	{
		str = list.at ( i );
		if ( text.compare ( str, Qt::CaseInsensitive ) == 0 )
			return i; // item already in list, do nothing
		// Insert item alphabetically
		for ( x = 0; x < text.count (); ++x )
		{
			if ( x >= str.count () )
				break;
			if ( text.at ( x ) > str.at ( x ) )
				break;
			else if ( text.at ( x ) == str.at ( x ) )
				continue;
			else
			{
				list.insert ( i, text );
				return i;
			}
		}
	}
	list.append ( text );
	return i;
}*/
