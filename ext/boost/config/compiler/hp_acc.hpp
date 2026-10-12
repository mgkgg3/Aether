
#if defined(__EDG__)
#include <boost/config/compiler/common_edg.hpp>
#endif

#if (__HP_aCC <= 33100)
#    define BOOST_NO_INTEGRAL_INT64_T
#    define BOOST_NO_OPERATORS_IN_NAMESPACE
#  if !defined(_NAMESPACE_STD)
#     define BOOST_NO_STD_LOCALE
#     define BOOST_NO_STRINGSTREAM
#  endif
#endif

#if (__HP_aCC <= 33300)

#    define BOOST_NO_MEMBER_TEMPLATES
#    define BOOST_NO_DEPENDENT_NESTED_DERIVATIONS
#    define BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE
#endif

#if (__HP_aCC <= 38000)
#  define BOOST_NO_TWO_PHASE_NAME_LOOKUP
#endif

#if (__HP_aCC > 50000) && (__HP_aCC < 60000)
#    define BOOST_NO_UNREACHABLE_RETURN_DETECTION
#    define BOOST_NO_TEMPLATE_TEMPLATES
#    define BOOST_NO_SWPRINTF
#    define BOOST_NO_DEPENDENT_TYPES_IN_TEMPLATE_VALUE_PARAMETERS
#    define BOOST_NO_IS_ABSTRACT
#    define BOOST_NO_MEMBER_TEMPLATE_FRIENDS
#endif

#if (__HP_aCC >= 33900)
#    define BOOST_HAS_LONG_LONG
#    define BOOST_HAS_PARTIAL_STD_ALLOCATOR
#endif

#if (__HP_aCC >= 50000 ) && (__HP_aCC <= 53800 ) || (__HP_aCC < 31300 )
#    define BOOST_NO_MEMBER_TEMPLATE_KEYWORD
#endif

#if (__HP_aCC >= 60000) || ((__HP_aCC > 38000) && defined(__hpxstd98))
#    define BOOST_NO_TWO_PHASE_NAME_LOOKUP
#endif

#define BOOST_COMPILER "HP aCC version " BOOST_STRINGIZE(__HP_aCC)

#if __HP_aCC < 33000
#  error "Compiler not supported or configured - please reconfigure"
#endif

#if __HP_aCC > 30000 && __HP_aCC < 50000
#  if __HP_aCC < 38000

#     error "Compiler version not supported - version A.03.80 or higher is required"
#  elif !defined(__hpxstd98)

#     error "Compiler option '+hpxstd98' is required for proper support"
#  endif
#endif

#if !defined(__EDG__)

#define BOOST_NO_CXX11_AUTO_DECLARATIONS
#define BOOST_NO_CXX11_AUTO_MULTIDECLARATIONS
#define BOOST_NO_CXX11_CHAR16_T
#define BOOST_NO_CXX11_CHAR32_T
#define BOOST_NO_CXX11_CONSTEXPR
#define BOOST_NO_CXX11_DECLTYPE
#define BOOST_NO_CXX11_DECLTYPE_N3276
#define BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
#define BOOST_NO_CXX11_DELETED_FUNCTIONS
#define BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
#define BOOST_NO_CXX11_EXTERN_TEMPLATE
#define BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS
#define BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#define BOOST_NO_CXX11_LAMBDAS
#define BOOST_NO_CXX11_LOCAL_CLASS_TEMPLATE_PARAMETERS
#define BOOST_NO_CXX11_NOEXCEPT
#define BOOST_NO_CXX11_NULLPTR
#define BOOST_NO_CXX11_RANGE_BASED_FOR
#define BOOST_NO_CXX11_RAW_LITERALS
#define BOOST_NO_CXX11_RVALUE_REFERENCES
#define BOOST_NO_CXX11_SCOPED_ENUMS
#define BOOST_NO_SFINAE_EXPR
#define BOOST_NO_CXX11_SFINAE_EXPR
#define BOOST_NO_CXX11_STATIC_ASSERT
#define BOOST_NO_CXX11_TEMPLATE_ALIASES
#define BOOST_NO_CXX11_UNICODE_LITERALS
#define BOOST_NO_CXX11_VARIADIC_TEMPLATES
#define BOOST_NO_CXX11_USER_DEFINED_LITERALS
#define BOOST_NO_CXX11_ALIGNAS
#define BOOST_NO_CXX11_ALIGNOF
#define BOOST_NO_CXX11_TRAILING_RESULT_TYPES
#define BOOST_NO_CXX11_INLINE_NAMESPACES
#define BOOST_NO_CXX11_REF_QUALIFIERS
#define BOOST_NO_CXX11_THREAD_LOCAL
#define BOOST_NO_CXX11_UNRESTRICTED_UNION

#if (__HP_aCC < 62500) || !defined(HP_CXX0x_SOURCE)
  #define BOOST_NO_CXX11_VARIADIC_MACROS
#endif

#if !defined(__cpp_nontype_template_parameter_auto) || (__cpp_nontype_template_parameter_auto < 201606)
#  define BOOST_NO_CXX17_AUTO_NONTYPE_TEMPLATE_PARAMS
#endif

#endif

#if ((__HP_aCC > 61300) || ((__HP_aCC > 38000) && defined(__hpxstd98)))
#  if defined(BOOST_ASSERT_CONFIG)
#     error "boost: Unknown compiler version - please run the configure tests and report the results"
#  endif
#endif
