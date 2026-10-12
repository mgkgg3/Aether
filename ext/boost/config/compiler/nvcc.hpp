
#ifndef BOOST_COMPILER
#  define BOOST_COMPILER "NVIDIA CUDA C++ Compiler"
#endif

#if defined(__CUDACC_VER_MAJOR__) && defined(__CUDACC_VER_MINOR__) && defined(__CUDACC_VER_BUILD__)
#  define BOOST_CUDA_VERSION (__CUDACC_VER_MAJOR__ * 1000000 + __CUDACC_VER_MINOR__ * 10000 + __CUDACC_VER_BUILD__)
#else

#  define BOOST_CUDA_VERSION 7000000
#endif

#define BOOST_GPU_ENABLED __host__ __device__

#if !defined(__clang__) || defined(__NVCC__)

#if BOOST_CUDA_VERSION < 7050000
#   define BOOST_NO_CXX11_VARIADIC_TEMPLATES
#endif

#if (BOOST_CUDA_VERSION > 8000000) && (BOOST_CUDA_VERSION < 8010000)
#   define BOOST_NO_CXX11_VARIADIC_TEMPLATES
#endif

#if defined(_MSC_VER) && (BOOST_CUDA_VERSION < 9000000)
#  define BOOST_NO_CXX11_CONSTEXPR
#endif

#endif

#ifdef __CUDACC__

#if defined(_MSC_VER)
#  define BOOST_NO_CXX14_DIGIT_SEPARATORS
#  define BOOST_NO_CXX11_UNICODE_LITERALS
#endif

#if (BOOST_CUDA_VERSION >= 8000000) && (BOOST_CUDA_VERSION < 8010000)
#  define BOOST_NO_CXX11_NOEXCEPT
#endif

#if !defined(__cpp_nontype_template_parameter_auto) || (__cpp_nontype_template_parameter_auto < 201606)
#  define BOOST_NO_CXX17_AUTO_NONTYPE_TEMPLATE_PARAMS
#endif

#endif
