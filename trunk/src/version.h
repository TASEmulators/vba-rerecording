#define VBA_NAME "VBA-RR"
#define VBA_FEATURE_STRING ""

#ifdef DEBUG
#define VBA_SUBVERSION_STRING " DEBUG"
#elif defined(PUBLIC_RELEASE)
#define VBA_SUBVERSION_STRING ""
#else
#ifdef WIN32
#include "svnrev.h"
#else
#ifdef SVN_REV
#define SVN_REV_STR SVN_REV
#else
#define SVN_REV_STR ""
#endif
#endif
#define VBA_SUBVERSION_STRING " svn" SVN_REV_STR
#endif

#if defined(_MSC_VER)
#define VBA_COMPILER ""
#define VBA_COMPILER_DETAIL " msvc " _Py_STRINGIZE(_MSC_VER)
#define _Py_STRINGIZE(X) _Py_STRINGIZE1((X))
#define _Py_STRINGIZE1(X) _Py_STRINGIZE2 ## X
#define _Py_STRINGIZE2(X) #X
//re: http://72.14.203.104/search?q=cache:HG-okth5NGkJ:mail.python.org/pipermail/python-checkins/2002-November/030704.html+_msc_ver+compiler+version+string&hl=en&gl=us&ct=clnk&cd=5
#else
// TODO: make for others compilers
#define VBA_COMPILER ""
#define VBA_COMPILER_DETAIL ""
#endif

#define VBA_VERSION_STRING "v23.4-interim" VBA_SUBVERSION_STRING VBA_FEATURE_STRING VBA_COMPILER
#define VBA_NAME_AND_VERSION VBA_NAME " " VBA_VERSION_STRING
