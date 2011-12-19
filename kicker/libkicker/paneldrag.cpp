/*****************************************************************
Copyright (c) 2004 Aaron J. Seigo <aseigo@kde.org>
              2004 Stephen Depooter <sbdep@woot.net>

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

#include <sys/types.h>
#include <unistd.h>

#include <tqbuffer.h>

#include "paneldrag.h"

#define PANELDRAG_BUFSIZE sizeof(BaseContainer*) + sizeof(pid_t)

PanelDrag::PanelDrag(BaseContainer* container, TQWidget* dragSource)
    : TQDragObject(dragSource, 0)
{
    pid_t source_pid = getpid();

    a.resize(PANELDRAG_BUFSIZE);
    memcpy(a.data(), &container, sizeof(BaseContainer*));
    memcpy(a.data() + sizeof(BaseContainer*), &source_pid, sizeof(pid_t));
}

PanelDrag::~PanelDrag()
{
}

bool PanelDrag::decode(const TQMimeSource* e, BaseContainer** container)
{
    TQByteArray a = e->encodedData("application/basecontainerptr");

    if (a.size() != PANELDRAG_BUFSIZE)
    {
        return false;
    }

    pid_t target_pid = getpid();
    pid_t source_pid;
    memcpy(&source_pid, a.data() + sizeof(TQObject*), sizeof(pid_t));

    if (source_pid == target_pid)
    {
        memcpy(container, a.data(), sizeof(TQObject*));
        return true;
    }

    return false;
}

bool PanelDrag::canDecode(const TQMimeSource *e)
{
    if (!e->provides("application/basecontainerptr"))
    {
        return false;
    }

    TQByteArray a = e->encodedData("application/basecontainerptr");
    if (a.size() != PANELDRAG_BUFSIZE)
    {
        return false;
    }

/*    pid_t target_pid = getpid();
    pid_t source_pid;
    memcpy(&source_pid, a.data() + sizeof(void*), sizeof(pid_t));

    if (source_pid != target_pid)
    {
        return true;
    } */

    return true;
}

TQByteArray PanelDrag::encodedData(const char * mimeType) const
{
    if (TQString("application/basecontainerptr") == mimeType &&
        a.size() == PANELDRAG_BUFSIZE)
    {
        return a;
    }

    return TQByteArray();
}

const char * PanelDrag::format(int i) const
{
    if (i == 0)
    {
        return "application/basecontainerptr";
    }

    return 0;
}


AppletInfoDrag::AppletInfoDrag(const AppletInfo& info, TQWidget *dragSource)
    : TQDragObject(dragSource, 0)
{
    TQBuffer buff(a);
    buff.open(IO_WriteOnly);
    TQDataStream s(&buff);
    s << info.desktopFile() << info.configFile() << info.type();
}

AppletInfoDrag::~AppletInfoDrag()
{
}

const char * AppletInfoDrag::format(int i) const
{
    if (i == 0)
    {
        return "application/appletinfo";
    }

    return 0;
}

TQByteArray AppletInfoDrag::encodedData(const char* mimeType) const
{
    if (TQString("application/appletinfo") == mimeType)
    {
        return a;
    }

    return TQByteArray();
}

bool AppletInfoDrag::canDecode(const TQMimeSource * e)
{
    if (!e->provides("application/appletinfo"))
    {
        return false;
    }

    return true;
}

bool AppletInfoDrag::decode(const TQMimeSource* e, AppletInfo& container)
{
    TQByteArray a = e->encodedData("application/appletinfo");

    if (a.isEmpty())
    {
        return false;
    }

    TQBuffer buff(a);
    buff.open(IO_ReadOnly);
    TQDataStream s(&buff);

    TQString desktopFile;
    TQString configFile;
    int type;
    s >> desktopFile >> configFile >> type;
    AppletInfo info(desktopFile, configFile, (AppletInfo::AppletType)type);
    container = info;
    return true;
}

