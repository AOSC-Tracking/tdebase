/*
 *  lookandfeeltab.h
 *
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
 *  Copyright (c) 2000 Aaron J. Seigo <aseigo@olympusproject.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 */


#ifndef __lookandfeeltab_h__
#define __lookandfeeltab_h__

#include "lookandfeeltab.h"

class advancedDialog;

class LookAndFeelTab : public LookAndFeelTabBase
{
    TQ_OBJECT

public:
    LookAndFeelTab(TQWidget *parent = 0, const char* name = 0);

    void load();
    void load(bool useDefaults);
    void save();
    void defaults();

    TQString quickHelp() const;

signals:
    void changed();

protected:
    void fillTileCombos();
    void previewBackground(const TQString& themepath, bool isNew);

protected slots:
    void browseTheme();
    void browseTheme(const TQString&);
    void enableTransparency( bool );

    void launchAdvancedDialog();
    void finishAdvancedDialog();

    void kmenuTileChanged(int i);
    void desktopTileChanged(int i);
    void browserTileChanged(int i);
    void urlTileChanged(int i);
    void wlTileChanged(int i);

private:
    TQPixmap theme_preview;
    TQStringList m_tilename;
    advancedDialog *m_advDialog;
};

#endif
