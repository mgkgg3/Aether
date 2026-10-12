
#if defined __CUDACC__

#   include "boost/config/compiler/nvcc.hpp"

#endif

#if defined(__GCCXML__)

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/gcc_xml.hpp"

#elif defined(_CRAYC)

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/cray.hpp"

#elif defined __COMO__

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/comeau.hpp"

#elif defined(__PATHSCALE__) && (__PATHCC__ >= 4)

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/pathscale.hpp"

#elif defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/intel.hpp"

#elif defined __clang__ && !defined(__ibmxl__) && !defined(__CODEGEARC__)

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/clang.hpp"

#elif defined __DMC__

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/digitalmars.hpp"

#elif defined __DCC__

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/diab.hpp"

#elif defined(__PGI)

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/pgi.hpp"

# elif defined(__GNUC__) && !defined(__ibmxl__)

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/gcc.hpp"

#elif defined __KCC

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/kai.hpp"

#elif defined __sgi

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/sgi_mipspro.hpp"

#elif defined __DECCXX

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/compaq_cxx.hpp"

#elif defined __ghs

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/greenhills.hpp"

#elif defined __CODEGEARC__

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/codegear.hpp"

#elif defined __BORLANDC__

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/borland.hpp"

#elif defined  __MWERKS__

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/metrowerks.hpp"

#elif defined  __SUNPRO_CC

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/sunpro_cc.hpp"

#elif defined __HP_aCC

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/hp_acc.hpp"

#elif defined(__MRC__) || defined(__SC__)

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/mpw.hpp"

#elif defined(__IBMCPP__) && defined(__COMPILER_VER__) && defined(__MVS__)

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/xlcpp_zos.hpp"

#elif defined(__ibmxl__)

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/xlcpp.hpp"

#elif defined(__IBMCPP__)

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/vacpp.hpp"

#elif defined _MSC_VER

#   define BOOST_COMPILER_CONFIG "boost/config/compiler/visualc.hpp"

#elif defined (BOOST_ASSERT_CONFIG)

#  error "Unknown compiler - please configure (http://www.boost.org/libs/config/config.htm#configuring) and report the results to the main boost mailing list (http://www.boost.org/more/mailing_lists.htm#main)"

#endif

#if 0

#include <boost/config/compiler/gcc_xml.hpp>
#include <boost/config/compiler/cray.hpp>
#include <boost/config/compiler/comeau.hpp>
#include <boost/config/compiler/pathscale.hpp>
#include <boost/config/compiler/intel.hpp>
#include <boost/config/compiler/clang.hpp>
#include <boost/config/compiler/digitalmars.hpp>
#include <boost/config/compiler/gcc.hpp>
#include <boost/config/compiler/kai.hpp>
#include <boost/config/compiler/sgi_mipspro.hpp>
#include <boost/config/compiler/compaq_cxx.hpp>
#include <boost/config/compiler/greenhills.hpp>
#include <boost/config/compiler/codegear.hpp>
#include <boost/config/compiler/borland.hpp>
#include <boost/config/compiler/metrowerks.hpp>
#include <boost/config/compiler/sunpro_cc.hpp>
#include <boost/config/compiler/hp_acc.hpp>
#include <boost/config/compiler/mpw.hpp>
#include <boost/config/compiler/xlcpp_zos.hpp>
#include <boost/config/compiler/xlcpp.hpp>
#include <boost/config/compiler/vacpp.hpp>
#include <boost/config/compiler/pgi.hpp>
#include <boost/config/compiler/visualc.hpp>

#endif
