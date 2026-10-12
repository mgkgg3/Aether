
#define BOOST_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if !defined(__CUDACC__)
#define BOOST_GCC BOOST_GCC_VERSION
#endif

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || (__cplusplus >= 201103L)
#  define BOOST_GCC_CXX11
#endif

#if __GNUC__ == 3
#  if defined (__PATHSCALE__)
#     define BOOST_NO_TWO_PHASE_NAME_LOOKUP
#     define BOOST_NO_IS_ABSTRACT
#  endif

#  if __GNUC_MINOR__ < 4
#     define BOOST_NO_IS_ABSTRACT
#  endif
#  define BOOST_NO_CXX11_EXTERN_TEMPLATE
#endif
#if __GNUC__ < 4

#define BOOST_NO_TWO_PHASE_NAME_LOOKUP
#  ifdef __OPEN64__
#     define BOOST_NO_IS_ABSTRACT
#  endif
#endif

#if BOOST_GCC_VERSION >= 30400
#define BOOST_HAS_PRAGMA_ONCE
#endif

#if BOOST_GCC_VERSION < 40400

#define BOOST_NO_COMPLETE_VALUE_INITIALIZATION
#endif

#if !defined(__EXCEPTIONS) && !defined(BOOST_NO_EXCEPTIONS)
# define BOOST_NO_EXCEPTIONS
#endif

#if !defined(__MINGW32__) && !defined(linux) && !defined(__linux) && !defined(__linux__)
# define BOOST_HAS_THREADS
#endif

#if !defined(__DARWIN_NO_LONG_LONG)
# define BOOST_HAS_LONG_LONG
#endif

#define BOOST_HAS_NRVO

#define BOOST_LIKELY(x) __builtin_expect(x, 1)
#define BOOST_UNLIKELY(x) __builtin_expect(x, 0)

#if __GNUC__ >= 4
#  if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__)

#    define BOOST_HAS_DECLSPEC
#    define BOOST_SYMBOL_EXPORT __attribute__((__dllexport__))
#    define BOOST_SYMBOL_IMPORT __attribute__((__dllimport__))
#  else
#    define BOOST_SYMBOL_EXPORT __attribute__((__visibility__("default")))
#    define BOOST_SYMBOL_IMPORT
#  endif
#  define BOOST_SYMBOL_VISIBLE __attribute__((__visibility__("default")))
#else

#  define BOOST_SYMBOL_EXPORT
#endif

#if BOOST_GCC_VERSION > 40300
#  ifndef __GXX_RTTI
#     ifndef BOOST_NO_TYPEID
#        define BOOST_NO_TYPEID
#     endif
#     ifndef BOOST_NO_RTTI
#        define BOOST_NO_RTTI
#     endif
#  endif
#endif

#if defined(__CUDACC__)
#  if defined(BOOST_GCC_CXX11)
#    define BOOST_NVCC_CXX11
#  else
#    define BOOST_NVCC_CXX03
#  endif
#endif

#if defined(__SIZEOF_INT128__) && !defined(BOOST_NVCC_CXX03)
#  define BOOST_HAS_INT128
#endif

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif
#if defined(_GLIBCXX_USE_FLOAT128) && !defined(__STRICT_ANSI__) && !defined(BOOST_NVCC_CXX03)
# define BOOST_HAS_FLOAT128
#endif

#if (BOOST_GCC_VERSION >= 40300) && defined(BOOST_GCC_CXX11)

#  define BOOST_HAS_DECLTYPE
#  define BOOST_HAS_RVALUE_REFS
#  define BOOST_HAS_STATIC_ASSERT
#  define BOOST_HAS_VARIADIC_TMPL
#else
#  define BOOST_NO_CXX11_DECLTYPE
#  define BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS
#  define BOOST_NO_CXX11_RVALUE_REFERENCES
#  define BOOST_NO_CXX11_STATIC_ASSERT
#endif

#if (BOOST_GCC_VERSION < 40400) || !defined(BOOST_GCC_CXX11)
#  define BOOST_NO_CXX11_AUTO_DECLARATIONS
#  define BOOST_NO_CXX11_AUTO_MULTIDECLARATIONS
#  define BOOST_NO_CXX11_CHAR16_T
#  define BOOST_NO_CXX11_CHAR32_T
#  define BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#  define BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
#  define BOOST_NO_CXX11_DELETED_FUNCTIONS
#  define BOOST_NO_CXX11_TRAILING_RESULT_TYPES
#  define BOOST_NO_CXX11_INLINE_NAMESPACES
#  define BOOST_NO_CXX11_VARIADIC_TEMPLATES
#endif

#if BOOST_GCC_VERSION < 40500
#  define BOOST_NO_SFINAE_EXPR
#endif

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ == 5) || !defined(BOOST_GCC_CXX11)
#  define BOOST_NO_CXX11_NON_PUBLIC_DEFAULTED_FUNCTIONS
#endif

#if (BOOST_GCC_VERSION < 40500) || !defined(BOOST_GCC_CXX11)
#  define BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
#  define BOOST_NO_CXX11_LAMBDAS
#  define BOOST_NO_CXX11_LOCAL_CLASS_TEMPLATE_PARAMETERS
#  define BOOST_NO_CXX11_RAW_LITERALS
#  define BOOST_NO_CXX11_UNICODE_LITERALS
#  define BOOST_NO_CXX11_ALIGNOF
#endif

#if (BOOST_GCC_VERSION < 40501) || !defined(BOOST_GCC_CXX11)

#  define BOOST_NO_CXX11_SCOPED_ENUMS
#endif

#if (BOOST_GCC_VERSION < 40600) || !defined(BOOST_GCC_CXX11)
#define BOOST_NO_CXX11_DEFAULTED_MOVES
#define BOOST_NO_CXX11_NOEXCEPT
#define BOOST_NO_CXX11_NULLPTR
#define BOOST_NO_CXX11_RANGE_BASED_FOR
#define BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX
#endif

#if (BOOST_GCC_VERSION < 40700) || !defined(BOOST_GCC_CXX11)

#  define BOOST_NO_CXX11_CONSTEXPR
#  define BOOST_NO_CXX11_FINAL
#  define BOOST_NO_CXX11_TEMPLATE_ALIASES
#  define BOOST_NO_CXX11_USER_DEFINED_LITERALS
#  define BOOST_NO_CXX11_FIXED_LENGTH_VARIADIC_TEMPLATE_EXPANSION_PACKS
#  define BOOST_NO_CXX11_OVERRIDE
#endif

#if (BOOST_GCC_VERSION < 40800) || !defined(BOOST_GCC_CXX11)
#  define BOOST_NO_CXX11_THREAD_LOCAL
#  define BOOST_NO_CXX11_SFINAE_EXPR
#endif

#if (BOOST_GCC_VERSION < 40801) || !defined(BOOST_GCC_CXX11)
#  define BOOST_NO_CXX11_DECLTYPE_N3276
#  define BOOST_NO_CXX11_REF_QUALIFIERS
#  define BOOST_NO_CXX14_BINARY_LITERALS
#endif

#if (BOOST_GCC_VERSION < 40900) || !defined(BOOST_GCC_CXX11)

#  define BOOST_NO_CXX11_ALIGNAS
#endif

#if (BOOST_GCC_VERSION < 50100) || !defined(BOOST_GCC_CXX11)
#  define BOOST_NO_CXX11_UNRESTRICTED_UNION
#endif

#if (BOOST_GCC_VERSION < 40900) || (__cplusplus < 201300)
#  define BOOST_NO_CXX14_RETURN_TYPE_DEDUCTION
#  define BOOST_NO_CXX14_GENERIC_LAMBDAS
#  define BOOST_NO_CXX14_DIGIT_SEPARATORS
#  define BOOST_NO_CXX14_DECLTYPE_AUTO
#  if !((BOOST_GCC_VERSION >= 40801) && (BOOST_GCC_VERSION < 40900) && defined(BOOST_GCC_CXX11))
#     define BOOST_NO_CXX14_INITIALIZED_LAMBDA_CAPTURES
#  endif
#endif

#if !defined(__cpp_aggregate_nsdmi) || (__cpp_aggregate_nsdmi < 201304)
#  define BOOST_NO_CXX14_AGGREGATE_NSDMI
#endif
#if !defined(__cpp_constexpr) || (__cpp_constexpr < 201304)
#  define BOOST_NO_CXX14_CONSTEXPR
#endif
#if (BOOST_GCC_VERSION < 50200) || !defined(__cpp_variable_templates) || (__cpp_variable_templates < 201304)
#  define BOOST_NO_CXX14_VARIABLE_TEMPLATES
#endif

#if !defined(__cpp_structured_bindings) || (__cpp_structured_bindings < 201606)
#  define BOOST_NO_CXX17_STRUCTURED_BINDINGS
#endif
#if !defined(__cpp_inline_variables) || (__cpp_inline_variables < 201606)
#  define BOOST_NO_CXX17_INLINE_VARIABLES
#endif
#if !defined(__cpp_fold_expressions) || (__cpp_fold_expressions < 201603)
#  define BOOST_NO_CXX17_FOLD_EXPRESSIONS
#endif
#if !defined(__cpp_if_constexpr) || (__cpp_if_constexpr < 201606)
#  define BOOST_NO_CXX17_IF_CONSTEXPR
#endif
#if (__GNUC__ < 7) || (__cplusplus < 201703L)
#  define BOOST_NO_CXX17_AUTO_NONTYPE_TEMPLATE_PARAMS
#endif

#if __GNUC__ >= 7
#  define BOOST_FALLTHROUGH __attribute__((fallthrough))
#endif

#if (__GNUC__ < 11) && defined(__MINGW32__) && !defined(__MINGW64__)

#define BOOST_NO_CXX11_THREAD_LOCAL
#endif

#if __GNUC__ >= 4
#  define BOOST_ATTRIBUTE_UNUSED __attribute__((__unused__))
#endif

#define BOOST_MAY_ALIAS __attribute__((__may_alias__))

#if BOOST_GCC_VERSION >= 40500
#define BOOST_UNREACHABLE_RETURN(x) __builtin_unreachable();
#endif

#if BOOST_GCC_VERSION >= 40500
#define BOOST_DEPRECATED(msg) __attribute__((deprecated(msg)))
#else
#define BOOST_DEPRECATED(msg) __attribute__((deprecated))
#endif

#ifndef BOOST_COMPILER
#  define BOOST_COMPILER "GNU C++ version " __VERSION__
#endif

#ifdef __GXX_CONCEPTS__
#  define BOOST_HAS_CONCEPTS
#  define BOOST_COMPILER "ConceptGCC version " __VERSION__
#endif

#if (BOOST_GCC_VERSION < 30300)
#  error "Compiler not configured - please reconfigure"
#endif

#if (BOOST_GCC_VERSION > 80100)
#  if defined(BOOST_ASSERT_CONFIG)
#     error "Boost.Config is older than your compiler - please check for an updated Boost release."
#  else

#  endif
#endif
