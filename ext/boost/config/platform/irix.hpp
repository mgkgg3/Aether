
#define BOOST_PLATFORM "SGI Irix"

#define BOOST_NO_SWPRINTF

#define BOOST_HAS_GETTIMEOFDAY
#define BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE

#ifdef __GNUC__

#  define BOOST_DISABLE_THREADS
#endif

#define BOOST_HAS_UNISTD_H
#include <boost/config/detail/posix_features.hpp>
