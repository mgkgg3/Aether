
#define BOOST_PLATFORM "Win32"

#if defined(__MINGW32__)
#  include <_mingw.h>
#endif

#if defined(__GNUC__) && !defined(BOOST_NO_SWPRINTF)
#  define BOOST_NO_SWPRINTF
#endif

#ifndef BOOST_SYMBOL_EXPORT
#  define BOOST_HAS_DECLSPEC
#  define BOOST_SYMBOL_EXPORT __declspec(dllexport)
#  define BOOST_SYMBOL_IMPORT __declspec(dllimport)
#endif

#if defined(__MINGW32__) && ((__MINGW32_MAJOR_VERSION > 2) || ((__MINGW32_MAJOR_VERSION == 2) && (__MINGW32_MINOR_VERSION >= 0)))
#  define BOOST_HAS_STDINT_H
#  ifndef __STDC_LIMIT_MACROS
#     define __STDC_LIMIT_MACROS
#  endif
#  define BOOST_HAS_DIRENT_H
#  define BOOST_HAS_UNISTD_H
#endif

#if defined(__MINGW32__) && (__GNUC__ >= 4)

#  define BOOST_HAS_GETTIMEOFDAY
#endif

#ifndef BOOST_HAS_PTHREADS
#  define BOOST_HAS_WINTHREADS
#endif

#if defined(_WIN32_WCE) || defined(UNDER_CE)
#  define BOOST_NO_ANSI_APIS

#  define BOOST_NO_SWPRINTF
#else
#  define BOOST_HAS_GETSYSTEMTIMEASFILETIME
#  define BOOST_HAS_THREADEX
#  define BOOST_HAS_GETSYSTEMTIMEASFILETIME
#endif

#if defined(WINAPI_FAMILY) && \
  (WINAPI_FAMILY == WINAPI_FAMILY_APP || WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
#  define BOOST_NO_ANSI_APIS
#endif

#ifndef BOOST_DISABLE_WIN32

#define BOOST_HAS_FTIME
#define BOOST_WINDOWS 1

#endif
