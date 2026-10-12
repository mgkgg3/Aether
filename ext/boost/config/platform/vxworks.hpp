
#include <version.h>
#if !defined(_WRS_VXWORKS_MAJOR) || (_WRS_VXWORKS_MAJOR < 6)
#  error "The vxWorks version you're using is so badly outdated,\
          it doesn't work at all with boost, sorry, no chance!"
#endif

#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR < 9)

#endif

#define BOOST_PLATFORM "vxWorks"

#define BOOST_HAS_UNISTD_H
#define BOOST_HAS_STDINT_H
#define BOOST_HAS_DIRENT_H

#ifndef BOOST_LOCALE_WITH_ICU
   #define BOOST_LOCALE_WITH_ICU
#endif

#define BOOST_HAS_THREADS
#define BOOST_HAS_NANOSLEEP
#define BOOST_HAS_GETTIMEOFDAY
#define BOOST_HAS_CLOCK_GETTIME
#define BOOST_HAS_MACRO_USE_FACET

#define BOOST_HAS_PTHREADS
#define BOOST_HAS_SCHED_YIELD
#define BOOST_HAS_SIGACTION

#ifdef __RTP__
#  define BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE
#  define BOOST_HAS_LOG1P
#  define BOOST_HAS_EXPM1
#endif

#ifdef _WRS_KERNEL

#endif

#ifdef _WRS_KERNEL
#  ifndef _POSIX_TIMERS
#    define _POSIX_TIMERS  1
#  endif
#  ifndef _POSIX_THREADS
#    define _POSIX_THREADS 1
#  endif

#  define BOOST_THREAD_USES_GETPAGESIZE
#endif

#if (_WRS_VXWORKS_MAJOR < 7)

#  ifdef __RTP__
#    include <time.h>
#    include <sysLib.h>
#  endif

#  include <cstdint>

#  define VX_JOIN(X, Y)     VX_DO_JOIN(X, Y)
#  define VX_DO_JOIN(X, Y)  VX_DO_JOIN2(X, Y)
#  define VX_DO_JOIN2(X, Y) X##Y

#  undef  INT32_C
#  undef  UINT32_C
#  undef  INT64_C
#  undef  UINT64_C
#  define INT32_C(x)  VX_JOIN(x, L)
#  define UINT32_C(x) VX_JOIN(x, UL)
#  define INT64_C(x)  VX_JOIN(x, LL)
#  define UINT64_C(x) VX_JOIN(x, ULL)

#  include <sys/time.h>
#endif

#include <ioLib.h>
#include <tickLib.h>

#if defined(_WRS_KERNEL) && (_CPPLIB_VER < 700)

#  define BOOST_NO_CWCHAR

#  define BOOST_NO_CWCTYPE
#  define BOOST_NO_SWPRINTF
#  define BOOST_NO_STD_WSTRING
#  define BOOST_NO_STD_WSTREAMBUF
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined ( __RTP__) &&  (_WRS_VXWORKS_MAJOR < 7)
  inline int getrlimit(int resource, struct rlimit *rlp){
    return getprlimit(0, 0, resource, rlp);
  }

  inline int setrlimit(int resource, const struct rlimit *rlp){
    return setprlimit(0, 0, resource, const_cast<struct rlimit*>(rlp));
  }
#endif

inline int truncate(const char *p, off_t l){
  int fd = open(p, O_WRONLY);
  if (fd == -1){
    errno = EACCES;
    return -1;
  }
  if (ftruncate(fd, l) == -1){
    close(fd);
    errno = EACCES;
    return -1;
  }
  return close(fd);
}

#ifdef __GNUC__
#  define ___unused __attribute__((unused))
#else
#  define ___unused
#endif

inline int symlink(const char* path1 ___unused, const char* path2 ___unused){

  errno = EACCES;
  return -1;
}

inline ssize_t readlink(const char* path1 ___unused, char* path2 ___unused, size_t size ___unused){

  errno = EACCES;
  return -1;
}

#if (_WRS_VXWORKS_MAJOR < 7)

inline int gettimeofday(struct timeval *tv, void * ) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  tv->tv_sec  = ts.tv_sec;
  tv->tv_usec = ts.tv_nsec / 1000;
  return 0;
}
#endif

#ifdef __cplusplus
}
#endif

#if (_WRS_VXWORKS_MAJOR < 7)
#  ifdef __cplusplus

struct tms{
  clock_t tms_utime;
  clock_t tms_stime;
  clock_t tms_cutime;
  clock_t tms_cstime;
};

 inline clock_t times(struct tms *t){
  struct timespec ts;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
  clock_t ticks(static_cast<clock_t>(static_cast<double>(ts.tv_sec)  * CLOCKS_PER_SEC +
                                     static_cast<double>(ts.tv_nsec) * CLOCKS_PER_SEC / 1000000.0));
  t->tms_utime  = ticks/2U;
  t->tms_stime  = ticks/2U;
  t->tms_cutime = 0;
  t->tms_cstime = 0;
  return ticks;
}

namespace std {
    using ::times;
}
#  endif
#endif

#ifdef __cplusplus
extern "C" void   bzero     (void *, size_t);

namespace std {
#  ifdef __RTP__
    using ::getrlimit;
    using ::setrlimit;
#  endif
  using ::truncate;
  using ::symlink;
  using ::readlink;
#  if (_WRS_VXWORKS_MAJOR < 7)
    using ::gettimeofday;
#  endif
}
#endif

#include <signal.h>

#if (_WRS_VXWORKS_MAJOR < 7)
#  define getpagesize()    sysconf(_SC_PAGESIZE)
inline int lstat(p, b) { return stat(p, b); }
#endif

#ifndef S_ISSOCK
#  define S_ISSOCK(mode) ((mode & S_IFMT) == S_IFSOCK)
#endif
#ifndef FPE_FLTINV
#  define FPE_FLTINV     (FPE_FLTSUB+1)
#endif
#if !defined(BUS_ADRALN) && defined(BUS_ADRALNR)
#  define BUS_ADRALN     BUS_ADRALNR
#endif
typedef int              locale_t;

#include <boost/config/detail/posix_features.hpp>

#undef BOOST_HAS_NL_TYPES_H

#if !( defined( _WRS_CONFIG_LANG_LIB_CPLUS_CPLUS_USER_2011) || defined(_WRS_CONFIG_LIBCPLUS_STD))
#  define BOOST_NO_CXX11_ADDRESSOF
#  define BOOST_NO_CXX11_ALLOCATOR
#  define BOOST_NO_CXX11_ATOMIC_SMART_PTR
#  define BOOST_NO_CXX11_NUMERIC_LIMITS
#  define BOOST_NO_CXX11_SMART_PTR
#  define BOOST_NO_CXX11_STD_ALIGN

#  define BOOST_NO_CXX11_HDR_ARRAY
#  define BOOST_NO_CXX11_HDR_ATOMIC
#  define BOOST_NO_CXX11_HDR_CHRONO
#  define BOOST_NO_CXX11_HDR_CONDITION_VARIABLE
#  define BOOST_NO_CXX11_HDR_FORWARD_LIST
#  define BOOST_NO_CXX11_HDR_FUNCTIONAL
#  define BOOST_NO_CXX11_HDR_FUTURE
#  define BOOST_NO_CXX11_HDR_MUTEX
#  define BOOST_NO_CXX11_HDR_RANDOM
#  define BOOST_NO_CXX11_HDR_RATIO
#  define BOOST_NO_CXX11_HDR_REGEX
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#  define BOOST_NO_CXX11_HDR_SYSTEM_ERROR
#  define BOOST_NO_CXX11_HDR_THREAD
#  define BOOST_NO_CXX11_HDR_TYPEINDEX
#  define BOOST_NO_CXX11_HDR_TYPE_TRAITS
#  define BOOST_NO_CXX11_HDR_TUPLE
#  define BOOST_NO_CXX11_HDR_UNORDERED_MAP
#  define BOOST_NO_CXX11_HDR_UNORDERED_SET
#else
#  ifndef  BOOST_SYSTEM_NO_DEPRECATED
#    define BOOST_SYSTEM_NO_DEPRECATED
#  endif
#endif

#undef NONE

#undef restrict

#undef V7

#if (_WRS_VXWORKS_MAJOR > 6)
#  ifndef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR
#    define BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR
#  endif
#else
#  define BOOST_ASIO_DISABLE_SERIAL_PORT
#endif
