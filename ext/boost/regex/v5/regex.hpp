
#ifndef BOOST_RE_REGEX_HPP_INCLUDED
#define BOOST_RE_REGEX_HPP_INCLUDED

#ifdef __cplusplus

#include <boost/regex/config.hpp>
#include <boost/regex/v5/regex_workaround.hpp>
#include <boost/regex_fwd.hpp>
#include <boost/regex/regex_traits.hpp>
#include <boost/regex/v5/error_type.hpp>
#include <boost/regex/v5/match_flags.hpp>
#include <boost/regex/v5/regex_raw_buffer.hpp>
#include <boost/regex/pattern_except.hpp>
#include <boost/regex/v5/char_regex_traits.hpp>
#include <boost/regex/v5/states.hpp>
#include <boost/regex/v5/regbase.hpp>
#include <boost/regex/v5/basic_regex.hpp>
#include <boost/regex/v5/basic_regex_creator.hpp>
#include <boost/regex/v5/basic_regex_parser.hpp>
#include <boost/regex/v5/sub_match.hpp>
#include <boost/regex/v5/regex_format.hpp>
#include <boost/regex/v5/match_results.hpp>
#include <boost/regex/v5/perl_matcher.hpp>

namespace boost{
#ifdef BOOST_REGEX_NO_FWD
typedef basic_regex<char, regex_traits<char> > regex;
#ifndef BOOST_NO_WREGEX
typedef basic_regex<wchar_t, regex_traits<wchar_t> > wregex;
#endif
#endif

typedef match_results<const char*> cmatch;
typedef match_results<std::string::const_iterator> smatch;
#ifndef BOOST_NO_WREGEX
typedef match_results<const wchar_t*> wcmatch;
typedef match_results<std::wstring::const_iterator> wsmatch;
#endif

}

#include <boost/regex/v5/regex_match.hpp>
#include <boost/regex/v5/regex_search.hpp>
#include <boost/regex/v5/regex_iterator.hpp>
#include <boost/regex/v5/regex_token_iterator.hpp>
#include <boost/regex/v5/regex_grep.hpp>
#include <boost/regex/v5/regex_replace.hpp>
#include <boost/regex/v5/regex_merge.hpp>
#include <boost/regex/v5/regex_split.hpp>

#endif

#endif
