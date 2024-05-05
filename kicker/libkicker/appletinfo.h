/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#ifndef __appletinfo_h__
#define __appletinfo_h__

#include <tqmap.h>
#include <tqptrlist.h>
#include <tqstring.h>
#include <tqvaluevector.h>

#include <kdemacros.h>

class TDE_EXPORT AppletInfo
{
    public:
        typedef TQValueVector<AppletInfo> List;
        typedef TQMap<TQObject*, AppletInfo*> Dict;

        enum AppletType { Undefined = 0,
                          Applet = 1,
                          BuiltinButton = 2,
                          SpecialButton = 4,
                          Extension = 8,
                          Button = BuiltinButton | SpecialButton };

        AppletInfo(const TQString& desktopFile = TQString::null,
                    const TQString& configFile = TQString::null,
                    const AppletType type = Undefined);

        TQString name() const { return m_name; }
        TQString comment() const { return m_comment; }
        TQString icon() const { return m_icon; }

        AppletType type() const { return m_type; }

        TQString library() const { return m_lib; }
        TQString desktopFile() const { return m_desktopFile; }
        TQString configFile() const { return m_configFile; }

        bool isUniqueApplet() const { return m_unique; }
        bool isHidden() const { return m_hidden; }

        void setConfigFile(TQString cf) { m_configFile = cf; }

        bool operator<(const AppletInfo& rhs) const;
        bool operator>(const AppletInfo& rhs) const;
        bool operator<=(const AppletInfo& rhs) const;
        bool operator!=(const AppletInfo& rhs) const;

        void setType(AppletType type) { m_type = type; }

    protected:
        void setName(TQString name) { m_name = name; }
        void setComment(TQString comment) { m_comment = comment; }
        void setIcon(TQString icon) { m_icon = icon; }
        void setLibrary(TQString lib) { m_lib = lib; }
        void setIsUnique(bool u) { m_unique = u; }

    private:
        TQString m_name;
        TQString m_comment;
        TQString m_icon;
        TQString m_lib;
        TQString m_desktopFile;
        TQString m_configFile;
        AppletType m_type;
        bool m_unique;
        bool m_hidden;
};

#endif
