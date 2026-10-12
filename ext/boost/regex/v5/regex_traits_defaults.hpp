
#ifndef BOOST_REGEX_TRAITS_DEFAULTS_HPP_INCLUDED
#define BOOST_REGEX_TRAITS_DEFAULTS_HPP_INCLUDED

#include <boost/regex/config.hpp>

#include <boost/regex/v5/syntax_type.hpp>
#include <boost/regex/v5/error_type.hpp>
#include <boost/regex/v5/regex_workaround.hpp>
#include <type_traits>
#include <cstdint>
#include <cctype>
#include <locale>
#include <cwctype>
#include <limits>

namespace boost{ namespace BOOST_REGEX_DETAIL_NS{

template <class charT>
inline bool is_extended(charT c)
{
   typedef typename std::make_unsigned<charT>::type unsigned_type;
   return (sizeof(charT) > 1) && (static_cast<unsigned_type>(c) >= 256u);
}
inline bool is_extended(char)
{ return false; }

inline const char*  get_default_syntax(regex_constants::syntax_type n)
{

   const char* messages[] = {
         "",
         "(",
         ")",
         "$",
         "^",
         ".",
         "*",
         "+",
         "?",
         "[",
         "]",
         "|",
         "\\",
         "#",
         "-",
         "{",
         "}",
         "0123456789",
         "b",
         "B",
         "<",
         ">",
         "",
         "",
         "A`",
         "z'",
         "\n",
         ",",
         "a",
         "f",
         "n",
         "r",
         "t",
         "v",
         "x",
         "c",
         ":",
         "=",
         "e",
         "",
         "",
         "",
         "",
         "",
         "",
         "",
         "",
         "E",
         "Q",
         "X",
         "C",
         "Z",
         "G",
         "!",
         "p",
         "P",
         "N",
         "gk",
         "K",
         "R",
   };

   return ((n >= (sizeof(messages) / sizeof(messages[1]))) ? "" : messages[n]);
}

inline const char*  get_default_error_string(regex_constants::error_type n)
{
   static const char* const s_default_error_messages[] = {
      "Success",
      "No match",
      "Invalid regular expression.",
      "Invalid collation character.",
      "Invalid character class name, collating name, or character range.",
      "Invalid or unterminated escape sequence.",
      "Invalid back reference: specified capturing group does not exist.",
      "Unmatched [ or [^ in character class declaration.",
      "Unmatched marking parenthesis ( or \\(.",
      "Unmatched quantified repeat operator { or \\{.",
      "Invalid content of repeat range.",
      "Invalid range end in character class",
      "Out of memory.",
      "Invalid preceding regular expression prior to repetition operator.",
      "Premature end of regular expression",
      "Regular expression is too large.",
      "Unmatched ) or \\)",
      "Empty regular expression.",
      "The complexity of matching the regular expression exceeded predefined bounds.  "
      "Try refactoring the regular expression to make each choice made by the state machine unambiguous.  "
      "This exception is thrown to prevent \"eternal\" matches that take an "
      "indefinite period time to locate.",
      "Ran out of stack space trying to match the regular expression.",
      "Invalid or unterminated Perl (?...) sequence.",
      "Unknown error.",
   };

   return (n > ::boost::regex_constants::error_unknown) ? s_default_error_messages[::boost::regex_constants::error_unknown] : s_default_error_messages[n];
}

inline regex_constants::syntax_type  get_default_syntax_type(char c)
{

   static regex_constants::syntax_type char_syntax[] = {
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_newline,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_not,
      regex_constants::syntax_char,
      regex_constants::syntax_hash,
      regex_constants::syntax_dollar,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::escape_type_end_buffer,
      regex_constants::syntax_open_mark,
      regex_constants::syntax_close_mark,
      regex_constants::syntax_star,
      regex_constants::syntax_plus,
      regex_constants::syntax_comma,
      regex_constants::syntax_dash,
      regex_constants::syntax_dot,
      regex_constants::syntax_char,
      regex_constants::syntax_digit,
      regex_constants::syntax_digit,
      regex_constants::syntax_digit,
      regex_constants::syntax_digit,
      regex_constants::syntax_digit,
      regex_constants::syntax_digit,
      regex_constants::syntax_digit,
      regex_constants::syntax_digit,
      regex_constants::syntax_digit,
      regex_constants::syntax_digit,
      regex_constants::syntax_colon,
      regex_constants::syntax_char,
      regex_constants::escape_type_left_word,
      regex_constants::syntax_equal,
      regex_constants::escape_type_right_word,
      regex_constants::syntax_question,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_open_set,
      regex_constants::syntax_escape,
      regex_constants::syntax_close_set,
      regex_constants::syntax_caret,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_open_brace,
      regex_constants::syntax_or,
      regex_constants::syntax_close_brace,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
      regex_constants::syntax_char,
   };

   return char_syntax[(unsigned char)c];
}

inline regex_constants::escape_syntax_type  get_default_escape_syntax_type(char c)
{

   static regex_constants::escape_syntax_type char_syntax[] = {
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_end_buffer,
      regex_constants::syntax_open_mark,
      regex_constants::syntax_close_mark,
      regex_constants::escape_type_identity,
      regex_constants::syntax_plus,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_decimal,
      regex_constants::escape_type_backref,
      regex_constants::escape_type_backref,
      regex_constants::escape_type_backref,
      regex_constants::escape_type_backref,
      regex_constants::escape_type_backref,
      regex_constants::escape_type_backref,
      regex_constants::escape_type_backref,
      regex_constants::escape_type_backref,
      regex_constants::escape_type_backref,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_left_word,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_right_word,
      regex_constants::syntax_question,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_start_buffer,
      regex_constants::escape_type_not_word_assert,
      regex_constants::escape_type_C,
      regex_constants::escape_type_not_class,
      regex_constants::escape_type_E,
      regex_constants::escape_type_not_class,
      regex_constants::escape_type_G,
      regex_constants::escape_type_not_class,
      regex_constants::escape_type_not_class,
      regex_constants::escape_type_not_class,
      regex_constants::escape_type_reset_start_mark,
      regex_constants::escape_type_not_class,
      regex_constants::escape_type_not_class,
      regex_constants::escape_type_named_char,
      regex_constants::escape_type_not_class,
      regex_constants::escape_type_not_property,
      regex_constants::escape_type_Q,
      regex_constants::escape_type_line_ending,
      regex_constants::escape_type_not_class,
      regex_constants::escape_type_not_class,
      regex_constants::escape_type_not_class,
      regex_constants::escape_type_not_class,
      regex_constants::escape_type_not_class,
      regex_constants::escape_type_X,
      regex_constants::escape_type_not_class,
      regex_constants::escape_type_Z,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_start_buffer,
      regex_constants::escape_type_control_a,
      regex_constants::escape_type_word_assert,
      regex_constants::escape_type_ascii_control,
      regex_constants::escape_type_class,
      regex_constants::escape_type_e,
      regex_constants::escape_type_control_f,
      regex_constants::escape_type_extended_backref,
      regex_constants::escape_type_class,
      regex_constants::escape_type_class,
      regex_constants::escape_type_class,
      regex_constants::escape_type_extended_backref,
      regex_constants::escape_type_class,
      regex_constants::escape_type_class,
      regex_constants::escape_type_control_n,
      regex_constants::escape_type_class,
      regex_constants::escape_type_property,
      regex_constants::escape_type_class,
      regex_constants::escape_type_control_r,
      regex_constants::escape_type_class,
      regex_constants::escape_type_control_t,
      regex_constants::escape_type_class,
      regex_constants::escape_type_control_v,
      regex_constants::escape_type_class,
      regex_constants::escape_type_hex,
      regex_constants::escape_type_class,
      regex_constants::escape_type_end_buffer,
      regex_constants::syntax_open_brace,
      regex_constants::syntax_or,
      regex_constants::syntax_close_brace,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
      regex_constants::escape_type_identity,
   };

   return char_syntax[(unsigned char)c];
}

inline bool  is_combining_implementation(std::uint_least16_t c)
{
   const std::uint_least16_t combining_ranges[] = { 0x0300, 0x0361,
                           0x0483, 0x0486,
                           0x0903, 0x0903,
                           0x093E, 0x0940,
                           0x0949, 0x094C,
                           0x0982, 0x0983,
                           0x09BE, 0x09C0,
                           0x09C7, 0x09CC,
                           0x09D7, 0x09D7,
                           0x0A3E, 0x0A40,
                           0x0A83, 0x0A83,
                           0x0ABE, 0x0AC0,
                           0x0AC9, 0x0ACC,
                           0x0B02, 0x0B03,
                           0x0B3E, 0x0B3E,
                           0x0B40, 0x0B40,
                           0x0B47, 0x0B4C,
                           0x0B57, 0x0B57,
                           0x0B83, 0x0B83,
                           0x0BBE, 0x0BBF,
                           0x0BC1, 0x0BCC,
                           0x0BD7, 0x0BD7,
                           0x0C01, 0x0C03,
                           0x0C41, 0x0C44,
                           0x0C82, 0x0C83,
                           0x0CBE, 0x0CBE,
                           0x0CC0, 0x0CC4,
                           0x0CC7, 0x0CCB,
                           0x0CD5, 0x0CD6,
                           0x0D02, 0x0D03,
                           0x0D3E, 0x0D40,
                           0x0D46, 0x0D4C,
                           0x0D57, 0x0D57,
                           0x0F7F, 0x0F7F,
                           0x20D0, 0x20E1,
                           0x3099, 0x309A,
                           0xFE20, 0xFE23,
                           0xffff, 0xffff, };

   const std::uint_least16_t* p = combining_ranges + 1;
   while (*p < c) p += 2;
   --p;
   if ((c >= *p) && (c <= *(p + 1)))
      return true;
   return false;
}

template <class charT>
inline bool is_combining(charT c)
{
   return (c <= static_cast<charT>(0)) ? false : ((c >= static_cast<charT>((std::numeric_limits<uint_least16_t>::max)())) ? false : is_combining_implementation(static_cast<unsigned short>(c)));
}
template <>
inline bool is_combining<char>(char)
{
   return false;
}
template <>
inline bool is_combining<signed char>(signed char)
{
   return false;
}
template <>
inline bool is_combining<unsigned char>(unsigned char)
{
   return false;
}
#ifdef _MSC_VER
template<>
inline bool is_combining<wchar_t>(wchar_t c)
{
   return is_combining_implementation(static_cast<unsigned short>(c));
}
#elif !defined(__DECCXX) && !defined(__osf__) && !defined(__OSF__) && defined(WCHAR_MIN) && (WCHAR_MIN == 0) && !defined(BOOST_NO_INTRINSIC_WCHAR_T)
#if defined(WCHAR_MAX) && (WCHAR_MAX <= USHRT_MAX)
template<>
inline bool is_combining<wchar_t>(wchar_t c)
{
   return is_combining_implementation(static_cast<unsigned short>(c));
}
#else
template<>
inline bool is_combining<wchar_t>(wchar_t c)
{
   return (c >= (std::numeric_limits<uint_least16_t>::max)()) ? false : is_combining_implementation(static_cast<unsigned short>(c));
}
#endif
#endif

template <class charT>
inline bool is_separator(charT c)
{
   return BOOST_REGEX_MAKE_BOOL(
      (c == static_cast<charT>('\n'))
      || (c == static_cast<charT>('\r'))
      || (c == static_cast<charT>('\f'))
      || (static_cast<std::uint16_t>(c) == 0x2028u)
      || (static_cast<std::uint16_t>(c) == 0x2029u)
      || (static_cast<std::uint16_t>(c) == 0x85u));
}
template <>
inline bool is_separator<char>(char c)
{
   return BOOST_REGEX_MAKE_BOOL((c == '\n') || (c == '\r') || (c == '\f'));
}

inline std::string  lookup_default_collate_name(const std::string& name)
{

   static const char* def_coll_names[] = {
   "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "alert", "backspace", "tab", "newline",
   "vertical-tab", "form-feed", "carriage-return", "SO", "SI", "DLE", "DC1", "DC2", "DC3", "DC4", "NAK",
   "SYN", "ETB", "CAN", "EM", "SUB", "ESC", "IS4", "IS3", "IS2", "IS1", "space", "exclamation-mark",
   "quotation-mark", "number-sign", "dollar-sign", "percent-sign", "ampersand", "apostrophe",
   "left-parenthesis", "right-parenthesis", "asterisk", "plus-sign", "comma", "hyphen",
   "period", "slash", "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
   "colon", "semicolon", "less-than-sign", "equals-sign", "greater-than-sign",
   "question-mark", "commercial-at", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
   "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "left-square-bracket", "backslash",
   "right-square-bracket", "circumflex", "underscore", "grave-accent", "a", "b", "c", "d", "e", "f",
   "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "left-curly-bracket",
   "vertical-line", "right-curly-bracket", "tilde", "DEL", "",
   };

   static const char* def_multi_coll[] = {
      "ae",
      "Ae",
      "AE",
      "ch",
      "Ch",
      "CH",
      "ll",
      "Ll",
      "LL",
      "ss",
      "Ss",
      "SS",
      "nj",
      "Nj",
      "NJ",
      "dz",
      "Dz",
      "DZ",
      "lj",
      "Lj",
      "LJ",
      "",
   };

   unsigned int i = 0;
   while (*def_coll_names[i])
   {
      if (def_coll_names[i] == name)
      {
         return std::string(1, char(i));
      }
      ++i;
   }
   i = 0;
   while (*def_multi_coll[i])
   {
      if (def_multi_coll[i] == name)
      {
         return def_multi_coll[i];
      }
      ++i;
   }
   return std::string();
}

template <class charT>
struct character_pointer_range
{
   const charT* p1;
   const charT* p2;

   bool operator < (const character_pointer_range& r)const
   {
      return std::lexicographical_compare(p1, p2, r.p1, r.p2);
   }
   bool operator == (const character_pointer_range& r)const
   {

#ifdef __cpp_lib_robust_nonmodifying_seq_ops
      return std::equal(p1, p2, r.p1, r.p2);
#elif defined(BOOST_REGEX_MSVC)
      if (((p2 - p1) != (r.p2 - r.p1)))
         return false;
      const charT* with = r.p1;
      const charT* pos = p1;
      while (pos != p2)
         if (*pos++ != *with++) return false;
      return true;

#else
      return ((p2 - p1) == (r.p2 - r.p1)) && std::equal(p1, p2, r.p1);
#endif
   }
};
template <class charT>
int get_default_class_id(const charT* p1, const charT* p2)
{
   static const charT data[73] = {
      'a', 'l', 'n', 'u', 'm',
      'a', 'l', 'p', 'h', 'a',
      'b', 'l', 'a', 'n', 'k',
      'c', 'n', 't', 'r', 'l',
      'd', 'i', 'g', 'i', 't',
      'g', 'r', 'a', 'p', 'h',
      'l', 'o', 'w', 'e', 'r',
      'p', 'r', 'i', 'n', 't',
      'p', 'u', 'n', 'c', 't',
      's', 'p', 'a', 'c', 'e',
      'u', 'n', 'i', 'c', 'o', 'd', 'e',
      'u', 'p', 'p', 'e', 'r',
      'v',
      'w', 'o', 'r', 'd',
      'x', 'd', 'i', 'g', 'i', 't',
   };

   static const character_pointer_range<charT> ranges[21] =
   {
      {data+0, data+5,},
      {data+5, data+10,},
      {data+10, data+15,},
      {data+15, data+20,},
      {data+20, data+21,},
      {data+20, data+25,},
      {data+25, data+30,},
      {data+29, data+30,},
      {data+30, data+31,},
      {data+30, data+35,},
      {data+35, data+40,},
      {data+40, data+45,},
      {data+45, data+46,},
      {data+45, data+50,},
      {data+57, data+58,},
      {data+50, data+57,},
      {data+57, data+62,},
      {data+62, data+63,},
      {data+63, data+64,},
      {data+63, data+67,},
      {data+67, data+73,},
   };
   const character_pointer_range<charT>* ranges_begin = ranges;
   const character_pointer_range<charT>* ranges_end = ranges + (sizeof(ranges)/sizeof(ranges[0]));

   character_pointer_range<charT> t = { p1, p2, };
   const character_pointer_range<charT>* p = std::lower_bound(ranges_begin, ranges_end, t);
   if((p != ranges_end) && (t == *p))
      return static_cast<int>(p - ranges);
   return -1;
}

template <class charT>
std::ptrdiff_t global_length(const charT* p)
{
   std::ptrdiff_t n = 0;
   while(*p)
   {
      ++p;
      ++n;
   }
   return n;
}
template<>
inline std::ptrdiff_t global_length<char>(const char* p)
{
   return (std::strlen)(p);
}
#ifndef BOOST_NO_WREGEX
template<>
inline std::ptrdiff_t global_length<wchar_t>(const wchar_t* p)
{
   return (std::ptrdiff_t)(std::wcslen)(p);
}
#endif
template <class charT>
inline charT  global_lower(charT c)
{
   return c;
}
template <class charT>
inline charT  global_upper(charT c)
{
   return c;
}

inline char  do_global_lower(char c)
{
   return static_cast<char>((std::tolower)((unsigned char)c));
}

inline char  do_global_upper(char c)
{
   return static_cast<char>((std::toupper)((unsigned char)c));
}
#ifndef BOOST_NO_WREGEX
inline wchar_t  do_global_lower(wchar_t c)
{
   return (std::towlower)(c);
}

inline wchar_t  do_global_upper(wchar_t c)
{
   return (std::towupper)(c);
}
#endif

template<> inline char  global_lower<char>(char c) { return do_global_lower(c); }
template<> inline char  global_upper<char>(char c) { return do_global_upper(c); }
#ifndef BOOST_NO_WREGEX
template<> inline wchar_t  global_lower<wchar_t>(wchar_t c) { return do_global_lower(c); }
template<> inline wchar_t  global_upper<wchar_t>(wchar_t c) { return do_global_upper(c); }
#endif

template <class charT>
int global_value(charT c)
{
   static const charT zero = '0';
   static const charT nine = '9';
   static const charT a = 'a';
   static const charT f = 'f';
   static const charT A = 'A';
   static const charT F = 'F';

   if(c > f) return -1;
   if(c >= a) return 10 + (c - a);
   if(c > F) return -1;
   if(c >= A) return 10 + (c - A);
   if(c > nine) return -1;
   if(c >= zero) return c - zero;
   return -1;
}
template <class charT, class traits>
std::intmax_t global_toi(const charT*& p1, const charT* p2, int radix, const traits& t)
{
   (void)t;
   std::intmax_t limit = (std::numeric_limits<std::intmax_t>::max)() / radix;
   std::intmax_t next_value = t.value(*p1, radix);
   if((p1 == p2) || (next_value < 0) || (next_value >= radix))
      return -1;
   std::intmax_t result = 0;
   while(p1 != p2)
   {
      next_value = t.value(*p1, radix);
      if((next_value < 0) || (next_value >= radix))
         break;
      result *= radix;
      result += next_value;
      ++p1;
      if (result > limit)
         return -1;
   }
   return result;
}

template <class charT>
inline typename std::enable_if<(sizeof(charT) > 1), const charT*>::type get_escape_R_string()
{
#ifdef BOOST_REGEX_MSVC
#  pragma warning(push)
#  pragma warning(disable:4309 4245)
#endif
   static const charT e1[] = { '(', '?', '-', 'x', ':', '(', '?', '>', '\x0D', '\x0A', '?',
      '|', '[', '\x0A', '\x0B', '\x0C', static_cast<charT>(0x85), static_cast<charT>(0x2028),
      static_cast<charT>(0x2029), ']', ')', ')', '\0' };
   static const charT e2[] = { '(', '?', '-', 'x', ':', '(', '?', '>', '\x0D', '\x0A', '?',
      '|', '[', '\x0A', '\x0B', '\x0C', static_cast<charT>(0x85), ']', ')', ')', '\0' };

   charT c = static_cast<charT>(0x2029u);
   bool b = (static_cast<unsigned>(c) == 0x2029u);

   return (b ? e1 : e2);
#ifdef BOOST_REGEX_MSVC
#  pragma warning(pop)
#endif
}

template <class charT>
inline typename std::enable_if<(sizeof(charT) == 1), const charT*>::type get_escape_R_string()
{
#ifdef BOOST_REGEX_MSVC
#  pragma warning(push)
#  pragma warning(disable:4309 4245)
#endif
   static const charT e2[] = {
      static_cast<charT>('('),
      static_cast<charT>('?'),
      static_cast<charT>('-'),
      static_cast<charT>('x'),
      static_cast<charT>(':'),
      static_cast<charT>('('),
      static_cast<charT>('?'),
      static_cast<charT>('>'),
      static_cast<charT>('\x0D'),
      static_cast<charT>('\x0A'),
      static_cast<charT>('?'),
      static_cast<charT>('|'),
      static_cast<charT>('['),
      static_cast<charT>('\x0A'),
      static_cast<charT>('\x0B'),
      static_cast<charT>('\x0C'),
      static_cast<charT>('\x85'),
      static_cast<charT>(']'),
      static_cast<charT>(')'),
      static_cast<charT>(')'),
      static_cast<charT>('\0')
   };
   return e2;
#ifdef BOOST_REGEX_MSVC
#  pragma warning(pop)
#endif
}

}
}

#endif
