
#ifndef BOOST_EXCEPTION_837060E885AF11E68DA91D15E31AC075
#define BOOST_EXCEPTION_837060E885AF11E68DA91D15E31AC075

#ifdef BOOST_EXCEPTION_MINI_BOOST
#include  <memory>
namespace boost { namespace exception_detail { using std::shared_ptr; } }
#else
#include <boost/shared_ptr.hpp>
namespace boost { namespace exception_detail { using boost::shared_ptr; } }
#endif

#endif
