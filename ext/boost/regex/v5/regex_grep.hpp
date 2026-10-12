
#ifndef BOOST_REGEX_V5_REGEX_GREP_HPP
#define BOOST_REGEX_V5_REGEX_GREP_HPP

namespace boost{

template <class Predicate, class BidiIterator, class charT, class traits>
inline unsigned int regex_grep(Predicate foo,
                               BidiIterator first,
                               BidiIterator last,
                               const basic_regex<charT, traits>& e,
                               match_flag_type flags = match_default)
{
   if(e.flags() & regex_constants::failbit)
      return false;

   typedef typename match_results<BidiIterator>::allocator_type match_allocator_type;

   match_results<BidiIterator> m;
   BOOST_REGEX_DETAIL_NS::perl_matcher<BidiIterator, match_allocator_type, traits> matcher(first, last, m, e, flags, first);
   unsigned int count = 0;
   while(matcher.find())
   {
      ++count;
      if(0 == foo(m))
         return count;
      if(m[0].second == last)
         return count;
      if(m.length() == 0)
      {
         if(m[0].second == last)
            return count;

         match_results<BidiIterator, match_allocator_type> m2(m);
         matcher.setf(match_not_null | match_continuous);
         if(matcher.find())
         {
            ++count;
            if(0 == foo(m))
               return count;
         }
         else
         {

            m = m2;
         }
         matcher.unsetf((match_not_null | match_continuous) & ~flags);
      }
   }
   return count;
}

template <class Predicate, class charT, class traits>
inline unsigned int regex_grep(Predicate foo, const charT* str,
                        const basic_regex<charT, traits>& e,
                        match_flag_type flags = match_default)
{
   return regex_grep(foo, str, str + traits::length(str), e, flags);
}

template <class Predicate, class ST, class SA, class charT, class traits>
inline unsigned int regex_grep(Predicate foo, const std::basic_string<charT, ST, SA>& s,
                 const basic_regex<charT, traits>& e,
                 match_flag_type flags = match_default)
{
   return regex_grep(foo, s.begin(), s.end(), e, flags);
}

}

#endif
