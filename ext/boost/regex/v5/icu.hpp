
#ifndef BOOST_REGEX_ICU_V5_HPP
#define BOOST_REGEX_ICU_V5_HPP

#include <unicode/utypes.h>
#include <unicode/uchar.h>
#include <unicode/coll.h>
#include <type_traits>
#include <functional>
#include <boost/regex.hpp>
#include <boost/regex/v5/unicode_iterator.hpp>

#ifdef BOOST_REGEX_MSVC
#pragma warning (push)
#pragma warning (disable: 4251)
#endif

namespace boost{

namespace BOOST_REGEX_DETAIL_NS{

class icu_regex_traits_implementation
{
   typedef UChar32                      char_type;
   typedef std::size_t                  size_type;
   typedef std::vector<char_type>       string_type;
   typedef U_NAMESPACE_QUALIFIER Locale locale_type;
   typedef std::uint_least32_t          char_class_type;
public:
   icu_regex_traits_implementation(const U_NAMESPACE_QUALIFIER Locale& l)
      : m_locale(l)
   {
      UErrorCode success = U_ZERO_ERROR;
      m_collator.reset(U_NAMESPACE_QUALIFIER Collator::createInstance(l, success));
      if(U_SUCCESS(success) == 0)
         init_error();
      m_collator->setStrength(U_NAMESPACE_QUALIFIER Collator::IDENTICAL);
      success = U_ZERO_ERROR;
      m_primary_collator.reset(U_NAMESPACE_QUALIFIER Collator::createInstance(l, success));
      if(U_SUCCESS(success) == 0)
         init_error();
      m_primary_collator->setStrength(U_NAMESPACE_QUALIFIER Collator::PRIMARY);
   }
   U_NAMESPACE_QUALIFIER Locale getloc()const
   {
      return m_locale;
   }
   string_type do_transform(const char_type* p1, const char_type* p2, const U_NAMESPACE_QUALIFIER Collator* pcoll) const
   {

      typedef u32_to_u16_iterator<const char_type*, ::UChar> itt;
      itt i(p1), j(p2);
      std::vector< ::UChar> t(i, j);
      std::uint8_t result[100];
      std::int32_t len;
      if (!t.empty())
         len = pcoll->getSortKey(&*t.begin(), static_cast<std::int32_t>(t.size()), result, sizeof(result));
      else
         len = pcoll->getSortKey(static_cast<UChar const*>(0), static_cast<std::int32_t>(0), result, sizeof(result));
      if (std::size_t(len) > sizeof(result))
      {
         std::unique_ptr< std::uint8_t[]> presult(new ::uint8_t[len + 1]);
         if (!t.empty())
            len = pcoll->getSortKey(&*t.begin(), static_cast<std::int32_t>(t.size()), presult.get(), len + 1);
         else
            len = pcoll->getSortKey(static_cast<UChar const*>(0), static_cast<std::int32_t>(0), presult.get(), len + 1);
         if ((0 == presult[len - 1]) && (len > 1))
            --len;
         return string_type(presult.get(), presult.get() + len);
      }
      if ((0 == result[len - 1]) && (len > 1))
         --len;
      return string_type(result, result + len);
   }
   string_type transform(const char_type* p1, const char_type* p2) const
   {
      return do_transform(p1, p2, m_collator.get());
   }
   string_type transform_primary(const char_type* p1, const char_type* p2) const
   {
      return do_transform(p1, p2, m_primary_collator.get());
   }
private:
   void init_error()
   {
      std::runtime_error e("Could not initialize ICU resources");
#ifndef BOOST_REGEX_STANDALONE
      boost::throw_exception(e);
#else
      throw e;
#endif
   }
   U_NAMESPACE_QUALIFIER Locale m_locale;
   std::unique_ptr< U_NAMESPACE_QUALIFIER Collator> m_collator;
   std::unique_ptr< U_NAMESPACE_QUALIFIER Collator> m_primary_collator;
};
inline std::shared_ptr<icu_regex_traits_implementation> get_icu_regex_traits_implementation(const U_NAMESPACE_QUALIFIER Locale& loc)
{
   return std::shared_ptr<icu_regex_traits_implementation>(new icu_regex_traits_implementation(loc));
}

}

class icu_regex_traits
{
public:
   typedef UChar32                      char_type;
   typedef std::size_t                  size_type;
   typedef std::vector<char_type>       string_type;
   typedef U_NAMESPACE_QUALIFIER Locale locale_type;
   typedef std::uint64_t                char_class_type;

   struct boost_extensions_tag{};

   icu_regex_traits()
      : m_pimpl(BOOST_REGEX_DETAIL_NS::get_icu_regex_traits_implementation(U_NAMESPACE_QUALIFIER Locale()))
   {
   }
   static size_type length(const char_type* p)
   {
      size_type result = 0;
      while (*p)
      {
         ++p;
         ++result;
      }
      return result;
   }

   ::boost::regex_constants::syntax_type syntax_type(char_type c)const
   {
      return ((c < 0x7f) && (c > 0)) ? BOOST_REGEX_DETAIL_NS::get_default_syntax_type(static_cast<char>(c)) : regex_constants::syntax_char;
   }
   ::boost::regex_constants::escape_syntax_type escape_syntax_type(char_type c) const
   {
      return ((c < 0x7f) && (c > 0)) ? BOOST_REGEX_DETAIL_NS::get_default_escape_syntax_type(static_cast<char>(c)) : regex_constants::syntax_char;
   }
   char_type translate(char_type c) const
   {
      return c;
   }
   char_type translate_nocase(char_type c) const
   {
      return ::u_foldCase(c, U_FOLD_CASE_DEFAULT);
   }
   char_type translate(char_type c, bool icase) const
   {
      return icase ? translate_nocase(c) : translate(c);
   }
   char_type tolower(char_type c) const
   {
      return ::u_tolower(c);
   }
   char_type toupper(char_type c) const
   {
      return ::u_toupper(c);
   }
   string_type transform(const char_type* p1, const char_type* p2) const
   {
      return m_pimpl->transform(p1, p2);
   }
   string_type transform_primary(const char_type* p1, const char_type* p2) const
   {
      return m_pimpl->transform_primary(p1, p2);
   }
   char_class_type lookup_classname(const char_type* p1, const char_type* p2) const
   {
      constexpr char_class_type mask_blank = char_class_type(1) << offset_blank;
      constexpr char_class_type mask_space = char_class_type(1) << offset_space;
      constexpr char_class_type mask_xdigit = char_class_type(1) << offset_xdigit;
      constexpr char_class_type mask_underscore = char_class_type(1) << offset_underscore;
      constexpr char_class_type mask_unicode = char_class_type(1) << offset_unicode;

      constexpr char_class_type mask_horizontal = char_class_type(1) << offset_horizontal;
      constexpr char_class_type mask_vertical = char_class_type(1) << offset_vertical;

      static const char_class_type masks[] =
      {
         0,
         U_GC_L_MASK | U_GC_ND_MASK,
         U_GC_L_MASK,
         mask_blank,
         U_GC_CC_MASK | U_GC_CF_MASK | U_GC_ZL_MASK | U_GC_ZP_MASK,
         U_GC_ND_MASK,
         U_GC_ND_MASK,
         (0x3FFFFFFFu) & ~(U_GC_CC_MASK | U_GC_CF_MASK | U_GC_CS_MASK | U_GC_CN_MASK | U_GC_Z_MASK),
         mask_horizontal,
         U_GC_LL_MASK,
         U_GC_LL_MASK,
         ~(U_GC_C_MASK),
         U_GC_P_MASK,
         char_class_type(U_GC_Z_MASK) | mask_space,
         char_class_type(U_GC_Z_MASK) | mask_space,
         U_GC_LU_MASK,
         mask_unicode,
         U_GC_LU_MASK,
         mask_vertical,
         char_class_type(U_GC_L_MASK | U_GC_ND_MASK | U_GC_MN_MASK) | mask_underscore,
         char_class_type(U_GC_L_MASK | U_GC_ND_MASK | U_GC_MN_MASK) | mask_underscore,
         char_class_type(U_GC_ND_MASK) | mask_xdigit,
      };

      int idx = ::boost::BOOST_REGEX_DETAIL_NS::get_default_class_id(p1, p2);
      if (idx >= 0)
         return masks[idx + 1];
      char_class_type result = lookup_icu_mask(p1, p2);
      if (result != 0)
         return result;

      if (idx < 0)
      {
         string_type s(p1, p2);
         string_type::size_type i = 0;
         while (i < s.size())
         {
            s[i] = static_cast<char>((::u_tolower)(s[i]));
            if (::u_isspace(s[i]) || (s[i] == '-') || (s[i] == '_'))
               s.erase(s.begin() + i, s.begin() + i + 1);
            else
            {
               s[i] = static_cast<char>((::u_tolower)(s[i]));
               ++i;
            }
         }
         if (!s.empty())
            idx = ::boost::BOOST_REGEX_DETAIL_NS::get_default_class_id(&*s.begin(), &*s.begin() + s.size());
         if (idx >= 0)
            return masks[idx + 1];
         if (!s.empty())
            result = lookup_icu_mask(&*s.begin(), &*s.begin() + s.size());
         if (result != 0)
            return result;
      }
      BOOST_REGEX_ASSERT(std::size_t(idx + 1) < sizeof(masks) / sizeof(masks[0]));
      return masks[idx + 1];
   }
   string_type lookup_collatename(const char_type* p1, const char_type* p2) const
   {
      string_type result;
      if (std::find_if(p1, p2, std::bind(std::greater< ::UChar32>(), std::placeholders::_1, 0x7f)) == p2)
      {
         std::string s(p1, p2);

         UErrorCode err = U_ZERO_ERROR;
         UChar32 c = ::u_charFromName(U_UNICODE_CHAR_NAME, s.c_str(), &err);
         if (U_SUCCESS(err))
         {
            result.push_back(c);
            return result;
         }

         err = U_ZERO_ERROR;
         c = ::u_charFromName(U_EXTENDED_CHAR_NAME, s.c_str(), &err);
         if (U_SUCCESS(err))
         {
            result.push_back(c);
            return result;
         }

         s = ::boost::BOOST_REGEX_DETAIL_NS::lookup_default_collate_name(s);
         result.assign(s.begin(), s.end());
      }
      if (result.empty() && (p2 - p1 == 1))
         result.push_back(*p1);
      return result;
   }
   bool isctype(char_type c, char_class_type f) const
   {
      constexpr char_class_type mask_blank = char_class_type(1) << offset_blank;
      constexpr char_class_type mask_space = char_class_type(1) << offset_space;
      constexpr char_class_type mask_xdigit = char_class_type(1) << offset_xdigit;
      constexpr char_class_type mask_underscore = char_class_type(1) << offset_underscore;
      constexpr char_class_type mask_unicode = char_class_type(1) << offset_unicode;
      constexpr char_class_type mask_any = char_class_type(1) << offset_any;
      constexpr char_class_type mask_ascii = char_class_type(1) << offset_ascii;
      constexpr char_class_type mask_horizontal = char_class_type(1) << offset_horizontal;
      constexpr char_class_type mask_vertical = char_class_type(1) << offset_vertical;

      char_class_type m = char_class_type(static_cast<char_class_type>(1) << u_charType(c));
      if ((m & f) != 0)
         return true;

      if (((f & mask_blank) != 0) && u_isblank(c))
         return true;
      if (((f & mask_space) != 0) && u_isspace(c))
         return true;
      if (((f & mask_xdigit) != 0) && (u_digit(c, 16) >= 0))
         return true;
      if (((f & mask_unicode) != 0) && (c >= 0x100))
         return true;
      if (((f & mask_underscore) != 0) && (c == '_'))
         return true;
      if (((f & mask_any) != 0) && (c <= 0x10FFFF))
         return true;
      if (((f & mask_ascii) != 0) && (c <= 0x7F))
         return true;
      if (((f & mask_vertical) != 0) && (::boost::BOOST_REGEX_DETAIL_NS::is_separator(c) || (c == static_cast<char_type>('\v')) || (m == U_GC_ZL_MASK) || (m == U_GC_ZP_MASK)))
         return true;
      if (((f & mask_horizontal) != 0) && !::boost::BOOST_REGEX_DETAIL_NS::is_separator(c) && u_isspace(c) && (c != static_cast<char_type>('\v')))
         return true;
      return false;
   }
   std::intmax_t toi(const char_type*& p1, const char_type* p2, int radix)const
   {
      return BOOST_REGEX_DETAIL_NS::global_toi(p1, p2, radix, *this);
   }
   int value(char_type c, int radix)const
   {
      return u_digit(c, static_cast< std::int8_t>(radix));
   }
   locale_type imbue(locale_type l)
   {
      locale_type result(m_pimpl->getloc());
      m_pimpl = BOOST_REGEX_DETAIL_NS::get_icu_regex_traits_implementation(l);
      return result;
   }
   locale_type getloc()const
   {
      return locale_type();
   }
   std::string error_string(::boost::regex_constants::error_type n) const
   {
      return BOOST_REGEX_DETAIL_NS::get_default_error_string(n);
   }
private:
   icu_regex_traits(const icu_regex_traits&);
   icu_regex_traits& operator=(const icu_regex_traits&);

   enum{
      offset_blank = U_CHAR_CATEGORY_COUNT,
      offset_space = U_CHAR_CATEGORY_COUNT+1,
      offset_xdigit = U_CHAR_CATEGORY_COUNT+2,
      offset_underscore = U_CHAR_CATEGORY_COUNT+3,
      offset_unicode = U_CHAR_CATEGORY_COUNT+4,
      offset_any = U_CHAR_CATEGORY_COUNT+5,
      offset_ascii = U_CHAR_CATEGORY_COUNT+6,
      offset_horizontal = U_CHAR_CATEGORY_COUNT+7,
      offset_vertical = U_CHAR_CATEGORY_COUNT+8
   };

   static char_class_type lookup_icu_mask(const ::UChar32* p1, const ::UChar32* p2)
   {

      constexpr char_class_type mask_any = char_class_type(1) << offset_any;
      constexpr char_class_type mask_ascii = char_class_type(1) << offset_ascii;

      static const ::UChar32 prop_name_table[] = {
           'a', 'n', 'y',
           'a', 's', 'c', 'i', 'i',
           'a', 's', 's', 'i', 'g', 'n', 'e', 'd',
           'c', '*',
           'c', 'c',
           'c', 'f',
           'c', 'l', 'o', 's', 'e', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
           'c', 'n',
           'c', 'o',
           'c', 'o', 'n', 'n', 'e', 'c', 't', 'o', 'r', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
           'c', 'o', 'n', 't', 'r', 'o', 'l',
           'c', 's',
           'c', 'u', 'r', 'r', 'e', 'n', 'c', 'y', 's', 'y', 'm', 'b', 'o', 'l',
           'd', 'a', 's', 'h', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
           'd', 'e', 'c', 'i', 'm', 'a', 'l', 'd', 'i', 'g', 'i', 't', 'n', 'u', 'm', 'b', 'e', 'r',
           'e', 'n', 'c', 'l', 'o', 's', 'i', 'n', 'g', 'm', 'a', 'r', 'k',
           'f', 'i', 'n', 'a', 'l', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
           'f', 'o', 'r', 'm', 'a', 't',
           'i', 'n', 'i', 't', 'i', 'a', 'l', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
           'l', '*',
           'l', 'e', 't', 't', 'e', 'r',
           'l', 'e', 't', 't', 'e', 'r', 'n', 'u', 'm', 'b', 'e', 'r',
           'l', 'i', 'n', 'e', 's', 'e', 'p', 'a', 'r', 'a', 't', 'o', 'r',
           'l', 'l',
           'l', 'm',
           'l', 'o',
           'l', 'o', 'w', 'e', 'r', 'c', 'a', 's', 'e', 'l', 'e', 't', 't', 'e', 'r',
           'l', 't',
           'l', 'u',
           'm', '*',
           'm', 'a', 'r', 'k',
           'm', 'a', 't', 'h', 's', 'y', 'm', 'b', 'o', 'l',
           'm', 'c',
           'm', 'e',
           'm', 'n',
           'm', 'o', 'd', 'i', 'f', 'i', 'e', 'r', 'l', 'e', 't', 't', 'e', 'r',
           'm', 'o', 'd', 'i', 'f', 'i', 'e', 'r', 's', 'y', 'm', 'b', 'o', 'l',
           'n', '*',
           'n', 'd',
           'n', 'l',
           'n', 'o',
           'n', 'o', 'n', 's', 'p', 'a', 'c', 'i', 'n', 'g', 'm', 'a', 'r', 'k',
           'n', 'o', 't', 'a', 's', 's', 'i', 'g', 'n', 'e', 'd',
           'n', 'u', 'm', 'b', 'e', 'r',
           'o', 'p', 'e', 'n', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
           'o', 't', 'h', 'e', 'r',
           'o', 't', 'h', 'e', 'r', 'l', 'e', 't', 't', 'e', 'r',
           'o', 't', 'h', 'e', 'r', 'n', 'u', 'm', 'b', 'e', 'r',
           'o', 't', 'h', 'e', 'r', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
           'o', 't', 'h', 'e', 'r', 's', 'y', 'm', 'b', 'o', 'l',
           'p', '*',
           'p', 'a', 'r', 'a', 'g', 'r', 'a', 'p', 'h', 's', 'e', 'p', 'a', 'r', 'a', 't', 'o', 'r',
           'p', 'c',
           'p', 'd',
           'p', 'e',
           'p', 'f',
           'p', 'i',
           'p', 'o',
           'p', 'r', 'i', 'v', 'a', 't', 'e', 'u', 's', 'e',
           'p', 's',
           'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
           's', '*',
           's', 'c',
           's', 'e', 'p', 'a', 'r', 'a', 't', 'o', 'r',
           's', 'k',
           's', 'm',
           's', 'o',
           's', 'p', 'a', 'c', 'e', 's', 'e', 'p', 'a', 'r', 'a', 't', 'o', 'r',
           's', 'p', 'a', 'c', 'i', 'n', 'g', 'c', 'o', 'm', 'b', 'i', 'n', 'i', 'n', 'g', 'm', 'a', 'r', 'k',
           's', 'u', 'r', 'r', 'o', 'g', 'a', 't', 'e',
           's', 'y', 'm', 'b', 'o', 'l',
           't', 'i', 't', 'l', 'e', 'c', 'a', 's', 'e',
           't', 'i', 't', 'l', 'e', 'c', 'a', 's', 'e', 'l', 'e', 't', 't', 'e', 'r',
           'u', 'p', 'p', 'e', 'r', 'c', 'a', 's', 'e', 'l', 'e', 't', 't', 'e', 'r',
           'z', '*',
           'z', 'l',
           'z', 'p',
           'z', 's',
      };

      static const BOOST_REGEX_DETAIL_NS::character_pointer_range< ::UChar32> range_data[] = {
         { prop_name_table + 0, prop_name_table + 3, },
         { prop_name_table + 3, prop_name_table + 8, },
         { prop_name_table + 8, prop_name_table + 16, },
         { prop_name_table + 16, prop_name_table + 18, },
         { prop_name_table + 18, prop_name_table + 20, },
         { prop_name_table + 20, prop_name_table + 22, },
         { prop_name_table + 22, prop_name_table + 38, },
         { prop_name_table + 38, prop_name_table + 40, },
         { prop_name_table + 40, prop_name_table + 42, },
         { prop_name_table + 42, prop_name_table + 62, },
         { prop_name_table + 62, prop_name_table + 69, },
         { prop_name_table + 69, prop_name_table + 71, },
         { prop_name_table + 71, prop_name_table + 85, },
         { prop_name_table + 85, prop_name_table + 100, },
         { prop_name_table + 100, prop_name_table + 118, },
         { prop_name_table + 118, prop_name_table + 131, },
         { prop_name_table + 131, prop_name_table + 147, },
         { prop_name_table + 147, prop_name_table + 153, },
         { prop_name_table + 153, prop_name_table + 171, },
         { prop_name_table + 171, prop_name_table + 173, },
         { prop_name_table + 173, prop_name_table + 179, },
         { prop_name_table + 179, prop_name_table + 191, },
         { prop_name_table + 191, prop_name_table + 204, },
         { prop_name_table + 204, prop_name_table + 206, },
         { prop_name_table + 206, prop_name_table + 208, },
         { prop_name_table + 208, prop_name_table + 210, },
         { prop_name_table + 210, prop_name_table + 225, },
         { prop_name_table + 225, prop_name_table + 227, },
         { prop_name_table + 227, prop_name_table + 229, },
         { prop_name_table + 229, prop_name_table + 231, },
         { prop_name_table + 231, prop_name_table + 235, },
         { prop_name_table + 235, prop_name_table + 245, },
         { prop_name_table + 245, prop_name_table + 247, },
         { prop_name_table + 247, prop_name_table + 249, },
         { prop_name_table + 249, prop_name_table + 251, },
         { prop_name_table + 251, prop_name_table + 265, },
         { prop_name_table + 265, prop_name_table + 279, },
         { prop_name_table + 279, prop_name_table + 281, },
         { prop_name_table + 281, prop_name_table + 283, },
         { prop_name_table + 283, prop_name_table + 285, },
         { prop_name_table + 285, prop_name_table + 287, },
         { prop_name_table + 287, prop_name_table + 301, },
         { prop_name_table + 301, prop_name_table + 312, },
         { prop_name_table + 312, prop_name_table + 318, },
         { prop_name_table + 318, prop_name_table + 333, },
         { prop_name_table + 333, prop_name_table + 338, },
         { prop_name_table + 338, prop_name_table + 349, },
         { prop_name_table + 349, prop_name_table + 360, },
         { prop_name_table + 360, prop_name_table + 376, },
         { prop_name_table + 376, prop_name_table + 387, },
         { prop_name_table + 387, prop_name_table + 389, },
         { prop_name_table + 389, prop_name_table + 407, },
         { prop_name_table + 407, prop_name_table + 409, },
         { prop_name_table + 409, prop_name_table + 411, },
         { prop_name_table + 411, prop_name_table + 413, },
         { prop_name_table + 413, prop_name_table + 415, },
         { prop_name_table + 415, prop_name_table + 417, },
         { prop_name_table + 417, prop_name_table + 419, },
         { prop_name_table + 419, prop_name_table + 429, },
         { prop_name_table + 429, prop_name_table + 431, },
         { prop_name_table + 431, prop_name_table + 442, },
         { prop_name_table + 442, prop_name_table + 444, },
         { prop_name_table + 444, prop_name_table + 446, },
         { prop_name_table + 446, prop_name_table + 455, },
         { prop_name_table + 455, prop_name_table + 457, },
         { prop_name_table + 457, prop_name_table + 459, },
         { prop_name_table + 459, prop_name_table + 461, },
         { prop_name_table + 461, prop_name_table + 475, },
         { prop_name_table + 475, prop_name_table + 495, },
         { prop_name_table + 495, prop_name_table + 504, },
         { prop_name_table + 504, prop_name_table + 510, },
         { prop_name_table + 510, prop_name_table + 519, },
         { prop_name_table + 519, prop_name_table + 534, },
         { prop_name_table + 534, prop_name_table + 549, },
         { prop_name_table + 549, prop_name_table + 551, },
         { prop_name_table + 551, prop_name_table + 553, },
         { prop_name_table + 553, prop_name_table + 555, },
         { prop_name_table + 555, prop_name_table + 557, },
      };

      static const icu_regex_traits::char_class_type icu_class_map[] = {
         mask_any,
         mask_ascii,
         (0x3FFFFFFFu) & ~(U_GC_CN_MASK),
         U_GC_C_MASK,
         U_GC_CC_MASK,
         U_GC_CF_MASK,
         U_GC_PE_MASK,
         U_GC_CN_MASK,
         U_GC_CO_MASK,
         U_GC_PC_MASK,
         U_GC_CC_MASK,
         U_GC_CS_MASK,
         U_GC_SC_MASK,
         U_GC_PD_MASK,
         U_GC_ND_MASK,
         U_GC_ME_MASK,
         U_GC_PF_MASK,
         U_GC_CF_MASK,
         U_GC_PI_MASK,
         U_GC_L_MASK,
         U_GC_L_MASK,
         U_GC_NL_MASK,
         U_GC_ZL_MASK,
         U_GC_LL_MASK,
         U_GC_LM_MASK,
         U_GC_LO_MASK,
         U_GC_LL_MASK,
         U_GC_LT_MASK,
         U_GC_LU_MASK,
         U_GC_M_MASK,
         U_GC_M_MASK,
         U_GC_SM_MASK,
         U_GC_MC_MASK,
         U_GC_ME_MASK,
         U_GC_MN_MASK,
         U_GC_LM_MASK,
         U_GC_SK_MASK,
         U_GC_N_MASK,
         U_GC_ND_MASK,
         U_GC_NL_MASK,
         U_GC_NO_MASK,
         U_GC_MN_MASK,
         U_GC_CN_MASK,
         U_GC_N_MASK,
         U_GC_PS_MASK,
         U_GC_C_MASK,
         U_GC_LO_MASK,
         U_GC_NO_MASK,
         U_GC_PO_MASK,
         U_GC_SO_MASK,
         U_GC_P_MASK,
         U_GC_ZP_MASK,
         U_GC_PC_MASK,
         U_GC_PD_MASK,
         U_GC_PE_MASK,
         U_GC_PF_MASK,
         U_GC_PI_MASK,
         U_GC_PO_MASK,
         U_GC_CO_MASK,
         U_GC_PS_MASK,
         U_GC_P_MASK,
         U_GC_S_MASK,
         U_GC_SC_MASK,
         U_GC_Z_MASK,
         U_GC_SK_MASK,
         U_GC_SM_MASK,
         U_GC_SO_MASK,
         U_GC_ZS_MASK,
         U_GC_MC_MASK,
         U_GC_CS_MASK,
         U_GC_S_MASK,
         U_GC_LT_MASK,
         U_GC_LT_MASK,
         U_GC_LU_MASK,
         U_GC_Z_MASK,
         U_GC_ZL_MASK,
         U_GC_ZP_MASK,
         U_GC_ZS_MASK,
      };

      const BOOST_REGEX_DETAIL_NS::character_pointer_range< ::UChar32>* ranges_begin = range_data;
      const BOOST_REGEX_DETAIL_NS::character_pointer_range< ::UChar32>* ranges_end = range_data + (sizeof(range_data) / sizeof(range_data[0]));

      BOOST_REGEX_DETAIL_NS::character_pointer_range< ::UChar32> t = { p1, p2, };
      const BOOST_REGEX_DETAIL_NS::character_pointer_range< ::UChar32>* p = std::lower_bound(ranges_begin, ranges_end, t);
      if ((p != ranges_end) && (t == *p))
         return icu_class_map[p - ranges_begin];
      return 0;
   }
   std::shared_ptr< ::boost::BOOST_REGEX_DETAIL_NS::icu_regex_traits_implementation> m_pimpl;
};

}

namespace boost{

typedef basic_regex< ::UChar32, icu_regex_traits> u32regex;
typedef match_results<const ::UChar32*> u32match;
typedef match_results<const ::UChar*> u16match;

namespace BOOST_REGEX_DETAIL_NS{

template <class InputIterator>
inline u32regex do_make_u32regex(InputIterator i,
                              InputIterator j,
                              boost::regex_constants::syntax_option_type opt,
                              const std::integral_constant<int, 1>*)
{
   typedef boost::u8_to_u32_iterator<InputIterator, UChar32> conv_type;
   return u32regex(conv_type(i, i, j), conv_type(j, i, j), opt);
}

template <class InputIterator>
inline u32regex do_make_u32regex(InputIterator i,
                              InputIterator j,
                              boost::regex_constants::syntax_option_type opt,
                              const std::integral_constant<int, 2>*)
{
   typedef boost::u16_to_u32_iterator<InputIterator, UChar32> conv_type;
   return u32regex(conv_type(i, i, j), conv_type(j, i, j), opt);
}

template <class InputIterator>
inline u32regex do_make_u32regex(InputIterator i,
                              InputIterator j,
                              boost::regex_constants::syntax_option_type opt,
                              const std::integral_constant<int, 4>*)
{
   return u32regex(i, j, opt);
}
}

#define BOOST_REGEX_UCHAR_IS_WCHAR_T (U_ICU_VERSION_MAJOR_NUM < 59 && U_SIZEOF_WCHAR_T == 2)

#if BOOST_REGEX_UCHAR_IS_WCHAR_T
  static_assert((std::is_same<UChar, wchar_t>::value), "Configuration logic has failed!");
#else
  static_assert(!(std::is_same<UChar, wchar_t>::value), "Configuration logic has failed!");
#endif

template <class InputIterator>
inline u32regex make_u32regex(InputIterator i,
                              InputIterator j,
                              boost::regex_constants::syntax_option_type opt)
{
   return BOOST_REGEX_DETAIL_NS::do_make_u32regex(i, j, opt, static_cast<std::integral_constant<int, sizeof(*i)> const*>(0));
}

inline u32regex make_u32regex(const char* p, boost::regex_constants::syntax_option_type opt = boost::regex_constants::perl)
{
   return BOOST_REGEX_DETAIL_NS::do_make_u32regex(p, p + std::strlen(p), opt, static_cast<std::integral_constant<int, 1> const*>(0));
}
inline u32regex make_u32regex(const unsigned char* p, boost::regex_constants::syntax_option_type opt = boost::regex_constants::perl)
{
   return BOOST_REGEX_DETAIL_NS::do_make_u32regex(p, p + std::strlen(reinterpret_cast<const char*>(p)), opt, static_cast<std::integral_constant<int, 1> const*>(0));
}

#ifndef BOOST_NO_WREGEX
inline u32regex make_u32regex(const wchar_t* p, boost::regex_constants::syntax_option_type opt = boost::regex_constants::perl)
{
   return BOOST_REGEX_DETAIL_NS::do_make_u32regex(p, p + std::wcslen(p), opt, static_cast<std::integral_constant<int, sizeof(wchar_t)> const*>(0));
}
#endif
#if !BOOST_REGEX_UCHAR_IS_WCHAR_T
inline u32regex make_u32regex(const UChar* p, boost::regex_constants::syntax_option_type opt = boost::regex_constants::perl)
{
   return BOOST_REGEX_DETAIL_NS::do_make_u32regex(p, p + u_strlen(p), opt, static_cast<std::integral_constant<int, 2> const*>(0));
}
#endif

template<class C, class T, class A>
inline u32regex make_u32regex(const std::basic_string<C, T, A>& s, boost::regex_constants::syntax_option_type opt = boost::regex_constants::perl)
{
   return BOOST_REGEX_DETAIL_NS::do_make_u32regex(s.begin(), s.end(), opt, static_cast<std::integral_constant<int, sizeof(C)> const*>(0));
}

inline u32regex make_u32regex(const U_NAMESPACE_QUALIFIER UnicodeString& s, boost::regex_constants::syntax_option_type opt = boost::regex_constants::perl)
{
   return BOOST_REGEX_DETAIL_NS::do_make_u32regex(s.getBuffer(), s.getBuffer() + s.length(), opt, static_cast<std::integral_constant<int, 2> const*>(0));
}

namespace BOOST_REGEX_DETAIL_NS{
template<class MR1, class MR2, class NSubs>
void copy_results(MR1& out, MR2 const& in, NSubs named_subs)
{

   out.set_size(in.size(), in.prefix().first.base(), in.suffix().second.base());
   out.set_base(in.base().base());
   out.set_named_subs(named_subs);
   for(int i = 0; i < (int)in.size(); ++i)
   {
      if(in[i].matched || !i)
      {
         out.set_first(in[i].first.base(), i);
         out.set_second(in[i].second.base(), i, in[i].matched);
      }
   }
#ifdef BOOST_REGEX_MATCH_EXTRA

   for(int i = 0; i < (int)in.size(); ++i)
   {
      if(in[i].captures().size())
      {
         out[i].get_captures().assign(in[i].captures().size(), typename MR1::value_type());
         for(int j = 0; j < (int)out[i].captures().size(); ++j)
         {
            out[i].get_captures()[j].first = in[i].captures()[j].first.base();
            out[i].get_captures()[j].second = in[i].captures()[j].second.base();
            out[i].get_captures()[j].matched = in[i].captures()[j].matched;
         }
      }
   }
#endif
}

template <class BidiIterator, class Allocator>
inline bool do_regex_match(BidiIterator first, BidiIterator last,
                 match_results<BidiIterator, Allocator>& m,
                 const u32regex& e,
                 match_flag_type flags,
                 std::integral_constant<int, 4> const*)
{
   return ::boost::regex_match(first, last, m, e, flags);
}
template <class BidiIterator, class Allocator>
bool do_regex_match(BidiIterator first, BidiIterator last,
                 match_results<BidiIterator, Allocator>& m,
                 const u32regex& e,
                 match_flag_type flags,
                 std::integral_constant<int, 2> const*)
{
   typedef u16_to_u32_iterator<BidiIterator, UChar32> conv_type;
   typedef match_results<conv_type>                   match_type;

   match_type what;
   bool result = ::boost::regex_match(conv_type(first, first, last), conv_type(last, first, last), what, e, flags);

   if(result) copy_results(m, what, e.get_named_subs());
   return result;
}
template <class BidiIterator, class Allocator>
bool do_regex_match(BidiIterator first, BidiIterator last,
                 match_results<BidiIterator, Allocator>& m,
                 const u32regex& e,
                 match_flag_type flags,
                 std::integral_constant<int, 1> const*)
{
   typedef u8_to_u32_iterator<BidiIterator, UChar32>  conv_type;
   typedef match_results<conv_type>                   match_type;

   match_type what;
   bool result = ::boost::regex_match(conv_type(first, first, last), conv_type(last, first, last), what, e, flags);

   if(result) copy_results(m, what, e.get_named_subs());
   return result;
}
}

template <class BidiIterator, class Allocator>
inline bool u32regex_match(BidiIterator first, BidiIterator last,
                 match_results<BidiIterator, Allocator>& m,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_match(first, last, m, e, flags, static_cast<std::integral_constant<int, sizeof(*first)> const*>(0));
}
inline bool u32regex_match(const UChar* p,
                 match_results<const UChar*>& m,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p+u_strlen(p), m, e, flags, static_cast<std::integral_constant<int, 2> const*>(0));
}
#if !BOOST_REGEX_UCHAR_IS_WCHAR_T && !defined(BOOST_NO_WREGEX)
inline bool u32regex_match(const wchar_t* p,
                 match_results<const wchar_t*>& m,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p+std::wcslen(p), m, e, flags, static_cast<std::integral_constant<int, sizeof(wchar_t)> const*>(0));
}
#endif
inline bool u32regex_match(const char* p,
                 match_results<const char*>& m,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p+std::strlen(p), m, e, flags, static_cast<std::integral_constant<int, 1> const*>(0));
}
inline bool u32regex_match(const unsigned char* p,
                 match_results<const unsigned char*>& m,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p+std::strlen((const char*)p), m, e, flags, static_cast<std::integral_constant<int, 1> const*>(0));
}
inline bool u32regex_match(const std::string& s,
                        match_results<std::string::const_iterator>& m,
                        const u32regex& e,
                        match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_match(s.begin(), s.end(), m, e, flags, static_cast<std::integral_constant<int, 1> const*>(0));
}
#ifndef BOOST_NO_STD_WSTRING
inline bool u32regex_match(const std::wstring& s,
                        match_results<std::wstring::const_iterator>& m,
                        const u32regex& e,
                        match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_match(s.begin(), s.end(), m, e, flags, static_cast<std::integral_constant<int, sizeof(wchar_t)> const*>(0));
}
#endif
inline bool u32regex_match(const U_NAMESPACE_QUALIFIER UnicodeString& s,
                        match_results<const UChar*>& m,
                        const u32regex& e,
                        match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_match(s.getBuffer(), s.getBuffer() + s.length(), m, e, flags, static_cast<std::integral_constant<int, 2> const*>(0));
}

template <class BidiIterator>
inline bool u32regex_match(BidiIterator first, BidiIterator last,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   match_results<BidiIterator> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_match(first, last, m, e, flags, static_cast<std::integral_constant<int, sizeof(*first)> const*>(0));
}
inline bool u32regex_match(const UChar* p,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   match_results<const UChar*> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p+u_strlen(p), m, e, flags, static_cast<std::integral_constant<int, 2> const*>(0));
}
#if !BOOST_REGEX_UCHAR_IS_WCHAR_T && !defined(BOOST_NO_WREGEX)
inline bool u32regex_match(const wchar_t* p,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   match_results<const wchar_t*> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p+std::wcslen(p), m, e, flags, static_cast<std::integral_constant<int, sizeof(wchar_t)> const*>(0));
}
#endif
inline bool u32regex_match(const char* p,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   match_results<const char*> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p+std::strlen(p), m, e, flags, static_cast<std::integral_constant<int, 1> const*>(0));
}
inline bool u32regex_match(const unsigned char* p,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   match_results<const unsigned char*> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p+std::strlen((const char*)p), m, e, flags, static_cast<std::integral_constant<int, 1> const*>(0));
}
inline bool u32regex_match(const std::string& s,
                        const u32regex& e,
                        match_flag_type flags = match_default)
{
   match_results<std::string::const_iterator> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_match(s.begin(), s.end(), m, e, flags, static_cast<std::integral_constant<int, 1> const*>(0));
}
#ifndef BOOST_NO_STD_WSTRING
inline bool u32regex_match(const std::wstring& s,
                        const u32regex& e,
                        match_flag_type flags = match_default)
{
   match_results<std::wstring::const_iterator> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_match(s.begin(), s.end(), m, e, flags, static_cast<std::integral_constant<int, sizeof(wchar_t)> const*>(0));
}
#endif
inline bool u32regex_match(const U_NAMESPACE_QUALIFIER UnicodeString& s,
                        const u32regex& e,
                        match_flag_type flags = match_default)
{
   match_results<const UChar*> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_match(s.getBuffer(), s.getBuffer() + s.length(), m, e, flags, static_cast<std::integral_constant<int, 2> const*>(0));
}

namespace BOOST_REGEX_DETAIL_NS{
template <class BidiIterator, class Allocator>
inline bool do_regex_search(BidiIterator first, BidiIterator last,
                 match_results<BidiIterator, Allocator>& m,
                 const u32regex& e,
                 match_flag_type flags,
                 BidiIterator base,
                 std::integral_constant<int, 4> const*)
{
   return ::boost::regex_search(first, last, m, e, flags, base);
}
template <class BidiIterator, class Allocator>
bool do_regex_search(BidiIterator first, BidiIterator last,
                 match_results<BidiIterator, Allocator>& m,
                 const u32regex& e,
                 match_flag_type flags,
                 BidiIterator base,
                 std::integral_constant<int, 2> const*)
{
   typedef u16_to_u32_iterator<BidiIterator, UChar32> conv_type;
   typedef match_results<conv_type>                   match_type;

   match_type what;
   bool result = ::boost::regex_search(conv_type(first, first, last), conv_type(last, first, last), what, e, flags, conv_type(base));

   if(result) copy_results(m, what, e.get_named_subs());
   return result;
}
template <class BidiIterator, class Allocator>
bool do_regex_search(BidiIterator first, BidiIterator last,
                 match_results<BidiIterator, Allocator>& m,
                 const u32regex& e,
                 match_flag_type flags,
                 BidiIterator base,
                 std::integral_constant<int, 1> const*)
{
   typedef u8_to_u32_iterator<BidiIterator, UChar32>  conv_type;
   typedef match_results<conv_type>                   match_type;

   match_type what;
   bool result = ::boost::regex_search(conv_type(first, first, last), conv_type(last, first, last), what, e, flags, conv_type(base));

   if(result) copy_results(m, what, e.get_named_subs());
   return result;
}
}

template <class BidiIterator, class Allocator>
inline bool u32regex_search(BidiIterator first, BidiIterator last,
                 match_results<BidiIterator, Allocator>& m,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_search(first, last, m, e, flags, first, static_cast<std::integral_constant<int, sizeof(*first)> const*>(0));
}
template <class BidiIterator, class Allocator>
inline bool u32regex_search(BidiIterator first, BidiIterator last,
                 match_results<BidiIterator, Allocator>& m,
                 const u32regex& e,
                 match_flag_type flags,
                 BidiIterator base)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_search(first, last, m, e, flags, base, static_cast<std::integral_constant<int, sizeof(*first)> const*>(0));
}
inline bool u32regex_search(const UChar* p,
                 match_results<const UChar*>& m,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p+u_strlen(p), m, e, flags, p, static_cast<std::integral_constant<int, 2> const*>(0));
}
#if !BOOST_REGEX_UCHAR_IS_WCHAR_T && !defined(BOOST_NO_WREGEX)
inline bool u32regex_search(const wchar_t* p,
                 match_results<const wchar_t*>& m,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p+std::wcslen(p), m, e, flags, p, static_cast<std::integral_constant<int, sizeof(wchar_t)> const*>(0));
}
#endif
inline bool u32regex_search(const char* p,
                 match_results<const char*>& m,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p+std::strlen(p), m, e, flags, p, static_cast<std::integral_constant<int, 1> const*>(0));
}
inline bool u32regex_search(const unsigned char* p,
                 match_results<const unsigned char*>& m,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p+std::strlen((const char*)p), m, e, flags, p, static_cast<std::integral_constant<int, 1> const*>(0));
}
inline bool u32regex_search(const std::string& s,
                        match_results<std::string::const_iterator>& m,
                        const u32regex& e,
                        match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_search(s.begin(), s.end(), m, e, flags, s.begin(), static_cast<std::integral_constant<int, 1> const*>(0));
}
#ifndef BOOST_NO_STD_WSTRING
inline bool u32regex_search(const std::wstring& s,
                        match_results<std::wstring::const_iterator>& m,
                        const u32regex& e,
                        match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_search(s.begin(), s.end(), m, e, flags, s.begin(), static_cast<std::integral_constant<int, sizeof(wchar_t)> const*>(0));
}
#endif
inline bool u32regex_search(const U_NAMESPACE_QUALIFIER UnicodeString& s,
                        match_results<const UChar*>& m,
                        const u32regex& e,
                        match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::do_regex_search(s.getBuffer(), s.getBuffer() + s.length(), m, e, flags, s.getBuffer(), static_cast<std::integral_constant<int, 2> const*>(0));
}
template <class BidiIterator>
inline bool u32regex_search(BidiIterator first, BidiIterator last,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   match_results<BidiIterator> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_search(first, last, m, e, flags, first, static_cast<std::integral_constant<int, sizeof(*first)> const*>(0));
}
inline bool u32regex_search(const UChar* p,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   match_results<const UChar*> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p+u_strlen(p), m, e, flags, p, static_cast<std::integral_constant<int, 2> const*>(0));
}
#if !BOOST_REGEX_UCHAR_IS_WCHAR_T && !defined(BOOST_NO_WREGEX)
inline bool u32regex_search(const wchar_t* p,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   match_results<const wchar_t*> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p+std::wcslen(p), m, e, flags, p, static_cast<std::integral_constant<int, sizeof(wchar_t)> const*>(0));
}
#endif
inline bool u32regex_search(const char* p,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   match_results<const char*> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p+std::strlen(p), m, e, flags, p, static_cast<std::integral_constant<int, 1> const*>(0));
}
inline bool u32regex_search(const unsigned char* p,
                 const u32regex& e,
                 match_flag_type flags = match_default)
{
   match_results<const unsigned char*> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p+std::strlen((const char*)p), m, e, flags, p, static_cast<std::integral_constant<int, 1> const*>(0));
}
inline bool u32regex_search(const std::string& s,
                        const u32regex& e,
                        match_flag_type flags = match_default)
{
   match_results<std::string::const_iterator> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_search(s.begin(), s.end(), m, e, flags, s.begin(), static_cast<std::integral_constant<int, 1> const*>(0));
}
#ifndef BOOST_NO_STD_WSTRING
inline bool u32regex_search(const std::wstring& s,
                        const u32regex& e,
                        match_flag_type flags = match_default)
{
   match_results<std::wstring::const_iterator> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_search(s.begin(), s.end(), m, e, flags, s.begin(), static_cast<std::integral_constant<int, sizeof(wchar_t)> const*>(0));
}
#endif
inline bool u32regex_search(const U_NAMESPACE_QUALIFIER UnicodeString& s,
                        const u32regex& e,
                        match_flag_type flags = match_default)
{
   match_results<const UChar*> m;
   return BOOST_REGEX_DETAIL_NS::do_regex_search(s.getBuffer(), s.getBuffer() + s.length(), m, e, flags, s.getBuffer(), static_cast<std::integral_constant<int, 2> const*>(0));
}

namespace BOOST_REGEX_DETAIL_NS{
template <class I>
inline std::pair< boost::u8_to_u32_iterator<I>, boost::u8_to_u32_iterator<I> >
   make_utf32_seq(I i, I j, std::integral_constant<int, 1> const*)
{
   return std::pair< boost::u8_to_u32_iterator<I>, boost::u8_to_u32_iterator<I> >(boost::u8_to_u32_iterator<I>(i, i, j), boost::u8_to_u32_iterator<I>(j, i, j));
}
template <class I>
inline std::pair< boost::u16_to_u32_iterator<I>, boost::u16_to_u32_iterator<I> >
   make_utf32_seq(I i, I j, std::integral_constant<int, 2> const*)
{
   return std::pair< boost::u16_to_u32_iterator<I>, boost::u16_to_u32_iterator<I> >(boost::u16_to_u32_iterator<I>(i, i, j), boost::u16_to_u32_iterator<I>(j, i, j));
}
template <class I>
inline std::pair< I, I >
   make_utf32_seq(I i, I j, std::integral_constant<int, 4> const*)
{
   return std::pair< I, I >(i, j);
}
template <class charT>
inline std::pair< boost::u8_to_u32_iterator<const charT*>, boost::u8_to_u32_iterator<const charT*> >
   make_utf32_seq(const charT* p, std::integral_constant<int, 1> const*)
{
   std::size_t len = std::strlen((const char*)p);
   return std::pair< boost::u8_to_u32_iterator<const charT*>, boost::u8_to_u32_iterator<const charT*> >(boost::u8_to_u32_iterator<const charT*>(p, p, p+len), boost::u8_to_u32_iterator<const charT*>(p+len, p, p+len));
}
template <class charT>
inline std::pair< boost::u16_to_u32_iterator<const charT*>, boost::u16_to_u32_iterator<const charT*> >
   make_utf32_seq(const charT* p, std::integral_constant<int, 2> const*)
{
   std::size_t len = u_strlen((const UChar*)p);
   return std::pair< boost::u16_to_u32_iterator<const charT*>, boost::u16_to_u32_iterator<const charT*> >(boost::u16_to_u32_iterator<const charT*>(p, p, p + len), boost::u16_to_u32_iterator<const charT*>(p+len, p, p + len));
}
template <class charT>
inline std::pair< const charT*, const charT* >
   make_utf32_seq(const charT* p, std::integral_constant<int, 4> const*)
{
   return std::pair< const charT*, const charT* >(p, p+icu_regex_traits::length((UChar32 const*)p));
}
template <class OutputIterator>
inline OutputIterator make_utf32_out(OutputIterator o, std::integral_constant<int, 4> const*)
{
   return o;
}
template <class OutputIterator>
inline utf16_output_iterator<OutputIterator> make_utf32_out(OutputIterator o, std::integral_constant<int, 2> const*)
{
   return o;
}
template <class OutputIterator>
inline utf8_output_iterator<OutputIterator> make_utf32_out(OutputIterator o, std::integral_constant<int, 1> const*)
{
   return o;
}

template <class OutputIterator, class I1, class I2>
OutputIterator do_regex_replace(OutputIterator out,
                                 std::pair<I1, I1> const& in,
                                 const u32regex& e,
                                 const std::pair<I2, I2>& fmt,
                                 match_flag_type flags
                                 )
{

   std::vector<UChar32> f;
   f.assign(fmt.first, fmt.second);

   regex_iterator<I1, UChar32, icu_regex_traits> i(in.first, in.second, e, flags);
   regex_iterator<I1, UChar32, icu_regex_traits> j;
   if(i == j)
   {
      if(!(flags & regex_constants::format_no_copy))
         out = std::copy(in.first, in.second, out);
   }
   else
   {
      I1 last_m = in.first;
      while(i != j)
      {
         if(!(flags & regex_constants::format_no_copy))
            out = std::copy(i->prefix().first, i->prefix().second, out);
         if(!f.empty())
            out = ::boost::BOOST_REGEX_DETAIL_NS::regex_format_imp(out, *i, &*f.begin(), &*f.begin() + f.size(), flags, e.get_traits());
         else
            out = ::boost::BOOST_REGEX_DETAIL_NS::regex_format_imp(out, *i, static_cast<UChar32 const*>(0), static_cast<UChar32 const*>(0), flags, e.get_traits());
         last_m = (*i)[0].second;
         if(flags & regex_constants::format_first_only)
            break;
         ++i;
      }
      if(!(flags & regex_constants::format_no_copy))
         out = std::copy(last_m, in.second, out);
   }
   return out;
}
template <class BaseIterator>
inline const BaseIterator& extract_output_base(const BaseIterator& b)
{
   return b;
}
template <class BaseIterator>
inline BaseIterator extract_output_base(const utf8_output_iterator<BaseIterator>& b)
{
   return b.base();
}
template <class BaseIterator>
inline BaseIterator extract_output_base(const utf16_output_iterator<BaseIterator>& b)
{
   return b.base();
}
}

template <class OutputIterator, class BidirectionalIterator, class charT>
inline OutputIterator u32regex_replace(OutputIterator out,
                         BidirectionalIterator first,
                         BidirectionalIterator last,
                         const u32regex& e,
                         const charT* fmt,
                         match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::extract_output_base
    (
      BOOST_REGEX_DETAIL_NS::do_regex_replace(
         BOOST_REGEX_DETAIL_NS::make_utf32_out(out, static_cast<std::integral_constant<int, sizeof(*first)> const*>(0)),
         BOOST_REGEX_DETAIL_NS::make_utf32_seq(first, last, static_cast<std::integral_constant<int, sizeof(*first)> const*>(0)),
         e,
         BOOST_REGEX_DETAIL_NS::make_utf32_seq(fmt, static_cast<std::integral_constant<int, sizeof(*fmt)> const*>(0)),
         flags)
      );
}

template <class OutputIterator, class Iterator, class charT>
inline OutputIterator u32regex_replace(OutputIterator out,
                         Iterator first,
                         Iterator last,
                         const u32regex& e,
                         const std::basic_string<charT>& fmt,
                         match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::extract_output_base
    (
      BOOST_REGEX_DETAIL_NS::do_regex_replace(
         BOOST_REGEX_DETAIL_NS::make_utf32_out(out, static_cast<std::integral_constant<int, sizeof(*first)> const*>(0)),
         BOOST_REGEX_DETAIL_NS::make_utf32_seq(first, last, static_cast<std::integral_constant<int, sizeof(*first)> const*>(0)),
         e,
         BOOST_REGEX_DETAIL_NS::make_utf32_seq(fmt.begin(), fmt.end(), static_cast<std::integral_constant<int, sizeof(charT)> const*>(0)),
         flags)
      );
}

template <class OutputIterator, class Iterator>
inline OutputIterator u32regex_replace(OutputIterator out,
                         Iterator first,
                         Iterator last,
                         const u32regex& e,
                         const U_NAMESPACE_QUALIFIER UnicodeString& fmt,
                         match_flag_type flags = match_default)
{
   return BOOST_REGEX_DETAIL_NS::extract_output_base
   (
      BOOST_REGEX_DETAIL_NS::do_regex_replace(
         BOOST_REGEX_DETAIL_NS::make_utf32_out(out, static_cast<std::integral_constant<int, sizeof(*first)> const*>(0)),
         BOOST_REGEX_DETAIL_NS::make_utf32_seq(first, last, static_cast<std::integral_constant<int, sizeof(*first)> const*>(0)),
         e,
         BOOST_REGEX_DETAIL_NS::make_utf32_seq(fmt.getBuffer(), fmt.getBuffer() + fmt.length(), static_cast<std::integral_constant<int, 2> const*>(0)),
         flags)
      );
}

template <class charT>
std::basic_string<charT> u32regex_replace(const std::basic_string<charT>& s,
                         const u32regex& e,
                         const charT* fmt,
                         match_flag_type flags = match_default)
{
   std::basic_string<charT> result;
   BOOST_REGEX_DETAIL_NS::string_out_iterator<std::basic_string<charT> > i(result);
   u32regex_replace(i, s.begin(), s.end(), e, fmt, flags);
   return result;
}

template <class charT>
std::basic_string<charT> u32regex_replace(const std::basic_string<charT>& s,
                         const u32regex& e,
                         const std::basic_string<charT>& fmt,
                         match_flag_type flags = match_default)
{
   std::basic_string<charT> result;
   BOOST_REGEX_DETAIL_NS::string_out_iterator<std::basic_string<charT> > i(result);
   u32regex_replace(i, s.begin(), s.end(), e, fmt.c_str(), flags);
   return result;
}

namespace BOOST_REGEX_DETAIL_NS{

class unicode_string_out_iterator
{
   U_NAMESPACE_QUALIFIER UnicodeString* out;
public:
   unicode_string_out_iterator(U_NAMESPACE_QUALIFIER UnicodeString& s) : out(&s) {}
   unicode_string_out_iterator& operator++() { return *this; }
   unicode_string_out_iterator& operator++(int) { return *this; }
   unicode_string_out_iterator& operator*() { return *this; }
   unicode_string_out_iterator& operator=(UChar v)
   {
      *out += v;
      return *this;
   }
   typedef std::ptrdiff_t difference_type;
   typedef UChar value_type;
   typedef value_type* pointer;
   typedef value_type& reference;
   typedef std::output_iterator_tag iterator_category;
};

}

inline U_NAMESPACE_QUALIFIER UnicodeString u32regex_replace(const U_NAMESPACE_QUALIFIER UnicodeString& s,
                         const u32regex& e,
                         const UChar* fmt,
                         match_flag_type flags = match_default)
{
   U_NAMESPACE_QUALIFIER UnicodeString result;
   BOOST_REGEX_DETAIL_NS::unicode_string_out_iterator i(result);
   u32regex_replace(i, s.getBuffer(), s.getBuffer()+s.length(), e, fmt, flags);
   return result;
}

inline U_NAMESPACE_QUALIFIER UnicodeString u32regex_replace(const U_NAMESPACE_QUALIFIER UnicodeString& s,
                         const u32regex& e,
                         const U_NAMESPACE_QUALIFIER UnicodeString& fmt,
                         match_flag_type flags = match_default)
{
   U_NAMESPACE_QUALIFIER UnicodeString result;
   BOOST_REGEX_DETAIL_NS::unicode_string_out_iterator i(result);
   BOOST_REGEX_DETAIL_NS::do_regex_replace(
         BOOST_REGEX_DETAIL_NS::make_utf32_out(i, static_cast<std::integral_constant<int, 2> const*>(0)),
         BOOST_REGEX_DETAIL_NS::make_utf32_seq(s.getBuffer(), s.getBuffer()+s.length(), static_cast<std::integral_constant<int, 2> const*>(0)),
         e,
         BOOST_REGEX_DETAIL_NS::make_utf32_seq(fmt.getBuffer(), fmt.getBuffer() + fmt.length(), static_cast<std::integral_constant<int, 2> const*>(0)),
         flags);
   return result;
}

}

#ifdef BOOST_REGEX_MSVC
#pragma warning (pop)
#endif

#include <boost/regex/v5/u32regex_iterator.hpp>
#include <boost/regex/v5/u32regex_token_iterator.hpp>

#endif
