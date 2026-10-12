
#define BOOST_GNU_STDLIB 1

#ifdef __GLIBCXX__
#define BOOST_STDLIB "GNU libstdc++ version " BOOST_STRINGIZE(__GLIBCXX__)
#else
#define BOOST_STDLIB "GNU libstdc++ version " BOOST_STRINGIZE(__GLIBCPP__)
#endif

#if !defined(_GLIBCPP_USE_WCHAR_T) && !defined(_GLIBCXX_USE_WCHAR_T)
#  define BOOST_NO_CWCHAR
#  define BOOST_NO_CWCTYPE
#  define BOOST_NO_STD_WSTRING
#  define BOOST_NO_STD_WSTREAMBUF
#endif

#if defined(__osf__) && !defined(_REENTRANT) \
  && ( defined(_GLIBCXX_HAVE_GTHR_DEFAULT) || defined(_GLIBCPP_HAVE_GTHR_DEFAULT) )

#  define _REENTRANT
#endif

#ifdef __GLIBCXX__
#  if defined(_GLIBCXX_HAVE_GTHR_DEFAULT) \
        || defined(_GLIBCXX__PTHREADS) \
        || defined(_GLIBCXX_HAS_GTHREADS) \
        || defined(_WIN32) \
        || defined(_AIX) \
        || defined(__HAIKU__)

#     define BOOST_HAS_THREADS
#  else
#     define BOOST_DISABLE_THREADS
#  endif
#elif defined(__GLIBCPP__) \
        && !defined(_GLIBCPP_HAVE_GTHR_DEFAULT) \
        && !defined(_GLIBCPP__PTHREADS)

#  define BOOST_DISABLE_THREADS
#endif

#if (defined(linux) || defined(__linux) || defined(__linux__)) && defined(__arm__) && defined(_GLIBCPP_HAVE_GTHR_DEFAULT)

#  define BOOST_HAS_THREADS
#endif

#if !defined(_GLIBCPP_USE_LONG_LONG) \
    && !defined(_GLIBCXX_USE_LONG_LONG)\
    && defined(BOOST_HAS_LONG_LONG)

#  undef BOOST_HAS_LONG_LONG
#endif

#if !defined(CYGWIN) && (  defined(__unix__)  \
                        || defined(__unix)    \
                        || defined(unix)      \
                        || defined(__APPLE__) \
                        || defined(__APPLE)   \
                        || defined(APPLE))
#  include <unistd.h>
#endif

#ifndef __VXWORKS__
#if defined(__GLIBCXX__) || (defined(__GLIBCPP__) && __GLIBCPP__>=20020514)
#  define BOOST_STD_EXTENSION_NAMESPACE __gnu_cxx
#  define BOOST_HAS_SLIST
#  define BOOST_HAS_HASH
#  define BOOST_SLIST_HEADER <ext/slist>
# if !defined(__GNUC__) || __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 3)
#   define BOOST_HASH_SET_HEADER <ext/hash_set>
#   define BOOST_HASH_MAP_HEADER <ext/hash_map>
# else
#   define BOOST_HASH_SET_HEADER <backward/hash_set>
#   define BOOST_HASH_MAP_HEADER <backward/hash_map>
# endif
#endif
#endif

#if defined(__has_include)
#if defined(BOOST_HAS_HASH)
#if !__has_include(BOOST_HASH_SET_HEADER) || (__GNUC__ >= 10)
#undef BOOST_HAS_HASH
#undef BOOST_HAS_SET_HEADER
#undef BOOST_HAS_MAP_HEADER
#endif
#if !__has_include(BOOST_SLIST_HEADER)
#undef BOOST_HAS_SLIST
#undef BOOST_HAS_SLIST_HEADER
#endif
#endif
#endif

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || (__cplusplus >= 201103)
#  define BOOST_LIBSTDCXX11
#endif

#ifdef __clang__

#ifdef _GLIBCXX_RELEASE
#  define BOOST_LIBSTDCXX_VERSION (_GLIBCXX_RELEASE * 10000 + 100)
#else

#if __has_include(<expected>)
#  define BOOST_LIBSTDCXX_VERSION 120100
#elif __has_include(<source_location>)
#  define BOOST_LIBSTDCXX_VERSION 110100
#elif __has_include(<compare>)
#  define BOOST_LIBSTDCXX_VERSION 100100
#elif __has_include(<memory_resource>)
#  define BOOST_LIBSTDCXX_VERSION 90100
#elif __has_include(<charconv>)
#  define BOOST_LIBSTDCXX_VERSION 80100
#elif __has_include(<variant>)
#  define BOOST_LIBSTDCXX_VERSION 70100
#elif __has_include(<experimental/memory_resource>)
#  define BOOST_LIBSTDCXX_VERSION 60100
#elif __has_include(<experimental/any>)
#  define BOOST_LIBSTDCXX_VERSION 50100
#elif __has_include(<shared_mutex>)
#  define BOOST_LIBSTDCXX_VERSION 40900
#elif __has_include(<ext/cmath>)
#  define BOOST_LIBSTDCXX_VERSION 40800
#elif __has_include(<scoped_allocator>)
#  define BOOST_LIBSTDCXX_VERSION 40700
#elif __has_include(<typeindex>)
#  define BOOST_LIBSTDCXX_VERSION 40600
#elif __has_include(<future>)
#  define BOOST_LIBSTDCXX_VERSION 40500
#elif  __has_include(<ratio>)
#  define BOOST_LIBSTDCXX_VERSION 40400
#elif __has_include(<array>)
#  define BOOST_LIBSTDCXX_VERSION 40300
#endif
#endif

#if defined(BOOST_HAS_FLOAT128) && !defined(_GLIBCXX_USE_FLOAT128)
#  undef BOOST_HAS_FLOAT128
#endif

#if (BOOST_LIBSTDCXX_VERSION >= 100000) && defined(BOOST_HAS_HASH)

#undef BOOST_HAS_HASH
#undef BOOST_HAS_SET_HEADER
#undef BOOST_HAS_MAP_HEADER
#endif

#if (BOOST_LIBSTDCXX_VERSION >= 100000) && defined(BOOST_HAS_HASH)

#undef BOOST_HAS_HASH
#undef BOOST_HAS_SET_HEADER
#undef BOOST_HAS_MAP_HEADER
#endif

#if (BOOST_LIBSTDCXX_VERSION < 50100)

extern "C" char *gets (char *__s);
#endif

#if BOOST_LIBSTDCXX_VERSION < 50000
#  define BOOST_NO_CXX11_HDR_REGEX
#endif

#if BOOST_LIBSTDCXX_VERSION < 40800
#  define BOOST_NO_CXX11_THREAD_LOCAL
#endif

#if (__clang_major__ < 5)
#  if BOOST_LIBSTDCXX_VERSION < 40800
#     define BOOST_NO_CXX11_HDR_FUTURE
#     define BOOST_NO_CXX11_HDR_MUTEX
#     define BOOST_NO_CXX11_HDR_CONDITION_VARIABLE
#     define BOOST_NO_CXX11_HDR_CHRONO
#  endif
#endif

#endif

#if defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x5130) && (__cplusplus >= 201103L)

#if __SUNPRO_CC >= 0x5140
#define BOOST_LIBSTDCXX_VERSION 50100
#else
#define BOOST_LIBSTDCXX_VERSION 40800
#endif
#endif

#if !defined(BOOST_LIBSTDCXX_VERSION)
#  define BOOST_LIBSTDCXX_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#if defined(BOOST_LIBSTDCXX11)
#  if BOOST_LIBSTDCXX_VERSION < 40600
#     if !_GLIBCXX_DEPRECATED
#        define BOOST_NO_AUTO_PTR
#     endif
#  elif !defined(_GLIBCXX_USE_DEPRECATED) || !_GLIBCXX_USE_DEPRECATED
#     define BOOST_NO_AUTO_PTR
#     define BOOST_NO_CXX98_BINDERS
#  endif
#endif

#if (BOOST_LIBSTDCXX_VERSION < 40300) || !defined(BOOST_LIBSTDCXX11)
#  define BOOST_NO_CXX11_HDR_ARRAY
#  define BOOST_NO_CXX11_HDR_TUPLE
#  define BOOST_NO_CXX11_HDR_UNORDERED_MAP
#  define BOOST_NO_CXX11_HDR_UNORDERED_SET
#  define BOOST_NO_CXX11_HDR_FUNCTIONAL
#endif

#if (BOOST_LIBSTDCXX_VERSION < 40400) || !defined(BOOST_LIBSTDCXX11)
#  define BOOST_NO_CXX11_HDR_CONDITION_VARIABLE
#  define BOOST_NO_CXX11_HDR_FORWARD_LIST
#  define BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#  define BOOST_NO_CXX11_HDR_MUTEX
#  define BOOST_NO_CXX11_HDR_RATIO
#  define BOOST_NO_CXX11_HDR_SYSTEM_ERROR
#  define BOOST_NO_CXX11_SMART_PTR
#  define BOOST_NO_CXX11_HDR_EXCEPTION
#else
#  define BOOST_HAS_TR1_COMPLEX_INVERSE_TRIG
#  define BOOST_HAS_TR1_COMPLEX_OVERLOADS
#endif

#if (BOOST_LIBSTDCXX_VERSION < 40500) || !defined(BOOST_LIBSTDCXX11)
#  define BOOST_NO_CXX11_NUMERIC_LIMITS
#  define BOOST_NO_CXX11_HDR_FUTURE
#  define BOOST_NO_CXX11_HDR_RANDOM
#endif

#if (BOOST_LIBSTDCXX_VERSION < 40600) || !defined(BOOST_LIBSTDCXX11)
#  define BOOST_NO_CXX11_HDR_TYPEINDEX
#  define BOOST_NO_CXX11_ADDRESSOF
#  define BOOST_NO_CXX17_ITERATOR_TRAITS
#endif

#if (BOOST_LIBSTDCXX_VERSION < 40700) || !defined(BOOST_LIBSTDCXX11)

#  define BOOST_NO_CXX11_HDR_CHRONO
#  define BOOST_NO_CXX11_ALLOCATOR
#  define BOOST_NO_CXX11_POINTER_TRAITS
#endif

#if (BOOST_LIBSTDCXX_VERSION < 40800) || !defined(BOOST_LIBSTDCXX11)

#  define BOOST_NO_CXX11_HDR_ATOMIC
#  define BOOST_NO_CXX11_HDR_THREAD
#endif

#if (BOOST_LIBSTDCXX_VERSION < 40900) || !defined(BOOST_LIBSTDCXX11)

#  define BOOST_NO_CXX11_HDR_REGEX
#endif
#if (BOOST_LIBSTDCXX_VERSION < 40900) || (__cplusplus <= 201103)
#  define BOOST_NO_CXX14_STD_EXCHANGE
#endif

#if (BOOST_LIBSTDCXX_VERSION < 50100) || !defined(BOOST_LIBSTDCXX11)
#  define BOOST_NO_CXX11_HDR_TYPE_TRAITS
#  define BOOST_NO_CXX11_HDR_CODECVT
#  define BOOST_NO_CXX11_ATOMIC_SMART_PTR
#  define BOOST_NO_CXX11_STD_ALIGN
#endif

#if (BOOST_LIBSTDCXX_VERSION < 70100) || (__cplusplus <= 201402L)
#  define BOOST_NO_CXX17_STD_INVOKE
#  define BOOST_NO_CXX17_STD_APPLY
#  define BOOST_NO_CXX17_HDR_OPTIONAL
#  define BOOST_NO_CXX17_HDR_STRING_VIEW
#  define BOOST_NO_CXX17_HDR_VARIANT
#endif

#if defined(__has_include)
#if !__has_include(<shared_mutex>)
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#elif __cplusplus <= 201103
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#endif

#if (BOOST_LIBSTDCXX_VERSION < 100100)
#if !__has_include(<tbb/tbb.h>)
#define BOOST_NO_CXX17_HDR_EXECUTION
#endif
#endif
#elif __cplusplus < 201402 || (BOOST_LIBSTDCXX_VERSION < 40900) || !defined(BOOST_LIBSTDCXX11)
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#endif

#if BOOST_LIBSTDCXX_VERSION < 100100

#  define BOOST_NO_CXX17_HDR_CHARCONV
#endif

#if BOOST_LIBSTDCXX_VERSION < 110000

#define BOOST_NO_CXX20_HDR_BIT
#endif

#if BOOST_LIBSTDCXX_VERSION >= 120000

#if __cplusplus >= 201103L
#define BOOST_NO_CXX98_FUNCTION_BASE
#endif
#endif

#ifndef __cpp_impl_coroutine
#  define BOOST_NO_CXX20_HDR_COROUTINE
#endif

#if !defined(__cpp_lib_concepts)
#if !defined(BOOST_NO_CXX20_HDR_COMPARE)
#  define BOOST_NO_CXX20_HDR_COMPARE
#endif
#if !defined(BOOST_NO_CXX20_HDR_CONCEPTS)
#  define BOOST_NO_CXX20_HDR_CONCEPTS
#endif
#if !defined(BOOST_NO_CXX20_HDR_SPAN)
#  define BOOST_NO_CXX20_HDR_SPAN
#endif
#if !defined(BOOST_NO_CXX20_HDR_RANGES)
#  define BOOST_NO_CXX20_HDR_RANGES
#endif
#endif

#if defined(__clang__)
#if (__clang_major__ < 11) && !defined(BOOST_NO_CXX20_HDR_RANGES)
#  define BOOST_NO_CXX20_HDR_RANGES
#endif
#if (__clang_major__ < 10) && (BOOST_LIBSTDCXX_VERSION >= 110000) && !defined(BOOST_NO_CXX11_HDR_CHRONO)

#  define BOOST_NO_CXX11_HDR_CHRONO
#  define BOOST_NO_CXX11_HDR_CONDITION_VARIABLE
#endif
#endif

#if defined(__clang__) && (BOOST_LIBSTDCXX_VERSION < 40300) && !defined(BOOST_NO_CXX11_NULLPTR)
#  define BOOST_NO_CXX11_NULLPTR
#endif
#if defined(__clang__) && (BOOST_LIBSTDCXX_VERSION < 40300) && defined(BOOST_HAS_INT128) && defined(__APPLE_CC__)
#undef BOOST_HAS_INT128
#endif

#if defined(__SUNPRO_CC) && (__SUNPRO_CC < 0x5140)
#define BOOST_NO_CXX11_HDR_FUTURE
#define BOOST_NO_CXX11_HDR_FORWARD_LIST
#define BOOST_NO_CXX11_HDR_ATOMIC

#define BOOST_NO_CXX11_SMART_PTR
#endif

#if (!defined(_GLIBCXX_HAS_GTHREADS) || !defined(_GLIBCXX_USE_C99_STDINT_TR1))

#  ifndef BOOST_NO_CXX11_HDR_CONDITION_VARIABLE
#     define BOOST_NO_CXX11_HDR_CONDITION_VARIABLE
#  endif
#  ifndef BOOST_NO_CXX11_HDR_MUTEX
#     define BOOST_NO_CXX11_HDR_MUTEX
#  endif
#  ifndef BOOST_NO_CXX11_HDR_THREAD
#     define BOOST_NO_CXX11_HDR_THREAD
#  endif
#  ifndef BOOST_NO_CXX14_HDR_SHARED_MUTEX
#     define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#  endif
#endif

#if (!defined(_GTHREAD_USE_MUTEX_TIMEDLOCK) || (_GTHREAD_USE_MUTEX_TIMEDLOCK == 0)) && !defined(BOOST_NO_CXX11_HDR_MUTEX) && (__GNUC__ < 6)

#  define BOOST_NO_CXX11_HDR_MUTEX
#endif
