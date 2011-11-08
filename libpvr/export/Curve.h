//-*-c++-*--------------------------------------------------------------------//

/*! \file Curve.h
  Contains the Curve class and related functions.
 */

//----------------------------------------------------------------------------//

#ifndef __INCLUDED_PVR_CURVE_H__
#define __INCLUDED_PVR_CURVE_H__

//----------------------------------------------------------------------------//
// Includes
//----------------------------------------------------------------------------//

// System headers

#include <cmath>
#include <vector>

// Library headers

#include <boost/lexical_cast.hpp>

#include <OpenEXR/ImathQuat.h>
#include <OpenEXR/ImathVec.h>

// Project headers

#include "pvr/AttrTable.h"
#include "pvr/Exception.h"
#include "pvr/Types.h"

//----------------------------------------------------------------------------//
// Namespaces
//----------------------------------------------------------------------------//

namespace pvr {
namespace Util {

//----------------------------------------------------------------------------//
// Curve
//----------------------------------------------------------------------------//

/*! \brief A simple function curve class. 

  The Curve class is most commonly used to pass time-varying parameters. 
  The class itself puts no restriction on the time values, but when feeding
  attributes that vary over the course of a single frame, the time used
  is usually [0.0, 1.0], corresponding to the shutter open and close times.

  The class can also be used for generic lookup curves.
 */

//----------------------------------------------------------------------------//

template <typename T>
class Curve
{
public:

  // Typedefs ------------------------------------------------------------------

  DECLARE_SMART_PTRS(Curve);

  typedef std::pair<float, T> Sample;
  typedef std::vector<Sample> SampleVec;

  // Constructor, destructor, factory ------------------------------------------

  Curve()
  { }
  Curve(const T &initialValue)
  { addSample(0.0f, initialValue); }

  //! Factory creation function. Always use this when creating objects
  //! that need lifespan management.
  static Ptr create()
  { return Ptr(new Curve); }

  // Main methods --------------------------------------------------------------

  //! Adds a sample point to the curve.
  //! \param t Sample position
  //! \param value Sample value
  void addSample(const float t, const T &value);

  //! Interpolates a value from the curve.
  //! \param t Position along curve
  T interpolate(const float t) const;

  //! Returns number of samples in curve
  size_t numSamples() const
  { return m_samples.size(); }
  
private:
  
  // Structs -------------------------------------------------------------------

  //! Used when looking up values in the m_samples vector.
  struct CheckTGreaterThan : 
    public std::unary_function<std::pair<float, T>, bool>
  {
    CheckTGreaterThan(float match)
      : m_match(match)
    { }
    bool operator()(const std::pair<float, T> &test)
    {
      return test.first > m_match;
    }
  private:
    float m_match;
  };

  // Utility methods -----------------------------------------------------------

  //! The default return value is used when no sample points are available.
  //! This defaults to zero, but for some types (for example Quaternion), 
  //! We need more arguments to the constructor. In these cases the method
  //! is specialized for the given T type.
  T defaultReturnValue() const
  { return T(0); }

  //! The default implementation for linear interpolation. Works for all classes
  //! for which Imath::lerp is implemented (i.e float/double, V2f, V3f).
  //! For other types this method needs to be specialized.
  T lerp(const Sample &lower, const Sample &upper, const float t) const
  { return Imath::lerp(lower.second, upper.second, t); }

  // Private data members ------------------------------------------------------

  //! Stores the samples that define the curve.
  SampleVec m_samples;

};

//----------------------------------------------------------------------------//

typedef Curve<float>  FloatCurve;
typedef Curve<Color>  TransmittanceFunction;
typedef Curve<Vector> VectorCurve;
typedef Curve<Quat>   QuatCurve;

//----------------------------------------------------------------------------//
// Template implementations
//----------------------------------------------------------------------------//

template <typename T>
void Util::Curve<T>::addSample(const float t, const T &value)
{
  using namespace std;
  // Find the first sample location that is greater than the interpolation
  // position
  typename SampleVec::iterator i = 
    find_if(m_samples.begin(), m_samples.end(), CheckTGreaterThan(t));
  // If we get something other than end() back then we insert the new
  // sample before that. If there wasn't a larger value we add this sample
  // to the end of the vector.
  if (i != m_samples.end()) {
    m_samples.insert(i, make_pair(t, value));
  } else {
    m_samples.push_back(make_pair(t, value));
  }
}

//----------------------------------------------------------------------------//

template <typename T>
T Curve<T>::interpolate(const float t) const
{
  using namespace std;
  // If there are no samples, return zero
  if (m_samples.size() == 0) {
    return defaultReturnValue();
  }
  // Find the first sample location that is greater than the interpolation
  // position
  typename SampleVec::const_iterator i = 
    find_if(m_samples.begin(), m_samples.end(), CheckTGreaterThan(t));
  // If we get end() back then there was no sample larger, so we return the
  // last value. If we got the first value then there is only one value and
  // we return that.
  if (i == m_samples.end()) {
    return m_samples.back().second;
  } else if (i == m_samples.begin()) {
    return m_samples.front().second;
  } 
  // Interpolate between the nearest two samples.
  const Sample &upper = *i;
  const Sample &lower = *(--i);
  const float interpT = Imath::lerpfactor(t, lower.first, upper.first);
  return lerp(lower, upper, interpT);
}

//----------------------------------------------------------------------------//
// Template specializations
//----------------------------------------------------------------------------//

//! Template specialization for quaternions. The default constructor gives
//! us the appropriate default orientation.
template <>
inline Quat Curve<Quat>::defaultReturnValue() const
{
  return Quat();
}

//----------------------------------------------------------------------------//

//! Template specialization for quaternions. We use slerp instead of lerp.
template <>
inline Quat Curve<Quat>::lerp(const Curve<Quat>::Sample &lower, 
                              const Curve<Quat>::Sample &upper, 
                              const float t) const
{
  return Imath::slerp(lower.second, upper.second, static_cast<double>(t));
}

//----------------------------------------------------------------------------//

} // namespace Util
} // namespace pvr

//----------------------------------------------------------------------------//

#endif // Include guard

//----------------------------------------------------------------------------//