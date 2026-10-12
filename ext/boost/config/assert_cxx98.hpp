
#include <boost/config.hpp>
#include <boost/config/assert_cxx17.hpp>

#ifdef BOOST_NO_CXX98_BINDERS
#  error "Your compiler appears not to be fully C++98 compliant.  Detected via defect macro BOOST_NO_CXX98_BINDERS."
#endif
#ifdef BOOST_NO_CXX98_FUNCTION_BASE
#  error "Your compiler appears not to be fully C++98 compliant.  Detected via defect macro BOOST_NO_CXX98_FUNCTION_BASE."
#endif
#ifdef BOOST_NO_CXX98_RANDOM_SHUFFLE
#  error "Your compiler appears not to be fully C++98 compliant.  Detected via defect macro BOOST_NO_CXX98_RANDOM_SHUFFLE."
#endif
