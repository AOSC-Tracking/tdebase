/*
 *
 * This file is part of the KDE project, module kdesktop.
 * Copyright (C) 1999 Geert Jansen <g.t.jansen@stud.tue.nl>
 *
 * You can Freely distribute this program under the GNU Library General
 * Public License. See the file "COPYING.LIB" for the exact licensing terms.
 */

#ifndef __BGSETTINGS_H__
#define __BGSETTINGS_H__


#include <tqstringlist.h>
#include <tqcolor.h>
#include <tqvaluevector.h>

template <class TQString, class T> class TQMap;
class TDEStandardDirs;
class KSimpleConfig;
class TDEConfig;
class TQString;
class TQImage;

/**
 * A class to manipulate/read/write/list KDE desktop patterns.
 *
 * A pattern is a raster image. An entry for earch pattern is
 * stored as a .desktop file in $(datadir)/kdesktop/patterns.
 */
class KBackgroundPattern
{
public:
    KBackgroundPattern(TQString name=TQString::null);
    ~KBackgroundPattern();

    void copyConfig(const KBackgroundPattern*);

    TQString name() const { return m_Name; }
    void load(TQString name);

    void setComment(const TQString &comment);
    TQString comment() const {return m_Comment; }

    void setPattern(TQString file);
    TQString pattern() const { return m_Pattern; }

    void readSettings();
    void writeSettings();

    bool isAvailable();
    bool isGlobal()const { return m_bReadOnly; }
    bool remove();

    int hash();

    static TQStringList list();

private:
    void init(bool force_rw=false);
    TQString fingerprint();

    bool dirty, hashdirty;
    bool m_bReadOnly;
    int m_Hash;
    TQString m_Name, m_Comment;
    TQString m_Pattern, m_File;
    TDEStandardDirs *m_pDirs;
    KSimpleConfig *m_pConfig;
};


/**
 * A class to manipulate/read/write/list KDE desktop programs (a la xearth).
 *
 * A program is described by a string like:
 *
 *   a_program -xres %x -yres %y -outfile %f
 *
 * Possible escape sequences:
 *
 *   %x    Horizontal resolution in pixels.
 *   %y    Vertical resulution in pixels.
 *   %f    Filename to dump to.
 *
 * An entry for each program is stored as a .desktop file in
 * $(datadir)/kdesktop/programs.
 */
class KBackgroundProgram
{
public:
    KBackgroundProgram(TQString name=TQString::null);
    ~KBackgroundProgram();

    void copyConfig(const KBackgroundProgram*);

    TQString name()const { return m_Name; }
    void load(const TQString & name);

    void setComment(const TQString &comment);
    TQString comment()const { return m_Comment; }

    void setCommand(const TQString &command);
    TQString command()const { return m_Command; }

    void setPreviewCommand(const TQString &command);
    TQString previewCommand()const { return m_PreviewCommand; }

    void setRefresh(int refresh);
    int refresh()const { return m_Refresh; }

    void setExecutable(const TQString &executable);
    TQString executable()const { return m_Executable; }

    void readSettings();
    void writeSettings();

    void update();
    bool needUpdate();

    int hash();

    bool isAvailable();
    bool isGlobal()const { return m_bReadOnly; }
    bool remove();

    static TQStringList list();

private:
    void init(bool force_rw=false);
    TQString fingerprint();

    bool dirty, hashdirty;
    bool m_bReadOnly;
    int m_Refresh, m_Hash, m_LastChange;
    TQString m_Name, m_Command;
    TQString m_PreviewCommand, m_Comment;
    TQString m_Executable, m_File;
    TDEStandardDirs *m_pDirs;
    KSimpleConfig *m_pConfig;
};


/**
 * KBackgroundSettings: A class to read/write/manipulate
 * KDE desktop settings.
 */
class KBackgroundSettings
    : public KBackgroundPattern,
      public KBackgroundProgram
{
public:
    /**
     * @param drawBackgroundPerScreen if false, then all screens (in xinerama
     * mode) will be treated as one big display, and the "screen" paramater
     * will be ignored.
     */
    KBackgroundSettings(int desk, int screen, bool drawBackgroundPerScreen, TDEConfig *config);
    ~KBackgroundSettings();

    void copyConfig(const KBackgroundSettings*);

    bool drawBackgroundPerScreen() const { return m_bDrawBackgroundPerScreen; }
    void setDrawBackgroundPerScreen(bool draw);

    int desk() const { return m_Desk; }
    int screen() const { return m_Screen; }
//     void load(int desk, int screen, bool drawBackgroundPerScreen, bool reparseConfig=true);
    void load(int desk, int screen, bool drawBackgroundPerScreen, bool reparseConfig);

    void setColorA(const TQColor &color);
    TQColor colorA() const { return m_ColorA; }
    void setColorB(const TQColor &color);
    TQColor colorB() const { return m_ColorB; }

    void setProgram(TQString program);
    void setPatternName(TQString pattern);

    enum BackgroundMode {
	Flat, Pattern, Program,
	HorizontalGradient, VerticalGradient, PyramidGradient,
	PipeCrossGradient, EllipticGradient, lastBackgroundMode
    };
    void setBackgroundMode(int mode);
    int backgroundMode() const { return m_BackgroundMode; }

    enum BlendMode {
        NoBlending, FlatBlending,
	HorizontalBlending, VerticalBlending, PyramidBlending,
	PipeCrossBlending, EllipticBlending,
	IntensityBlending, SaturateBlending, ContrastBlending,
	HueShiftBlending, lastBlendMode
    };
    void setBlendMode(int mode);
    int blendMode() const { return m_BlendMode; }

    void setReverseBlending(bool value);
    bool reverseBlending() const { return m_ReverseBlending; }

    void setCrossFadeBg(bool value);
    bool crossFadeBg() const { return m_CrossFadeBg; }

    void setBlendBalance(int value);
    int blendBalance() const { return m_BlendBalance; }

    void setWallpaper(TQString name);
    TQString wallpaper() const { return m_Wallpaper; }

    enum WallpaperMode {
	NoWallpaper, Centred, Tiled, CenterTiled, CentredMaxpect, TiledMaxpect,
	Scaled, CentredAutoFit, ScaleAndCrop, lastWallpaperMode
    };
    void setWallpaperMode(int mode);
    int wallpaperMode() const { return m_WallpaperMode; }

    void setWallpaperList(TQStringList);
    TQStringList wallpaperList() const;
    TQStringList wallpaperFiles() const;

    void setWallpaperChangeInterval(int);
    int wallpaperChangeInterval() const { return m_Interval; }

    enum MultiMode {
	NoMulti, InOrder, Random, NoMultiRandom
    };
    void setMultiWallpaperMode(int mode);
    int multiWallpaperMode() const { return m_MultiMode; }

    enum MinOptDepth {
        AlwaysOpt, Opt16bpp, Opt15bpp, NeverOpt };

    void setMinOptimizationDepth( int mode );
    int minOptimizationDepth() const { return m_MinOptimizationDepth; }
    bool optimize() const;

    void setUseShm( bool use );
    bool useShm() const { return m_bShm; }

    void changeWallpaper(bool init=false);
    void updateWallpaperFiles();
    void randomizeWallpaperFiles();

    TQString currentWallpaper() const;
    /**
     * @return true if the currentWallpaper has changed
     */
    bool discardCurrentWallpaper();
    int lastWallpaperChange() const { return m_LastChange; }
    bool needWallpaperChange();

    void readSettings(bool reparse=false);
    void writeSettings();
    TQString configGroupName() const;

    int hash();
    TQString fingerprint();

    void setEnabled( const bool enable );
    bool enabled() const { return m_bEnabled; }

private:
    void updateHash();

    bool dirty;
    bool hashdirty;
    int m_Screen, m_Desk, m_Hash;

    TQColor m_ColorA, defColorA;
    TQColor m_ColorB, defColorB;
    TQString m_Wallpaper;
    TQStringList m_WallpaperList, m_WallpaperFiles;

    int m_BackgroundMode, defBackgroundMode;
    int m_WallpaperMode, defWallpaperMode;
    int m_BlendMode, defBlendMode;
    int m_BlendBalance, defBlendBalance;
    bool m_ReverseBlending, defReverseBlending;
    bool m_CrossFadeBg, defCrossFadeBg;
    int m_MinOptimizationDepth;
    bool m_bShm;
    bool m_bDrawBackgroundPerScreen;

    int m_MultiMode, defMultiMode;
    int m_Interval, m_LastChange;
    int m_CurrentWallpaper;
    TQString m_CurrentWallpaperName;

    TDEConfig *m_pConfig;
    TDEStandardDirs *m_pDirs;
    bool m_bDeleteConfig;
    bool m_bEnabled;

public:
    TQMap<TQString,int> m_BMMap;
    TQMap<TQString,int> m_WMMap;
    TQMap<TQString,int> m_MMMap;
    TQMap<TQString,int> m_BlMMap;
    char *m_BMRevMap[16];
    char *m_WMRevMap[16];
    char *m_MMRevMap[16];
    char *m_BlMRevMap[16];
};


/**
 * A class to read/modify the global desktop background settings.
 */
class TDEGlobalBackgroundSettings
{
public:
    TDEGlobalBackgroundSettings(TDEConfig *config);

    TQString deskName(int desk);
    //void setDeskName(int desk, TQString name);

    int cacheSize() { return m_CacheSize; }
    void setCacheSize(int size);

    bool drawBackgroundPerScreen(int desk) const;
    void setDrawBackgroundPerScreen(int desk, bool perScreen);

    bool limitCache() { return m_bLimitCache; }
    void setLimitCache(bool limit);

    bool commonScreenBackground() { return m_bCommonScreen; }
    void setCommonScreenBackground(bool common);

    bool commonDeskBackground() { return m_bCommonDesk; }
    void setCommonDeskBackground(bool common);

    bool dockPanel() { return m_bDock; }
    void setDockPanel(bool dock);

    bool exportBackground() {return m_bExport; }
    void setExportBackground(bool _export);

    void setTextColor(TQColor _color);
    TQColor textColor() const { return m_TextColor; }

    void setTextBackgroundColor(TQColor _color);
    TQColor textBackgroundColor() const { return m_TextBackgroundColor; }

    void setShadowEnabled(bool enabled);
    bool shadowEnabled() const { return m_shadowEnabled; }

    void setTextLines(int lines);
    int textLines() const { return m_textLines; }
    void setTextWidth(int width);
    int textWidth() const { return m_textWidth; }

    void readSettings();
    void writeSettings();
    TDEConfig* getConfig() { return m_pConfig; }

private:
    bool dirty;
    bool m_bCommonDesk;
    bool m_bCommonScreen;
    bool m_bDock;
    bool m_bLimitCache, m_bExport;
    int m_CacheSize;
    TQStringList m_Names;

    TQColor m_TextColor;
    TQColor m_TextBackgroundColor;
    bool m_shadowEnabled;
    int m_textLines;
    int m_textWidth;
    TDEConfig *m_pConfig;
    bool m_bDeleteConfig;
    TQValueVector<bool> m_bDrawBackgroundPerScreen; // m_bDrawBackgroundPerScreen[desk]
};


#endif // __BGSETTINGS_H__
