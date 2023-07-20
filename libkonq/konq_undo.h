/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

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
#ifndef __konq_undo_h__
#define __konq_undo_h__

#include <tqobject.h>
#include <tqstring.h>
#include <tqvaluestack.h>

#include <dcopobject.h>

#include <kurl.h>
#include <libkonq_export.h>

namespace TDEIO
{
  class Job;
}

class KonqUndoJob;

struct KonqBasicOperation
{
  typedef TQValueStack<KonqBasicOperation> Stack;

  KonqBasicOperation()
  { m_valid = false; }

  bool m_valid;
  bool m_directory;
  bool m_renamed;
  bool m_link;
  KURL m_src;
  KURL m_dst;
  TQString m_target;
};

struct KonqCommand
{
  typedef TQValueStack<KonqCommand> Stack;

  enum Type { COPY, MOVE, LINK, MKDIR, TRASH };

  KonqCommand()
  { m_valid = false; }

  bool m_valid;

  Type m_type;
  KonqBasicOperation::Stack m_opStack;
  KURL::List m_src;
  KURL m_dst;
};

class KonqCommandRecorder : public TQObject
{
  TQ_OBJECT
public:
  KonqCommandRecorder( KonqCommand::Type op, const KURL::List &src, const KURL &dst, TDEIO::Job *job );
  virtual ~KonqCommandRecorder();

private slots:
  void slotResult( TDEIO::Job *job );

  void slotCopyingDone( TDEIO::Job *, const KURL &from, const KURL &to, bool directory, bool renamed );
  void slotCopyingLinkDone( TDEIO::Job *, const KURL &from, const TQString &target, const KURL &to );

private:
  class KonqCommandRecorderPrivate;
  KonqCommandRecorderPrivate *d;
};

class LIBKONQ_EXPORT KonqUndoManager : public TQObject, public DCOPObject
{
  TQ_OBJECT
  K_DCOP
  friend class KonqUndoJob;
public:
  enum UndoState { MAKINGDIRS, MOVINGFILES, REMOVINGDIRS, REMOVINGFILES };

  KonqUndoManager();
  virtual ~KonqUndoManager();

  static void incRef();
  static void decRef();
  static KonqUndoManager *self();

  void addCommand( const KonqCommand &cmd );

  bool undoAvailable() const;
  TQString undoText() const;

public slots:
  void undo();

signals:
  void undoAvailable( bool avail );
  void undoTextChanged( const TQString &text );

protected:
  /**
   * @internal
   */
  void stopUndo( bool step );

private:
k_dcop:
  virtual ASYNC push( const KonqCommand &cmd );
  virtual ASYNC pop();
  virtual ASYNC lock();
  virtual ASYNC unlock();

  virtual KonqCommand::Stack get() const;

private slots:
  void slotResult( TDEIO::Job *job );

private:
  void undoStep();

  void undoMakingDirectories();
  void undoMovingFiles();
  void undoRemovingFiles();
  void undoRemovingDirectories();

  void broadcastPush( const KonqCommand &cmd );
  void broadcastPop();
  void broadcastLock();
  void broadcastUnlock();

  void addDirToUpdate( const KURL& url );
  bool initializeFromKDesky();

  class KonqUndoManagerPrivate;
  KonqUndoManagerPrivate *d;
  static KonqUndoManager *s_self;
  static unsigned long s_refCnt;
};

TQDataStream &operator<<( TQDataStream &stream, const KonqBasicOperation &op );
TQDataStream &operator>>( TQDataStream &stream, KonqBasicOperation &op );

TQDataStream &operator<<( TQDataStream &stream, const KonqCommand &cmd );
TQDataStream &operator>>( TQDataStream &stream, KonqCommand &cmd );

#endif
