
#define BOOST_COMPILER "Greenhills C++ version " BOOST_STRINGIZE(__ghs)

#include <boost/config/compiler/common_edg.hpp>

#if __ghs < 0
#  error "Compiler not supported or configured - please reconfigure"
#endif

#if (__ghs > 0)
#  if defined(BOOST_ASSERT_CONFIG)
#     error "boost: Unknown compiler version - please run the configure tests and report the results"
#  endif
#endif
