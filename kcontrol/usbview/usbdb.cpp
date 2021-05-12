/***************************************************************************
 *   Copyright (C) 2001 by Matthias Hoelzer-Kluepfel <mhk@caldera.de>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <config.h>

#include <iostream>


#include <tqfile.h>
#include <tqregexp.h>


#include <kstandarddirs.h>


#include "usbdb.h"

USBDB::USBDB()
{
  TQString db = USBIDS_FILE;
  
  if (!TQFile::exists(db))
	TQString db = locate("data", "kcmusb/usb.ids");
  
  if (db.isEmpty())
    return;

  _classes.setAutoDelete(true);
  _ids.setAutoDelete(true);

  TQFile f(db);

  if (f.open(IO_ReadOnly))
    {
      TQTextStream ts(&f);

      TQString line, name;
      int id=0, subid=0, protid=0;
      TQRegExp vendor("[0-9a-fA-F]+ ");
      TQRegExp product("\\s+[0-9a-fA-F]+ ");
      TQRegExp cls("C [0-9a-fA-F][0-9a-fA-F]");
      TQRegExp subclass("\\s+[0-9a-fA-F][0-9a-fA-F]  ");
      TQRegExp prot("\\s+[0-9a-fA-F][0-9a-fA-F]  ");
      while (!ts.eof())
	{
	  line = ts.readLine();
	  if (line.left(1) == "#" || line.stripWhiteSpace().isEmpty())
	    continue;

	  // skip AT lines
	  if (line.left(2) == "AT")
	    continue;

	  if (cls.search(line) == 0 && cls.matchedLength() == 4)
	    {
	      id = line.mid(2,2).toInt(0, 16);
	      name = line.mid(4).stripWhiteSpace();
	      _classes.insert(TQString("%1").arg(id), new TQString(name));
	    }
	  else if (prot.search(line) == 0 && prot.matchedLength() > 5)
	    {
	      line = line.stripWhiteSpace();
	      protid = line.left(2).toInt(0, 16);
	      name = line.mid(4).stripWhiteSpace();
	      _classes.insert(TQString("%1-%2-%3").arg(id).arg(subid).arg(protid), new TQString(name));
	    }
	  else if (subclass.search(line) == 0 && subclass.matchedLength() > 4)
	    {
	      line = line.stripWhiteSpace();
	      subid = line.left(2).toInt(0, 16);
	      name = line.mid(4).stripWhiteSpace();
	      _classes.insert(TQString("%1-%2").arg(id).arg(subid), new TQString(name));
	    }
	  else if (vendor.search(line) == 0 && vendor.matchedLength() == 5)
	    {
	      id = line.left(4).toInt(0,16);
	      name = line.mid(6);
	      _ids.insert(TQString("%1").arg(id), new TQString(name));
	    }
	  else if (product.search(line) == 0 && product.matchedLength() > 5 )
	    {
	      line = line.stripWhiteSpace();
	      subid = line.left(4).toInt(0,16);
	      name = line.mid(6);
	      _ids.insert(TQString("%1-%2").arg(id).arg(subid), new TQString(name));
	    }

	}

      f.close();
    }
}


TQString USBDB::vendor(int id)
{
  TQString *s = _ids[TQString("%1").arg(id)];
  if ((id!= 0) && s)
    {
      return *s;
    }
  return TQString::null;
}


TQString USBDB::device(int vendor, int id)
{
  TQString *s = _ids[TQString("%1-%2").arg(vendor).arg(id)];
  if ((id != 0) && (vendor != 0) && s)
    return *s;
  return TQString::null;
}


TQString USBDB::cls(int cls)
{
  TQString *s = _classes[TQString("%1").arg(cls)];
  if (s)
    return *s;
  return TQString::null;
}


TQString USBDB::subclass(int cls, int sub)
{
  TQString *s = _classes[TQString("%1-%2").arg(cls).arg(sub)];
  if (s)
    return *s;
  return TQString::null;
}


TQString USBDB::protocol(int cls, int sub, int prot)
{
  TQString *s = _classes[TQString("%1-%2-%2").arg(cls).arg(sub).arg(prot)];
  if (s)
    return *s;
  return TQString::null;
}

