
#if !defined(__STL_CONFIG_H)
#  include <boost/config/no_tr1/utility.hpp>
#  if !defined(__STL_CONFIG_H)
#      error "This is not the SGI STL!"
#  endif
#endif

#if !defined(__STL_CLASS_PARTIAL_SPECIALIZATION)
#  define BOOST_NO_STD_ITERATOR_TRAITS
#endif

#if defined(__GNUC__) && (__GNUC__ < 3) && \
     ((__GNUC_MINOR__ < 95) || (__GNUC_MINOR__ == 96)) && \
     !defined(__STL_USE_NEW_IOSTREAMS) || \
   defined(__APPLE_CC__)

#  define BOOST_NO_STRINGSTREAM
#endif

#if !defined(CYGWIN) && (  defined(__unix__)  \
                        || defined(__unix)    \
                        || defined(unix)      \
                        || defined(__APPLE__) \
                        || defined(__APPLE)   \
                        || defined(APPLE))
#  include <unistd.h>
#endif

#if !defined(__SGI_STL_OWN_IOSTREAMS) && !defined(__STL_USE_NEW_IOSTREAMS)
#  define BOOST_NO_STD_LOCALE
#endif

#if defined(__sgi) && (_COMPILER_VERSION <= 650) && !defined(__SGI_STL_OWN_IOSTREAMS)
#  define BOOST_NO_STD_LOCALE
#endif

#if defined(__sgi) && (_COMPILER_VERSION <= 740) && defined(__STL_USE_NEW_IOSTREAMS)
#  define BOOST_NO_STD_MESSAGES
#endif

#if !defined(__STL_MEMBER_TEMPLATES)
#  define BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
#  define BOOST_NO_STD_ALLOCATOR
#endif

#define BOOST_HAS_HASH
#define BOOST_HAS_SLIST

#if (defined(__GNUC__) && (__GNUC__ < 3))
#  include <string>
#  if defined(__BASTRING__)
#     define BOOST_NO_LIMITS

#     undef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
#     define BOOST_NO_STD_WSTRING
#  endif
#endif

#if !defined(__STL_USE_NAMESPACES)
#  define BOOST_NO_STD_ITERATOR
#endif

#define BOOST_HAS_SGI_TYPE_TRAITS

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

#define BOOST_STDLIB "SGI standard library"
