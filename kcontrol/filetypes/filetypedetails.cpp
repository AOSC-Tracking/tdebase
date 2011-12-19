#include <tqcheckbox.h>
#include <tqlayout.h>
#include <tqradiobutton.h>
#include <tqvbuttongroup.h>
#include <tqwhatsthis.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kicondialog.h>
#include <klineedit.h>
#include <kinputdialog.h>
#include <klocale.h>

#include "kservicelistwidget.h"
#include "filetypedetails.h"
#include "typeslistitem.h"

FileTypeDetails::FileTypeDetails( TQWidget * parent, const char * name )
  : TQTabWidget( parent, name ), m_item( 0L )
{
  TQString wtstr;
  // First tab - General
  TQWidget * firstWidget = new TQWidget(this);
  TQVBoxLayout *firstLayout = new TQVBoxLayout(firstWidget,KDialog::marginHint(),
                                       KDialog::spacingHint());

  TQHBoxLayout *hBox = new TQHBoxLayout(0L, 0, KDialog::spacingHint());
  firstLayout->addLayout(hBox, 1);

  iconButton = new KIconButton(firstWidget);
  iconButton->setIconType(KIcon::Desktop, KIcon::MimeType);
  connect(iconButton, TQT_SIGNAL(iconChanged(TQString)), TQT_SLOT(updateIcon(TQString)));

  iconButton->setFixedSize(70, 70);
  hBox->addWidget(iconButton);

  TQWhatsThis::add( iconButton, i18n("This button displays the icon associated"
    " with the selected file type. Click on it to choose a different icon.") );

  TQGroupBox *gb = new TQGroupBox(i18n("Filename Patterns"), firstWidget);
  hBox->addWidget(gb);

  TQGridLayout *grid = new TQGridLayout(gb, 3, 2, KDialog::marginHint(),
                                      KDialog::spacingHint());
  grid->addRowSpacing(0, fontMetrics().lineSpacing());

  extensionLB = new TQListBox(gb);
  connect(extensionLB, TQT_SIGNAL(highlighted(int)), TQT_SLOT(enableExtButtons(int)));
  grid->addMultiCellWidget(extensionLB, 1, 2, 0, 0);
  grid->setRowStretch(0, 0);
  grid->setRowStretch(1, 1);
  grid->setRowStretch(2, 0);

  TQWhatsThis::add( extensionLB, i18n("This box contains a list of patterns that can be"
    " used to identify files of the selected type. For example, the pattern *.txt is"
    " associated with the file type 'text/plain'; all files ending in '.txt' are recognized"
    " as plain text files.") );

  addExtButton = new TQPushButton(i18n("Add..."), gb);
  addExtButton->setEnabled(false);
  connect(addExtButton, TQT_SIGNAL(clicked()),
          this, TQT_SLOT(addExtension()));
  grid->addWidget(addExtButton, 1, 1);

  TQWhatsThis::add( addExtButton, i18n("Add a new pattern for the selected file type.") );

  removeExtButton = new TQPushButton(i18n("Remove"), gb);
  removeExtButton->setEnabled(false);
  connect(removeExtButton, TQT_SIGNAL(clicked()),
          this, TQT_SLOT(removeExtension()));
  grid->addWidget(removeExtButton, 2, 1);

  TQWhatsThis::add( removeExtButton, i18n("Remove the selected filename pattern.") );

  gb = new TQGroupBox(i18n("Description"), firstWidget);
  firstLayout->addWidget(gb);

  gb->setColumnLayout(1, Qt::Horizontal);
  description = new KLineEdit(gb);
  connect(description, TQT_SIGNAL(textChanged(const TQString &)),
          TQT_SLOT(updateDescription(const TQString &)));

  wtstr = i18n("You can enter a short description for files of the selected"
    " file type (e.g. 'HTML Page'). This description will be used by applications"
    " like Konqueror to display directory content.");
  TQWhatsThis::add( gb, wtstr );
  TQWhatsThis::add( description, wtstr );

  serviceListWidget = new KServiceListWidget( KServiceListWidget::SERVICELIST_APPLICATIONS, firstWidget );
  connect( serviceListWidget, TQT_SIGNAL(changed(bool)), this, TQT_SIGNAL(changed(bool)));
  firstLayout->addWidget(serviceListWidget, 5);

  // Second tab - Embedding
  TQWidget * secondWidget = new TQWidget(this);
  TQVBoxLayout *secondLayout = new TQVBoxLayout(secondWidget, KDialog::marginHint(),
                                       KDialog::spacingHint());

  m_autoEmbed = new TQVButtonGroup( i18n("Left Click Action"), secondWidget );
  m_autoEmbed->layout()->setSpacing( KDialog::spacingHint() );
  secondLayout->addWidget( m_autoEmbed, 1 );

  m_autoEmbed->setSizePolicy( TQSizePolicy( (TQSizePolicy::SizeType)3, (TQSizePolicy::SizeType)0, m_autoEmbed->sizePolicy().hasHeightForWidth() ) );

  // The order of those three items is very important. If you change it, fix typeslistitem.cpp !
  new TQRadioButton( i18n("Show file in embedded viewer"), m_autoEmbed );
  new TQRadioButton( i18n("Show file in separate viewer"), m_autoEmbed );
  m_rbGroupSettings = new TQRadioButton( i18n("Use settings for '%1' group"), m_autoEmbed );
  connect(m_autoEmbed, TQT_SIGNAL( clicked( int ) ), TQT_SLOT( slotAutoEmbedClicked( int ) ));

  m_chkAskSave = new TQCheckBox( i18n("Ask whether to save to disk instead"), m_autoEmbed);
  connect(m_chkAskSave, TQT_SIGNAL( toggled(bool) ), TQT_SLOT( slotAskSaveToggled(bool) ));

  TQWhatsThis::add( m_autoEmbed, i18n("Here you can configure what the Konqueror file manager"
    " will do when you click on a file of this type. Konqueror can display the file in"
    " an embedded viewer or start up a separate application. If set to 'Use settings for G group',"
    " Konqueror will behave according to the settings of the group G this type belongs to,"
    " for instance 'image' if the current file type is image/png.") );

  secondLayout->addSpacing(10);

  embedServiceListWidget = new KServiceListWidget( KServiceListWidget::SERVICELIST_SERVICES, secondWidget );
  embedServiceListWidget->setMinimumHeight( serviceListWidget->sizeHint().height() );
  connect( embedServiceListWidget, TQT_SIGNAL(changed(bool)), this, TQT_SIGNAL(changed(bool)));
  secondLayout->addWidget(embedServiceListWidget, 3);

  addTab( firstWidget, i18n("&General") );
  addTab( secondWidget, i18n("&Embedding") );
}

void FileTypeDetails::updateRemoveButton()
{
    removeExtButton->setEnabled(extensionLB->count()>0);
}

void FileTypeDetails::updateIcon(TQString icon)
{
  if (!m_item)
    return;

  m_item->setIcon(icon);

  emit changed(true);
}

void FileTypeDetails::updateDescription(const TQString &desc)
{
  if (!m_item)
    return;

  m_item->setComment(desc);

  emit changed(true);
}

void FileTypeDetails::addExtension()
{
  if ( !m_item )
    return;

  bool ok;
  TQString ext = KInputDialog::getText( i18n( "Add New Extension" ),
    i18n( "Extension:" ), "*.", &ok, this );
  if (ok) {
    extensionLB->insertItem(ext);
    TQStringList patt = m_item->patterns();
    patt += ext;
    m_item->setPatterns(patt);
    updateRemoveButton();
    emit changed(true);
  }
}

void FileTypeDetails::removeExtension()
{
  if (extensionLB->currentItem() == -1)
    return;
  if ( !m_item )
    return;
  TQStringList patt = m_item->patterns();
  patt.remove(extensionLB->text(extensionLB->currentItem()));
  m_item->setPatterns(patt);
  extensionLB->removeItem(extensionLB->currentItem());
  updateRemoveButton();
  emit changed(true);
}

void FileTypeDetails::slotAutoEmbedClicked( int button )
{
  if ( !m_item || (button > 2))
    return;

  m_item->setAutoEmbed( button );

  updateAskSave();

  emit changed(true);
}

void FileTypeDetails::updateAskSave()
{
  if ( !m_item )
    return;

  int button = m_item->autoEmbed();
  if (button == 2)
  {
    bool embedParent = TypesListItem::defaultEmbeddingSetting(m_item->majorType());
    emit embedMajor(m_item->majorType(), embedParent);
    button = embedParent ? 0 : 1;
  }

  TQString mimeType = m_item->name();

  TQString dontAskAgainName;

  if (button == 0) // Embedded
    dontAskAgainName = "askEmbedOrSave"+mimeType;
  else
    dontAskAgainName = "askSave"+mimeType;

  KSharedConfig::Ptr config = KSharedConfig::openConfig("konquerorrc", false, false);
  config->setGroup("Notification Messages");
  bool ask = config->readEntry(dontAskAgainName).isEmpty();
  m_item->getAskSave(ask);

  bool neverAsk = false;

  if (button == 0)
  {
    KMimeType::Ptr mime = KMimeType::mimeType( mimeType );
    // Don't ask for:
    // - html (even new tabs would ask, due to about:blank!)
    // - dirs obviously (though not common over HTTP :),
    // - images (reasoning: no need to save, most of the time, because fast to see)
    // e.g. postscript is different, because takes longer to read, so
    // it's more likely that the user might want to save it.
    // - multipart/* ("server push", see kmultipart)
    // - other strange 'internal' mimetypes like print/manager...
    if ( mime->is( "text/html" ) ||
         mime->is( "text/xml" ) ||
         mime->is( "inode/directory" ) ||
         mimeType.startsWith( "image" ) ||
         mime->is( "multipart/x-mixed-replace" ) ||
         mime->is( "multipart/replace" ) ||
         mimeType.startsWith( "print" ) )
    {
        neverAsk = true;
    }
  }

  m_chkAskSave->blockSignals(true);
  m_chkAskSave->setChecked(ask && !neverAsk);
  m_chkAskSave->setEnabled(!neverAsk);
  m_chkAskSave->blockSignals(false);
}

void FileTypeDetails::slotAskSaveToggled(bool askSave)
{
  if (!m_item)
    return;

  m_item->setAskSave(askSave);
  emit changed(true);
}

void FileTypeDetails::setTypeItem( TypesListItem * tlitem )
{
  m_item = tlitem;
  if ( tlitem )
    iconButton->setIcon(tlitem->icon());
  else
    iconButton->resetIcon();
  description->setText(tlitem ? tlitem->comment() : TQString::null);
  if ( tlitem )
    m_rbGroupSettings->setText( i18n("Use settings for '%1' group").arg( tlitem->majorType() ) );
  extensionLB->clear();
  addExtButton->setEnabled(true);
  removeExtButton->setEnabled(false);

  serviceListWidget->setTypeItem( tlitem );
  embedServiceListWidget->setTypeItem( tlitem );
  m_autoEmbed->setButton( tlitem ? tlitem->autoEmbed() : -1 );
  m_rbGroupSettings->setEnabled( tlitem->canUseGroupSetting() );

  if ( tlitem )
    extensionLB->insertStringList(tlitem->patterns());
  else
    extensionLB->clear();

  updateAskSave();
}

void FileTypeDetails::enableExtButtons(int /*index*/)
{
  removeExtButton->setEnabled(true);
}

#include "filetypedetails.moc"
