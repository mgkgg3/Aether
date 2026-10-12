
#    if __SUNPRO_CC <= 0x500
#      define BOOST_NO_MEMBER_TEMPLATES
#      define BOOST_NO_FUNCTION_TEMPLATE_ORDERING
#    endif

#    if (__SUNPRO_CC <= 0x520)

#      define BOOST_NO_INCLASS_MEMBER_INITIALIZATION

#      define BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
#    endif

#    if (__SUNPRO_CC <= 0x530)

#      define BOOST_NO_INCLASS_MEMBER_INITIALIZATION

#      define BOOST_NO_INTEGRAL_INT64_T
#    endif

#    if (__SUNPRO_CC < 0x570)
#      define BOOST_NO_TEMPLATE_TEMPLATES

#      define BOOST_NO_INCLASS_MEMBER_INITIALIZATION
#      define BOOST_NO_SFINAE
#      define BOOST_NO_ARRAY_TYPE_SPECIALIZATIONS
#    endif
#    if (__SUNPRO_CC <= 0x580)
#      define BOOST_NO_IS_ABSTRACT
#    endif

#    if (__SUNPRO_CC <= 0x5100)

#      define BOOST_NO_COMPLETE_VALUE_INITIALIZATION
#    endif

#if __SUNPRO_CC > 0x500
#  define BOOST_SYMBOL_EXPORT __global
#  define BOOST_SYMBOL_IMPORT __global
#  define BOOST_SYMBOL_VISIBLE __global
#endif

#if (__SUNPRO_CC >= 0x5130)
#define BOOST_DEPRECATED(msg) __attribute__((deprecated(msg)))
#endif

#if (__SUNPRO_CC < 0x5130)

#define BOOST_NO_TWO_PHASE_NAME_LOOKUP
#define BOOST_NO_SFINAE_EXPR
#define BOOST_NO_ADL_BARRIER
#define BOOST_NO_CXX11_VARIADIC_MACROS
#endif

#if (__SUNPRO_CC < 0x5130) || (__cplusplus < 201100)

#define BOOST_NO_CXX11_AUTO_DECLARATIONS
#define BOOST_NO_CXX11_AUTO_MULTIDECLARATIONS
#define BOOST_NO_CXX11_CHAR16_T
#define BOOST_NO_CXX11_CHAR32_T
#define BOOST_NO_CXX11_CONSTEXPR
#define BOOST_NO_CXX11_DECLTYPE
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
#define BOOST_NO_CXX11_STATIC_ASSERT
#define BOOST_NO_CXX11_TEMPLATE_ALIASES
#define BOOST_NO_CXX11_UNICODE_LITERALS
#define BOOST_NO_CXX11_ALIGNAS
#define BOOST_NO_CXX11_ALIGNOF
#define BOOST_NO_CXX11_TRAILING_RESULT_TYPES
#define BOOST_NO_CXX11_INLINE_NAMESPACES
#define BOOST_NO_CXX11_FINAL
#define BOOST_NO_CXX11_OVERRIDE
#define BOOST_NO_CXX11_UNRESTRICTED_UNION
#endif

#if (__SUNPRO_CC < 0x5140) || (__cplusplus < 201103)
#define BOOST_NO_CXX11_VARIADIC_TEMPLATES
#define BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX
#define BOOST_NO_CXX11_FIXED_LENGTH_VARIADIC_TEMPLATE_EXPANSION_PACKS
#define BOOST_NO_CXX11_DECLTYPE_N3276
#define BOOST_NO_CXX11_USER_DEFINED_LITERALS
#define BOOST_NO_CXX11_REF_QUALIFIERS
#define BOOST_NO_CXX11_THREAD_LOCAL
#endif

#define BOOST_NO_COMPLETE_VALUE_INITIALIZATION

#  define BOOST_HAS_LONG_LONG

#define BOOST_NO_CXX11_SFINAE_EXPR

#if !defined(__cpp_aggregate_nsdmi) || (__cpp_aggregate_nsdmi < 201304)
#  define BOOST_NO_CXX14_AGGREGATE_NSDMI
#endif
#if !defined(__cpp_binary_literals) || (__cpp_binary_literals < 201304)
#  define BOOST_NO_CXX14_BINARY_LITERALS
#endif
#if !defined(__cpp_constexpr) || (__cpp_constexpr < 201304)
#  define BOOST_NO_CXX14_CONSTEXPR
#endif
#if !defined(__cpp_decltype_auto) || (__cpp_decltype_auto < 201304) || (__cplusplus < 201402L)
#  define BOOST_NO_CXX14_DECLTYPE_AUTO
#endif
#if (__cplusplus < 201304)
#  define BOOST_NO_CXX14_DIGIT_SEPARATORS
#endif
#if !defined(__cpp_generic_lambdas) || (__cpp_generic_lambdas < 201304)
#  define BOOST_NO_CXX14_GENERIC_LAMBDAS
#endif
#if !defined(__cpp_init_captures) || (__cpp_init_captures < 201304)
#  define BOOST_NO_CXX14_INITIALIZED_LAMBDA_CAPTURES
#endif
#if !defined(__cpp_return_type_deduction) || (__cpp_return_type_deduction < 201304)
#  define BOOST_NO_CXX14_RETURN_TYPE_DEDUCTION
#endif
#if !defined(__cpp_variable_templates) || (__cpp_variable_templates < 201304)
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
#if !defined(__cpp_nontype_template_parameter_auto) || (__cpp_nontype_template_parameter_auto < 201606)
#  define BOOST_NO_CXX17_AUTO_NONTYPE_TEMPLATE_PARAMS
#endif

#if (__SUNPRO_CC >= 0x5140) && defined(__SunOS_5_12) && !defined(BOOST_HAS_THREADS)
# define BOOST_HAS_THREADS
#endif

#define BOOST_COMPILER "Sun compiler version " BOOST_STRINGIZE(__SUNPRO_CC)

#if __SUNPRO_CC < 0x400
#error "Compiler not supported or configured - please reconfigure"
#endif

#if (__SUNPRO_CC > 0x5150)
#  if defined(BOOST_ASSERT_CONFIG)
#     error "Boost.Config is older than your compiler - please check for an updated Boost release."
#  endif
#endif
