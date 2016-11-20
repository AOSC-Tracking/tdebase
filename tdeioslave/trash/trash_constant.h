/* This file is part of the TDE project
   Copyright (C) 2016 Michele Calgaro <michele__DOT__calgaro__AT__yahoo__DOT__it

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef TRASH_CONSTANT_H
#define TRASH_CONSTANT_H

namespace TrashConstant
{
  	enum
  	{
  		SIZE_LIMIT_PERCENT = 0,
  		SIZE_LIMIT_FIXED = 1,
  	};
  	
  	enum
  	{
  		SIZE_ID_B = 0,
  		SIZE_ID_KB = 1,
  		SIZE_ID_MB = 2,
  		SIZE_ID_GB = 3,
  		SIZE_ID_TB = 4
  	};
}
  	
#endif
