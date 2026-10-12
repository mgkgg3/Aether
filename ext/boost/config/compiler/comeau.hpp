
#include <boost/config/compiler/common_edg.hpp>

#if (__COMO_VERSION__ <= 4245)

#  if defined(_MSC_VER) && _MSC_VER <= 1300
#     if _MSC_VER > 100

#        define BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
#     endif
#  endif

#  if defined(_MSC_VER) && (_MSC_VER < 1300)
#     define BOOST_NO_VOID_RETURNS
#  endif

#endif

#  if defined(_MSC_VER) && (_MSC_VER >= 1200)
#     define BOOST_HAS_MS_INT64
#  endif

#define BOOST_COMPILER "Comeau compiler version " BOOST_STRINGIZE(__COMO_VERSION__)

#if __COMO_VERSION__ < 4245
#  error "Compiler not configured - please reconfigure"
#endif

#if (__COMO_VERSION__ > 4245)
#  if defined(BOOST_ASSERT_CONFIG)
#     error "boost: Unknown compiler version - please run the configure tests and report the results"
#  endif
#endif
