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

#include "kmanpart.h"
#include <tqstring.h>

#include <kinstance.h>
#include <tdeglobal.h>
#include <kdebug.h>
#include <tdelocale.h>
#include <kstandarddirs.h>
#include <tdeaboutdata.h>
#include <tdeversion.h>

extern "C"
{
   KDE_EXPORT void* init_libkmanpart()
   {
      return new KManPartFactory;
   }
}

TDEInstance* KManPartFactory::s_instance = 0L;
TDEAboutData* KManPartFactory::s_about = 0L;

KManPartFactory::KManPartFactory( TQObject* parent, const char* name )
   : KParts::Factory( parent, name )
{}

KManPartFactory::~KManPartFactory()
{
   delete s_instance;
   s_instance = 0L;
   delete s_about;
}

KParts::Part* KManPartFactory::createPartObject( TQWidget * parentWidget, const char* /*widgetName*/, TQObject *,
                                 const char* name, const char* /*className*/,const TQStringList & )
{
   KManPart* part = new KManPart(parentWidget, name );
   return part;
}

TDEInstance* KManPartFactory::instance()
{
   if( !s_instance )
   {
      s_about = new TDEAboutData( "kmanpart",
                                I18N_NOOP( "KMan" ), TDE_VERSION_STRING );
      s_instance = new TDEInstance( s_about );
   }
   return s_instance;
}


KManPart::KManPart( TQWidget * parent, const char * name )
: TDEHTMLPart( parent, name )
,m_job(0)
{
   TDEInstance * instance = new TDEInstance( "kmanpart" );
   setInstance( instance );
   m_extension=new KParts::BrowserExtension(this);
}

bool KManPart::openURL( const KURL &url )
{
   return KParts::ReadOnlyPart::openURL(url);
}

bool KManPart::openFile()
{
   if (m_job!=0)
      m_job->kill();

   begin();

   KURL url;
   url.setProtocol( "man" );
   url.setPath( m_file );

   m_job = TDEIO::get( url, true, false );
   connect( m_job, TQ_SIGNAL( data( TDEIO::Job *, const TQByteArray &) ), TQ_SLOT( readData( TDEIO::Job *, const TQByteArray &) ) );
   connect( m_job, TQ_SIGNAL( result( TDEIO::Job * ) ), TQ_SLOT( jobDone( TDEIO::Job * ) ) );
   return true;
}

void KManPart::readData(TDEIO::Job * , const TQByteArray & data)
{
   write(data,data.size());
}

void KManPart::jobDone( TDEIO::Job *)
{
   m_job=0;
   end();
}

#include "kmanpart.moc"

