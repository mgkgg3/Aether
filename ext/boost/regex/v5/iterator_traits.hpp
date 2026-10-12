
#ifndef BOOST_REGEX_V5_ITERATOR_TRAITS_HPP
#define BOOST_REGEX_V5_ITERATOR_TRAITS_HPP

namespace boost{
namespace BOOST_REGEX_DETAIL_NS{

template <class T>
struct regex_iterator_traits : public std::iterator_traits<T> {};

}
}

#endif
