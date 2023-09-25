/*
    This file is part of KHelpCenter.

    Copyright (c) 2005 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef KHC_SEARCHHANDLER_H
#define KHC_SEARCHHANDLER_H

#include "searchengine.h"

#include <tqobject.h>
#include <tqstringlist.h>

namespace TDEIO {
class Job;
}

namespace KHC {

class SearchJob
{
  public:
    SearchJob() : mProcess( 0 ), mKioJob( 0 ) {}

    DocEntry *mEntry;
  
    TDEProcess *mProcess;
    TDEIO::Job *mKioJob;
    
    TQString mCmd;
    
    TQString mResult;
    TQString mError;
};

class SearchHandler : public TQObject
{
    TQ_OBJECT
  public:
    static SearchHandler *initFromFile( const TQString &filename );

    void search( DocEntry *, const TQStringList &words,
      int maxResults = 10,
      SearchEngine::Operation operation = SearchEngine::And );

    TQString indexCommand( const TQString &identifier );

    TQStringList documentTypes() const;

    bool checkPaths() const;

  signals:
    void searchFinished( SearchHandler *, DocEntry *, const TQString & );
    void searchError( SearchHandler *, DocEntry *, const TQString & );

  protected:
    bool checkBinary( const TQString &cmd ) const;

  protected slots:
    void searchStdout( TDEProcess *proc, char *buffer, int buflen );
    void searchStderr( TDEProcess *proc, char *buffer, int buflen );
    void searchExited( TDEProcess *proc ); 

    void slotJobResult( TDEIO::Job *job );
    void slotJobData( TDEIO::Job *, const TQByteArray &data );

  private:
    SearchHandler();

    TQString mLang;

    TQString mSearchCommand;
    TQString mSearchUrl;
    TQString mIndexCommand;
    TQStringList mDocumentTypes;

    TQMap<TDEProcess *,SearchJob *> mProcessJobs;
    TQMap<TDEIO::Job *,SearchJob *> mKioJobs;
};

}

#endif
