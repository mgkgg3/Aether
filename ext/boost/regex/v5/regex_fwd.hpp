
#ifndef BOOST_REGEX_FWD_HPP_INCLUDED
#define BOOST_REGEX_FWD_HPP_INCLUDED

#ifndef BOOST_REGEX_CONFIG_HPP
#include <boost/regex/config.hpp>
#endif

#ifdef BOOST_REGEX_NO_FWD
#  ifndef BOOST_RE_REGEX_HPP
#     include <boost/regex.hpp>
#  endif
#else

namespace boost{

template <class charT>
class cpp_regex_traits;
template <class charT>
struct c_regex_traits;
template <class charT>
class w32_regex_traits;

#ifdef BOOST_REGEX_USE_WIN32_LOCALE
template <class charT, class implementationT = w32_regex_traits<charT> >
struct regex_traits;
#elif defined(BOOST_REGEX_USE_CPP_LOCALE)
template <class charT, class implementationT = cpp_regex_traits<charT> >
struct regex_traits;
#else
template <class charT, class implementationT = c_regex_traits<charT> >
struct regex_traits;
#endif

template <class charT, class traits = regex_traits<charT> >
class basic_regex;

typedef basic_regex<char, regex_traits<char> > regex;
#ifndef BOOST_NO_WREGEX
typedef basic_regex<wchar_t, regex_traits<wchar_t> > wregex;
#endif

}

#endif

#endif
