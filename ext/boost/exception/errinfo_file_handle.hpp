
#ifndef BOOST_EXCEPTION_F79E6EE26C1211DEB26E929155D89593
#define BOOST_EXCEPTION_F79E6EE26C1211DEB26E929155D89593

#include <stdio.h>

namespace
boost
    {
    template <class> class weak_ptr;
    template <class Tag,class T> class error_info;

    typedef error_info<struct errinfo_file_handle_,weak_ptr<FILE> > errinfo_file_handle;
    }

#endif
