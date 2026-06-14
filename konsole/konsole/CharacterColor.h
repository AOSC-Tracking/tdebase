/*
    This file is part of Konsole, an X terminal.
    Copyright (C) 1997,1998 by Lars Doelle <lars.doelle@on-line.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/

#ifndef CHARACTERCOLOR_H
#define CHARACTERCOLOR_H

#include <cstdint>

#include <tqcolor.h>


// Represent colors in the default table and color schemas
class ColorEntry
{
  public:
    explicit ColorEntry(TQColor c, bool tr, bool b) : m_color(c), m_transparent(tr), m_bold(b) {}
    ColorEntry() : m_transparent(false), m_bold(false) {} // default constructors

    ColorEntry& operator=(const ColorEntry &rhs)
    {
      if (this != &rhs)
      {
        m_color = rhs.m_color;
        m_transparent = rhs.m_transparent;
        m_bold = rhs.m_bold;
      }
      return *this;
    }

  public:
    TQColor m_color;
    bool    m_transparent; // if used on background color
    bool    m_bold;        // if used on foreground color
};


// Color spaces
#define COLOR_SPACE_UNDEFINED 0
#define COLOR_SPACE_DEFAULT   1
#define COLOR_SPACE_SYSTEM    2
#define COLOR_SPACE_256       3
#define COLOR_SPACE_RGB       4

/*
   Color index in the base color table:
         0 : default foreground  (normal)   (COLOR_SPACE_DEFAULT)
         1 : default background  (normal)   (COLOR_SPACE_DEFAULT)
     2 - 9 : system colors       (normal)   (COLOR_SPACE_SYSTEM)
        10 : default foreground  (intense)  (COLOR_SPACE_DEFAULT)
        11 : default background  (intense)  (COLOR_SPACE_DEFAULT)
   12 - 19 : system colors       (intense)  (COLOR_SPACE_SYSTEM)


   Color index in the 256 color table:
     0 -   7 : system colors     (normal)   (COLOR_SPACE_SYSTEM)
     8 -  15 : system colors     (intense)  (COLOR_SPACE_SYSTEM)
    16 - 231 : 6 × 6 × 6 cube (216 colors)  (COLOR_SPACE_RGB)
   232 - 255 : grayscale from dark to light (COLOR_SPACE_RGB)

*/

// Base color table contains default and system color spaces
#define BASE_COLORS   (2+8)
#define INTENSITIES   2
#define TABLE_COLORS  (INTENSITIES * BASE_COLORS)

// Default foreground, default background color index
#define DEFAULT_FORE_COLOR 0
#define DEFAULT_BACK_COLOR 1


/*
   CharacterColor describes the color of a single character on the
   terminal and is represented as one of the available color spaces.

   m_cs  - color space - other members's value
   0     - Undefined   - m_u:  0,      m_v:0        m_w:0
   1     - Default     - m_u:  0..1    m_v:intense  m_w:0
   2     - System      - m_u:  0..7    m_v:intense  m_w:0
   3     - Index(256)  - m_u: 16..255  m_v:0        m_w:0
   4     - RGB         - m_u:  0..255  m_v:0..255   m_w:0..255

   `intense' is either 0 (normal) or 1 (intense)
*/
class CharacterColor
{
  friend class Character;

  public:
    constexpr CharacterColor();
    constexpr CharacterColor(uint8_t cs, int color);

    constexpr bool isValid() const;
    void setIntensive();
    TQColor color(const ColorEntry *base) const;

    constexpr bool operator==(const CharacterColor &other) const;
    constexpr bool operator!=(const CharacterColor &other) const;

  private:
    uint8_t m_cs; // color space
    uint8_t m_u;
    uint8_t m_v;
    uint8_t m_w;
};

// Create an undefined character color
inline constexpr CharacterColor::CharacterColor()
        : m_cs(COLOR_SPACE_UNDEFINED), m_u(0), m_v(0), m_w(0)
{
}

// Create a new CharacterColor with colorSpace 'cs' and value 'color'
inline constexpr CharacterColor::CharacterColor(uint8_t cs, int color)
        : m_cs(cs), m_u(0), m_v(0), m_w(0)
{
  switch (m_cs)
  {
    case COLOR_SPACE_DEFAULT:
      m_u = color & 0x01;
      break;

    case COLOR_SPACE_SYSTEM:
      m_u = color & 0x07;
      m_v = (color >> 3) & 0x01;
      break;

    case COLOR_SPACE_256:
      m_u = color & 0xFF;
      break;

    case COLOR_SPACE_RGB:
      m_u = (color >> 16) & 0xFF; // r
      m_v = (color >> 8) & 0xFF;  // g
      m_w = color & 0xFF;         // b
      break;

    default:
      m_cs = COLOR_SPACE_UNDEFINED;
      break;
  }
}

// Return true if this is a valid character color
inline constexpr bool CharacterColor::isValid() const
{
  return m_cs != COLOR_SPACE_UNDEFINED;
}

inline void CharacterColor::setIntensive()
{
  if (m_cs == COLOR_SPACE_SYSTEM || m_cs == COLOR_SPACE_DEFAULT)
  {
    m_v = 1;
  }
}

inline constexpr bool CharacterColor::operator==(const CharacterColor &other) const
{
  return m_cs == other.m_cs &&
         m_u  == other.m_u  &&
         m_v  == other.m_v  &&
         m_w  == other.m_w;
}

inline constexpr bool CharacterColor::operator!=(const CharacterColor &other) const
{
  return !operator==(other);
}

inline const TQColor color256(uint8_t u, const ColorEntry *base)
{
  //   0..15: system colors
  if (u < 8) return base[u+2].m_color;
  u -= 8;
  if (u < 8) return base[u+2+BASE_COLORS].m_color;
  u -= 8;

  //  16..231: 6x6x6 rgb color cube
  if (u < 216) return TQColor(255 * ((u / 36) % 6) / 5,
                              255 * ((u /  6) % 6) / 5,
                              255 * ((u /  1) % 6) / 5);
  u -= 216;

  // 232..255: gray, leaving out black and white
  int gray = u*10+8; return TQColor(gray,gray,gray);
}

inline TQColor CharacterColor::color(const ColorEntry *base) const
{
  switch (m_cs)
  {
    case COLOR_SPACE_DEFAULT:
      return base[m_u + (m_v ? BASE_COLORS : 0)].m_color;

    case COLOR_SPACE_SYSTEM:
      return base[m_u + 2 + (m_v ? BASE_COLORS : 0)].m_color;

    case COLOR_SPACE_256:
      return color256(m_u, base);

    case COLOR_SPACE_RGB:
      return TQColor(m_u, m_v, m_w);

    default:
      return TQColor();
  }
  return TQColor();
}

#endif
