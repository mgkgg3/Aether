
#ifndef BOOST_REGEX_MATCH_HPP
#define BOOST_REGEX_MATCH_HPP

namespace boost{

template <class BidiIterator, class Allocator, class charT, class traits>
bool regex_match(BidiIterator first, BidiIterator last,
                 match_results<BidiIterator, Allocator>& m,
                 const basic_regex<charT, traits>& e,
                 match_flag_type flags = match_default)
{
   BOOST_REGEX_DETAIL_NS::perl_matcher<BidiIterator, Allocator, traits> matcher(first, last, m, e, flags, first);
   return matcher.match();
}
template <class iterator, class charT, class traits>
bool regex_match(iterator first, iterator last,
                 const basic_regex<charT, traits>& e,
                 match_flag_type flags = match_default)
{
   match_results<iterator> m;
   return regex_match(first, last, m, e, flags | regex_constants::match_any);
}

template <class charT, class Allocator, class traits>
inline bool regex_match(const charT* str,
                        match_results<const charT*, Allocator>& m,
                        const basic_regex<charT, traits>& e,
                        match_flag_type flags = match_default)
{
   return regex_match(str, str + traits::length(str), m, e, flags);
}

template <class ST, class SA, class Allocator, class charT, class traits>
inline bool regex_match(const std::basic_string<charT, ST, SA>& s,
                 match_results<typename std::basic_string<charT, ST, SA>::const_iterator, Allocator>& m,
                 const basic_regex<charT, traits>& e,
                 match_flag_type flags = match_default)
{
   return regex_match(s.begin(), s.end(), m, e, flags);
}
template <class charT, class traits>
inline bool regex_match(const charT* str,
                        const basic_regex<charT, traits>& e,
                        match_flag_type flags = match_default)
{
   match_results<const charT*> m;
   return regex_match(str, str + traits::length(str), m, e, flags | regex_constants::match_any);
}

template <class ST, class SA, class charT, class traits>
inline bool regex_match(const std::basic_string<charT, ST, SA>& s,
                 const basic_regex<charT, traits>& e,
                 match_flag_type flags = match_default)
{
   typedef typename std::basic_string<charT, ST, SA>::const_iterator iterator;
   match_results<iterator> m;
   return regex_match(s.begin(), s.end(), m, e, flags | regex_constants::match_any);
}

}

#endif
