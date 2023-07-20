/*  This file is part of the KDE project
    Copyright (C) 2002 Alexander Neundorf <neundorf@kde.org>

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


#ifndef KMANPART_H
#define KMANPART_H

#include <tdeparts/factory.h>
#include <tdeparts/part.h>
#include <tdeparts/browserextension.h>
#include <tdehtml_part.h>
#include <tdeio/job.h>
#include <tdeio/jobclasses.h>

#include <tqcstring.h>

class TDEInstance;
class TDEAboutData;

/**
 * Man Page Viewer
 * \todo: Why is it needed? Why is TDEHTML alone not possible?
 */
class KManPartFactory: public KParts::Factory
{
   TQ_OBJECT
   public:
      KManPartFactory( TQObject * parent = 0, const char * name = 0 );
      virtual ~KManPartFactory();

      virtual KParts::Part* createPartObject( TQWidget * parentWidget, const char * widgetName ,
                                TQObject* parent, const char* name, const char * classname,
                                const TQStringList &args);

      static TDEInstance * instance();

   private:
      static TDEInstance * s_instance;
      static TDEAboutData * s_about;

};

class KManPart : public TDEHTMLPart
{
   TQ_OBJECT
   public:
      KManPart( TQWidget * parent, const char * name = 0L );
      KParts::BrowserExtension * extension() {return m_extension;}

   public slots:
      virtual bool openURL( const KURL &url );
   protected slots:
      void readData(TDEIO::Job * , const TQByteArray & data);
      void jobDone( TDEIO::Job *);
   protected:
      virtual bool openFile();
      TDEInstance *m_instance;
      KParts::BrowserExtension * m_extension;
      TDEIO::TransferJob *m_job;
};

#endif

