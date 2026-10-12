
#ifndef BOOST_CONFIG_REQUIRES_THREADS_HPP
#define BOOST_CONFIG_REQUIRES_THREADS_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_DISABLE_THREADS)

#if defined(__GNUC__) && ((__GNUC__ < 3) || (__GNUC_MINOR__ <= 3) || !defined(BOOST_STRICT_CONFIG))

#if defined(__sgi) || defined(__hpux)
#  error "Multi-threaded programs are not supported by gcc on HPUX or Irix (last checked with gcc 3.3)"
#endif

#endif

#  error "Threading support unavaliable: it has been explicitly disabled with BOOST_DISABLE_THREADS"

#elif !defined(BOOST_HAS_THREADS)

# if defined __COMO__

#   error "Compiler threading support is not turned on. Please set the correct command line options for threading: -D_MT (Windows) or -D_REENTRANT (Unix)"

#elif defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)

#ifdef _WIN32
#  error "Compiler threading support is not turned on. Please set the correct command line options for threading: either /MT /MTd /MD or /MDd"
#else
#   error "Compiler threading support is not turned on. Please set the correct command line options for threading: -openmp"
#endif

# elif defined __GNUC__

#   error "Compiler threading support is not turned on. Please set the correct command line options for threading: -pthread (Linux), -pthreads (Solaris) or -mthreads (Mingw32)"

#elif defined __sgi

#   error "Compiler threading support is not turned on. Please set the correct command line options for threading: -D_SGI_MP_SOURCE"

#elif defined __DECCXX

#   error "Compiler threading support is not turned on. Please set the correct command line options for threading: -pthread"

#elif defined BOOST_BORLANDC

#   error "Compiler threading support is not turned on. Please set the correct command line options for threading: -tWM"

#elif defined  __MWERKS__

#   error "Compiler threading support is not turned on. Please set the correct command line options for threading: either -runtime sm, -runtime smd, -runtime dm, or -runtime dmd"

#elif defined  __SUNPRO_CC

#   error "Compiler threading support is not turned on. Please set the correct command line options for threading: -mt"

#elif defined __HP_aCC

#   error "Compiler threading support is not turned on. Please set the correct command line options for threading: -mt"

#elif defined(__IBMCPP__)

#   error "Compiler threading support is not turned on. Please compile the code with the xlC_r compiler"

#elif defined _MSC_VER

#  error "Compiler threading support is not turned on. Please set the correct command line options for threading: either /MT /MTd /MD or /MDd"

#else

#  error "Compiler threading support is not turned on.  Please consult your compiler's documentation for the appropriate options to use"

#endif

#endif

#endif
