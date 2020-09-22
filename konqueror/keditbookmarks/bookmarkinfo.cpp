/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "bookmarkinfo.h"
#include "commands.h"
#include "toplevel.h"

#include <stdlib.h>

#include <tqtimer.h>
#include <tqclipboard.h>
#include <tqsplitter.h>
#include <tqlayout.h>
#include <tqlabel.h>

#include <tdelocale.h>
#include <kdebug.h>

#include <tdeapplication.h>
#include <kstdaction.h>
#include <tdeaction.h>
#include <dcopclient.h>
#include <dcopref.h>

#include <kkeydialog.h>
#include <kedittoolbar.h>
#include <tdemessagebox.h>
#include <klineedit.h>
#include <tdefiledialog.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>

// SHUFFLE all these functions around, the order is just plain stupid

BookmarkLineEdit::BookmarkLineEdit( TQWidget *parent )
    : KLineEdit( parent )
{
}

void BookmarkLineEdit::cut()
{
    TQString select( selectedText() );
    int pos( selectionStart() );
    TQString newText(  text().remove( pos, select.length() ) );
    KLineEdit::cut();
    setEdited( true ); //KDE 4 setModified( true );
    emit textChanged( newText );
    setText( newText );
}


void BookmarkInfoWidget::showBookmark(const KBookmark &bk) {
    commitChanges();
    m_bk = bk;

    if (m_bk.isNull()) {
        // all read only and blank

        m_title_le->setReadOnly(true);
        m_title_le->setText(TQString::null);

        m_url_le->setReadOnly(true);
        m_url_le->setText(TQString::null);

        m_comment_le->setReadOnly(true);
        m_comment_le->setText(TQString::null);

        m_visitdate_le->setReadOnly(true);
        m_visitdate_le->setText(TQString::null);

        m_credate_le->setReadOnly(true);
        m_credate_le->setText(TQString::null);

        m_visitcount_le->setReadOnly(true);
        m_visitcount_le->setText(TQString::null);

        return;
    }

    // read/write fields
    m_title_le->setReadOnly( (bk.isSeparator()|| !bk.hasParent() )? true : false);
    m_title_le->setText(bk.fullText());

    m_url_le->setReadOnly(bk.isGroup() || bk.isSeparator());
    m_url_le->setText(bk.isGroup() ? TQString::null : bk.url().pathOrURL());

    m_comment_le->setReadOnly((bk.isSeparator()|| !bk.hasParent()) ? true : false );
    m_comment_le->setText(
            NodeEditCommand::getNodeText(bk, TQStringList() << "desc"));

    // readonly fields
    updateStatus();
 
}

void BookmarkInfoWidget::updateStatus()
{
   TQString visitDate =
        CurrentMgr::makeTimeStr( NodeEditCommand::getNodeText(m_bk, TQStringList() << "info" << "metadata"
                                                             << "time_visited" ));
    m_visitdate_le->setReadOnly(true);
    m_visitdate_le->setText(visitDate);

    TQString creationDate =
        CurrentMgr::makeTimeStr( NodeEditCommand::getNodeText(m_bk, TQStringList() << "info" << "metadata"
                                                             << "time_added" ));
    m_credate_le->setReadOnly(true);
    m_credate_le->setText(creationDate);

    // TODO - get the actual field name from the spec if it exists, else copy galeon
    m_visitcount_le->setReadOnly(true);
    m_visitcount_le->setText(
            NodeEditCommand::getNodeText(m_bk, TQStringList() << "info" << "metadata"
                                                           << "visit_count" ));
}

void BookmarkInfoWidget::commitChanges()
{
    commitTitle();
    commitURL();
    commitComment();
}

void BookmarkInfoWidget::commitTitle()
{
    if(titlecmd)
    {
        emit updateListViewItem();
        CurrentMgr::self()->notifyManagers(CurrentMgr::bookmarkAt(titlecmd->affectedBookmarks()).toGroup());
        titlecmd = 0;
    }
}

void BookmarkInfoWidget::slotTextChangedTitle(const TQString &str) 
{
    if (m_bk.isNull() || !m_title_le->isModified())
        return;

    timer->start(1000, true);

    if(titlecmd)
    {
        NodeEditCommand::setNodeText(m_bk, TQStringList() << "title", str);
        titlecmd->modify(str);
    }
    else
    {
        titlecmd = new NodeEditCommand(m_bk.address(), str, "title");
        titlecmd->execute();
        CmdHistory::self()->addInFlightCommand(titlecmd);
    }
}

void BookmarkInfoWidget::commitURL()
{
    if(urlcmd)
    {
        emit updateListViewItem();
        CurrentMgr::self()->notifyManagers(CurrentMgr::bookmarkAt(urlcmd->affectedBookmarks()).toGroup());
        urlcmd = 0;
    }
}

void BookmarkInfoWidget::slotTextChangedURL(const TQString &str) {
    if (m_bk.isNull() || !m_url_le->isModified())
        return;

    timer->start(1000, true);

    if(urlcmd)
    {
        KURL u = KURL::fromPathOrURL(str);
        m_bk.internalElement().setAttribute("href", u.url(0, 106));
        urlcmd->modify("href", u.url(0, 106));
    }
    else
    {
        KURL u = KURL::fromPathOrURL(str);
        urlcmd = new EditCommand(m_bk.address(), EditCommand::Edition("href", u.url(0, 106)), i18n("URL"));
        urlcmd->execute();
        CmdHistory::self()->addInFlightCommand(urlcmd);
    }
}

void BookmarkInfoWidget::commitComment()
{
    if(commentcmd)
    {
        emit updateListViewItem();
        CurrentMgr::self()->notifyManagers( CurrentMgr::bookmarkAt( commentcmd->affectedBookmarks() ).toGroup());
        commentcmd = 0;
    }
}

void BookmarkInfoWidget::slotTextChangedComment(const TQString &str) {
    if (m_bk.isNull() || !m_comment_le->isModified())
        return;

    timer->start(1000, true);

    if(commentcmd)
    {
        NodeEditCommand::setNodeText(m_bk, TQStringList() << "desc", str);
        commentcmd->modify(str);
    }
    else
    {
        commentcmd = new NodeEditCommand(m_bk.address(), str, "desc");
        commentcmd->execute();
        CmdHistory::self()->addInFlightCommand(commentcmd);
    }
}

BookmarkInfoWidget::BookmarkInfoWidget(TQWidget *parent, const char *name)
    : TQWidget(parent, name), m_connected(false) {

    timer = new TQTimer(this);
    connect(timer, TQT_SIGNAL( timeout() ), TQT_SLOT( commitChanges()));

    titlecmd = 0;
    urlcmd = 0;
    commentcmd = 0;

    TQBoxLayout *vbox = new TQVBoxLayout(this);
    TQGridLayout *grid = new TQGridLayout(vbox, 3, 4, 4);

    m_title_le = new BookmarkLineEdit(this);
    grid->addWidget(m_title_le, 0, 1);
    grid->addWidget(
            new TQLabel(m_title_le, i18n("Name:"), this),
            0, 0);

    connect(m_title_le, TQT_SIGNAL( textChanged(const TQString &) ),
                        TQT_SLOT( slotTextChangedTitle(const TQString &) ));
    connect(m_title_le, TQT_SIGNAL( lostFocus() ), TQT_SLOT( commitTitle() ));

    m_url_le = new BookmarkLineEdit(this);
    grid->addWidget(m_url_le, 1, 1);
    grid->addWidget(
            new TQLabel(m_url_le, i18n("Location:"), this),
            1, 0);

    connect(m_url_le, TQT_SIGNAL( textChanged(const TQString &) ),
                      TQT_SLOT( slotTextChangedURL(const TQString &) ));
    connect(m_url_le, TQT_SIGNAL( lostFocus() ), TQT_SLOT( commitURL() ));

    m_comment_le = new BookmarkLineEdit(this);
    grid->addWidget(m_comment_le, 2, 1);
    grid->addWidget(
            new TQLabel(m_comment_le, i18n("Comment:"), this),
            2, 0);
    connect(m_comment_le, TQT_SIGNAL( textChanged(const TQString &) ),
                          TQT_SLOT( slotTextChangedComment(const TQString &) ));
    connect(m_comment_le, TQT_SIGNAL( lostFocus() ), TQT_SLOT( commitComment() ));

    m_credate_le = new KLineEdit(this);
    grid->addWidget(m_credate_le, 0, 3);
    grid->addWidget(
            new TQLabel(m_credate_le, i18n("First viewed:"), this),
            0, 2);

    m_visitdate_le = new KLineEdit(this);
    grid->addWidget(m_visitdate_le, 1, 3);
    grid->addWidget(
            new TQLabel(m_visitdate_le, i18n("Viewed last:"), this),
            1, 2 );

    m_visitcount_le = new KLineEdit(this);
    grid->addWidget(m_visitcount_le, 2, 3);
    grid->addWidget(
            new TQLabel(m_visitcount_le, i18n("Times visited:"), this),
            2, 2);
}

#include "bookmarkinfo.moc"

