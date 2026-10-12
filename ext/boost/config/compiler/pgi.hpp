
#define BOOST_COMPILER_VERSION __PGIC__##__PGIC_MINOR__
#define BOOST_COMPILER "PGI compiler version " BOOST_STRINGIZE(BOOST_COMPILER_VERSION)

#include <boost/config/compiler/gcc.hpp>

#undef BOOST_HAS_FLOAT128

#undef BOOST_HAS_INT128
