
#if !defined(__SGI_STL_PORT) && !defined(_STLPORT_VERSION)
#  include <cstddef>
#  if !defined(__SGI_STL_PORT) && !defined(_STLPORT_VERSION)
#      error "This is not STLPort!"
#  endif
#endif

#if !defined(CYGWIN) && (  defined(__unix__)  \
                        || defined(__unix)    \
                        || defined(unix)      \
                        || defined(__APPLE__) \
                        || defined(__APPLE)   \
                        || defined(APPLE))
#  include <unistd.h>
#endif

#if (defined(__STL_STATIC_CONST_INIT_BUG) || defined(_STLP_STATIC_CONST_INIT_BUG)) && (__SGI_STL_PORT <= 0x400)
#  define BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
#endif

#if !(defined(_STLP_CLASS_PARTIAL_SPECIALIZATION) || defined(__STL_CLASS_PARTIAL_SPECIALIZATION))
#  define BOOST_NO_STD_ITERATOR_TRAITS
#endif

#if (defined(__GNUC__) && (__GNUC__ < 3)) && !(defined(__SGI_STL_OWN_IOSTREAMS) || defined(_STLP_OWN_IOSTREAMS))
#  define BOOST_NO_STRINGSTREAM
#endif

#if defined(__STL_NO_IOSTREAMS) || defined(__STL_NO_NEW_IOSTREAMS) || defined(_STLP_NO_IOSTREAMS) || defined(_STLP_NO_NEW_IOSTREAMS)
#  define BOOST_NO_STD_LOCALE
#  define BOOST_NO_STRINGSTREAM
#endif

#ifdef _STLPORT_VERSION
#  if !(_STLPORT_VERSION >= 0x500) && !defined(_STLP_OWN_IOSTREAMS) && defined(_STLP_USE_NAMESPACES) && defined(BOOST_NO_USING_TEMPLATE) && !defined(BOOST_BORLANDC)
#     define BOOST_NO_STD_LOCALE
#  endif
#else
#  if !defined(__SGI_STL_OWN_IOSTREAMS) && defined(__STL_USE_NAMESPACES) && defined(BOOST_NO_USING_TEMPLATE) && !defined(BOOST_BORLANDC)
#     define BOOST_NO_STD_LOCALE
#  endif
#endif

#if defined(_STLPORT_VERSION) && (_STLPORT_VERSION >= 0x520)
#  define BOOST_HAS_TR1_UNORDERED_SET
#  define BOOST_HAS_TR1_UNORDERED_MAP
#endif

#if !(defined(__STL_MEMBER_TEMPLATES) || defined(_STLP_MEMBER_TEMPLATES))
#  define BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
#  define BOOST_NO_STD_ALLOCATOR
#endif

#define BOOST_HAS_PARTIAL_STD_ALLOCATOR

#if !defined(_STLP_MEMBER_TEMPLATE_CLASSES) || defined(_STLP_DONT_SUPPORT_REBIND_MEMBER_TEMPLATE)
#  define BOOST_NO_STD_ALLOCATOR
#endif

#if defined(_STLP_NO_MEMBER_TEMPLATE_KEYWORD) && defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
#  define BOOST_NO_STD_ALLOCATOR
#endif

#if !defined(_STLP_HAS_WCHAR_T) && !defined(_STLP_WCHAR_T_IS_USHORT)
#  ifndef  BOOST_NO_STD_WSTRING
#     define BOOST_NO_STD_WSTRING
#  endif
#  ifndef  BOOST_NO_STD_WSTREAMBUF
#     define BOOST_NO_STD_WSTREAMBUF
#  endif
#endif

#ifndef _STLP_NO_EXTENSIONS
#define BOOST_HAS_HASH
#define BOOST_HAS_SLIST
#endif

#if !defined(BOOST_BORLANDC) && !defined(__DMC__)

#  if (defined(__STL_IMPORT_VENDOR_CSTD) \
         || defined(__STL_USE_OWN_NAMESPACE) \
         || defined(_STLP_IMPORT_VENDOR_CSTD) \
         || defined(_STLP_USE_OWN_NAMESPACE)) \
      && (defined(__STL_VENDOR_GLOBAL_CSTD) || defined (_STLP_VENDOR_GLOBAL_CSTD))
#     define BOOST_NO_STDC_NAMESPACE
#     define BOOST_NO_EXCEPTION_STD_NAMESPACE
#  endif
#elif defined(BOOST_BORLANDC) && BOOST_BORLANDC < 0x560

#include <stdlib.h>
namespace std { using ::abs; }

#include <string.h>
#ifdef strcpy
#  undef strcpy
#endif
#ifdef strcmp
#  undef strcmp
#endif
#ifdef _STLP_VENDOR_CSTD
namespace std{ using _STLP_VENDOR_CSTD::strcmp; using _STLP_VENDOR_CSTD::strcpy; }
#endif
#endif

#if defined(__STL_NO_EXPLICIT_FUNCTION_TMPL_ARGS) || defined(_STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS)
#  define BOOST_NO_STD_USE_FACET
#  define BOOST_HAS_STLP_USE_FACET
#endif

#if defined(_STLP_NO_NATIVE_WIDE_FUNCTIONS) && !defined(BOOST_NO_STDC_NAMESPACE)
#  define BOOST_NO_CWCHAR
#  define BOOST_NO_CWCTYPE
#endif

#if !defined( _STLP_HAS_WCHAR_T) || defined(_STLP_WCHAR_T_IS_USHORT)
#  undef  BOOST_NO_INTRINSIC_WCHAR_T
#  define BOOST_NO_INTRINSIC_WCHAR_T
#endif

#if defined(BOOST_BORLANDC) && (BOOST_BORLANDC == 0x560)
#  undef BOOST_HAS_HASH
#endif

#if defined(__GNUC__) && (__GNUC__ < 3)
#  include <algorithm>
#  define BOOST_USING_STD_MIN() ((void)0)
#  define BOOST_USING_STD_MAX() ((void)0)
namespace boost { using std::min; using std::max; }
#endif

#  define BOOST_NO_CXX11_HDR_ARRAY
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
#  define BOOST_NO_CXX11_HDR_TYPE_TRAITS
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

#define BOOST_STDLIB "STLPort standard library version " BOOST_STRINGIZE(__SGI_STL_PORT)
