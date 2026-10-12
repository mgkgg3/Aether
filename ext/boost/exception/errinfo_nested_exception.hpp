
#ifndef BOOST_EXCEPTION_45CC9A82B77511DEB330FC4956D89593
#define BOOST_EXCEPTION_45CC9A82B77511DEB330FC4956D89593

namespace
boost
    {
    namespace exception_detail { class clone_base; }
    template <class Tag,class T> class error_info;
    class exception_ptr;
    typedef error_info<struct errinfo_nested_exception_,exception_ptr> errinfo_nested_exception;
    }

#endif
