/* schema.C
**
** Copyright (C) 1998-1999	by Lars Doelle <lars.doelle@on-line.de>
** Copyright (C) 2000		by Adriaan de Groot <groot@kde.org>
**
** A file that defines the objects for storing color schema's
** in konsole. This file is part of the KDE project, see
**	http://www.kde.org/
** for more information.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program in a file called COPYING; if not, write to
** the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
** MA 02110-1301, USA.
*/


/* Lars' comments were the following. They are not true anymore: */

/*
**  This is new stuff, so no docs yet.
**
**  The identifier is the path. `m_numb' is guarantied to range from 0 to
**  count-1. Note when reloading the path can be assumed to still identify
**  a know schema, while the `m_numb' may vary.
*/


/*
** NUMB IS NOT GUARANTEED ANYMORE. Since schema's may be created and
** destroyed as the list is checked, there may be gaps in the serial
** m_numbers and m_numb may be .. whatever. The default schema always has
** m_number 0, the rest may vary. Use find(int) to find a schema with
** a particular m_number, but remember that find may return NULL.
*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "schema.h"
#include "kapplication.h"

#include <tqdir.h>
#include <tqdatetime.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>


typedef TQPtrListIterator<ColorSchema> ColorSchemaListIterator;


// Number all the new color schema's (non-default) from 1.
//
//
int ColorSchema::serial = 1;

// Names of all the colors, to be used as group names
// in the config files. These do not have to be i18n'ed.
//
//
static const char *colornames[TABLE_COLORS] =
{
  "fgnormal",
  "bgnormal",
  "bg0",
  "bg1",
  "bg2",
  "bg3",
  "bg4",
  "bg5",
  "bg6",
  "bg7",
  "fgintense",
  "bgintense",
  "bg0i",
  "bg1i",
  "bg2i",
  "bg3i",
  "bg4i",
  "bg5i",
  "bg6i",
  "bg7i"
} ;


static const ColorEntry default_table[TABLE_COLORS] =
 // The following are almost IBM standard color codes, with some slight
 // gamma correction for the dim colors to compensate for bright X screens.
 // It contains the 8 ansiterm/xterm colors in 2 intensities.
{
    ColorEntry( TQColor(0x00,0x00,0x00), 0, 0 ), ColorEntry(
TQColor(0xFF,0xFF,0xFF), 1, 0 ), // Dfore, Dback
    ColorEntry( TQColor(0x00,0x00,0x00), 0, 0 ), ColorEntry(
TQColor(0xB2,0x18,0x18), 0, 0 ), // Black, Red
    ColorEntry( TQColor(0x18,0xB2,0x18), 0, 0 ), ColorEntry(
TQColor(0xB2,0x68,0x18), 0, 0 ), // Green, Yellow
    ColorEntry( TQColor(0x18,0x18,0xB2), 0, 0 ), ColorEntry(
TQColor(0xB2,0x18,0xB2), 0, 0 ), // Blue,  Magenta
    ColorEntry( TQColor(0x18,0xB2,0xB2), 0, 0 ), ColorEntry(
TQColor(0xB2,0xB2,0xB2), 0, 0 ), // Cyan,  White
    // intensive
    ColorEntry( TQColor(0x00,0x00,0x00), 0, 1 ), ColorEntry(
TQColor(0xFF,0xFF,0xFF), 1, 0 ),
    ColorEntry( TQColor(0x68,0x68,0x68), 0, 0 ), ColorEntry(
TQColor(0xFF,0x54,0x54), 0, 0 ),
    ColorEntry( TQColor(0x54,0xFF,0x54), 0, 0 ), ColorEntry(
TQColor(0xFF,0xFF,0x54), 0, 0 ),
    ColorEntry( TQColor(0x54,0x54,0xFF), 0, 0 ), ColorEntry(
TQColor(0xFF,0x54,0xFF), 0, 0 ),
    ColorEntry( TQColor(0x54,0xFF,0xFF), 0, 0 ), ColorEntry(
TQColor(0xFF,0xFF,0xFF), 0, 0 )
};

ColorSchema::ColorSchema(const TQString& pathname)
:m_fileRead(false)
,lastRead(new TQDateTime())
{
  //start with a valid time, aleXXX
  *lastRead = TQDateTime::currentDateTime();
  TQString fPath = pathname.startsWith("/") ? pathname : locate("data", "konsole/"+pathname);
  if (fPath.isEmpty() || !TQFile::exists(fPath))
  {
    fRelPath = TQString::null;
    setDefaultSchema();
  }
  else
  {
    fRelPath = pathname;
    clearSchema();
/*  this is done on demand, see the headers, aleXXX
    (void) rereadSchemaFile(); */
  }

  m_numb = serial++;
}

ColorSchema::ColorSchema()
:m_fileRead(false)
,fRelPath(TQString::null)
,lastRead(0L)
{
  setDefaultSchema();
  m_numb = 0;
}

ColorSchema::ColorSchema(KConfig& c)
:m_fileRead(false)
,fRelPath(TQString::null)
,lastRead(0L)
{
  clearSchema();

  c.setGroup("SchemaGeneral");

  m_title = c.readEntry("Title",i18n("[no title]"));
  m_imagePath = c.readEntry("ImagePath");
  m_tqalignment = c.readNumEntry("ImageAlignment",1);
  m_useTransparency = c.readBoolEntry("UseTransparency",false);

  m_tr_r = c.readNumEntry("TransparentR",0);
  m_tr_g = c.readNumEntry("TransparentG",0);
  m_tr_b = c.readNumEntry("TransparentB",0);
  m_tr_x = c.readDoubleNumEntry("TransparentX",0.0);

  for (int i=0; i < TABLE_COLORS; i++)
  {
    readConfigColor(c,colorName(i),m_table[i]);
  }

  m_numb = serial++;
}


ColorSchema::~ColorSchema()
{
    delete lastRead;
}

void ColorSchema::clearSchema()
{
  int i;

  for (i = 0; i < TABLE_COLORS; i++)
  {
    m_table[i].color       = TQColor(0,0,0);
    m_table[i].transparent = 0;
    m_table[i].bold        = 0;
  }
  m_title     = i18n("[no title]");
  m_imagePath = "";
  m_tqalignment = 1;
  m_useTransparency = false;
  m_tr_x = 0.0;
  m_tr_r = 0;
  m_tr_g = 0;
  m_tr_b = 0;
}

void ColorSchema::setDefaultSchema()
{
  m_numb = 0;
  m_title = i18n("Konsole Default");
  m_imagePath = ""; // background pixmap
  m_tqalignment = 1;  // none
  m_useTransparency = false; // not use pseudo-transparency by default
  m_tr_r = m_tr_g = m_tr_b = 0; // just to be on the safe side
  m_tr_x = 0.0;
  for (int i = 0; i < TABLE_COLORS; i++)
  {
    m_table[i] = default_table[i];
  }
}

/* static */ TQString ColorSchema::colorName(int i)
{
  if ((i<0) || (i>=TABLE_COLORS))
  {
    kdWarning() << "Request for color name "
      << i
      << " out of range."
      << endl;
    return TQString::null;
  }

  return TQString(colornames[i]);
}

void ColorSchema::writeConfigColor(KConfig& c,
  const TQString& name,
  const ColorEntry& e) const
{
  KConfigGroupSaver(&c,name);
  c.setGroup(name);
  c.writeEntry("Color",e.color);
  c.writeEntry("Transparency",(bool) e.transparent);
  c.writeEntry("Bold",(bool) e.bold);
}

void ColorSchema::readConfigColor(KConfig& c,
  const TQString& name,
  ColorEntry& e)
{
  KConfigGroupSaver(&c,name);
  c.setGroup(name);

  e.color = c.readColorEntry("Color");
  e.transparent = c.readBoolEntry("Transparent",false);
  e.bold = c.readBoolEntry("Bold",false);
}


void ColorSchema::writeConfig(const TQString& path) const
{
//  KONSOLEDEBUG << "Writing schema " << relPath << " to file " << path << endl;

  KConfig c(path,false,false);

  c.setGroup("SchemaGeneral");
  c.writeEntry("Title",m_title);
  c.writeEntry("ImagePath",m_imagePath);
  c.writeEntry("ImageAlignment",m_tqalignment);
  c.writeEntry("UseTransparency",m_useTransparency);

  c.writeEntry("TransparentR",m_tr_r);
  c.writeEntry("TransparentG",m_tr_g);
  c.writeEntry("TransparentB",m_tr_b);
  c.writeEntry("TransparentX",m_tr_x);

  for (int i=0; i < TABLE_COLORS; i++)
  {
    writeConfigColor(c,colorName(i),m_table[i]);
  }
}

static int random_hue = -1;

bool ColorSchema::rereadSchemaFile()
{
  TQString fPath = fRelPath.isEmpty() ? "" : (fRelPath.startsWith("/") ? fRelPath : locate("data", "konsole/"+fRelPath));
  if (fPath.isEmpty() || !TQFile::exists(fPath))
     return false;

  //KONSOLEDEBUG << "Rereading schema file " << fPath << endl;

  FILE *sysin = fopen(TQFile::encodeName(fPath),"r");
  if (!sysin)
  {
    int e = errno;

    kdWarning() << "Schema file "
      << fPath
      << " could not be opened ("
      << strerror(e)
      << ")"
      << endl;
    return false;
  }

  char line[100];

  *lastRead = TQDateTime::currentDateTime();

  while (fscanf(sysin,"%80[^\n]\n",line) > 0)
  {
    if (strlen(line) > 5)
    {
      if (!strncmp(line,"title",5))
      {
        m_title = i18n(line+6);
      }
      if (!strncmp(line,"image",5))
      { char rend[100], path[100]; int attr = 1;
        if (sscanf(line,"image %s %s",rend,path) != 2)
          continue;
        if (!strcmp(rend,"tile"  )) attr = 2; else
        if (!strcmp(rend,"center")) attr = 3; else
        if (!strcmp(rend,"full"  )) attr = 4; else
          continue;

        TQString qline(line);
        m_imagePath = locate("wallpaper", qline.mid( qline.find(" ",7)+1 ) );
        m_tqalignment = attr;
      }
      if (!strncmp(line,"transparency",12))
      { float rx;
        int rr, rg, rb;

  // Transparency needs 4 parameters: fade strength and the 3
  // components of the fade color.
        if (sscanf(line,"transparency %g %d %d %d",&rx,&rr,&rg,&rb) != 4)
          continue;
  m_useTransparency=true;
  m_tr_x=rx;
  m_tr_r=rr;
  m_tr_g=rg;
  m_tr_b=rb;
      }
      if (!strncmp(line,"rcolor",6))
      { int fi,ch,cs,cv,tr,bo;
        if(sscanf(line,"rcolor %d %d %d %d %d",&fi,&cs,&cv,&tr,&bo) != 5)
          continue;
        if (!(0 <= fi && fi <= TABLE_COLORS)) continue;
        if (random_hue == -1)
          random_hue = (KApplication::random()%32) * 11;
        ch = random_hue;
        if (!(0 <= cs && cs <= 255         )) continue;
        if (!(0 <= cv && cv <= 255         )) continue;
        if (!(0 <= tr && tr <= 1           )) continue;
        if (!(0 <= bo && bo <= 1           )) continue;
        m_table[fi].color       = TQColor();
        m_table[fi].color.setHsv(ch,cs,cv);
        m_table[fi].transparent = tr;
        m_table[fi].bold        = bo;
      }
      if (!strncmp(line,"color",5))
      { int fi,cr,cg,cb,tr,bo;
        if(sscanf(line,"color %d %d %d %d %d %d",&fi,&cr,&cg,&cb,&tr,&bo) != 6)
          continue;
        if (!(0 <= fi && fi <= TABLE_COLORS)) continue;
        if (!(0 <= cr && cr <= 255         )) continue;
        if (!(0 <= cg && cg <= 255         )) continue;
        if (!(0 <= cb && cb <= 255         )) continue;
        if (!(0 <= tr && tr <= 1           )) continue;
        if (!(0 <= bo && bo <= 1           )) continue;
        m_table[fi].color       = TQColor(cr,cg,cb);
        m_table[fi].transparent = tr;
        m_table[fi].bold        = bo;
      }
      if (!strncmp(line,"sysfg",5))
      { int fi,tr,bo;
        if(sscanf(line,"sysfg %d %d %d",&fi,&tr,&bo) != 3)
          continue;
        if (!(0 <= fi && fi <= TABLE_COLORS)) continue;
        if (!(0 <= tr && tr <= 1           )) continue;
        if (!(0 <= bo && bo <= 1           )) continue;
        m_table[fi].color       = kapp->tqpalette().active().text();
        m_table[fi].transparent = tr;
        m_table[fi].bold        = bo;
      }
      if (!strncmp(line,"sysbg",5))
      { int fi,tr,bo;
        if(sscanf(line,"sysbg %d %d %d",&fi,&tr,&bo) != 3)
          continue;
        if (!(0 <= fi && fi <= TABLE_COLORS)) continue;
        if (!(0 <= tr && tr <= 1           )) continue;
        if (!(0 <= bo && bo <= 1           )) continue;
        m_table[fi].color       = kapp->tqpalette().active().base();
        m_table[fi].transparent = tr;
        m_table[fi].bold        = bo;
      }
    }
  }
  fclose(sysin);
  m_fileRead=true;
  return true;
}

bool ColorSchema::hasSchemaFileChanged() const
{
  TQString fPath = fRelPath.isEmpty() ? "" : locate("data", "konsole/"+fRelPath);

  //KONSOLEDEBUG << "Checking schema file " << fPath << endl;

  // The default color schema never changes.
  //
  //
  if (fPath.isEmpty()) return false;

  TQFileInfo i(fPath);

  if (i.exists())
  {
    TQDateTime written = i.lastModified();

    if (written != (*lastRead))
    {
//      KONSOLEDEBUG << "Schema file was modified " << written.toString() << endl;

      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    kdWarning() << "Schema file no longer exists."
      << endl;
    return false;
  }
}

void ColorSchema::updateLastRead(const TQDateTime& dt)
{
  if (lastRead)
  {
    *lastRead=dt;
  }
}


ColorSchemaList::ColorSchemaList() :
  TQPtrList<ColorSchema> ()
{
//  KONSOLEDEBUG << "Got new color list" << endl;

  defaultSchema = new ColorSchema();
  append(defaultSchema);
  setAutoDelete(true);

  ColorSchema::serial=1;   // Needed for detached sessions
}

ColorSchemaList::~ColorSchemaList()
{
   ColorSchema::serial=1;
}


ColorSchema *ColorSchemaList::find(const TQString& path)
{
   if (path.isEmpty())
      return find(0);
   //KONSOLEDEBUG << "Looking for schema " << path << endl;
   //kdDebug(1211)<<"ColorSchema::find() count()=="<<count()<<endl;
   ColorSchemaListIterator it(*this);
   ColorSchema *c;

   if (path.startsWith("/")) {
      //KONSOLEDEBUG << " schema given as a full path... " << path << endl;
      ColorSchema *newSchema = new ColorSchema(path);
      if (newSchema)
         append(newSchema);
      return newSchema;
   }

   while ((c=it.current()))
   {
      if ((*it)->relPath() == path)
        return *it;
      ++it;
   }

   //list is empty except the default schema
   if (count()==1)
   {
      //kdDebug(1211)<<"ColorSchema::find() empty"<<endl;
      ColorSchema *newSchema = new ColorSchema(path);
      if (newSchema)
         append(newSchema);
      return newSchema;
   };
   return 0;
}

ColorSchema *ColorSchemaList::find(int i)
{
  //KONSOLEDEBUG << "Looking for schema m_number " << i << endl;

  ColorSchemaListIterator it(*this);
  ColorSchema *c;

  while ((c=it.current()))
  {
    if ((*it)->numb() == i) return *it;
    ++it;
  }

  return 0;
}

bool ColorSchemaList::updateAllSchemaTimes(const TQDateTime& now)
{
//  KONSOLEDEBUG << "Updating time stamps" << endl;

  TQStringList list;
  KGlobal::dirs()->findAllResources("data", "konsole/*.schema", false, true, list);
  TQStringList::ConstIterator it;
  bool r = false;

  for (it=list.begin(); it!=list.end(); ++it)
  {
    TQString filename=*it;
    int j=filename.findRev('/');
    if (j>-1)
      filename = filename.mid(8);

    ColorSchema *sc = find(filename);

    if (!sc)
    {
//      KONSOLEDEBUG << "Found new schema " << filename << endl;

      ColorSchema *newSchema = new ColorSchema(filename);
      if (newSchema)
      {
        append(newSchema);
        r=true;
      }
    }
    else
    {
      if (sc->hasSchemaFileChanged())
      {
        sc->rereadSchemaFile();
      }
      else
      {
        sc->updateLastRead(now);
      }
    }
  }
  //this has to be done explicitly now, to avoid reading all schema files on startup, aleXXX
//   sort();
  return r;
}

bool ColorSchemaList::deleteOldSchemas(const TQDateTime& now)
{
//  KONSOLEDEBUG << "Checking for vanished schemas" << endl;

  ColorSchemaListIterator it(*this);
  ColorSchema *p ;
  bool r = false;

  while ((p=it.current()))
  {
    if ((p->getLastRead()) && (*(p->getLastRead())) < now)
    {
      KONSOLEDEBUG << "Found deleted schema "
        << p->relPath()
        << endl;
      ++it;
      remove(p);
      r=true;
      if (!it.current())
      {
        break;
      }
    }
    else
    {
      ++it;
    }
  }

  return r;
}


bool ColorSchemaList::checkSchemas()
{
//  KONSOLEDEBUG << "Checking for new schemas" << endl;

  bool r = false;  // Any new schema's found?

  // All schemas whose schema files can still be found
  // will have their lastRead timestamps updated to
  // now.
  //
  //
  TQDateTime now = TQDateTime::currentDateTime();


  r = updateAllSchemaTimes(now);
  r = r || deleteOldSchemas(now);

  return r;
}

int ColorSchemaList::compareItems(TQPtrCollection::Item item1, TQPtrCollection::Item item2)
{
   ColorSchema* schema1=(ColorSchema*)item1;
   ColorSchema* schema2=(ColorSchema*)item2;
   return -1*TQString::compare(schema1->title(),schema2->title());
}


