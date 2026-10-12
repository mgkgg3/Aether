
#if (defined(linux) || defined(__linux) || defined(__linux__) || defined(__GNU__) || defined(__GLIBC__)) && !defined(_CRAYC)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/linux.hpp"

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/bsd.hpp"

#elif defined(sun) || defined(__sun)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/solaris.hpp"

#elif defined(__sgi)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/irix.hpp"

#elif defined(__hpux)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/hpux.hpp"

#elif defined(__CYGWIN__)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/cygwin.hpp"

#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/win32.hpp"

#elif defined(__HAIKU__)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/haiku.hpp"

#elif defined(__BEOS__)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/beos.hpp"

#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/macos.hpp"

#elif defined(__TOS_MVS__)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/zos.hpp"

#elif defined(__IBMCPP__) || defined(_AIX)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/aix.hpp"

#elif defined(__amigaos__)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/amigaos.hpp"

#elif defined(__QNXNTO__)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/qnxnto.hpp"

#elif defined(__VXWORKS__)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/vxworks.hpp"

#elif defined(__SYMBIAN32__)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/symbian.hpp"

#elif defined(_CRAYC)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/cray.hpp"

#elif defined(__VMS)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/vms.hpp"

#elif defined(__CloudABI__)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/cloudabi.hpp"

#elif defined (__wasm__)

#  define BOOST_PLATFORM_CONFIG "boost/config/platform/wasm.hpp"

#else

#  if defined(unix) \
      || defined(__unix) \
      || defined(_XOPEN_SOURCE) \
      || defined(_POSIX_SOURCE)

#  ifndef BOOST_HAS_UNISTD_H
#     define BOOST_HAS_UNISTD_H
#  endif

#  include <boost/config/detail/posix_features.hpp>

#  endif

#  if defined (BOOST_ASSERT_CONFIG)

#     error "Unknown platform - please configure and report the results to boost.org"
#  endif

#endif

#if 0

#  include "boost/config/platform/linux.hpp"
#  include "boost/config/platform/bsd.hpp"
#  include "boost/config/platform/solaris.hpp"
#  include "boost/config/platform/irix.hpp"
#  include "boost/config/platform/hpux.hpp"
#  include "boost/config/platform/cygwin.hpp"
#  include "boost/config/platform/win32.hpp"
#  include "boost/config/platform/beos.hpp"
#  include "boost/config/platform/macos.hpp"
#  include "boost/config/platform/zos.hpp"
#  include "boost/config/platform/aix.hpp"
#  include "boost/config/platform/amigaos.hpp"
#  include "boost/config/platform/qnxnto.hpp"
#  include "boost/config/platform/vxworks.hpp"
#  include "boost/config/platform/symbian.hpp"
#  include "boost/config/platform/cray.hpp"
#  include "boost/config/platform/vms.hpp"
#  include <boost/config/detail/posix_features.hpp>

#endif
