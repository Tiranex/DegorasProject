#ifndef DPCORE_GLOBAL_H
#define DPCORE_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(DP_CORE_LIBRARY)
#  define DP_CORE_EXPORT Q_DECL_EXPORT
#else
#  define DP_CORE_EXPORT Q_DECL_IMPORT
#endif

#endif // DPCORE_GLOBAL_H
