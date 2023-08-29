#ifndef VMUTILS_GLOBAL_H
#define VMUTILS_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(VMUTILS_LIBRARY)
#  define VMUTILSSHARED_EXPORT Q_DECL_EXPORT
#else
#  define VMUTILSSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // VMUTILS_GLOBAL_H
