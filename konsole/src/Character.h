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

#ifndef CHARACTER_H
#define CHARACTER_H

#include <CharacterColor.h>


// Rendition attributes
#define DEFAULT_RENDITION  0
#define RE_BOLD            (1 << 0)
#define RE_BLINK           (1 << 1)
#define RE_UNDERLINE       (1 << 2)
#define RE_REVERSE         (1 << 3)
#define RE_INTENSIVE       (1 << 3)
#define RE_CURSOR          (1 << 4)

namespace Konsole
{

/*
   Character represents a single character in the terminal.
   It consists of a unicode character value, foreground and background colors
   and a set of rendition attributes which specify how it should be drawn.
*/
class Character
{
  public:
    /*
       Constructs a new character.

       @param _c The unicode character value of this character.
       @param _f The foreground color used to draw the character.
       @param _b The color used to draw the character's background.
       @param _r A set of rendition flags which specify how this character is to be drawn.
    */
    explicit constexpr Character(
            uint16_t _c       = ' ',
            CharacterColor _f = CharacterColor(COLOR_SPACE_DEFAULT, DEFAULT_FORE_COLOR),
            CharacterColor _b = CharacterColor(COLOR_SPACE_DEFAULT, DEFAULT_BACK_COLOR),
            uint8_t  _r       = DEFAULT_RENDITION)
         : m_character(_c), m_rendition(_r), m_fgColor(_f), m_bgColor(_b) {}

    bool isTransparent(const ColorEntry *base) const;
    bool isBold(const ColorEntry *base) const;

    constexpr bool operator==(const Character &other) const;
    constexpr bool operator!=(const Character &other) const;

  public:
    uint16_t        m_character; // character
    uint8_t         m_rendition; // rendition
    CharacterColor  m_fgColor;   // foreground color
    CharacterColor  m_bgColor;   // background color
};

inline constexpr bool Character::operator==(const Character &other) const
{
  return m_character == other.m_character &&
         m_rendition == other.m_rendition &&
         m_fgColor   == other.m_fgColor   &&
         m_bgColor   == other.m_bgColor;
}

inline constexpr bool Character::operator!=(const Character &other) const
{
  return !operator==(other);
}

inline bool Character::isTransparent(const ColorEntry* base) const
{
  return (m_bgColor.m_cs == COLOR_SPACE_DEFAULT && base[m_bgColor.m_u     + (m_bgColor.m_v ? BASE_COLORS : 0)].m_transparent) ||
         (m_bgColor.m_cs == COLOR_SPACE_SYSTEM  && base[m_bgColor.m_u + 2 + (m_bgColor.m_v ? BASE_COLORS : 0)].m_transparent);
}

inline bool Character::isBold(const ColorEntry* base) const
{
  return (m_fgColor.m_cs == COLOR_SPACE_DEFAULT && base[m_fgColor.m_u     + (m_fgColor.m_v ? BASE_COLORS : 0)].m_bold) ||
         (m_fgColor.m_cs == COLOR_SPACE_SYSTEM  && base[m_fgColor.m_u + 2 + (m_fgColor.m_v ? BASE_COLORS : 0)].m_bold);
}

}

#endif
