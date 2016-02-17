//----------------------------------------------------------------------------//

/*
    This file is part of PVR. Copyright (C) 2012 Magnus Wrenninge

    PVR is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PVR is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//----------------------------------------------------------------------------//

/*! \file Image.cpp
  Contains implementations of Image class and related functions.
 */

//----------------------------------------------------------------------------//
// Includes
//----------------------------------------------------------------------------//

// Header include

#include "pvr/Image.h"


// Library includes

#include <OpenImageIO/color.h>

// Project includes

#include "pvr/Log.h"

//----------------------------------------------------------------------------//
// Namespaces
//----------------------------------------------------------------------------//

using namespace pvr::Util;
using namespace OpenImageIO;

//----------------------------------------------------------------------------//

namespace pvr {
namespace Render {

//----------------------------------------------------------------------------//
// Image
//----------------------------------------------------------------------------//

Image::Ptr Image::create()
{ 
  return Ptr(new Image); 
}

//----------------------------------------------------------------------------//

Image::Ptr Image::clone()
{ 
  return Ptr(new Image(*this)); 
}

//----------------------------------------------------------------------------//

void Image::setSize(const size_t width, const size_t height)
{
  using namespace OpenImageIO;
  
  ImageSpec spec(width, height, 4, TypeDesc::FLOAT);
  spec.attribute("oiio:ColorSpace", "Linear");
  m_buf.reset(spec);
}
  
//----------------------------------------------------------------------------//

void Image::setPixel(const size_t x, const size_t y, const Color &value)
{
  assert(static_cast<int>(x) <= m_buf.xmax() && "x out of range");
  assert(static_cast<int>(y) <= m_buf.ymax() && "y out of range");
  m_buf.setpixel(x, y, &value.x, 3);
}
  
//----------------------------------------------------------------------------//

void Image::setPixelAlpha(const size_t x, const size_t y, const float value)
{
  float col[4];
  m_buf.getpixel(x, y, col, 3);
  col[3] = value;
  m_buf.setpixel(x, y, col, 4);
}
  
//----------------------------------------------------------------------------//

Imath::V2i Image::size() const
{ 
  return Imath::V2i(m_buf.xmax() + 1, m_buf.ymax() + 1); 
}

//----------------------------------------------------------------------------//

Color Image::pixel(const size_t x, const size_t y) const
{
  Color col;
  m_buf.getpixel(x, y, &col.x, 3);
  return col;
}

//----------------------------------------------------------------------------//

float Image::pixelAlpha(const size_t x, const size_t y) const
{
  float value[4];
  m_buf.getpixel(x, y, value, 4);
  return value[3];
}

//----------------------------------------------------------------------------//

void Image::write(const std::string &filename, Channels channels) const
{
  Log::print("Writing image: " + filename);

  size_t len = filename.size();
  if (filename.substr(len-3,len) != "exr") {
    
    ImageSpec spec = m_buf.spec();
    spec.attribute("oiio:ColorSpace", "sRGB");

    ImageBuf buf("", spec);
    
    for (int j = 0, ymax = buf.ymax(); j <= ymax; ++j) {
      int invertedJ = buf.ymax() - j;
      for (int i = 0, xmax = buf.xmax(); i <= xmax; ++i) {
        float pixel[4];
        m_buf.getpixel(i, j, pixel, 4);
        for (int c = 0; c < 3; c++) {
          pixel[c] = linear_to_sRGB(pixel[c]);
        }
        pixel[3] = 1.0f;
        buf.setpixel(i, invertedJ, pixel);
      }
    }
    
    buf.write(filename);

  } else {

    ImageBuf buf("", m_buf.spec());
    
    for (int j = 0, ymax = buf.ymax(); j <= ymax; ++j) {
      int invertedJ = ymax - j;
      for (int i = 0, xmax = buf.xmax(); i <= xmax; ++i) {
        float pixel[4];
        m_buf.getpixel(i, j, pixel, 4);
        buf.setpixel(i, invertedJ, pixel);
      }
    }
    
    buf.write(filename);

  }

  Log::print("  Done.");
}

//----------------------------------------------------------------------------//

Image::pixel_iterator Image::begin() 
{ 
  return pixel_iterator(*this, 0, 0); 
}

//----------------------------------------------------------------------------//

Image::pixel_iterator Image::end() 
{ 
  return pixel_iterator(*this, m_buf.xmin(), m_buf.ymax() + 1); 
}

//----------------------------------------------------------------------------//
// Image::pixel_iterator
//----------------------------------------------------------------------------//

Image::pixel_iterator::pixel_iterator(Image &image, const size_t xPos, 
                                      const size_t yPos)
  : x(xPos), y(yPos), m_image(image), m_size(image.size())
{ 

}
  
//----------------------------------------------------------------------------//

const Image::pixel_iterator& 
Image::pixel_iterator::operator ++ ()
{ 
  x++;
  if (x == m_size.x) {
    y++;
    x = 0;
  } 
  return *this;
}

//----------------------------------------------------------------------------//
  
bool Image::pixel_iterator::operator != (const pixel_iterator &rhs) const
{ 
  return x != rhs.x || y != rhs.y; 
}

//----------------------------------------------------------------------------//

void Image::pixel_iterator::setPixel(const Color &color)
{ 
  m_image.setPixel(x, y, color); 
}

//----------------------------------------------------------------------------//

void Image::pixel_iterator::setPixelAlpha(const float alpha)
{ 
  m_image.setPixelAlpha(x, y, alpha); 
}

//----------------------------------------------------------------------------//

float Image::pixel_iterator::progress() const
{ 
  return static_cast<float>(x + y * m_size.x) / 
    static_cast<float>(m_size.x * m_size.y);
}

//----------------------------------------------------------------------------//

float Image::pixel_iterator::rsX() const
{ 
  return Field3D::discToCont(x); 
}

//----------------------------------------------------------------------------//

float Image::pixel_iterator::rsY() const
{ 
  return Field3D::discToCont(y); 
}

//----------------------------------------------------------------------------//

} // namespace Render
} // namespace pvr

//----------------------------------------------------------------------------//
