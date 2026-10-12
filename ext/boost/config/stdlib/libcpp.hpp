
#if !defined(_LIBCPP_VERSION)
#  include <ciso646>
#  if !defined(_LIBCPP_VERSION)
#      error "This is not libc++!"
#  endif
#endif

#define BOOST_STDLIB "libc++ version " BOOST_STRINGIZE(_LIBCPP_VERSION)

#define BOOST_HAS_THREADS

#ifdef _LIBCPP_HAS_NO_VARIADICS
#    define BOOST_NO_CXX11_HDR_TUPLE
#endif

#if defined(_LIBCPP_HAS_NO_TEMPLATE_ALIASES)
#    define BOOST_NO_CXX11_ALLOCATOR
#    define BOOST_NO_CXX11_POINTER_TRAITS
#endif

#if __cplusplus < 201103

#  define BOOST_NO_CXX11_HDR_CODECVT
#  define BOOST_NO_CXX11_HDR_CONDITION_VARIABLE
#  define BOOST_NO_CXX11_HDR_EXCEPTION
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
#  define BOOST_NO_CXX11_SMART_PTR
#  define BOOST_NO_CXX11_HDR_FUNCTIONAL
#  define BOOST_NO_CXX11_STD_ALIGN
#  define BOOST_NO_CXX11_ADDRESSOF
#  define BOOST_NO_CXX11_HDR_ATOMIC
#  define BOOST_NO_CXX11_ATOMIC_SMART_PTR
#  define BOOST_NO_CXX11_HDR_CHRONO
#  define BOOST_NO_CXX11_HDR_TYPE_TRAITS
#  define BOOST_NO_CXX11_HDR_FUTURE
#elif _LIBCPP_VERSION < 3700

#  define BOOST_NO_CXX11_HDR_ATOMIC
#  define BOOST_NO_CXX11_ATOMIC_SMART_PTR
#  define BOOST_NO_CXX11_HDR_CHRONO
#  define BOOST_NO_CXX11_HDR_TYPE_TRAITS
#  define BOOST_NO_CXX11_HDR_FUTURE
#endif

#if _LIBCPP_VERSION < 3700

#define BOOST_NO_STD_MESSAGES
#endif

#if (_LIBCPP_VERSION < 3700) || (__cplusplus <= 201402L)
#  define BOOST_NO_CXX14_STD_EXCHANGE
#endif

#if (_LIBCPP_VERSION < 4000) || (__cplusplus <= 201402L)
#  define BOOST_NO_CXX17_STD_APPLY
#  define BOOST_NO_CXX17_HDR_OPTIONAL
#  define BOOST_NO_CXX17_HDR_STRING_VIEW
#  define BOOST_NO_CXX17_HDR_VARIANT
#endif
#if (_LIBCPP_VERSION > 4000) && (__cplusplus > 201402L) && !defined(_LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR)
#  define BOOST_NO_AUTO_PTR
#endif
#if (_LIBCPP_VERSION > 4000) && (__cplusplus > 201402L) && !defined(_LIBCPP_ENABLE_CXX17_REMOVED_RANDOM_SHUFFLE)
#  define BOOST_NO_CXX98_RANDOM_SHUFFLE
#endif
#if (_LIBCPP_VERSION > 4000) && (__cplusplus > 201402L) && !defined(_LIBCPP_ENABLE_CXX17_REMOVED_BINDERS)
#  define BOOST_NO_CXX98_BINDERS
#endif

#if defined(__cplusplus) && defined(__has_include)
#if __has_include(<version>)
#include <version>

#if !defined(__cpp_lib_execution) || (__cpp_lib_execution < 201603L)
#  define BOOST_NO_CXX17_HDR_EXECUTION
#endif
#if !defined(__cpp_lib_invoke) || (__cpp_lib_invoke < 201411L)
#define BOOST_NO_CXX17_STD_INVOKE
#endif

#if(_LIBCPP_VERSION < 9000)

#  define BOOST_NO_CXX20_HDR_SPAN
#endif

#else
#define BOOST_NO_CXX17_STD_INVOKE
#define BOOST_NO_CXX17_HDR_EXECUTION
#endif
#else
#define BOOST_NO_CXX17_STD_INVOKE
#define BOOST_NO_CXX17_HDR_EXECUTION
#endif

#if _LIBCPP_VERSION < 10000
#define BOOST_NO_CXX17_ITERATOR_TRAITS
#endif

#if (_LIBCPP_VERSION <= 1101) && !defined(BOOST_NO_CXX11_THREAD_LOCAL)

#  define BOOST_NO_CXX11_THREAD_LOCAL
#endif

#if defined(__linux__) && (_LIBCPP_VERSION < 6000) && !defined(BOOST_NO_CXX11_THREAD_LOCAL)

#  define BOOST_NO_CXX11_THREAD_LOCAL
#endif

#if defined(__has_include)
#if !__has_include(<shared_mutex>)
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#elif __cplusplus <= 201103
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#endif
#elif __cplusplus < 201402
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#endif

#if !defined(BOOST_NO_CXX14_HDR_SHARED_MUTEX) && (_LIBCPP_VERSION < 5000)
#  define BOOST_NO_CXX14_HDR_SHARED_MUTEX
#endif

#if _LIBCPP_VERSION >= 15000

#if __cplusplus >= 201103L
#define BOOST_NO_CXX98_FUNCTION_BASE
#endif
#endif
