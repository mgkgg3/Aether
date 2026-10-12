#ifndef BOOST_ASSERT_SOURCE_LOCATION_HPP_INCLUDED
#define BOOST_ASSERT_SOURCE_LOCATION_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <iosfwd>
#include <string>
#include <cstdio>
#include <cstring>

#if defined(__cpp_lib_source_location) && __cpp_lib_source_location >= 201907L
# include <source_location>
#endif

namespace boost
{

struct source_location
{
private:

    char const * file_;
    char const * function_;
    boost::uint_least32_t line_;
    boost::uint_least32_t column_;

public:

    BOOST_CONSTEXPR source_location() BOOST_NOEXCEPT: file_( "" ), function_( "" ), line_( 0 ), column_( 0 )
    {
    }

    BOOST_CONSTEXPR source_location( char const * file, boost::uint_least32_t ln, char const * function, boost::uint_least32_t col = 0 ) BOOST_NOEXCEPT: file_( file ), function_( function ), line_( ln ), column_( col )
    {
    }

#if defined(__cpp_lib_source_location) && __cpp_lib_source_location >= 201907L

    BOOST_CONSTEXPR source_location( std::source_location const& loc ) BOOST_NOEXCEPT: file_( loc.file_name() ), function_( loc.function_name() ), line_( loc.line() ), column_( loc.column() )
    {
    }

#endif

    BOOST_CONSTEXPR char const * file_name() const BOOST_NOEXCEPT
    {
        return file_;
    }

    BOOST_CONSTEXPR char const * function_name() const BOOST_NOEXCEPT
    {
        return function_;
    }

    BOOST_CONSTEXPR boost::uint_least32_t line() const BOOST_NOEXCEPT
    {
        return line_;
    }

    BOOST_CONSTEXPR boost::uint_least32_t column() const BOOST_NOEXCEPT
    {
        return column_;
    }

#if defined(BOOST_MSVC)
# pragma warning( push )
# pragma warning( disable: 4996 )
#endif

#if ( defined(_MSC_VER) && _MSC_VER < 1900 ) || ( defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR) )
# define BOOST_ASSERT_SNPRINTF(buffer, format, arg) std::sprintf(buffer, format, arg)
#else
# define BOOST_ASSERT_SNPRINTF(buffer, format, arg) std::snprintf(buffer, sizeof(buffer)/sizeof(buffer[0]), format, arg)
#endif

    std::string to_string() const
    {
        unsigned long ln = line();

        if( ln == 0 )
        {
            return "(unknown source location)";
        }

        std::string r = file_name();

        char buffer[ 16 ];

        BOOST_ASSERT_SNPRINTF( buffer, ":%lu", ln );
        r += buffer;

        unsigned long co = column();

        if( co )
        {
            BOOST_ASSERT_SNPRINTF( buffer, ":%lu", co );
            r += buffer;
        }

        char const* fn = function_name();

        if( *fn != 0 )
        {
            r += " in function '";
            r += fn;
            r += '\'';
        }

        return r;
    }

#undef BOOST_ASSERT_SNPRINTF

#if defined(BOOST_MSVC)
# pragma warning( pop )
#endif

    inline friend bool operator==( source_location const& s1, source_location const& s2 ) BOOST_NOEXCEPT
    {
        return std::strcmp( s1.file_, s2.file_ ) == 0 && std::strcmp( s1.function_, s2.function_ ) == 0 && s1.line_ == s2.line_ && s1.column_ == s2.column_;
    }

    inline friend bool operator!=( source_location const& s1, source_location const& s2 ) BOOST_NOEXCEPT
    {
        return !( s1 == s2 );
    }
};

template<class E, class T> std::basic_ostream<E, T> & operator<<( std::basic_ostream<E, T> & os, source_location const & loc )
{
    os << loc.to_string();
    return os;
}

}

#if defined(BOOST_DISABLE_CURRENT_LOCATION)

# define BOOST_CURRENT_LOCATION ::boost::source_location()

#elif defined(BOOST_MSVC) && BOOST_MSVC >= 1935

# define BOOST_CURRENT_LOCATION ::boost::source_location(__builtin_FILE(), __builtin_LINE(), __builtin_FUNCSIG(), __builtin_COLUMN())

#elif defined(BOOST_MSVC) && BOOST_MSVC >= 1926

# define BOOST_CURRENT_LOCATION ::boost::source_location(__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION(), __builtin_COLUMN())

#elif defined(BOOST_MSVC)

# define BOOST_CURRENT_LOCATION_IMPL_1(x) BOOST_CURRENT_LOCATION_IMPL_2(x)
# define BOOST_CURRENT_LOCATION_IMPL_2(x) (x##0 / 10)

# define BOOST_CURRENT_LOCATION ::boost::source_location(__FILE__, BOOST_CURRENT_LOCATION_IMPL_1(__LINE__), "")

#elif defined(__cpp_lib_source_location) && __cpp_lib_source_location >= 201907L && !defined(__NVCC__)

# define BOOST_CURRENT_LOCATION ::boost::source_location(::std::source_location::current())

#elif defined(BOOST_CLANG) && BOOST_CLANG_VERSION >= 90000

# define BOOST_CURRENT_LOCATION ::boost::source_location(__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION(), __builtin_COLUMN())

#elif defined(BOOST_GCC) && BOOST_GCC >= 70000

# define BOOST_CURRENT_LOCATION ::boost::source_location(__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION())

#elif defined(BOOST_GCC) && BOOST_GCC >= 50000

# define BOOST_CURRENT_LOCATION ::boost::source_location(__FILE__, __LINE__, __PRETTY_FUNCTION__)

#else

# define BOOST_CURRENT_LOCATION ::boost::source_location(__FILE__, __LINE__, "")

#endif

#endif
