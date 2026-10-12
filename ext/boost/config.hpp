
#ifndef BOOST_CONFIG_HPP
#define BOOST_CONFIG_HPP

#if !defined(BOOST_USER_CONFIG) && !defined(BOOST_NO_USER_CONFIG)
#  define BOOST_USER_CONFIG <boost/config/user.hpp>
#if 0

#  include <boost/config/user.hpp>
#endif
#endif

#ifdef BOOST_USER_CONFIG
#  include BOOST_USER_CONFIG
#endif

#if !defined(BOOST_COMPILER_CONFIG) && !defined(BOOST_NO_COMPILER_CONFIG) && !defined(BOOST_NO_CONFIG)
#  include <boost/config/detail/select_compiler_config.hpp>
#endif

#ifdef BOOST_COMPILER_CONFIG
#  include BOOST_COMPILER_CONFIG
#endif

#if !defined(BOOST_STDLIB_CONFIG) && !defined(BOOST_NO_STDLIB_CONFIG) && !defined(BOOST_NO_CONFIG) && defined(__cplusplus)
#  include <boost/config/detail/select_stdlib_config.hpp>
#endif

#ifdef BOOST_STDLIB_CONFIG
#  include BOOST_STDLIB_CONFIG
#endif

#if !defined(BOOST_PLATFORM_CONFIG) && !defined(BOOST_NO_PLATFORM_CONFIG) && !defined(BOOST_NO_CONFIG)
#  include <boost/config/detail/select_platform_config.hpp>
#endif

#ifdef BOOST_PLATFORM_CONFIG
#  include BOOST_PLATFORM_CONFIG
#endif

#include <boost/config/detail/suffix.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#endif
