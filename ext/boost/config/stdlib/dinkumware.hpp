
#if !defined(_YVALS) && !defined(_CPPLIB_VER)
#include <boost/config/no_tr1/utility.hpp>
#if !defined(_YVALS) && !defined(_CPPLIB_VER)
#error This is not the Dinkumware lib!
#endif
#endif

#if defined(_CPPLIB_VER) && (_CPPLIB_VER >= 306)

#  if !(defined(_GLOBAL_USING) && (_GLOBAL_USING+0 > 0)) && !defined(BOOST_BORLANDC) && !defined(_STD) && !(defined(__ICC) && (__ICC >= 700))
#     define BOOST_NO_STDC_NAMESPACE
#  endif
#  if !(defined(_HAS_MEMBER_TEMPLATES_REBIND) && (_HAS_MEMBER_TEMPLATES_REBIND+0 > 0)) && !(defined(_MSC_VER) && (_MSC_VER > 1300)) && defined(BOOST_MSVC)
#     define BOOST_NO_STD_ALLOCATOR
#  endif
#  define BOOST_HAS_PARTIAL_STD_ALLOCATOR
#  if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)

#     define BOOST_NO_STD_USE_FACET
#     define BOOST_HAS_TWO_ARG_USE_FACET

#     define BOOST_NO_STDC_NAMESPACE

#     define BOOST_NO_EXCEPTION_STD_NAMESPACE
#  endif

#  if !defined(_LONGLONG) && (_CPPLIB_VER <= 310)
#     define BOOST_NO_MS_INT64_NUMERIC_LIMITS
#  endif

#else
#  define BOOST_MSVC_STD_ITERATOR 1
#  define BOOST_NO_STD_ITERATOR
#  define BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
#  define BOOST_NO_STD_ALLOCATOR
#  define BOOST_NO_STDC_NAMESPACE
#  define BOOST_NO_STD_USE_FACET
#  define BOOST_NO_STD_OUTPUT_ITERATOR_ASSIGN
#  define BOOST_HAS_MACRO_USE_FACET
#  ifndef _CPPLIB_VER

#     ifndef __MTA__
#        define BOOST_NO_STD_MIN_MAX
#     endif
#     define BOOST_NO_MS_INT64_NUMERIC_LIMITS
#  endif
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1310) && !defined(BOOST_BORLANDC)
#  define BOOST_STD_EXTENSION_NAMESPACE stdext
#endif

#if (defined(_MSC_VER) && (_MSC_VER <= 1300) && !defined(BOOST_BORLANDC)) || !defined(_CPPLIB_VER) || (_CPPLIB_VER < 306)

#  define BOOST_NO_STD_ITERATOR_TRAITS
#endif

#if defined(__ICL) && (__ICL < 800) && defined(_CPPLIB_VER) && (_CPPLIB_VER <= 310)

#  define BOOST_NO_STD_LOCALE
#endif

#if ((defined(BOOST_MSVC) && BOOST_MSVC >= 1400) || (defined(__clang__) && defined(_MSC_VER))) && (_MSC_VER < 1800)

#if !_HAS_EXCEPTIONS
#include <exception>
#endif
#include <typeinfo>
#if !_HAS_EXCEPTIONS
#  define BOOST_NO_STD_TYPEINFO
#endif
#endif
#if defined(__ghs__) && !_HAS_NAMESPACE
#  define BOOST_NO_STD_TYPEINFO
#endif

#if !defined(_CPPLIB_VER) || _CPPLIB_VER < 520
#  define BOOST_NO_CXX11_HDR_ARRAY
#  define BOOST_NO_CXX11_HDR_CODECVT
#  define BOOST_NO_CXX11_HDR_FORWARD_LIST
#  define BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#  define BOOST_NO_CXX11_HDR_RANDOM
#  define BOOST_NO_CXX11_HDR_REGEX
#  define BOOST_NO_CXX11_HDR_SYSTEM_ERROR
#  define BOOST_NO_CXX11_HDR_UNORDERED_MAP
#  define BOOST_NO_CXX11_HDR_UNORDERED_SET
#  define BOOST_NO_CXX11_HDR_TUPLE
#  define BOOST_NO_CXX11_HDR_TYPEINDEX
#  define BOOST_NO_CXX11_HDR_FUNCTIONAL
#  define BOOST_NO_CXX11_NUMERIC_LIMITS
#  define BOOST_NO_CXX11_SMART_PTR
#endif

#if ((!defined(_HAS_TR1_IMPORTS) || (_HAS_TR1_IMPORTS+0 == 0)) && !defined(BOOST_NO_CXX11_HDR_TUPLE)) \
  && (!defined(_CPPLIB_VER) || _CPPLIB_VER < 610)
#  define BOOST_NO_CXX11_HDR_TUPLE
#endif

#if !defined(_CPPLIB_VER) || _CPPLIB_VER < 540
#  define BOOST_NO_CXX11_HDR_TYPE_TRAITS
#  define BOOST_NO_CXX11_HDR_CHRONO
#  define BOOST_NO_CXX11_HDR_CONDITION_VARIABLE
#  define BOOST_NO_CXX11_HDR_FUTURE
#  define BOOST_NO_CXX11_HDR_MUTEX
#  define BOOST_NO_CXX11_HDR_RATIO
#  define BOOST_NO_CXX11_HDR_THREAD
#  define BOOST_NO_CXX11_ATOMIC_SMART_PTR
#  define BOOST_NO_CXX11_HDR_EXCEPTION
#endif

#if !defined(_CPPLIB_VER) || _CPPLIB_VER < 610
#  define BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#  define BOOST_NO_CXX11_HDR_ATOMIC
#  define BOOST_NO_CXX11_ALLOCATOR

#  define BOOST_NO_CXX11_STD_ALIGN
#endif

#if !defined(_CPPLIB_VER) || _CPPLIB_VER < 650
#  define BOOST_NO_CXX11_POINTER_TRAITS
#elif defined(BOOST_MSVC) && BOOST_MSVC < 1910
#  define BOOST_NO_CXX11_POINTER_TRAITS
#endif

#if defined(__has_include)
#if !__has_include(<shared_mutex>)
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#elif (__cplusplus < 201402) && !defined(_MSC_VER)
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#endif
#elif !defined(_CPPLIB_VER) || (_CPPLIB_VER < 650)
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#endif

#if !defined(_CPPLIB_VER) || (_CPPLIB_VER < 650)
#  define BOOST_NO_CXX14_STD_EXCHANGE
#endif

#if !defined(_CPPLIB_VER) || (_CPPLIB_VER < 650) \
 || ((!defined(BOOST_MSVC) || (BOOST_MSVC < 1910))) && (!defined(__clang__) || !defined(_MSC_VER) || (_MSC_VER < 1929))\
 || !defined(_HAS_CXX17) || (_HAS_CXX17 == 0)
#  define BOOST_NO_CXX17_STD_APPLY
#  define BOOST_NO_CXX17_ITERATOR_TRAITS
#  define BOOST_NO_CXX17_HDR_STRING_VIEW
#  define BOOST_NO_CXX17_HDR_OPTIONAL
#  define BOOST_NO_CXX17_HDR_VARIANT
#  define BOOST_NO_CXX17_HDR_ANY
#  define BOOST_NO_CXX17_HDR_MEMORY_RESOURCE
#  define BOOST_NO_CXX17_HDR_CHARCONV
#  define BOOST_NO_CXX17_HDR_EXECUTION
#  define BOOST_NO_CXX17_HDR_FILESYSTEM
#endif
#if !defined(_CPPLIB_VER) || (_CPPLIB_VER < 650) || !defined(_HAS_CXX17) || (_HAS_CXX17 == 0) || !defined(_MSVC_STL_UPDATE) || (_MSVC_STL_UPDATE < 201709)
#  define BOOST_NO_CXX17_STD_INVOKE
#endif

#if !defined(_MSVC_STL_UPDATE) || (_MSVC_STL_UPDATE < 202008L) || !defined(_HAS_CXX20) || (_HAS_CXX20 == 0)
#  define BOOST_NO_CXX20_HDR_CONCEPTS
#endif

#if !(!defined(_CPPLIB_VER) || (_CPPLIB_VER < 650) || !defined(BOOST_MSVC) || (BOOST_MSVC < 1912) || !defined(_HAS_CXX17) || (_HAS_CXX17 == 0))

#  define BOOST_NO_STD_ITERATOR
#endif

#if defined(BOOST_INTEL) && (BOOST_INTEL <= 1400)

#  define BOOST_NO_CXX11_HDR_ATOMIC
#endif

#if !defined(_CPPLIB_VER) || _CPPLIB_VER < 650
#  define BOOST_NO_CXX11_ADDRESSOF
#endif

#if defined(_CPPLIB_VER) && (_CPPLIB_VER == 650) && (!defined(_MSVC_STL_VERSION) || (_MSVC_STL_VERSION < 142))
#  define BOOST_NO_CXX11_HDR_CODECVT
#endif

#if (_MSVC_LANG > 201700) && !defined(BOOST_NO_CXX11_HDR_CODECVT)

#  define BOOST_NO_CXX11_HDR_CODECVT
#endif

#if defined(_CPPLIB_VER) && (_CPPLIB_VER >= 650)

#  if defined(_HAS_AUTO_PTR_ETC) && (_HAS_AUTO_PTR_ETC == 0)
#    define BOOST_NO_AUTO_PTR
#    define BOOST_NO_CXX98_RANDOM_SHUFFLE
#    define BOOST_NO_CXX98_FUNCTION_BASE
#    define BOOST_NO_CXX98_BINDERS
#  elif defined(_HAS_DEPRECATED_ADAPTOR_TYPEDEFS) && (_HAS_DEPRECATED_ADAPTOR_TYPEDEFS == 0)
#    define BOOST_NO_CXX98_BINDERS
#  endif
#endif

#if defined(_HAS_CXX20)
#  define BOOST_NO_CXX11_ATOMIC_SMART_PTR
#endif

#ifdef _M_CEE
#ifndef BOOST_NO_CXX11_HDR_MUTEX
#  define BOOST_NO_CXX11_HDR_MUTEX
#endif
#ifndef BOOST_NO_CXX11_HDR_ATOMIC
#  define BOOST_NO_CXX11_HDR_ATOMIC
#endif
#ifndef BOOST_NO_CXX11_HDR_FUTURE
#  define BOOST_NO_CXX11_HDR_FUTURE
#endif
#ifndef BOOST_NO_CXX11_HDR_CONDITION_VARIABLE
#  define BOOST_NO_CXX11_HDR_CONDITION_VARIABLE
#endif
#ifndef BOOST_NO_CXX11_HDR_THREAD
#  define BOOST_NO_CXX11_HDR_THREAD
#endif
#ifndef BOOST_NO_CXX14_HDR_SHARED_MUTEX
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#endif
#ifndef BOOST_NO_CXX14_STD_EXCHANGE
#  define BOOST_NO_CXX14_STD_EXCHANGE
#endif
#ifndef BOOST_NO_FENV_H
#  define BOOST_NO_FENV_H
#endif
#endif

#ifdef _CPPLIB_VER
#  define BOOST_DINKUMWARE_STDLIB _CPPLIB_VER
#else
#  define BOOST_DINKUMWARE_STDLIB 1
#endif

#if defined(_MSVC_STL_VERSION)
#  define BOOST_MSSTL_VERSION _MSVC_STL_VERSION

#elif defined(_CPPLIB_VER) && _CPPLIB_VER >= 650
#  define BOOST_MSSTL_VERSION 140

#elif defined(_CPPLIB_VER) && _CPPLIB_VER >= 610
#  define BOOST_MSSTL_VERSION 120

#elif defined(_CPPLIB_VER) && _CPPLIB_VER >= 540
#  define BOOST_MSSTL_VERSION 110

#elif defined(_CPPLIB_VER) && _CPPLIB_VER >= 520
#  define BOOST_MSSTL_VERSION 100

#elif defined(_CPPLIB_VER) && _CPPLIB_VER >= 505
#  define BOOST_MSSTL_VERSION 91

#elif defined(_CPPLIB_VER) && _CPPLIB_VER >= 503
#  define BOOST_MSSTL_VERSION 90

#elif defined(_CPPLIB_VER) && _CPPLIB_VER >= 405
#  define BOOST_MSSTL_VERSION 80

#endif

#ifdef _CPPLIB_VER
#  define BOOST_STDLIB "Dinkumware standard library version " BOOST_STRINGIZE(_CPPLIB_VER)
#else
#  define BOOST_STDLIB "Dinkumware standard library version 1.x"
#endif
