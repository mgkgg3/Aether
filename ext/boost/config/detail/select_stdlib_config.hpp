
#if defined(__cplusplus) && defined(__has_include)
#  if __has_include(<version>)

#    include <version>
#  else
#    include <cstddef>
#  endif
#elif defined(__cplusplus)
#  include <cstddef>
#else
#  include <stddef.h>
#endif

#if defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION)

#  define BOOST_STDLIB_CONFIG "boost/config/stdlib/stlport.hpp"

#else

#if !defined(__LIBCOMO__) && !defined(__STD_RWCOMPILER_H__) && !defined(_RWSTD_VER)\
   && !defined(_LIBCPP_VERSION) && !defined(__GLIBCPP__) && !defined(__GLIBCXX__)\
   && !defined(__STL_CONFIG_H) && !defined(__MSL_CPP__) && !defined(__IBMCPP__)\
   && !defined(MSIPL_COMPILE_H) && !defined(_YVALS) && !defined(_CPPLIB_VER)
#include <utility>
#endif

#if defined(__LIBCOMO__)

#define BOOST_STDLIB_CONFIG "boost/config/stdlib/libcomo.hpp"

#elif defined(__STD_RWCOMPILER_H__) || defined(_RWSTD_VER)

#  define BOOST_STDLIB_CONFIG "boost/config/stdlib/roguewave.hpp"

#elif defined(_LIBCPP_VERSION)

#  define BOOST_STDLIB_CONFIG "boost/config/stdlib/libcpp.hpp"

#elif defined(__GLIBCPP__) || defined(__GLIBCXX__)

#  define BOOST_STDLIB_CONFIG "boost/config/stdlib/libstdcpp3.hpp"

#elif defined(__STL_CONFIG_H)

#  define BOOST_STDLIB_CONFIG "boost/config/stdlib/sgi.hpp"

#elif defined(__MSL_CPP__)

#  define BOOST_STDLIB_CONFIG "boost/config/stdlib/msl.hpp"

#elif defined(__IBMCPP__) && defined(__COMPILER_VER__) && defined(__MVS__)

#  define BOOST_STDLIB_CONFIG "boost/config/stdlib/xlcpp_zos.hpp"

#elif defined(__IBMCPP__)

#  define BOOST_STDLIB_CONFIG "boost/config/stdlib/vacpp.hpp"

#elif defined(MSIPL_COMPILE_H)

#  define BOOST_STDLIB_CONFIG "boost/config/stdlib/modena.hpp"

#elif (defined(_YVALS) && !defined(__IBMCPP__)) || defined(_CPPLIB_VER)

#  define BOOST_STDLIB_CONFIG "boost/config/stdlib/dinkumware.hpp"

#elif defined (BOOST_ASSERT_CONFIG)

#  error "Unknown standard library - please configure and report the results to boost.org"

#endif

#endif

#if 0

#  include "boost/config/stdlib/stlport.hpp"
#  include "boost/config/stdlib/libcomo.hpp"
#  include "boost/config/stdlib/roguewave.hpp"
#  include "boost/config/stdlib/libcpp.hpp"
#  include "boost/config/stdlib/libstdcpp3.hpp"
#  include "boost/config/stdlib/sgi.hpp"
#  include "boost/config/stdlib/msl.hpp"
#  include "boost/config/stdlib/xlcpp_zos.hpp"
#  include "boost/config/stdlib/vacpp.hpp"
#  include "boost/config/stdlib/modena.hpp"
#  include "boost/config/stdlib/dinkumware.hpp"
#endif
