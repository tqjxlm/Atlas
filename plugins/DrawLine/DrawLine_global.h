#ifndef DRAWLINE_GLOBAL_H
#define DRAWLINE_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef DRAWLINE_LIB
# define DRAWLINE_EXPORT Q_DECL_EXPORT
#else
# define DRAWLINE_EXPORT Q_DECL_IMPORT
#endif

#endif // DRAWLINE_GLOBAL_H
