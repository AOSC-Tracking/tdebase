#include <tqlayout.h>
#include <tqlabel.h>
#include <tqcombobox.h>
#include <tqtabwidget.h>
#include <tqvgroupbox.h>
#include <tqpushbutton.h>
#include <tqlistview.h>
#include <tqheader.h>
#include <tqwhatsthis.h>
#include <tqcheckbox.h>
#include <tqradiobutton.h>
#include <tqlineedit.h>
#include <tqlistview.h>
#include <tqbuttongroup.h>
#include <tqspinbox.h>

#include <kkeydialog.h>
#include <tdeglobal.h>
#include <tdeconfig.h>
#include <tdelocale.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <tdeapplication.h>
#include <kiconloader.h>

#include "extension.h"
#include "kxkbconfig.h"
#include "rules.h"
#include "pixmap.h"
#include "kcmmisc.h"
#include "kcmlayoutwidget.h"

#include "kcmlayout.h"
#include "kcmlayout.moc"


enum {
 LAYOUT_COLUMN_FLAG = 0,
 LAYOUT_COLUMN_NAME = 1,
 LAYOUT_COLUMN_MAP = 2,
 LAYOUT_COLUMN_VARIANT = 3,
 LAYOUT_COLUMN_INCLUDE = 4,
 LAYOUT_COLUMN_DISPLAY_NAME = 5,
 SRC_LAYOUT_COLUMN_COUNT = 3,
 DST_LAYOUT_COLUMN_COUNT = 6
};

static const TQString DEFAULT_VARIANT_NAME("<default>");


class OptionListItem : public TQCheckListItem
{
	public:

		OptionListItem(  OptionListItem *parent, const TQString &text, Type tt,
						 const TQString &optionName );
		OptionListItem(  TQListView *parent, const TQString &text, Type tt,
						 const TQString &optionName );
		~OptionListItem() {}

		TQString optionName() const { return m_OptionName; }

		OptionListItem *findChildItem(  const TQString& text );

	protected:
		TQString m_OptionName;
};


static TQString lookupLocalized(const TQDict<char> &dict, const TQString& text)
{
  TQDictIterator<char> it(dict);
  while (it.current())
    {
      if ( i18n(it.current()) == text )
        return it.currentKey();
      ++it;
    }

  return TQString::null;
}

static TQListViewItem* copyLVI(const TQListViewItem* src, TQListView* parent)
{
    TQListViewItem* ret = new TQListViewItem(parent);
	for(int i = 0; i < SRC_LAYOUT_COLUMN_COUNT; i++)
    {
        ret->setText(i, src->text(i));
        if ( src->pixmap(i) )
            ret->setPixmap(i, *src->pixmap(i));
    }

    return ret;
}


LayoutConfig::LayoutConfig(TQWidget *parent, const char *name)
  : TDECModule(parent, name), 
    m_rules(NULL)
{
  TQVBoxLayout *main = new TQVBoxLayout(this, 0, KDialog::spacingHint());

  widget = new LayoutConfigWidget(this, "widget");
  main->addWidget(TQT_TQWIDGET(widget));

  connect( TQT_TQOBJECT(widget->chkEnable), TQT_SIGNAL( toggled( bool )), TQT_TQOBJECT(this), TQT_SLOT(changed()));
  connect( TQT_TQOBJECT(widget->chkShowSingle), TQT_SIGNAL( toggled( bool )), TQT_TQOBJECT(this), TQT_SLOT(changed()));
  connect( TQT_TQOBJECT(widget->chkShowFlag), TQT_SIGNAL( toggled( bool )), TQT_TQOBJECT(this), TQT_SLOT(changed()));
  connect( TQT_TQOBJECT(widget->comboModel), TQT_SIGNAL(activated(int)), TQT_TQOBJECT(this), TQT_SLOT(changed()));

  connect( TQT_TQOBJECT(widget->listLayoutsSrc), TQT_SIGNAL(doubleClicked(TQListViewItem*,const TQPoint&, int)),
									TQT_TQOBJECT(this), TQT_SLOT(add()));
  connect( TQT_TQOBJECT(widget->btnAdd), TQT_SIGNAL(clicked()), TQT_TQOBJECT(this), TQT_SLOT(add()));
  connect( TQT_TQOBJECT(widget->btnRemove), TQT_SIGNAL(clicked()), TQT_TQOBJECT(this), TQT_SLOT(remove()));

  connect( TQT_TQOBJECT(widget->comboVariant), TQT_SIGNAL(activated(int)), TQT_TQOBJECT(this), TQT_SLOT(changed()));
  connect( TQT_TQOBJECT(widget->comboVariant), TQT_SIGNAL(activated(int)), TQT_TQOBJECT(this), TQT_SLOT(variantChanged()));
  connect( TQT_TQOBJECT(widget->listLayoutsDst), TQT_SIGNAL(selectionChanged(TQListViewItem *)),
		TQT_TQOBJECT(this), TQT_SLOT(layoutSelChanged(TQListViewItem *)));

  connect( widget->editDisplayName, TQT_SIGNAL(textChanged(const TQString&)), TQT_TQOBJECT(this), TQT_SLOT(displayNameChanged(const TQString&)));

  connect( widget->chkLatin, TQT_SIGNAL(clicked()), TQT_TQOBJECT(this), TQT_SLOT(changed()));
  connect( widget->chkLatin, TQT_SIGNAL(clicked()), TQT_TQOBJECT(this), TQT_SLOT(latinChanged()));

  widget->btnUp->setIconSet(SmallIconSet("1uparrow"));
  connect( widget->btnUp, TQT_SIGNAL(clicked()), TQT_TQOBJECT(this), TQT_SLOT(changed()));
  connect( widget->btnUp, TQT_SIGNAL(clicked()), TQT_TQOBJECT(this), TQT_SLOT(moveUp()));
  widget->btnDown->setIconSet(SmallIconSet("1downarrow"));
  connect( widget->btnDown, TQT_SIGNAL(clicked()), TQT_TQOBJECT(this), TQT_SLOT(changed()));
  connect( widget->btnDown, TQT_SIGNAL(clicked()), TQT_TQOBJECT(this), TQT_SLOT(moveDown()));

  connect( widget->grpSwitching, TQT_SIGNAL( clicked( int ) ), TQT_SLOT(changed()));

  connect( widget->chkEnableSticky, TQT_SIGNAL(toggled(bool)), TQT_TQOBJECT(this), TQT_SLOT(changed()));
  connect( widget->spinStickyDepth, TQT_SIGNAL(valueChanged(int)), TQT_TQOBJECT(this), TQT_SLOT(changed()));

  widget->listLayoutsSrc->setColumnText(LAYOUT_COLUMN_FLAG, "");
  widget->listLayoutsDst->setColumnText(LAYOUT_COLUMN_FLAG, "");
  widget->listLayoutsDst->setColumnText(LAYOUT_COLUMN_INCLUDE, "");
//  widget->listLayoutsDst->setColumnText(LAYOUT_COLUMN_DISPLAY_NAME, "");

  widget->listLayoutsSrc->setColumnWidth(LAYOUT_COLUMN_FLAG, 28);
  widget->listLayoutsDst->setColumnWidth(LAYOUT_COLUMN_FLAG, 28);

  widget->listLayoutsDst->header()->setResizeEnabled(FALSE, LAYOUT_COLUMN_INCLUDE);
  widget->listLayoutsDst->header()->setResizeEnabled(FALSE, LAYOUT_COLUMN_DISPLAY_NAME);
  widget->listLayoutsDst->setColumnWidthMode(LAYOUT_COLUMN_INCLUDE, TQListView::Manual);
  widget->listLayoutsDst->setColumnWidth(LAYOUT_COLUMN_INCLUDE, 0);
//  widget->listLayoutsDst->setColumnWidth(LAYOUT_COLUMN_DISPLAY_NAME, 0);
  
  widget->listLayoutsDst->setSorting(-1);
#if 0
  widget->listLayoutsDst->setResizeMode(TQListView::LastColumn);
  widget->listLayoutsSrc->setResizeMode(TQListView::LastColumn);
#endif
  widget->listLayoutsDst->setResizeMode(TQListView::LastColumn);

  //Read rules - we _must_ read _before_ creating xkb-options comboboxes
  loadRules();

  makeOptionsTab();

  load();
}


LayoutConfig::~LayoutConfig()
{
	delete m_rules;
}


void LayoutConfig::load()
{
	m_kxkbConfig.load(KxkbConfig::LOAD_ALL);

	initUI();
}
	
void LayoutConfig::initUI() {
	const char* modelName = m_rules->models()[m_kxkbConfig.m_model];
	if( modelName == NULL )
		modelName = DEFAULT_MODEL;
	
	widget->comboModel->setCurrentText(i18n(modelName));

	TQValueList<LayoutUnit> otherLayouts = m_kxkbConfig.m_layouts;
	widget->listLayoutsDst->clear();
// to optimize we should have gone from it.end to it.begin
	TQValueList<LayoutUnit>::ConstIterator it;
	for (it = otherLayouts.begin(); it != otherLayouts.end(); ++it ) {
		TQListViewItemIterator src_it( widget->listLayoutsSrc );
		LayoutUnit layoutUnit = *it;
		
		for ( ; src_it.current(); ++src_it ) {
			TQListViewItem* srcItem = src_it.current();
			
			if ( layoutUnit.layout == src_it.current()->text(LAYOUT_COLUMN_MAP) ) {	// check if current config knows about this layout
				TQListViewItem* newItem = copyLVI(srcItem, widget->listLayoutsDst);
				
				newItem->setText(LAYOUT_COLUMN_VARIANT, layoutUnit.variant);
				newItem->setText(LAYOUT_COLUMN_INCLUDE, layoutUnit.includeGroup);
				newItem->setText(LAYOUT_COLUMN_DISPLAY_NAME, layoutUnit.displayName);
				widget->listLayoutsDst->insertItem(newItem);
				newItem->moveItem(widget->listLayoutsDst->lastItem());

				break;
			}
		}
	}

	// display KXKB switching options
	widget->chkShowSingle->setChecked(m_kxkbConfig.m_showSingle);
	widget->chkShowFlag->setChecked(m_kxkbConfig.m_showFlag);

	widget->chkEnableOptions->setChecked( m_kxkbConfig.m_enableXkbOptions );
	widget->checkResetOld->setChecked(m_kxkbConfig.m_resetOldOptions);

	switch( m_kxkbConfig.m_switchingPolicy ) {
		default:
		case SWITCH_POLICY_GLOBAL:
			widget->grpSwitching->setButton(0);
			break;
		case SWITCH_POLICY_WIN_CLASS:
			widget->grpSwitching->setButton(1);
			break;
		case SWITCH_POLICY_WINDOW:
			widget->grpSwitching->setButton(2);
			break;
	}

	widget->chkEnableSticky->setChecked(m_kxkbConfig.m_stickySwitching);
	widget->spinStickyDepth->setEnabled(m_kxkbConfig.m_stickySwitching);
	widget->spinStickyDepth->setValue( m_kxkbConfig.m_stickySwitchingDepth);

	updateStickyLimit();

	widget->chkEnable->setChecked( m_kxkbConfig.m_useKxkb );
	widget->grpLayouts->setEnabled( m_kxkbConfig.m_useKxkb );
	widget->optionsFrame->setEnabled( m_kxkbConfig.m_useKxkb );

	// display xkb options
	TQStringList options = TQStringList::split(',', m_kxkbConfig.m_options);
	for (TQStringList::ConstIterator it = options.begin(); it != options.end(); ++it)
	{
		TQString option = *it;
		TQString optionKey = option.mid(0, option.find(':'));
		TQString optionName = m_rules->options()[option];
		OptionListItem *item = m_optionGroups[i18n(optionKey.latin1())];
		
		if (item != NULL) {
			OptionListItem *child = item->findChildItem( option );

			if ( child )
				child->setState( TQCheckListItem::On );
			else
				kdDebug() << "load: Unknown option: " << option << endl;
		}
		else {
			kdDebug() << "load: Unknown option group: " << optionKey << " of " << option << endl;
		}
	}

	updateOptionsCommand();
	emit TDECModule::changed( false );
}


void LayoutConfig::save()
{
	TQString model = lookupLocalized(m_rules->models(), widget->comboModel->currentText());
	m_kxkbConfig.m_model = model;

	m_kxkbConfig.m_enableXkbOptions = widget->chkEnableOptions->isChecked();
	m_kxkbConfig.m_resetOldOptions = widget->checkResetOld->isChecked();
	m_kxkbConfig.m_options = createOptionString();

	TQListViewItem *item = widget->listLayoutsDst->firstChild();
	TQValueList<LayoutUnit> layouts;
	while (item) {
		TQString layout = item->text(LAYOUT_COLUMN_MAP);
		TQString variant = item->text(LAYOUT_COLUMN_VARIANT);
		TQString includes = item->text(LAYOUT_COLUMN_INCLUDE);
		TQString displayName = item->text(LAYOUT_COLUMN_DISPLAY_NAME);
		
		LayoutUnit layoutUnit(layout, variant);
		layoutUnit.includeGroup = includes;
		layoutUnit.displayName = displayName;
		layouts.append( layoutUnit );
		
		item = item->nextSibling();
		kdDebug() << "To save: layout " << layoutUnit.toPair() 
				<< ", inc: " << layoutUnit.includeGroup 
				<< ", disp: " << layoutUnit.displayName << endl;
	}
	m_kxkbConfig.m_layouts = layouts;

	if( m_kxkbConfig.m_layouts.count() == 0 ) {
		m_kxkbConfig.m_layouts.append(LayoutUnit(DEFAULT_LAYOUT_UNIT));
 		widget->chkEnable->setChecked(false);
 	}

	m_kxkbConfig.m_useKxkb = widget->chkEnable->isChecked();
	m_kxkbConfig.m_showSingle = widget->chkShowSingle->isChecked();
	m_kxkbConfig.m_showFlag = widget->chkShowFlag->isChecked();

	int modeId = widget->grpSwitching->id(widget->grpSwitching->selected());
	switch( modeId ) {
		default:
		case 0:
			m_kxkbConfig.m_switchingPolicy = SWITCH_POLICY_GLOBAL;
			break;
		case 1:
			m_kxkbConfig.m_switchingPolicy = SWITCH_POLICY_WIN_CLASS;
			break;
		case 2:
			m_kxkbConfig.m_switchingPolicy = SWITCH_POLICY_WINDOW;
			break;
	}

	m_kxkbConfig.m_stickySwitching = widget->chkEnableSticky->isChecked();
	m_kxkbConfig.m_stickySwitchingDepth = widget->spinStickyDepth->value();

	m_kxkbConfig.save();
	
	kapp->tdeinitExec("kxkb");
	emit TDECModule::changed( false );
}


TQString LayoutConfig::handbookDocPath() const
{
 	int index = widget->tabWidget->currentPageIndex();
 	if (index == 0)
		return "kxkb/layout-config.html";
	else if (index == 1)
		return "kxkb/switching-config.html";
	else if (index == 2)
		return "kxkb/xkboptions-config.html";
	else
 		return TQString::null;
}


void LayoutConfig::updateStickyLimit()
{
    int layoutsCnt = widget->listLayoutsDst->childCount();
	int maxDepth = layoutsCnt - 1;
	
	if( maxDepth < 2 ) {
		maxDepth = 2;
	}
	
	widget->spinStickyDepth->setMaxValue(maxDepth);
/*	if( value > maxDepth )
		setValue(maxDepth);*/
}

void LayoutConfig::add()
{
    TQListViewItem* sel = widget->listLayoutsSrc->selectedItem();
    if( sel == 0 )
		return;

    // Create a copy of the sel widget, as one might add the same layout more
    // than one time, with different variants.
    TQListViewItem* toadd = copyLVI(sel, widget->listLayoutsDst);
    
    // Turn on "Include Latin layout" for new language by default (bnc:204402)
    toadd->setText(LAYOUT_COLUMN_INCLUDE, "us");

    widget->listLayoutsDst->insertItem(toadd);
    if( widget->listLayoutsDst->childCount() > 1 )
		toadd->moveItem(widget->listLayoutsDst->lastItem());
// disabling temporary: does not work reliable in Qt :(
//    widget->listLayoutsDst->setSelected(sel, true);
//    layoutSelChanged(sel);
	
    updateStickyLimit();
    changed();
}

void LayoutConfig::remove() 
{
    TQListViewItem* sel = widget->listLayoutsDst->selectedItem();
    TQListViewItem* newSel = 0;

    if( sel == 0 )
        return;

    if( sel->itemBelow() )
        newSel = sel->itemBelow();
    else
        if( sel->itemAbove() )
            newSel = sel->itemAbove();

    delete sel;
    if( newSel )
        widget->listLayoutsSrc->setSelected(newSel, true);
    layoutSelChanged(newSel);

    updateStickyLimit();
    changed();
}

void LayoutConfig::moveUp()
{
    TQListViewItem* sel = widget->listLayoutsDst->selectedItem();
    if( sel == 0 || sel->itemAbove() == 0 )
		return;

    if( sel->itemAbove()->itemAbove() == 0 ) {
		widget->listLayoutsDst->takeItem(sel);
		widget->listLayoutsDst->insertItem(sel);
		widget->listLayoutsDst->setSelected(sel, true);
    }
    else
		sel->moveItem(sel->itemAbove()->itemAbove());
}

void LayoutConfig::moveDown()
{
    TQListViewItem* sel = widget->listLayoutsDst->selectedItem();
    if( sel == 0 || sel->itemBelow() == 0 )
	return;

    sel->moveItem(sel->itemBelow());
}

void LayoutConfig::variantChanged()
{
    TQListViewItem* selLayout = widget->listLayoutsDst->selectedItem();
    if( selLayout == NULL ) {
      widget->comboVariant->clear();
      widget->comboVariant->setEnabled(false);
      return;
    }

	TQString selectedVariant = widget->comboVariant->currentText();
	if( selectedVariant == DEFAULT_VARIANT_NAME )
		selectedVariant = "";
	selLayout->setText(LAYOUT_COLUMN_VARIANT, selectedVariant);
}

// helper
LayoutUnit LayoutConfig::getLayoutUnitKey(TQListViewItem *sel)
{
	TQString kbdLayout = sel->text(LAYOUT_COLUMN_MAP);
	TQString kbdVariant = sel->text(LAYOUT_COLUMN_VARIANT);
	return LayoutUnit(kbdLayout, kbdVariant);
}

void LayoutConfig::displayNameChanged(const TQString& newDisplayName)
{
	TQListViewItem* selLayout = widget->listLayoutsDst->selectedItem();
	if( selLayout == NULL )
		return;
	
	const LayoutUnit layoutUnitKey = getLayoutUnitKey( selLayout );
	LayoutUnit& layoutUnit = *m_kxkbConfig.m_layouts.find(layoutUnitKey);
	
	TQString oldName = selLayout->text(LAYOUT_COLUMN_DISPLAY_NAME);
	 
	if( oldName.isEmpty() )
		oldName = KxkbConfig::getDefaultDisplayName( layoutUnit );
	
	if( oldName != newDisplayName ) {
		kdDebug() << "setting label for " << layoutUnit.toPair() << " : " << newDisplayName << endl;
		selLayout->setText(LAYOUT_COLUMN_DISPLAY_NAME, newDisplayName);
		updateIndicator(selLayout);
		emit changed();
	}
}

/** will update flag with label if layout label has been edited
*/
void LayoutConfig::updateIndicator(TQListViewItem* selLayout)
{
}


void LayoutConfig::latinChanged()
{
    TQListViewItem* selLayout = widget->listLayoutsDst->selectedItem();
    if (  !selLayout ) {
      widget->chkLatin->setChecked( false );
      widget->chkLatin->setEnabled( false );
      return;
    }

	TQString include;
	if( widget->chkLatin->isChecked() )
		include = "us";
    else
		include = "";
	selLayout->setText(LAYOUT_COLUMN_INCLUDE, include);

 	LayoutUnit layoutUnitKey = getLayoutUnitKey(selLayout);
	kdDebug() << "layout " << layoutUnitKey.toPair() << ", inc: " << include << endl;
}

void LayoutConfig::layoutSelChanged(TQListViewItem *sel)
{
    widget->comboVariant->clear();
    widget->comboVariant->setEnabled( sel != NULL );
    widget->chkLatin->setChecked( false );
    widget->chkLatin->setEnabled( sel != NULL );

    if( sel == NULL ) {
        updateLayoutCommand();
        return;
    }


	LayoutUnit layoutUnitKey = getLayoutUnitKey(sel);
	TQString kbdLayout = layoutUnitKey.layout;

	// TODO: need better algorithm here for determining if needs us group
    if (  ! m_rules->isSingleGroup(kbdLayout) 
	    		|| kbdLayout.startsWith("us") || kbdLayout.startsWith("en") ) {
        widget->chkLatin->setEnabled( false );
    }
    else {
		TQString inc = sel->text(LAYOUT_COLUMN_INCLUDE);
		if ( inc.startsWith("us") || inc.startsWith("en") ) {
            widget->chkLatin->setChecked(true);
        }
        else {
            widget->chkLatin->setChecked(false);
        }
    }

	TQStringList vars = m_rules->getAvailableVariants(kbdLayout);
	kdDebug() << "layout " << kbdLayout << " has " << vars.count() << " variants" << endl;
    
	if( vars.count() > 0 ) {
		vars.prepend(DEFAULT_VARIANT_NAME);
		widget->comboVariant->insertStringList(vars);
	
		TQString variant = sel->text(LAYOUT_COLUMN_VARIANT);
		if( variant != NULL && variant.isEmpty() == false ) {
			widget->comboVariant->setCurrentText(variant);
		}
		else {
			widget->comboVariant->setCurrentItem(0);
		}
	}
    updateLayoutCommand();
}

TQWidget* LayoutConfig::makeOptionsTab()
{
  TQListView *listView = widget->listOptions;

  listView->setMinimumHeight(150);
  listView->setSortColumn( -1 );
  listView->setColumnText( 0, i18n( "Options" ) );
  listView->clear();

  connect(listView, TQT_SIGNAL(clicked(TQListViewItem *)), TQT_SLOT(changed()));
  connect(listView, TQT_SIGNAL(clicked(TQListViewItem *)), TQT_SLOT(updateOptionsCommand()));

  connect(widget->chkEnableOptions, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));

  connect(widget->checkResetOld, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(widget->checkResetOld, TQT_SIGNAL(toggled(bool)), TQT_SLOT(updateOptionsCommand()));

  //Create controllers for all options
  TQDictIterator<char> it(m_rules->options());
  OptionListItem *parent;
  for (; it.current(); ++it)
  {
    if (!it.currentKey().contains(':'))
    {
      if( it.currentKey() == "ctrl" || it.currentKey() == "caps"
          || it.currentKey() == "altwin" ) {
        parent = new OptionListItem(listView, i18n( it.current() ),
            TQCheckListItem::RadioButtonController, it.currentKey());
        OptionListItem *item = new OptionListItem(parent, i18n( "None" ),
            TQCheckListItem::RadioButton, "none");
        item->setState(TQCheckListItem::On);
      }
      else {
        parent = new OptionListItem(listView, i18n( it.current() ),
            TQCheckListItem::CheckBoxController, it.currentKey());
      }
      parent->setOpen(true);
      m_optionGroups.insert(i18n(it.currentKey().local8Bit()), parent);
    }
  }

  it.toFirst();
  for( ; it.current(); ++it)
  {
    TQString key = it.currentKey();
    int pos = key.find(':');
    if (pos >= 0)
    {
      OptionListItem *parent = m_optionGroups[key.left(pos)];
      if (parent == NULL )
        parent = m_optionGroups["misc"];
      if (parent != NULL) {
      // workaroung for mistake in rules file for xkb options in XFree 4.2.0
        TQString text(it.current());
        text = text.replace( "Cap$", "Caps." );
        if( parent->type() == TQCheckListItem::RadioButtonController )
            new OptionListItem(parent, i18n(text.utf8()),
                TQCheckListItem::RadioButton, key);
        else
            new OptionListItem(parent, i18n(text.utf8()),
                TQCheckListItem::CheckBox, key);
      }
    }
  }

  //scroll->setMinimumSize(450, 330);

  return listView;
}

void LayoutConfig::updateOptionsCommand()
{
  TQString setxkbmap;
  TQString options = createOptionString();

  if( !options.isEmpty() ) {
    setxkbmap = "setxkbmap -option "; //-rules " + m_rule
    if( widget->checkResetOld->isChecked() )
      setxkbmap += "-option ";
    setxkbmap += options;
  }
  widget->editCmdLineOpt->setText(setxkbmap);
}

void LayoutConfig::updateLayoutCommand()
{
  TQString setxkbmap;
  TQString layoutDisplayName;
  TQListViewItem* sel = widget->listLayoutsDst->selectedItem();

  if( sel != NULL ) {
    TQString kbdLayout = sel->text(LAYOUT_COLUMN_MAP);
    TQString variant = widget->comboVariant->currentText();
	if( variant == DEFAULT_VARIANT_NAME )
		variant = "";

    setxkbmap = "setxkbmap"; //-rules " + m_rule
    setxkbmap += " -model " + lookupLocalized(m_rules->models(), widget->comboModel->currentText())
      + " -layout ";
    setxkbmap += kbdLayout;
    if( widget->chkLatin->isChecked() )
      setxkbmap += ",us";

/*	LayoutUnit layoutUnitKey = getLayoutUnitKey(sel);
	layoutDisplayName = m_kxkbConfig.getLayoutDisplayName( *m_kxkbConfig.m_layouts.find(layoutUnitKey) );*/
	layoutDisplayName = sel->text(LAYOUT_COLUMN_DISPLAY_NAME);
	if( layoutDisplayName.isEmpty() ) {
		int count = 0;
		TQListViewItem *item = widget->listLayoutsDst->firstChild();
		while (item) {
			TQString layout_ = item->text(LAYOUT_COLUMN_MAP);
			if( layout_ == kbdLayout )
				++count;
			item = item->nextSibling();
		}
		bool single = count < 2;
		layoutDisplayName = m_kxkbConfig.getDefaultDisplayName(LayoutUnit(kbdLayout, variant), single);
	}
	kdDebug() << "disp: '" << layoutDisplayName << "'" << endl;
	
    if( !variant.isEmpty() ) {
      setxkbmap += " -variant ";
      if( widget->chkLatin->isChecked() )
        setxkbmap += ",";
      setxkbmap += variant;
    }
  }
  
  widget->editCmdLine->setText(setxkbmap);
  
  widget->editDisplayName->setEnabled( sel != NULL );
  widget->editDisplayName->setText(layoutDisplayName);
}

void LayoutConfig::changed()
{
  updateLayoutCommand();
  emit TDECModule::changed( true );
}


void LayoutConfig::loadRules()
{
    // do we need this ?
    // this could obly be used if rules are changed and 'Defaults' is pressed
    delete m_rules;
    m_rules = new XkbRules();

    TQStringList modelsList;
    TQDictIterator<char> it(m_rules->models());
    while (it.current()) {
        modelsList.append(i18n(it.current()));
        ++it;
    }
    modelsList.sort();
	
	widget->comboModel->clear();
	widget->comboModel->insertStringList(modelsList);
	widget->comboModel->setCurrentItem(0);

	// fill in the additional layouts
	widget->listLayoutsSrc->clear();
	widget->listLayoutsDst->clear();
	TQDictIterator<char> it2(m_rules->layouts());
	
	while (it2.current())
	{
		TQString layout = it2.currentKey();
		TQString layoutName = it2.current();
		TQListViewItem *item = new TQListViewItem(widget->listLayoutsSrc);
		
		item->setPixmap(LAYOUT_COLUMN_FLAG, LayoutIcon::getInstance().findPixmap(layout, true));
		item->setText(LAYOUT_COLUMN_NAME, i18n(layoutName.latin1()));
		item->setText(LAYOUT_COLUMN_MAP, layout);
		++it2;
	}
	widget->listLayoutsSrc->setSorting(LAYOUT_COLUMN_NAME);	// from Qt3 TQListView sorts by language
	
	//TODO: reset options and xkb options
}


TQString LayoutConfig::createOptionString()
{
  TQString options;
  for (TQDictIterator<char> it(m_rules->options()); it.current(); ++it)
  {
    TQString option(it.currentKey());

    if (option.contains(':')) {

      TQString optionKey = option.mid(0, option.find(':'));
      OptionListItem *item = m_optionGroups[optionKey];

      if( !item ) {
        kdDebug() << "WARNING: skipping empty group for " << it.currentKey()
          << endl;
        continue;
      }

      OptionListItem *child = item->findChildItem( option );

      if ( child ) {
        if ( child->state() == TQCheckListItem::On ) {
          TQString selectedName = child->optionName();
          if ( !selectedName.isEmpty() && selectedName != "none" ) {
            if (!options.isEmpty())
              options.append(',');
            options.append(selectedName);
          }
        }
      }
      else
        kdDebug() << "Empty option button for group " << it.currentKey() << endl;
    }
  }
  return options;
}


void LayoutConfig::defaults()
{
	loadRules();
	m_kxkbConfig.setDefaults();

	initUI();

	emit TDECModule::changed( true );
}


OptionListItem::OptionListItem( OptionListItem *parent, const TQString &text,
								Type tt, const TQString &optionName )
	: TQCheckListItem( parent, text, tt ), m_OptionName( optionName )
{
}

OptionListItem::OptionListItem( TQListView *parent, const TQString &text,
								Type tt, const TQString &optionName )
	: TQCheckListItem( parent, text, tt ), m_OptionName( optionName )
{
}

OptionListItem * OptionListItem::findChildItem( const TQString& optionName )
{
	OptionListItem *child = static_cast<OptionListItem *>( firstChild() );

	while ( child )
	{
		if ( child->optionName() == optionName )
			break;
		child = static_cast<OptionListItem *>( child->nextSibling() );
	}

	return child;
}


extern "C"
{
	KDE_EXPORT TDECModule *create_keyboard_layout(TQWidget *parent, const char *)
	{
		return new LayoutConfig(parent, "kcmlayout");
	}
	
	KDE_EXPORT TDECModule *create_keyboard(TQWidget *parent, const char *)
	{
		return new KeyboardConfig(parent, "kcmlayout");
	}
	
	KDE_EXPORT void init_keyboard()
	{
		KeyboardConfig::init_keyboard();
		
		KxkbConfig m_kxkbConfig;
		m_kxkbConfig.load(KxkbConfig::LOAD_INIT_OPTIONS);
	
		if( m_kxkbConfig.m_useKxkb == true ) {
			kapp->startServiceByDesktopName("kxkb");
		}
		else {
		// Even if the layouts have been disabled we still want to set Xkb options
		// user can always switch them off now in the "Options" tab
			if( m_kxkbConfig.m_enableXkbOptions ) {
				if( !XKBExtension::setXkbOptions(m_kxkbConfig.m_options, m_kxkbConfig.m_resetOldOptions) ) {
					kdDebug() << "Setting XKB options failed!" << endl;
				}
			}
		}
	}
}



#if 0// do not remove!
// please don't change/fix messages below
// they're taken from XFree86 as is and should stay the same
   I18N_NOOP("Brazilian ABNT2");
   I18N_NOOP("Dell 101-key PC");
   I18N_NOOP("Everex STEPnote");
   I18N_NOOP("Generic 101-key PC");
   I18N_NOOP("Generic 102-key (Intl) PC");
   I18N_NOOP("Generic 104-key PC");
   I18N_NOOP("Generic 105-key (Intl) PC");
   I18N_NOOP("Japanese 106-key");
   I18N_NOOP("Microsoft Natural");
   I18N_NOOP("Northgate OmniKey 101");
   I18N_NOOP("Keytronic FlexPro");
   I18N_NOOP("Winbook Model XP5");

// These options are from XFree 4.1.0
 I18N_NOOP("Group Shift/Lock behavior");
 I18N_NOOP("R-Alt switches group while pressed");
 I18N_NOOP("Right Alt key changes group");
 I18N_NOOP("Caps Lock key changes group");
 I18N_NOOP("Menu key changes group");
 I18N_NOOP("Both Shift keys together change group");
 I18N_NOOP("Control+Shift changes group");
 I18N_NOOP("Alt+Control changes group");
 I18N_NOOP("Alt+Shift changes group");
 I18N_NOOP("Control Key Position");
 I18N_NOOP("Make CapsLock an additional Control");
 I18N_NOOP("Swap Control and Caps Lock");
 I18N_NOOP("Control key at left of 'A'");
 I18N_NOOP("Control key at bottom left");
 I18N_NOOP("Use keyboard LED to show alternative group");
 I18N_NOOP("Num_Lock LED shows alternative group");
 I18N_NOOP("Caps_Lock LED shows alternative group");
 I18N_NOOP("Scroll_Lock LED shows alternative group");

//these seem to be new in XFree86 4.2.0
 I18N_NOOP("Left Win-key switches group while pressed");
 I18N_NOOP("Right Win-key switches group while pressed");
 I18N_NOOP("Both Win-keys switch group while pressed");
 I18N_NOOP("Left Win-key changes group");
 I18N_NOOP("Right Win-key changes group");
 I18N_NOOP("Third level choosers");
 I18N_NOOP("Press Right Control to choose 3rd level");
 I18N_NOOP("Press Menu key to choose 3rd level");
 I18N_NOOP("Press any of Win-keys to choose 3rd level");
 I18N_NOOP("Press Left Win-key to choose 3rd level");
 I18N_NOOP("Press Right Win-key to choose 3rd level");
 I18N_NOOP("CapsLock key behavior");
 I18N_NOOP("uses internal capitalization. Shift cancels Caps.");
 I18N_NOOP("uses internal capitalization. Shift doesn't cancel Caps.");
 I18N_NOOP("acts as Shift with locking. Shift cancels Caps.");
 I18N_NOOP("acts as Shift with locking. Shift doesn't cancel Caps.");
 I18N_NOOP("Alt/Win key behavior");
 I18N_NOOP("Add the standard behavior to Menu key.");
 I18N_NOOP("Alt and Meta on the Alt keys (default).");
 I18N_NOOP("Meta is mapped to the Win-keys.");
 I18N_NOOP("Meta is mapped to the left Win-key.");
 I18N_NOOP("Super is mapped to the Win-keys (default).");
 I18N_NOOP("Hyper is mapped to the Win-keys.");
 I18N_NOOP("Right Alt is Compose");
 I18N_NOOP("Right Win-key is Compose");
 I18N_NOOP("Menu is Compose");

//these seem to be new in XFree86 4.3.0
 I18N_NOOP( "Both Ctrl keys together change group" );
 I18N_NOOP( "Both Alt keys together change group" );
 I18N_NOOP( "Left Shift key changes group" );
 I18N_NOOP( "Right Shift key changes group" );
 I18N_NOOP( "Right Ctrl key changes group" );
 I18N_NOOP( "Left Alt key changes group" );
 I18N_NOOP( "Left Ctrl key changes group" );
 I18N_NOOP( "Compose Key" );
 
//these seem to be new in XFree86 4.4.0
 I18N_NOOP("Shift with numpad keys works as in MS Windows.");
 I18N_NOOP("Special keys (Ctrl+Alt+<key>) handled in a server.");
 I18N_NOOP("Miscellaneous compatibility options");
 I18N_NOOP("Right Control key works as Right Alt");

//these seem to be in x.org and Debian XFree86 4.3
 I18N_NOOP("Right Alt key switches group while pressed");
 I18N_NOOP("Left Alt key switches group while pressed");
 I18N_NOOP("Press Right Alt-key to choose 3rd level");

//new in Xorg 6.9
 I18N_NOOP("R-Alt switches group while pressed.");
 I18N_NOOP("Left Alt key switches group while pressed.");
 I18N_NOOP("Left Win-key switches group while pressed.");
 I18N_NOOP("Right Win-key switches group while pressed.");
 I18N_NOOP("Both Win-keys switch group while pressed.");
 I18N_NOOP("Right Ctrl key switches group while pressed.");
 I18N_NOOP("Right Alt key changes group.");
 I18N_NOOP("Left Alt key changes group.");
 I18N_NOOP("CapsLock key changes group.");
 I18N_NOOP("Shift+CapsLock changes group.");
 I18N_NOOP("Both Shift keys together change group.");
 I18N_NOOP("Both Alt keys together change group.");
 I18N_NOOP("Both Ctrl keys together change group.");
 I18N_NOOP("Ctrl+Shift changes group.");
 I18N_NOOP("Alt+Ctrl changes group.");
 I18N_NOOP("Alt+Shift changes group.");
 I18N_NOOP("Menu key changes group.");
 I18N_NOOP("Left Win-key changes group.");
 I18N_NOOP("Right Win-key changes group.");
 I18N_NOOP("Left Shift key changes group.");
 I18N_NOOP("Right Shift key changes group.");
 I18N_NOOP("Left Ctrl key changes group.");
 I18N_NOOP("Right Ctrl key changes group.");
 I18N_NOOP("Press Right Ctrl to choose 3rd level.");
 I18N_NOOP("Press Menu key to choose 3rd level.");
 I18N_NOOP("Press any of Win-keys to choose 3rd level.");
 I18N_NOOP("Press Left Win-key to choose 3rd level.");
 I18N_NOOP("Press Right Win-key to choose 3rd level.");
 I18N_NOOP("Press any of Alt keys to choose 3rd level.");
 I18N_NOOP("Press Left Alt key to choose 3rd level.");
 I18N_NOOP("Press Right Alt key to choose 3rd level.");
 I18N_NOOP("Ctrl key position");
 I18N_NOOP("Make CapsLock an additional Ctrl.");
 I18N_NOOP("Swap Ctrl and CapsLock.");
 I18N_NOOP("Ctrl key at left of 'A'");
 I18N_NOOP("Ctrl key at bottom left");
 I18N_NOOP("Right Ctrl key works as Right Alt.");
 I18N_NOOP("Use keyboard LED to show alternative group.");
 I18N_NOOP("NumLock LED shows alternative group.");
 I18N_NOOP("CapsLock LED shows alternative group.");
 I18N_NOOP("ScrollLock LED shows alternative group.");
 I18N_NOOP("CapsLock uses internal capitalization. Shift cancels CapsLock.");
 I18N_NOOP("CapsLock uses internal capitalization. Shift doesn't cancel CapsLock.");
 I18N_NOOP("CapsLock acts as Shift with locking. Shift cancels CapsLock.");
 I18N_NOOP("CapsLock acts as Shift with locking. Shift doesn't cancel CapsLock.");
 I18N_NOOP("CapsLock just locks the Shift modifier.");
 I18N_NOOP("CapsLock toggles normal capitalization of alphabetic characters.");
 I18N_NOOP("CapsLock toggles Shift so all keys are affected.");
 I18N_NOOP("Alt and Meta are on the Alt keys (default).");
 I18N_NOOP("Alt is mapped to the right Win-key and Super to Menu.");
 I18N_NOOP("Compose key position");
 I18N_NOOP("Right Alt is Compose.");
 I18N_NOOP("Right Win-key is Compose.");
 I18N_NOOP("Menu is Compose.");
 I18N_NOOP("Right Ctrl is Compose.");
 I18N_NOOP("Caps Lock is Compose.");
 I18N_NOOP("Special keys (Ctrl+Alt+&lt;key&gt;) handled in a server.");
 I18N_NOOP("Adding the EuroSign to certain keys");
 I18N_NOOP("Add the EuroSign to the E key.");
 I18N_NOOP("Add the EuroSign to the 5 key.");
 I18N_NOOP("Add the EuroSign to the 2 key.");
#endif
