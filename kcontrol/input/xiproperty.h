/*******************************************************************************
 XIGetProperty/XIChangeProperty wrapper

 Copyright © 2013 Alexandr Mezin <mezin.alexander@gmail.com>
 Copyright © 2024 Mavridis Philippe <mavridisf@gmail.com>

 This program is free software: you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation, either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with
 this program. If not, see <https://www.gnu.org/licenses/>.

*******************************************************************************/

#ifndef __XI_PROPERTY_H__
#define __XI_PROPERTY_H__

// TQt
#include <tqobject.h> // tqt_xdisplay()
#include <tqvariant.h>

// X11
#include <X11/Xatom.h>


class XIProperty
{
    public:
        XIProperty()
        : device(-1),
          type(0),
          format(0),
          num_items(0),
          data(0),
          b(nullptr),
          i(nullptr),
          f(nullptr)
        {}

        XIProperty(int device, TQCString propertyName)
        : device(device),
          type(0),
          format(0),
          num_items(0),
          data(0),
          b(nullptr),
          i(nullptr),
          f(nullptr)
        {
            Display *disp = tqt_xdisplay();

            property = XInternAtom(disp, propertyName, true);

            unsigned char *ptr = nullptr;
            unsigned long bytes_after;

            XIGetProperty(disp, device, property, 0, 1000, False, AnyPropertyType,
                        &type, &format, &num_items, &bytes_after, &ptr);

            data = ptr;

            if (format == CHAR_BIT && type == XA_INTEGER)
            {
                b = reinterpret_cast<char *>(data);
            }

            if (format == sizeof(int) * CHAR_BIT
            && (type == XA_INTEGER || type == XA_CARDINAL))
            {
                i = reinterpret_cast<int *>(data);
            }

            Atom floatType = XInternAtom(disp, "FLOAT", true);

            if (format == sizeof(float) * CHAR_BIT && floatType && type == floatType)
            {
                f = reinterpret_cast<float *>(data);
            }
        }

        ~XIProperty()
        {
            XFree(data);
        }

        TQVariant operator[](int offset)
        {
            if (offset >= num_items) return TQVariant();

            if (b) return TQVariant(static_cast<int>(b[offset]));
            if (i) return TQVariant(i[offset]);
            if (f) return TQVariant(f[offset]);

            return TQVariant();
        }

        void set()
        {
            XIChangeProperty(tqt_xdisplay(), device, property, type, format, XIPropModeReplace,
                            data, num_items);
        }

        int count() { return num_items; }

    public:
        char *b;
        int *i;
        float *f;

    private:
        Atom property, type;
        int device, format;
        unsigned long num_items;
        unsigned char *data;
};

#endif // __XI_PROPERTY_H__