#ifndef $upper_projectname$_GLOBAL_H
#define $upper_projectname$_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef $upper_projectname$_LIB
# define $upper_projectname$_EXPORT Q_DECL_EXPORT
#else
# define $upper_projectname$_EXPORT Q_DECL_IMPORT
#endif

#endif // $upper_projectname$_GLOBAL_H
