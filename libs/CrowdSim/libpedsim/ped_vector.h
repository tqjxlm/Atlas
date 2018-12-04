//
// pedsim - A microscopic pedestrian simulation system.
// Copyright (c) by Christian Gloor
//

#ifndef _ped_vector_h_
#define _ped_vector_h_ 1

// disable warnings on 255 char debug symbols
#pragma warning (disable : 4786)
// disable warnings on extern before template instantiation
#pragma warning (disable : 4231)

#ifdef _WIN32
#ifdef _DLL
#    define LIBEXPORT __declspec(dllexport)
#    define EXPIMP_TEMPLATE
#else
#    define LIBEXPORT __declspec(dllimport)
#    define EXPIMP_TEMPLATE extern
#endif
#else
#    define LIBEXPORT
#    define EXPIMP_TEMPLATE
#endif

#include <string>
#include <math.h>

namespace Ped
{
/// Vector helper class. This is basically a struct with some related functions attached.
/// x, y, and z are public, so that they can be accessed easily.
class LIBEXPORT  Tvector
{
public:
  /// Default constructor, which makes sure that all the values are set to 0.
  /// \date    2012-01-16
  Tvector():
    x(0), y(0), z(0)
  {
  }

  // Initializing constructor
  Tvector(double px, double py, double pz = 0):
    x(px), y(py), z(pz)
  {
  }

  // Methods

  /// Returns the length of the vector.
  /// \return the length
  double  length() const
  {
    if ((x == 0) && (y == 0) && (z == 0)) { return 0; }

    return sqrt(lengthSquared());
  }

  /// Returns the length of the vector squared. This is faster than the real length.
  /// \return the length squared
  double  lengthSquared() const
  {
    return x * x + y * y + z * z;
  }

  std::string  to_string() const
  {
    return std::to_string(x) + "/" + std::to_string(y) + "/" + std::to_string(z);
  }

  /// Normalizes the vector to a length of 1.
  /// \date    2010-02-12
  void  normalize()
  {
    double  len = length();

    // null vectors cannot be normalized
    if (len == 0) { return; }

    x /= len;
    y /= len;
    z /= len;
  }

  /// Normalizes the vector to a length of 1.
  /// \date    2013-08-02
  Ped::Tvector  normalized() const
  {
    double  len = length();

    // null vectors cannot be normalized
    if (len == 0) { return Ped::Tvector(); }

    return Ped::Tvector(x / len, y / len, z / len);
  }

  /// Vector scalar product helper: calculates the scalar product of two vectors.
  /// \date    2012-01-14
  /// \return  The scalar product.
  /// \param   &a The first vector
  /// \param   &b The second vector
  static double  scalar(const Ped::Tvector &a, const Ped::Tvector &b)
  {
    return acos(dotProduct(a, b) / (a.length() * b.length()));
  }

  /// Vector dot product helper: calculates the dot product of two vectors.
  /// \date    2012-01-14
  /// \return  The dot product.
  /// \param   &a The first vector
  /// \param   &b The second vector
  static double  dotProduct(const Ped::Tvector &a, const Ped::Tvector &b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }

  /// Calculates the cross product of two vectors.
  /// \date    2010-02-12
  /// \param   &a The first vector
  /// \param   &b The second vector
  static Ped::Tvector  crossProduct(const Ped::Tvector &a, const Ped::Tvector &b)
  {
    return Ped::Tvector(
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x);
  }

  /// Scales this vector by a given factor in each dimension.
  /// \date    2013-08-02
  /// \param   factor The scalar value to multiply with.
  void  scale(double factor)
  {
    x *= factor;
    y *= factor;
    z *= factor;
  }

  /// Returns a copy of this vector which is multiplied in each dimension by a given factor.
  /// \date    2013-07-16
  /// \return  The scaled vector.
  /// \param   factor The scalar value to multiply with.
  Ped::Tvector  scaled(double factor) const
  {
    return Ped::Tvector(factor * x, factor * y, factor * z);
  }

  // \warning: This is in 2D only!
  Ped::Tvector  leftNormalVector() const
  {
    return Ped::Tvector(-y, x);
  }

  // \warning: This is in 2D only!
  Ped::Tvector  rightNormalVector() const
  {
    return Ped::Tvector(y, -x);
  }

  double  polarRadius() const
  {
    return length();
  }

  // \warning: This is in 2D only!
  double  polarAngle() const
  {
    return atan2(y, x);
  }

  // \warning: This is in 2D only!
  double        angleTo(const Tvector &other) const;


  Ped::Tvector  operator+(const Tvector &other) const
  {
    return Ped::Tvector(
      x + other.x,
      y + other.y,
      z + other.z);
  }

  Ped::Tvector  operator-(const Tvector &other) const
  {
    return Ped::Tvector(
      x - other.x,
      y - other.y,
      z - other.z);
  }

  Ped::Tvector  operator*(double factor) const
  {
    return scaled(factor);
  }

  Ped::Tvector  operator/(double divisor) const
  {
    return scaled(1 / divisor);
  }

  Ped::Tvector & operator+=(const Tvector &vectorIn)
  {
    x += vectorIn.x;
    y += vectorIn.y;
    z += vectorIn.z;

    return *this;
  }

  Ped::Tvector & operator-=(const Tvector &vectorIn)
  {
    x -= vectorIn.x;
    y -= vectorIn.y;
    z -= vectorIn.z;

    return *this;
  }

  Ped::Tvector & operator*=(double factor)
  {
    scale(factor);

    return *this;
  }

  Ped::Tvector & operator*=(const Tvector &vectorIn)
  {
    x *= vectorIn.x;
    y *= vectorIn.y;
    z *= vectorIn.z;

    return *this;
  }

  Ped::Tvector & operator/=(double divisor)
  {
    scale(1 / divisor);

    return *this;
  }

  /// Calculates the itnersection point of two lines, defined by Ped::Tvectors p0, p1, and p2, p3 respectively.
  /// Based on an algorithm in Andre LeMothe's "Tricks of the Windows Game Programming Gurus"
  /// \return bool True if there is an intersection, false otherwise
  /// \return *intersection If the supplied pointer to a Ped::Tvector is not NULL, it will contain the intersection point, if there is an intersection.
  static bool  lineIntersection(const Ped::Tvector &p0, const Ped::Tvector &p1, const Ped::Tvector &p2, const Ped::Tvector &p3, Ped::Tvector *intersection)
  {
    Ped::Tvector  s1(p1.x - p0.x, p1.y - p0.y);
    Ped::Tvector  s2(p3.x - p2.x, p3.y - p2.y);

    double  s = (-s1.y * (p0.x - p2.x) + s1.x * (p0.y - p2.y)) / (-s2.x * s1.y + s1.x * s2.y);
    double  t = (s2.x * (p0.y - p2.y) - s2.y * (p0.x - p2.x)) / (-s2.x * s1.y + s1.x * s2.y);

    if ((s >= 0) && (s <= 1) && (t >= 0) && (t <= 1)) // intersection
    {
      if (intersection != NULL)
      {
        intersection->x = p0.x + (t * s1.x);
        intersection->y = p0.y + (t * s1.y);
      }

      return true;
		}

    return false;   // No intersection
  }

  /// Rotates a vector.
  /// Rotates around 0,0 in 2 dimensions only (z unchanged)v
  /// \param theta in rad
  void  rotate(double theta)    // theta in rad.
  {
    double  xt = x * cos(theta) - y * sin(theta);
    double  yt = x * sin(theta) + y * cos(theta);

    x = xt;
    y = yt;
  }

  /// Rotates a vector.
  /// Rotates around 0,0 in 2 dimensions only (z set to 0.0)
  /// \param theta in rad
  Ped::Tvector  rotated(double theta) const    // theta in rad
  {
    return Ped::Tvector(x * cos(theta) - y * sin(theta), x * sin(theta) + y * cos(theta));   // let's hope the compiler reuses sin/cos
  }

  // Attributes
  double  x;
  double  y;
  double  z;
};
}

bool          operator==(const Ped::Tvector &vector1In, const Ped::Tvector &vector2In);

bool          operator!=(const Ped::Tvector &vector1In, const Ped::Tvector &vector2In);

Ped::Tvector  operator-(const Ped::Tvector &vectorIn);

Ped::Tvector  operator*(double factor, const Ped::Tvector &vector);


#endif
