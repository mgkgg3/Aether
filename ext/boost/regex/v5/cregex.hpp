
#ifndef BOOST_RE_CREGEX_HPP_INCLUDED
#define BOOST_RE_CREGEX_HPP_INCLUDED

#ifndef BOOST_REGEX_CONFIG_HPP
#include <boost/regex/config.hpp>
#endif
#include <boost/regex/v5/match_flags.hpp>
#include <boost/regex/v5/error_type.hpp>

#ifndef BOOST_REGEX_STANDALONE
#if !defined(BOOST_REGEX_NO_LIB) && !defined(BOOST_REGEX_SOURCE) && !defined(BOOST_ALL_NO_LIB) && defined(__cplusplus)
#  define BOOST_LIB_NAME boost_regex
#  if defined(BOOST_REGEX_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)
#     define BOOST_DYN_LINK
#  endif
#  ifdef BOOST_REGEX_DIAG
#     define BOOST_LIB_DIAGNOSTIC
#  endif
#  include <boost/config/auto_link.hpp>
#endif
#endif

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#ifdef __cplusplus
namespace boost{
extern "C" {
#endif

#if defined(__cplusplus)
typedef std::ptrdiff_t regoff_t;
typedef std::size_t regsize_t;
#else
typedef ptrdiff_t regoff_t;
typedef size_t regsize_t;
#endif

typedef struct
{
   unsigned int re_magic;
#ifdef __cplusplus
   std::size_t  re_nsub;
#else
   size_t re_nsub;
#endif
   const char*  re_endp;
   void* guts;
   match_flag_type eflags;
} regex_tA;

#ifndef BOOST_NO_WREGEX
typedef struct
{
   unsigned int re_magic;
#ifdef __cplusplus
   std::size_t  re_nsub;
#else
   size_t re_nsub;
#endif
   const wchar_t* re_endp;
   void* guts;
   match_flag_type eflags;
} regex_tW;
#endif

typedef struct
{
   regoff_t rm_so;
   regoff_t rm_eo;
} regmatch_t;

typedef enum{
   REG_BASIC = 0000,
   REG_EXTENDED = 0001,
   REG_ICASE = 0002,
   REG_NOSUB = 0004,
   REG_NEWLINE = 0010,
   REG_NOSPEC = 0020,
   REG_PEND = 0040,
   REG_DUMP = 0200,
   REG_NOCOLLATE = 0400,
   REG_ESCAPE_IN_LISTS = 01000,
   REG_NEWLINE_ALT = 02000,
   REG_PERLEX = 04000,

   REG_PERL = REG_EXTENDED | REG_NOCOLLATE | REG_ESCAPE_IN_LISTS | REG_PERLEX,
   REG_AWK = REG_EXTENDED | REG_ESCAPE_IN_LISTS,
   REG_GREP = REG_BASIC | REG_NEWLINE_ALT,
   REG_EGREP = REG_EXTENDED | REG_NEWLINE_ALT,

   REG_ASSERT = 15,
   REG_INVARG = 16,
   REG_ATOI = 255,
   REG_ITOA = 0400
} reg_comp_flags;

typedef enum{
   REG_NOTBOL =    00001,
   REG_NOTEOL =    00002,
   REG_STARTEND =  00004
} reg_exec_flags;

typedef unsigned reg_error_t;
typedef reg_error_t reg_errcode_t;

static const reg_error_t REG_NOERROR = 0;
static const reg_error_t REG_NOMATCH = 1;

static const reg_error_t REG_BADPAT = 2;
static const reg_error_t REG_ECOLLATE = 3;
static const reg_error_t REG_ECTYPE = 4;
static const reg_error_t REG_EESCAPE = 5;
static const reg_error_t REG_ESUBREG = 6;
static const reg_error_t REG_EBRACK = 7;
static const reg_error_t REG_EPAREN = 8;
static const reg_error_t REG_EBRACE = 9;
static const reg_error_t REG_BADBR = 10;
static const reg_error_t REG_ERANGE = 11;
static const reg_error_t REG_ESPACE = 12;
static const reg_error_t REG_BADRPT = 13;
static const reg_error_t REG_EEND = 14;
static const reg_error_t REG_ESIZE = 15;
static const reg_error_t REG_ERPAREN = 8;
static const reg_error_t REG_EMPTY = 17;
static const reg_error_t REG_E_MEMORY = 15;
static const reg_error_t REG_ECOMPLEXITY = 18;
static const reg_error_t REG_ESTACK = 19;
static const reg_error_t REG_E_PERL = 20;
static const reg_error_t REG_E_UNKNOWN = 21;
static const reg_error_t REG_ENOSYS = 21;

BOOST_REGEX_DECL int BOOST_REGEX_CCALL regcompA(regex_tA*, const char*, int);
BOOST_REGEX_DECL regsize_t BOOST_REGEX_CCALL regerrorA(int, const regex_tA*, char*, regsize_t);
BOOST_REGEX_DECL int BOOST_REGEX_CCALL regexecA(const regex_tA*, const char*, regsize_t, regmatch_t*, int);
BOOST_REGEX_DECL void BOOST_REGEX_CCALL regfreeA(regex_tA*);

#ifndef BOOST_NO_WREGEX
BOOST_REGEX_DECL int BOOST_REGEX_CCALL regcompW(regex_tW*, const wchar_t*, int);
BOOST_REGEX_DECL regsize_t BOOST_REGEX_CCALL regerrorW(int, const regex_tW*, wchar_t*, regsize_t);
BOOST_REGEX_DECL int BOOST_REGEX_CCALL regexecW(const regex_tW*, const wchar_t*, regsize_t, regmatch_t*, int);
BOOST_REGEX_DECL void BOOST_REGEX_CCALL regfreeW(regex_tW*);
#endif

#ifdef UNICODE
#define regcomp regcompW
#define regerror regerrorW
#define regexec regexecW
#define regfree regfreeW
#define regex_t regex_tW
#else
#define regcomp regcompA
#define regerror regerrorA
#define regexec regexecA
#define regfree regfreeA
#define regex_t regex_tA
#endif

#ifdef __cplusplus
}
}
#endif

#endif
