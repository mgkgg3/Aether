#ifndef BOOST_EXCEPTION_DETAIL_REQUIRES_CXX11_HPP_INCLUDED
#define BOOST_EXCEPTION_DETAIL_REQUIRES_CXX11_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/config/pragma_message.hpp>

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || \
    defined(BOOST_NO_CXX11_RVALUE_REFERENCES) || \
    defined(BOOST_NO_CXX11_DECLTYPE) || \
    defined(BOOST_NO_CXX11_CONSTEXPR) || \
    defined(BOOST_NO_CXX11_NOEXCEPT) || \
    defined(BOOST_NO_CXX11_NULLPTR) || \
    defined(BOOST_NO_CXX11_SMART_PTR)

BOOST_PRAGMA_MESSAGE("C++03 support was deprecated in Boost.Exception 1.85 and will be removed in Boost.Exception 1.87.")

#endif

#endif