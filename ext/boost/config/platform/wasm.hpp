
#define BOOST_PLATFORM "Wasm"

#ifdef __has_include
#if __has_include(<unistd.h>)
#  define BOOST_HAS_UNISTD_H
#endif
#endif

#include <boost/config/detail/posix_features.hpp>

#define BOOST_NO_FENV_H
