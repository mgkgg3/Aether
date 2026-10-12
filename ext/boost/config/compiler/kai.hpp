
#include <boost/config/compiler/common_edg.hpp>

#   if (__KCC_VERSION <= 4001) || !defined(BOOST_STRICT_CONFIG)

#     define BOOST_NO_STDC_NAMESPACE
#   endif

# if !defined(_EXCEPTIONS) && !defined(BOOST_NO_EXCEPTIONS)
#     define BOOST_NO_EXCEPTIONS
# endif

#if (__KCC_VERSION > 4001)
#  if defined(BOOST_ASSERT_CONFIG)
#     error "boost: Unknown compiler version - please run the configure tests and report the results"
#  endif
#endif
