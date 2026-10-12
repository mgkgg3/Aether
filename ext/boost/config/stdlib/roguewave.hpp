
#define BOOST_RW_STDLIB 1

#if !defined(__STD_RWCOMPILER_H__) && !defined(_RWSTD_VER)
#  include <boost/config/no_tr1/utility.hpp>
#  if !defined(__STD_RWCOMPILER_H__) && !defined(_RWSTD_VER)
#     error This is not the Rogue Wave standard library
#  endif
#endif

#ifndef _RWSTD_VER
#  define BOOST_RWSTD_VER 0x010000
#elif _RWSTD_VER < 0x010000
#  define BOOST_RWSTD_VER (_RWSTD_VER << 8)
#else
#  define BOOST_RWSTD_VER _RWSTD_VER
#endif

#ifndef _RWSTD_VER
#  define BOOST_STDLIB "Rogue Wave standard library version (Unknown version)"
#elif _RWSTD_VER < 0x04010200
 #  define BOOST_STDLIB "Rogue Wave standard library version " BOOST_STRINGIZE(_RWSTD_VER)
#else
#  ifdef _RWSTD_VER_STR
#    define BOOST_STDLIB "Apache STDCXX standard library version " _RWSTD_VER_STR
#  else
#    define BOOST_STDLIB "Apache STDCXX standard library version " BOOST_STRINGIZE(_RWSTD_VER)
#  endif
#endif

#if BOOST_RWSTD_VER < 0x020200
#  define BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
#endif

#if BOOST_RWSTD_VER <= 0x020101 && (!defined(__SUNPRO_CC) || (__SUNPRO_CC < 0x550))
#  define BOOST_NO_LONG_LONG_NUMERIC_LIMITS
# endif

#ifdef BOOST_BORLANDC
#  define BOOST_NO_MS_INT64_NUMERIC_LIMITS
#endif

#if defined(_RWSTD_NO_SIMPLE_DEFAULT_TEMPLATES) || defined(RWSTD_NO_SIMPLE_DEFAULT_TEMPLATES) || (BOOST_RWSTD_VER < 0x020000)
#  define BOOST_NO_STD_ITERATOR
#endif

#if defined(_RWSTD_NO_CLASS_PARTIAL_SPEC) || defined(RWSTD_NO_CLASS_PARTIAL_SPEC)
#  define BOOST_NO_STD_ITERATOR_TRAITS
#endif

#if (BOOST_RWSTD_VER < 0x020000)
#  define BOOST_NO_AUTO_PTR
#  define BOOST_NO_STRINGSTREAM
#  define BOOST_NO_STD_ALLOCATOR
#  define BOOST_NO_STD_LOCALE
#endif

#if defined(RWSTD_NO_MEMBER_TEMPLATES) || defined(_RWSTD_NO_MEMBER_TEMPLATES)
#  define BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
#endif

#if !defined(_RWSTD_ALLOCATOR) || (defined(__HP_aCC) && __HP_aCC <= 33100)
#  define BOOST_NO_STD_ALLOCATOR
#endif

#if defined(_RWSTD_NO_TEMPLATE_ON_RETURN_TYPE) && !defined(BOOST_NO_STD_LOCALE)
#  define BOOST_NO_STD_USE_FACET
#  define BOOST_HAS_TWO_ARG_USE_FACET
#endif

#if (BOOST_RWSTD_VER < 0x020000) || defined(_RWSTD_NO_CLASS_PARTIAL_SPEC)
    #define BOOST_NO_STD_DISTANCE
#endif

#if BOOST_RWSTD_VER < 0x020100
#  define BOOST_NO_STD_OUTPUT_ITERATOR_ASSIGN
#endif

#if !defined(_RWSTD_LONG_LONG) && defined(BOOST_HAS_LONG_LONG)
#  undef BOOST_HAS_LONG_LONG
#endif

#if defined(__HP_aCC) && !defined(_HP_NAMESPACE_STD)
#  error "Boost requires Standard RW library. Please compile and link with -AA"
#endif

#if defined(__HP_aCC) && (BOOST_RWSTD_VER == 0x02020100)
#  ifndef __HP_TC1_MAKE_PAIR
#    define __HP_TC1_MAKE_PAIR
#  endif
#  ifndef _HP_INSTANTIATE_STD2_VL
#    define _HP_INSTANTIATE_STD2_VL
#  endif
#endif

#if _RWSTD_VER < 0x05000000
#  define BOOST_NO_CXX11_HDR_ARRAY
#endif

#  define BOOST_NO_CXX11_HDR_TYPE_TRAITS

#  define BOOST_NO_CXX11_HDR_CHRONO
#  define BOOST_NO_CXX11_HDR_CODECVT
#  define BOOST_NO_CXX11_HDR_CONDITION_VARIABLE
#  define BOOST_NO_CXX11_HDR_FORWARD_LIST
#  define BOOST_NO_CXX11_HDR_FUTURE
#  define BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#  define BOOST_NO_CXX11_HDR_MUTEX
#  define BOOST_NO_CXX11_HDR_RANDOM
#  define BOOST_NO_CXX11_HDR_RATIO
#  define BOOST_NO_CXX11_HDR_REGEX
#  define BOOST_NO_CXX11_HDR_SYSTEM_ERROR
#  define BOOST_NO_CXX11_HDR_THREAD
#  define BOOST_NO_CXX11_HDR_TUPLE
#  define BOOST_NO_CXX11_HDR_TYPEINDEX
#  define BOOST_NO_CXX11_HDR_UNORDERED_MAP
#  define BOOST_NO_CXX11_HDR_UNORDERED_SET
#  define BOOST_NO_CXX11_NUMERIC_LIMITS
#  define BOOST_NO_CXX11_ALLOCATOR
#  define BOOST_NO_CXX11_POINTER_TRAITS
#  define BOOST_NO_CXX11_ATOMIC_SMART_PTR
#  define BOOST_NO_CXX11_SMART_PTR
#  define BOOST_NO_CXX11_HDR_FUNCTIONAL
#  define BOOST_NO_CXX11_HDR_ATOMIC
#  define BOOST_NO_CXX11_STD_ALIGN
#  define BOOST_NO_CXX11_ADDRESSOF
#  define BOOST_NO_CXX11_HDR_EXCEPTION

#if defined(__has_include)
#if !__has_include(<shared_mutex>)
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#elif __cplusplus < 201402
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#endif
#else
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#endif

#  define BOOST_NO_CXX14_STD_EXCHANGE

#  define BOOST_NO_CXX17_STD_APPLY
#  define BOOST_NO_CXX17_STD_INVOKE
#  define BOOST_NO_CXX17_ITERATOR_TRAITS
