
#ifndef BOOST_REGEX_STATIC_MUTEX_HPP
#define BOOST_REGEX_STATIC_MUTEX_HPP

#include <boost/config.hpp>
#include <boost/regex/config.hpp>

#ifdef BOOST_HAS_PTHREADS
#include <pthread.h>
#endif

#if defined(BOOST_HAS_PTHREADS) && defined(PTHREAD_MUTEX_INITIALIZER)

namespace boost{

class static_mutex;

#define BOOST_STATIC_MUTEX_INIT { PTHREAD_MUTEX_INITIALIZER, }

class BOOST_REGEX_DECL scoped_static_mutex_lock
{
public:
   scoped_static_mutex_lock(static_mutex& mut, bool lk = true);
   ~scoped_static_mutex_lock();
   inline bool locked()const
   {
      return m_have_lock;
   }
   inline operator void const*()const
   {
      return locked() ? this : 0;
   }
   void lock();
   void unlock();
private:
   static_mutex& m_mutex;
   bool m_have_lock;
};

class static_mutex
{
public:
   typedef scoped_static_mutex_lock scoped_lock;
   pthread_mutex_t m_mutex;
};

}
#elif defined(BOOST_HAS_WINTHREADS)

#include <boost/cstdint.hpp>

namespace boost{

class BOOST_REGEX_DECL scoped_static_mutex_lock;

class static_mutex
{
public:
   typedef scoped_static_mutex_lock scoped_lock;
   boost::int32_t m_mutex;
};

#define BOOST_STATIC_MUTEX_INIT { 0, }

class BOOST_REGEX_DECL scoped_static_mutex_lock
{
public:
   scoped_static_mutex_lock(static_mutex& mut, bool lk = true);
   ~scoped_static_mutex_lock();
   operator void const*()const
   {
      return locked() ? this : 0;
   }
   bool locked()const
   {
      return m_have_lock;
   }
   void lock();
   void unlock();
private:
   static_mutex& m_mutex;
   bool m_have_lock;
   scoped_static_mutex_lock(const scoped_static_mutex_lock&);
   scoped_static_mutex_lock& operator=(const scoped_static_mutex_lock&);
};

}

#else

#define BOOST_REGEX_H1 <boost/thread/once.hpp>
#define BOOST_REGEX_H2 <boost/thread/recursive_mutex.hpp>
#define BOOST_REGEX_H3 <boost/thread/lock_types.hpp>
#include BOOST_REGEX_H1
#include BOOST_REGEX_H2
#include BOOST_REGEX_H3
#undef BOOST_REGEX_H1
#undef BOOST_REGEX_H2
#undef BOOST_REGEX_H3

namespace boost{

class BOOST_REGEX_DECL scoped_static_mutex_lock;
extern "C" BOOST_REGEX_DECL void boost_regex_free_static_mutex();

class BOOST_REGEX_DECL static_mutex
{
public:
   typedef scoped_static_mutex_lock scoped_lock;
   static void init();
   static boost::recursive_mutex* m_pmutex;
   static boost::once_flag m_once;
};

#define BOOST_STATIC_MUTEX_INIT {  }

class BOOST_REGEX_DECL scoped_static_mutex_lock
{
public:
   scoped_static_mutex_lock(static_mutex& mut, bool lk = true);
   ~scoped_static_mutex_lock();
   operator void const*()const;
   bool locked()const;
   void lock();
   void unlock();
private:
   boost::unique_lock<boost::recursive_mutex>* m_plock;
   bool m_have_lock;
};

inline scoped_static_mutex_lock::operator void const*()const
{
   return locked() ? this : 0;
}

inline bool scoped_static_mutex_lock::locked()const
{
   return m_have_lock;
}

}

#endif

#endif
