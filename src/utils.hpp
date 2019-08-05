#ifndef MACH_UTILS
#define MACH_UTILS

#include <exception>
#include <iostream>
#include "mfem.hpp"

namespace mach
{
/// Handles (high-level) exceptions in both serial and parallel
class MachException: public std::exception
{
public:
   /// Class constructor.
   /// \param[in] err_msg - the error message to be printed
   MachException(std::string err_msg) : error_msg(err_msg) {}
   
   /// Overwrites inherieted member that returns a c-string.
   virtual const char* what() const noexcept
   {
      return error_msg.c_str();
   }

   /// Use this to print the message; prints only on root for parallel runs.
   void print_message()
   {
      // TODO: handle parallel runs!!!
      std::cerr << error_msg << std::endl;
   }
protected:
   /// message printed to std::cerr
   std::string error_msg;
};

/// performs the Hadamard (elementwise) product: `v(i) = v1(i)*v2(i)`
void multiplyElementwise(const mfem::Vector &v1, const mfem::Vector &v2,
                         mfem::Vector &v);

/// performs an elementwise division: `v(i) = v1(i)/v2(i)`
void divideElementwise(const mfem::Vector &v1, const mfem::Vector &v2,
                       mfem::Vector &v);

/// performs elementwise inversion: `y(i) = 1/x(i)`
void invertElementwise(const mfem::Vector &x, mfem::Vector &y);

/// for performing loop unrolling of dot-products using meta-programming
/// \tparam xdouble - `double` or `adept::adouble`
/// \tparam dim - number of dimensions for array
/// This was adapted from http://www.informit.com/articles/article.aspx?p=30667&seqNum=7
template <typename xdouble, int dim>
class DotProduct {
  public:
    static xdouble result(const xdouble *a, const xdouble *b)
    {
        return *a * *b  +  DotProduct<xdouble,dim-1>::result(a+1,b+1);
    }
};

// partial specialization as end criteria
template <typename xdouble>
class DotProduct<xdouble,1> {
  public:
    static xdouble result(const xdouble *a, const xdouble *b)
    {
        return *a * *b;
    }
};

/// dot product of two arrays that uses an unrolled loop
/// \param[in] a - first vector involved in product
/// \param[in] b - second vector involved in product
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of array dimensions
template <typename xdouble, int dim>
inline xdouble dot(const xdouble *a, const xdouble *b)
{
    return DotProduct<xdouble,dim>::result(a,b);
}

/// Handles print in parallel case
template<typename _CharT, typename _Traits>

class basic_oblackholestream
    : virtual public std::basic_ostream<_CharT, _Traits>
{
public:   
  /// called when rank is not root, prints nothing 
    explicit basic_oblackholestream() : std::basic_ostream<_CharT, _Traits>(NULL) {}
}; // end class basic_oblackholestream

using oblackholestream = basic_oblackholestream<char,std::char_traits<char> >;
static oblackholestream obj;

static std::ostream *getOutStream(int rank) 
{
   /// print only on root
   if (0==rank)
   {
      return &std::cout;
   }
   else
   {
      return &obj;
   }
}
} // namespace mach

#endif 
