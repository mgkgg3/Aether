
#ifndef BOOST_REGEX_V5_CHAR_REGEX_TRAITS_HPP
#define BOOST_REGEX_V5_CHAR_REGEX_TRAITS_HPP

namespace boost{

namespace deprecated{

template <class charT>
class char_regex_traits_i : public regex_traits<charT> {};

template<>
class char_regex_traits_i<char> : public regex_traits<char>
{
public:
   typedef char char_type;
   typedef unsigned char uchar_type;
   typedef unsigned int size_type;
   typedef regex_traits<char> base_type;

};

#ifndef BOOST_NO_WREGEX
template<>
class char_regex_traits_i<wchar_t> : public regex_traits<wchar_t>
{
public:
   typedef wchar_t char_type;
   typedef unsigned short uchar_type;
   typedef unsigned int size_type;
   typedef regex_traits<wchar_t> base_type;

};
#endif
}
}

#endif
