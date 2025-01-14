/*
 *   tdeprintfax - a small fax utility
 *   Copyright (C) 2001  Michael Goffioul
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "tdeprintfax.h"
#include "faxab.h"
#include "faxctrl.h"
#include "configdlg.h"

#include <tqcheckbox.h>
#include <tqlineedit.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqtextedit.h>
#include <tqdatetimeedit.h>
#include <tqcombobox.h>
#include <tqtooltip.h>

#include <tdeapplication.h>
#include <kstdaction.h>
#include <tdemenubar.h>
#include <tdetoolbar.h>
#include <tdeaction.h>
#include <tdelistbox.h>
#include <tdelistview.h>
#include <tqheader.h>
#include <tdelocale.h>
#include <kiconloader.h>
#include <tdeio/netaccess.h>
#include <tdemessagebox.h>
#include <tdefiledialog.h>
#include <kmimetype.h>
#include <kseparator.h>
#include <ksystemtray.h>
#include <kstatusbar.h>
#include <ksqueezedtextlabel.h>
#include <krun.h>
#include <kopenwith.h>
#include <kpushbutton.h>
#include <kurldrag.h>
#include <kdebug.h>

KdeprintFax::KdeprintFax(TQWidget *parent, const char *name)
: TDEMainWindow(parent, name)
{
	m_faxctrl = new FaxCtrl(this);
	m_quitAfterSend = false;
	connect(m_faxctrl, TQ_SIGNAL(message(const TQString&)), TQ_SLOT(slotMessage(const TQString&)));
	connect(m_faxctrl, TQ_SIGNAL(faxSent(bool)), TQ_SLOT(slotFaxSent(bool)));

	TQWidget	*mainw = new TQWidget(this);
	setCentralWidget(mainw);
	m_files = new TDEListBox(mainw);
	connect( m_files, TQ_SIGNAL( currentChanged( TQListBoxItem* ) ), TQ_SLOT( slotCurrentChanged() ) );
	m_upbtn = new KPushButton( mainw );
	m_upbtn->setIconSet( SmallIconSet( "go-up" ) );
	TQToolTip::add( m_upbtn, i18n( "Move up" ) );
	connect( m_upbtn, TQ_SIGNAL( clicked() ), TQ_SLOT( slotMoveUp() ) );
	m_upbtn->setEnabled( false );
	m_downbtn = new KPushButton( mainw );
	m_downbtn->setIconSet( SmallIconSet( "go-down" ) );
	TQToolTip::add( m_downbtn, i18n( "Move down" ) );
	connect( m_downbtn, TQ_SIGNAL( clicked() ), TQ_SLOT( slotMoveDown() ) );
	m_downbtn->setEnabled( false );
	TQLabel	*m_filelabel = new TQLabel(i18n("F&iles:"), mainw);
        m_filelabel->setBuddy(m_files);
        KSeparator*m_line = new KSeparator( KSeparator::HLine, mainw);
		KSeparator *m_line2 = new KSeparator( KSeparator::HLine, mainw );
	m_numbers = new TDEListView( mainw );
	m_numbers->addColumn( i18n("Fax Number") );
	m_numbers->addColumn( i18n("Name") );
	m_numbers->addColumn( i18n("Enterprise") );
	m_numbers->header()->setStretchEnabled( true );
	m_numbers->setSelectionMode( TQListView::Extended );
	connect( m_numbers, TQ_SIGNAL( selectionChanged() ), TQ_SLOT( slotFaxSelectionChanged() ) );
	connect( m_numbers, TQ_SIGNAL( executed( TQListViewItem* ) ), TQ_SLOT( slotFaxExecuted( TQListViewItem* ) ) );
	m_newbtn = new KPushButton( mainw );
	m_newbtn->setPixmap( SmallIcon( "edit" ) );
	TQToolTip::add( m_newbtn, i18n( "Add fax number" ) );
	connect( m_newbtn, TQ_SIGNAL( clicked() ), TQ_SLOT( slotFaxAdd() ) );
	m_abbtn = new KPushButton( mainw );
	m_abbtn->setPixmap( SmallIcon( "kaddressbook" ) );
	TQToolTip::add( m_abbtn, i18n( "Add fax number from addressbook" ) );
	connect( m_abbtn, TQ_SIGNAL( clicked() ), TQ_SLOT( slotKab() ) );
	m_delbtn = new KPushButton( mainw );
	m_delbtn->setIconSet( SmallIconSet( "edittrash" ) );
	TQToolTip::add( m_delbtn, i18n( "Remove fax number" ) );
	m_delbtn->setEnabled( false );
	connect( m_delbtn, TQ_SIGNAL( clicked() ), TQ_SLOT( slotFaxRemove() ) );
	TQLabel	*m_commentlabel = new TQLabel(i18n("&Comment:"), mainw);
	KSystemTray	*m_tray = new KSystemTray(this);
	m_tray->setPixmap(SmallIcon("tdeprintfax"));
	m_tray->show();
	m_comment = new TQTextEdit(mainw);
// I don't understand why anyone would want to turn off word wrap. It makes
// the text hard to read and write. It provides no benefit. Therefore,
// I commented out the next line. [Ray Lischner]
//	m_comment->setWordWrap(TQTextEdit::NoWrap);
	m_comment->setLineWidth(1);
	m_commentlabel->setBuddy(m_comment);
	TQLabel	*m_timelabel = new TQLabel(i18n("Sched&ule:"), mainw);
	m_timecombo = new TQComboBox(mainw);
	m_timecombo->insertItem(i18n("Now"));
	m_timecombo->insertItem(i18n("At Specified Time"));
	m_timecombo->setCurrentItem(0);
	m_timelabel->setBuddy(m_timecombo);
	m_time = new TQTimeEdit(mainw);
	m_time->setTime(TQTime::currentTime());
	m_time->setEnabled(false);
	connect(m_timecombo, TQ_SIGNAL(activated(int)), TQ_SLOT(slotTimeComboActivated(int)));
	m_cover = new TQCheckBox(i18n("Send Co&ver Sheet"), mainw);
	connect(m_cover, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotCoverToggled(bool)));
	m_subject = new TQLineEdit( mainw );
	TQLabel *m_subjectlabel = new TQLabel( i18n( "Su&bject:" ), mainw );
	m_subjectlabel->setBuddy( m_subject );

	TQGridLayout	*l0 = new TQGridLayout(mainw, 10, 2, 10, 5);
	l0->setColStretch(1,1);
	l0->addWidget(m_filelabel, 0, 0, TQt::AlignLeft|TQt::AlignTop);
	TQHBoxLayout *l2 = new TQHBoxLayout( 0, 0, 10 );
	TQVBoxLayout *l3 = new TQVBoxLayout( 0, 0, 5 );
	l0->addLayout( l2, 0, 1 );
	l2->addWidget( m_files );
	l2->addLayout( l3 );
	//l3->addStretch( 1 );
	l3->addWidget( m_upbtn );
	l3->addWidget( m_downbtn );
	l3->addStretch( 1 );
	l0->addMultiCellWidget(m_line, 1, 1, 0, 1);
	l0->addRowSpacing(1, 10);
	TQHBoxLayout *l5 = new TQHBoxLayout( 0, 0, 10 );
	TQVBoxLayout *l6 = new TQVBoxLayout( 0, 0, 5 );
	l0->addMultiCellLayout( l5, 2, 4, 0, 1 );
	l5->addWidget( m_numbers );
	l5->addLayout( l6 );
	l6->addWidget( m_newbtn );
	l6->addWidget( m_delbtn );
	l6->addWidget( m_abbtn );
	l6->addStretch( 1 );
	l0->addMultiCellWidget( m_line2, 5, 5, 0, 1 );
	l0->addRowSpacing( 5, 10 );
	l0->addWidget( m_cover, 6, 1 );
	l0->addWidget( m_subjectlabel, 7, 0 );
	l0->addWidget( m_subject, 7, 1 );
	l0->addWidget(m_commentlabel, 8, 0, TQt::AlignTop|TQt::AlignLeft);
	l0->addWidget(m_comment, 8, 1);
	l0->addWidget(m_timelabel, 9, 0);
	TQHBoxLayout	*l1 = new TQHBoxLayout(0, 0, 5);
	l0->addLayout(l1, 9, 1);
	l1->addWidget(m_timecombo, 1);
	l1->addWidget(m_time, 0);

	m_msglabel = new KSqueezedTextLabel(statusBar());
	statusBar()->addWidget(m_msglabel, 1);
	statusBar()->insertFixedItem(i18n("Processing..."), 1);
	statusBar()->changeItem(i18n("Idle"), 1);
	statusBar()->insertFixedItem("hylafax/efax", 2);
	initActions();
	setAcceptDrops(true);
	setCaption(i18n("Send to Fax"));
	updateState();

	resize(550,500);
	TQWidget	*d = tdeApp->desktop();
	move((d->width()-width())/2, (d->height()-height())/2);
}

KdeprintFax::~KdeprintFax()
{
}

void KdeprintFax::initActions()
{
	new TDEAction(i18n("&Add File..."), "document-new", TQt::Key_Insert, this, TQ_SLOT(slotAdd()), actionCollection(), "file_add");
	new TDEAction(i18n("&Remove File"), "remove", TQt::Key_Delete, this, TQ_SLOT(slotRemove()), actionCollection(), "file_remove");
	new TDEAction(i18n("&Send Fax"), "connect_established", TQt::Key_Return, this, TQ_SLOT(slotFax()), actionCollection(), "fax_send");
	new TDEAction(i18n("A&bort"), "process-stop", TQt::Key_Escape, this, TQ_SLOT(slotAbort()), actionCollection(), "fax_stop");
	new TDEAction(i18n("A&ddress Book"), "kaddressbook", TQt::CTRL+TQt::Key_A, this, TQ_SLOT(slotKab()), actionCollection(), "fax_ab");
	new TDEAction(i18n("V&iew Log"), "contents", TQt::CTRL+TQt::Key_L, this, TQ_SLOT(slotViewLog()), actionCollection(), "fax_log");
	new TDEAction(i18n("Vi&ew File"), "filefind", TQt::CTRL+TQt::Key_O, this, TQ_SLOT(slotView()), actionCollection(), "file_view");
	new TDEAction( i18n( "&New Fax Recipient..." ), "edit", TQt::CTRL+TQt::Key_N, this, TQ_SLOT( slotFaxAdd() ), actionCollection(), "fax_add" );

	KStdAction::quit(this, TQ_SLOT(slotQuit()), actionCollection());
	setStandardToolBarMenuEnabled(true);
	KStdAction::showMenubar(this, TQ_SLOT(slotToggleMenuBar()), actionCollection());
	KStdAction::preferences(this, TQ_SLOT(slotConfigure()), actionCollection());
        KStdAction::keyBindings(guiFactory(), TQ_SLOT(configureShortcuts()), 
actionCollection());
	actionCollection()->action("fax_stop")->setEnabled(false);
	connect(actionCollection()->action("file_remove"), TQ_SIGNAL(enabled(bool)), actionCollection()->action("file_view"), TQ_SLOT(setEnabled(bool)));
	actionCollection()->action("file_remove")->setEnabled(false);

	createGUI();
}

void KdeprintFax::slotToggleMenuBar()
{
	if (menuBar()->isVisible()) menuBar()->hide();
	else menuBar()->show();
}

void KdeprintFax::slotAdd()
{
	KURL	url = KFileDialog::getOpenURL(TQString::null, TQString::null, this);
	if (!url.isEmpty())
		addURL(url);
}

void KdeprintFax::slotRemove()
{
	if (m_files->currentItem() >= 0)
		m_files->removeItem(m_files->currentItem());
	if (m_files->count() == 0)
		actionCollection()->action("file_remove")->setEnabled(false);
}

void KdeprintFax::slotView()
{
	if (m_files->currentItem() >= 0)
	{
		new KRun(KURL( m_files->currentText() ));
	}
}

void KdeprintFax::slotFax()
{
	if (m_files->count() == 0)
		KMessageBox::error(this, i18n("No file to fax."));
	else if ( m_numbers->childCount() == 0 )
		KMessageBox::error(this, i18n("No fax number specified."));
	else if (m_faxctrl->send(this))
	{
		actionCollection()->action("fax_send")->setEnabled(false);
		actionCollection()->action("fax_stop")->setEnabled(true);
		statusBar()->changeItem(i18n("Processing..."), 1);
	}
	else
		KMessageBox::error(this, i18n("Unable to start Fax process."));
}

void KdeprintFax::slotAbort()
{
	if (!m_faxctrl->abort())
		KMessageBox::error(this, i18n("Unable to stop Fax process."));
}

void KdeprintFax::slotKab()
{
	TQStringList	number, name, enterprise;
	if (FaxAB::getEntry(number, name, enterprise, this))
	{
		for ( unsigned int i = 0; i<number.count(); i++ )
			new TQListViewItem( m_numbers, number[ i ], name[ i ], enterprise[ i ] );
	}
}

void KdeprintFax::addURL(KURL url)
{
	TQString	target;
	if (TDEIO::NetAccess::download(url,target,this))
	{
		m_files->insertItem(KMimeType::pixmapForURL(url,0,TDEIcon::Small),target);
		actionCollection()->action("file_remove")->setEnabled(true);
		slotCurrentChanged();
	}
	else
		KMessageBox::error(this, i18n("Unable to retrieve %1.").arg(url.prettyURL()));
}

void KdeprintFax::setPhone(TQString phone)
{
	TQString name, enterprise;
	FaxAB::getEntryByNumber(phone, name, enterprise);
	new TQListViewItem( m_numbers, phone, name, enterprise );
}

void KdeprintFax::sendFax( bool quitAfterSend )
{
	slotFax();
	m_quitAfterSend = quitAfterSend;
}

void KdeprintFax::dragEnterEvent(TQDragEnterEvent *e)
{
	e->accept(KURLDrag::canDecode(e));
}

void KdeprintFax::dropEvent(TQDropEvent *e)
{
	KURL::List l;
	if (KURLDrag::decode(e, l))
	{
		for (KURL::List::ConstIterator it = l.begin(); it != l.end(); ++it)
			addURL(*it);
	}
}

TQStringList KdeprintFax::files()
{
	TQStringList	l;
	for (uint i=0; i<m_files->count(); i++)
		l.append(m_files->text(i));
	return l;
}


int KdeprintFax::faxCount() const
{
	return m_numbers->childCount();
}

/*
TQListViewItem* KdeprintFax::faxItem( int i ) const
{
	TQListViewItem *item = m_numbers->firstChild();
	while ( i && item && item->nextSibling() )
	{
		item = item->nextSibling();
		i--;
	}
	if ( i || !item )
		kdError() << "KdeprintFax::faxItem(" << i << ") => fax item index out of bound" << endl;
	return item;
}

TQString KdeprintFax::number( int i ) const
{
	TQListViewItem *item = faxItem( i );
	return ( item ? item->text( 0 ) : TQString::null );
}

TQString KdeprintFax::name( int i ) const
{
	TQListViewItem *item = faxItem( i );
	return ( item ? item->text( 1 ) : TQString::null );
}

TQString KdeprintFax::enterprise( int i ) const
{
	TQListViewItem *item = faxItem( i );
	return ( item ? item->text( 2 ) : TQString::null );
}
*/

KdeprintFax::FaxItemList KdeprintFax::faxList() const
{
	FaxItemList list;
	TQListViewItemIterator it( m_numbers );
	while ( it.current() )
	{
		FaxItem item;
		item.number = it.current()->text( 0 );
		item.name = it.current()->text( 1 );
		item.enterprise = it.current()->text( 2 );
		list << item;
		++it;
	}
	return list;
}

TQString KdeprintFax::comment() const
{
	return m_comment->text();
}

bool KdeprintFax::cover() const
{
	return m_cover->isChecked();
}

TQString KdeprintFax::subject() const
{
	return m_subject->text();
}

void KdeprintFax::slotMessage(const TQString& msg)
{
	m_msglabel->setText(msg);
}

void KdeprintFax::slotFaxSent(bool status)
{
	actionCollection()->action("fax_send")->setEnabled(true);
	actionCollection()->action("fax_stop")->setEnabled(false);
	statusBar()->changeItem(i18n("Idle"), 1);

 	if( m_quitAfterSend ) {
		slotQuit();
	}
	else {
		if (!status)
			KMessageBox::error(this, i18n("Fax error: see log message for more information."));
		slotMessage(TQString::null);
	}
}

void KdeprintFax::slotViewLog()
{
	m_faxctrl->viewLog(this);
}

void KdeprintFax::slotConfigure()
{
	if (ConfigDlg::configure(this))
		updateState();
}

void KdeprintFax::updateState()
{
	TQString	cmd = m_faxctrl->faxCommand();
	m_cover->setEnabled(cmd.find("%cover") != -1);
	if ( !m_cover->isEnabled() )
		m_cover->setChecked(false);
	m_comment->setEnabled(cmd.find("%comment") != -1 && m_cover->isChecked());
	//m_comment->setPaper(m_comment->isEnabled() ? colorGroup().brush(TQColorGroup::Base) : colorGroup().brush(TQColorGroup::Background));
	if (!m_comment->isEnabled())
	{
		m_comment->setText("");
		m_comment->setPaper( colorGroup().background() );
	}
	else
		m_comment->setPaper( colorGroup().base() );
	/*
	m_enterprise->setEnabled(cmd.find("%enterprise") != -1);
	if (!m_enterprise->isEnabled())
		m_enterprise->setText("");
	*/
	if (cmd.find("%time") == -1)
	{
		m_timecombo->setCurrentItem(0);
		m_timecombo->setEnabled(false);
		slotTimeComboActivated(0);
	}
	else
		m_timecombo->setEnabled( true );
	/*m_name->setEnabled( cmd.find( "%name" ) != -1 );*/
	m_subject->setEnabled( cmd.find( "%subject" ) != -1 && m_cover->isChecked() );
	statusBar()->changeItem(m_faxctrl->faxSystem(), 2);
}

void KdeprintFax::slotQuit()
{
	close(true);
}

void KdeprintFax::slotTimeComboActivated(int ID)
{
	m_time->setEnabled(ID == 1);
}

TQString KdeprintFax::time() const
{
	if (!m_time->isEnabled())
		return TQString::null;
	return m_time->time().toString("hh:mm");
}

void KdeprintFax::slotMoveUp()
{
	int index = m_files->currentItem();
	if ( index > 0 )
	{
		TQListBoxItem *item = m_files->item( index );
		m_files->takeItem( item );
		m_files->insertItem( item, index-1 );
		m_files->setCurrentItem( index-1 );
	}
}

void KdeprintFax::slotMoveDown()
{
	int index = m_files->currentItem();
	if ( index >= 0 && index < ( int )m_files->count()-1 )
	{
		TQListBoxItem *item = m_files->item( index );
		m_files->takeItem( item );
		m_files->insertItem( item, index+1 );
		m_files->setCurrentItem( index+1 );
	}
}

/** The user or program toggled the "Cover Sheet" check box.
 * Update the state of the other controls to reflect the
 * new status.
 */
void KdeprintFax::slotCoverToggled(bool)
{
	updateState();
}

void KdeprintFax::slotCurrentChanged()
{
	int index = m_files->currentItem();
	m_upbtn->setEnabled( index > 0 );
	m_downbtn->setEnabled( index >=0 && index < ( int )m_files->count()-1 );
}

void KdeprintFax::slotFaxSelectionChanged()
{
	TQListViewItemIterator it( m_numbers, TQListViewItemIterator::Selected );
	m_delbtn->setEnabled( it.current() != NULL );
}

void KdeprintFax::slotFaxRemove()
{
	TQListViewItemIterator it( m_numbers, TQListViewItemIterator::Selected );
	TQPtrList<TQListViewItem> items;
	items.setAutoDelete( true );
	while ( it.current() )
	{
		items.append( it.current() );
		++it;
	}
	items.clear();
	/* force this slot to be called, to update buttons state */
	slotFaxSelectionChanged();
}

void KdeprintFax::slotFaxAdd()
{
	TQString number, name, enterprise;
	if ( manualFaxDialog( number, name, enterprise ) )
	{
		new TQListViewItem( m_numbers, number, name, enterprise );
	}
}

void KdeprintFax::slotFaxExecuted( TQListViewItem *item )
{
	if ( item )
	{
		TQString number = item->text( 0 ), name = item->text( 1 ), enterprise = item->text( 2 );
		if ( manualFaxDialog( number, name, enterprise ) )
		{
			item->setText( 0, number );
			item->setText( 1, name );
			item->setText( 2, enterprise );
		}
	}
}

bool KdeprintFax::manualFaxDialog( TQString& number, TQString& name, TQString& enterprise )
{
	/* dialog construction */
	KDialogBase dlg( this, "manualFaxDialog", true, i18n( "Fax Number" ), KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, true );
	TQWidget *mainw = new TQWidget( &dlg );
	TQLabel *lab0 = new TQLabel( i18n( "Enter recipient fax properties." ), mainw );
	TQLabel *lab1 = new TQLabel( i18n( "&Number:" ), mainw );
	TQLabel *lab2 = new TQLabel( i18n( "N&ame:" ), mainw );
	TQLabel *lab3 = new TQLabel( i18n( "&Enterprise:" ), mainw );
	TQLineEdit *edit_number = new TQLineEdit( number, mainw );
	TQLineEdit *edit_name = new TQLineEdit( name, mainw );
	TQLineEdit *edit_enterprise = new TQLineEdit( enterprise, mainw );
	lab1->setBuddy( edit_number );
	lab2->setBuddy( edit_name );
	lab3->setBuddy( edit_enterprise );
	TQGridLayout *l0 = new TQGridLayout( mainw, 5, 2, 0, 5 );
	l0->setColStretch( 1, 1 );
	l0->addMultiCellWidget( lab0, 0, 0, 0, 1 );
	l0->setRowSpacing( 1, 10 );
	l0->addWidget( lab1, 2, 0 );
	l0->addWidget( lab2, 3, 0 );
	l0->addWidget( lab3, 4, 0 );
	l0->addWidget( edit_number, 2, 1 );
	l0->addWidget( edit_name, 3, 1 );
	l0->addWidget( edit_enterprise, 4, 1 );
	dlg.setMainWidget( mainw );
	dlg.resize( 300, 10 );

	/* dialog execution */
	while ( 1 )
		if ( dlg.exec() )
		{
			if ( edit_number->text().isEmpty() )
			{
				KMessageBox::error( this, i18n( "Invalid fax number." ) );
			}
			else
			{
				number = edit_number->text();
				name = edit_name->text();
				enterprise = edit_enterprise->text();
				return true;
			}
		}
		else
			return false;
}

#include "tdeprintfax.moc"
