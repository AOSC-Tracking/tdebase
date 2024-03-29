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
#include <tqvbox.h>
#include <tqtimer.h>

#include <tdefontrequester.h>
#include <kcolorbutton.h>
#include <kkeydialog.h>
#include <tdeglobal.h>
#include <tdeconfig.h>
#include <tdelocale.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <tdeapplication.h>
#include <kiconloader.h>
#include <tdemessagebox.h>
#include <kglobalaccel.h>
#include <dcopref.h>
#include <dcopclient.h>

#include "extension.h"
#include "kxkbconfig.h"
#include "rules.h"
#include "pixmap.h"
#include "kcmmisc.h"
#include "kcmlayoutwidget.h"
#include "x11helper.h"

#include "kcmlayout.h"
#include "kcmlayout.moc"


enum {
 LAYOUT_COLUMN_FLAG = 0,
 LAYOUT_COLUMN_NAME = 1,
 LAYOUT_COLUMN_MAP = 2,
 LAYOUT_COLUMN_VARIANT = 3,
 LAYOUT_COLUMN_DISPLAY_NAME = 4,
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
    m_rules(NULL),
    m_forceGrpOverwrite(false)
{
  X11Helper::initializeTranslations();

  m_icoMgr = new LayoutIconManager(&m_kxkbConfig);

  TQVBoxLayout *main = new TQVBoxLayout(this, 0, KDialog::spacingHint());

  widget = new LayoutConfigWidget(this, "widget");
  main->addWidget(widget);

  connect( widget->chkEnable, TQ_SIGNAL( toggled( bool )), this, TQ_SLOT(changed()));
  connect( widget->chkShowSingle, TQ_SIGNAL( toggled( bool )), this, TQ_SLOT(changed()));

  connect( widget->comboHotkey, TQ_SIGNAL(activated(int)), this, TQ_SLOT(hotkeyComboChanged()));
  connect( widget->comboHotkey, TQ_SIGNAL(activated(int)), this, TQ_SLOT(updateOptionsCommand()));
  connect( widget->comboHotkey, TQ_SIGNAL(activated(int)), this, TQ_SLOT(changed()));
  connect( widget->comboModel, TQ_SIGNAL(activated(int)), this, TQ_SLOT(changed()));

  connect( widget->listLayoutsSrc, TQ_SIGNAL(doubleClicked(TQListViewItem*,const TQPoint&, int)),
									this, TQ_SLOT(add()));
  connect( widget->btnAdd, TQ_SIGNAL(clicked()), this, TQ_SLOT(add()));
  connect( widget->btnRemove, TQ_SIGNAL(clicked()), this, TQ_SLOT(remove()));

  connect( widget->comboVariant, TQ_SIGNAL(activated(int)), this, TQ_SLOT(changed()));
  connect( widget->comboVariant, TQ_SIGNAL(activated(int)), this, TQ_SLOT(variantChanged()));
  connect( widget->listLayoutsDst, TQ_SIGNAL(selectionChanged(TQListViewItem *)),
		this, TQ_SLOT(layoutSelChanged(TQListViewItem *)));

  connect( widget->editDisplayName, TQ_SIGNAL(textChanged(const TQString&)), this, TQ_SLOT(displayNameChanged(const TQString&)));

  widget->btnUp->setIconSet(SmallIconSet("1uparrow"));
  connect( widget->btnUp, TQ_SIGNAL(clicked()), this, TQ_SLOT(changed()));
  connect( widget->btnUp, TQ_SIGNAL(clicked()), this, TQ_SLOT(moveUp()));
  widget->btnDown->setIconSet(SmallIconSet("1downarrow"));
  connect( widget->btnDown, TQ_SIGNAL(clicked()), this, TQ_SLOT(changed()));
  connect( widget->btnDown, TQ_SIGNAL(clicked()), this, TQ_SLOT(moveDown()));

  connect( widget->grpStyle, TQ_SIGNAL( clicked( int ) ), TQ_SLOT(changed()));
  connect( widget->grpSwitching, TQ_SIGNAL( clicked( int ) ), TQ_SLOT(changed()));
  connect( widget->grpLabel, TQ_SIGNAL( clicked( int ) ), TQ_SLOT(changed()));

  connect( widget->chkFitToBox, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(changed()));
  connect( widget->chkDimFlag, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(changed()));

  connect( widget->bgColor, TQ_SIGNAL( changed(const TQColor&) ), this, TQ_SLOT(changed()));
  connect( widget->fgColor, TQ_SIGNAL( changed(const TQColor&) ), this, TQ_SLOT(changed()));
  connect( widget->chkBgTransparent, TQ_SIGNAL( toggled(bool) ), this, TQ_SLOT(changed()));
  connect( widget->labelFont, TQ_SIGNAL( fontSelected(const TQFont&) ), this, TQ_SLOT(changed()));
  connect( widget->chkLabelShadow, TQ_SIGNAL( toggled( bool ) ), this, TQ_SLOT(changed()));
  connect( widget->shColor, TQ_SIGNAL( changed(const TQColor&) ), this, TQ_SLOT(changed()));

  connect( widget->chkBevel, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(changed()));

  connect( widget->chkEnableSticky, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(changed()));
  connect( widget->spinStickyDepth, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(changed()));

  connect(widget->chkEnableNotify,       TQ_SIGNAL(toggled(bool)), TQ_SLOT(changed()));
  connect(widget->chkNotifyUseKMilo,     TQ_SIGNAL(toggled(bool)), TQ_SLOT(changed()));
  connect(widget->chkEnableNotify, TQ_SIGNAL(toggled(bool)), widget->chkNotifyUseKMilo, TQ_SLOT(setEnabled(bool)));

  widget->listLayoutsSrc->setColumnText(LAYOUT_COLUMN_FLAG, "");
  widget->listLayoutsDst->setColumnText(LAYOUT_COLUMN_FLAG, "");
//  widget->listLayoutsDst->setColumnText(LAYOUT_COLUMN_DISPLAY_NAME, "");

  widget->listLayoutsSrc->setColumnWidth(LAYOUT_COLUMN_FLAG, 28);
  widget->listLayoutsDst->setColumnWidth(LAYOUT_COLUMN_FLAG, 28);

  widget->listLayoutsDst->header()->setResizeEnabled(false, LAYOUT_COLUMN_DISPLAY_NAME);
//  widget->listLayoutsDst->setColumnWidth(LAYOUT_COLUMN_DISPLAY_NAME, 0);

  widget->listLayoutsDst->setSorting(-1);
#if 0
  widget->listLayoutsDst->setResizeMode(TQListView::LastColumn);
  widget->listLayoutsSrc->setResizeMode(TQListView::LastColumn);
#endif
  widget->listLayoutsDst->setResizeMode(TQListView::LastColumn);

  //Read rules - we _must_ read _before_ creating xkb-options comboboxes
  loadRules();

  // Load global shortcuts
#define NOSLOTS
  keys = new TDEGlobalAccel(this);
#include "kxkbbindings.cpp"
  keys->readSettings();

  makeOptionsTab();
  makeShortcutsTab();
  TQTimer::singleShot(0, this, TQ_SLOT(load()));
}


LayoutConfig::~LayoutConfig()
{
    delete m_rules;
    delete m_icoMgr;
}


void LayoutConfig::load()
{
	bool modified = false;
	m_kxkbConfig.load(KxkbConfig::LOAD_ALL_OPTIONS);

	// Check if the active settings are different from the saved settings
	if (m_kxkbConfig.m_useKxkb)
	{
		XkbOptions options = XKBExtension::the()->getServerOptions();
		modified = m_kxkbConfig.setFromXkbOptions(options);
	}

	m_kxkbConfig.load(KxkbConfig::LOAD_ALL_OPTIONS);
	keys->readSettings();
	initUI(modified);
}

void LayoutConfig::initUI(bool modified) {
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
				newItem->setText(LAYOUT_COLUMN_DISPLAY_NAME, layoutUnit.displayName);
				widget->listLayoutsDst->insertItem(newItem);
				newItem->moveItem(widget->listLayoutsDst->lastItem());

				break;
			}
		}
	}

	// initialize hotkey combo
	TQDict<char> allOptions = m_rules->options();

	TQStringList commonHotkeys;
	commonHotkeys << "alt_shift_toggle" << "ctrl_shift_toggle"
	              << "win_space_toggle" << "alt_space_toggle"
	              << "caps_toggle" << "menu_toggle"
	              << "lwin_toggle" << "rwin_toggle";

	for (TQStringList::ConstIterator hk = commonHotkeys.begin(); hk != commonHotkeys.end(); ++hk ) {
		const char *hkOpt  = tqstrdup(TQString("grp:" + (*hk)).ascii());
		const char *hkDesc = allOptions[hkOpt];
		if (hkDesc != 0) { // the option exists
			widget->comboHotkey->insertItem(XkbRules::trOpt(hkDesc));
		}
	}
	widget->comboHotkey->insertItem(i18n("None"));
	widget->comboHotkey->insertItem(i18n("Other..."));

	// display KXKB switching options
	widget->chkShowSingle->setChecked(m_kxkbConfig.m_showSingle);

	bool showFlag = m_kxkbConfig.m_showFlag;
	bool showLabel = m_kxkbConfig.m_showLabel;
	widget->radFlagLabel->setChecked( showFlag && showLabel );
	widget->radFlagOnly->setChecked( showFlag && !showLabel );
	widget->radLabelOnly->setChecked( !showFlag && showLabel );
	widget->chkFitToBox->setChecked(m_kxkbConfig.m_fitToBox);
	widget->chkDimFlag->setChecked(m_kxkbConfig.m_dimFlag);

	widget->xkbOptsMode->setButton(m_kxkbConfig.m_resetOldOptions ? 0 : 1);

	widget->grpLabel->setButton( ( m_kxkbConfig.m_useThemeColors ? 0 : 1 )  );
	widget->bgColor->setColor( m_kxkbConfig.m_colorBackground );
	widget->fgColor->setColor( m_kxkbConfig.m_colorLabel );
	widget->chkBgTransparent->setChecked( m_kxkbConfig.m_bgTransparent );
	widget->labelFont->setFont( m_kxkbConfig.m_labelFont );
	widget->chkLabelShadow->setChecked( m_kxkbConfig.m_labelShadow );
	widget->shColor->setColor( m_kxkbConfig.m_colorShadow );

	widget->chkBevel->setChecked(m_kxkbConfig.m_bevel);

	widget->grpLabel->setDisabled(showFlag && !showLabel);
	widget->grpLabelColors->setDisabled(m_kxkbConfig.m_useThemeColors);
	widget->labelBgColor->setDisabled(showFlag);
	widget->bgColor->setDisabled(showFlag);
	widget->chkBgTransparent->setDisabled(showFlag);
	widget->grpFlag->setEnabled(showFlag);
	widget->chkDimFlag->setEnabled(showFlag && showLabel);

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

	widget->chkEnableNotify->setChecked(m_kxkbConfig.m_enableNotify);
	widget->chkNotifyUseKMilo->setChecked(m_kxkbConfig.m_notifyUseKMilo);
	widget->chkNotifyUseKMilo->setEnabled(m_kxkbConfig.m_enableNotify);

	updateStickyLimit();

	widget->chkEnable->setChecked( m_kxkbConfig.m_useKxkb );
	widget->grpLayouts->setEnabled( m_kxkbConfig.m_useKxkb );
	widget->swOptsFrame->setEnabled( m_kxkbConfig.m_useKxkb );
	widget->indOptsFrame->setEnabled( m_kxkbConfig.m_useKxkb );

	// display xkb options
	TQStringList activeOptions = TQStringList::split(',', m_kxkbConfig.m_options);
	bool foundGrp = false;
	for (TQStringList::ConstIterator it = activeOptions.begin(); it != activeOptions.end(); ++it)
	{
		TQString option = *it;
		TQString optionKey = option.mid(0, option.find(':'));
		TQString optionName = m_rules->options()[option];

		if (optionKey == "grp") {
			foundGrp = true;
		}

		OptionListItem *item = m_optionGroups[optionKey];

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

	if (!foundGrp) {
		OptionListItem *grpNone = itemForOption("grp:none");
		if (grpNone) {
			grpNone->setOn(true);
		}
	}

	updateOptionsCommand();
	updateHotkeyCombo(true);
	emit TDECModule::changed(modified);
}


void LayoutConfig::save()
{
	TQString model = lookupLocalized(m_rules->models(), widget->comboModel->currentText());
	m_kxkbConfig.m_model = model;

	m_kxkbConfig.m_resetOldOptions = widget->radXkbOverwrite->isOn();
	m_kxkbConfig.m_options = createOptionString();

	m_kxkbConfig.m_fitToBox = widget->chkFitToBox->isChecked();
	m_kxkbConfig.m_dimFlag = widget->chkDimFlag->isChecked();

	m_kxkbConfig.m_useThemeColors = widget->radLabelUseTheme->isChecked();
	m_kxkbConfig.m_colorBackground = widget->bgColor->color();
	m_kxkbConfig.m_colorLabel = widget->fgColor->color();
	m_kxkbConfig.m_bgTransparent = widget->chkBgTransparent->isChecked();
	m_kxkbConfig.m_labelFont = widget->labelFont->font();
	m_kxkbConfig.m_labelShadow = widget->chkLabelShadow->isChecked();
	m_kxkbConfig.m_colorShadow = widget->shColor->color();

	m_kxkbConfig.m_bevel = widget->chkBevel->isChecked();

	TQListViewItem *item = widget->listLayoutsDst->firstChild();
	TQValueList<LayoutUnit> layouts;
	while (item) {
		TQString layout = item->text(LAYOUT_COLUMN_MAP);
		TQString variant = item->text(LAYOUT_COLUMN_VARIANT);
		TQString displayName = item->text(LAYOUT_COLUMN_DISPLAY_NAME);

		LayoutUnit layoutUnit(layout, variant);
		layoutUnit.displayName = displayName;
		layouts.append( layoutUnit );

		item = item->nextSibling();
		kdDebug() << "To save: layout " << layoutUnit.toPair()
				<< ", disp: " << layoutUnit.displayName << endl;
	}
	m_kxkbConfig.m_layouts = layouts;

	if( m_kxkbConfig.m_layouts.count() == 0 ) {
		m_kxkbConfig.m_layouts.append(LayoutUnit(DEFAULT_LAYOUT_UNIT));
 		widget->chkEnable->setChecked(false);
 	}

	m_kxkbConfig.m_useKxkb = widget->chkEnable->isChecked();
	m_kxkbConfig.m_showSingle = widget->chkShowSingle->isChecked();

	m_kxkbConfig.m_showFlag = ( widget->radFlagLabel->isChecked() || widget->radFlagOnly->isChecked() );
	m_kxkbConfig.m_showLabel = ( widget->radFlagLabel->isChecked() || widget->radLabelOnly->isChecked() );

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

	m_kxkbConfig.m_enableNotify = widget->chkEnableNotify->isChecked();
	m_kxkbConfig.m_notifyUseKMilo = widget->chkNotifyUseKMilo->isChecked();

	m_kxkbConfig.save();

	// We might need to unset previous hotkey options
	if (m_forceGrpOverwrite)
	{
		// First get all the server's options
		XkbOptions _opt = XKBExtension::the()->getServerOptions();
		TQStringList srvOptions = TQStringList::split(",", _opt.options);
		TQStringList newOptions;

		// Then remove all grp: options
		for (TQStringList::Iterator it = srvOptions.begin(); it != srvOptions.end(); ++it)
		{
			TQString opt(*it);
			if (!opt.startsWith("grp:"))
			{
				newOptions << opt;
			}
		}

		XkbOptions xkbOptions;
		xkbOptions.options = newOptions.join(",");
		xkbOptions.resetOld = true;

		if (!XKBExtension::the()->setXkbOptions(xkbOptions))
		{
			kdWarning() << "[LayoutConfig::save] Could not overwrite previous grp: options!" << endl;
		}

		m_forceGrpOverwrite = false;
	}

	// Save and apply global shortcuts
	m_keyChooser->commitChanges();
	keys->writeSettings(0, true);

	// Get current layout from Kxkb
	if (!kapp->dcopClient()->isAttached())
		kapp->dcopClient()->attach();

	DCOPRef kxkbref("kxkb", "kxkb");
	DCOPReply reply = kxkbref.call( "getCurrentLayout" );

	TQString currentLayout;
	if ( reply.isValid() ) {
		reply.get(currentLayout);
	} else {
		kdDebug() << "Warning: cannot get current layout! (invalid DCOP reply from Kxkb)" << endl;
	}

	// Cause Kxkb to reread configuration
	kapp->tdeinitExecWait("kxkb");

	// If previous call was valid, try to change layout
	if ( reply.isValid() ) {
		DCOPReply successReply = kxkbref.call( "setLayout", currentLayout );

		if ( successReply.isValid() ) {
			bool success;
			successReply.get(success);

			if ( ! success )
				kdDebug() << "Warning: restoring previous layout failed!" << endl;
		} else {
			kdDebug() << "Warning: cannot restore previous layout! (invalid DCOP reply from Kxkb)" << endl;
		}
	}

	updateHotkeyCombo();

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
	updateLayoutCommand();
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

void LayoutConfig::layoutSelChanged(TQListViewItem *sel)
{
    widget->comboVariant->clear();
    widget->comboVariant->setEnabled( sel != NULL );

    if( sel == NULL ) {
        updateLayoutCommand();
        return;
    }


	LayoutUnit layoutUnitKey = getLayoutUnitKey(sel);
	TQString kbdLayout = layoutUnitKey.layout;

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

  connect(listView, TQ_SIGNAL(clicked(TQListViewItem *)), TQ_SLOT(changed()));
  connect(listView, TQ_SIGNAL(clicked(TQListViewItem *)), TQ_SLOT(resolveConflicts(TQListViewItem *)));
  connect(listView, TQ_SIGNAL(clicked(TQListViewItem *)), TQ_SLOT(updateHotkeyCombo()));

  connect(widget->xkbOptsMode, TQ_SIGNAL(released(int)), TQ_SLOT(changed()));
  connect(widget->xkbOptsMode, TQ_SIGNAL(released(int)), TQ_SLOT(updateOptionsCommand()));
  connect(widget->xkbOptsMode, TQ_SIGNAL(released(int)), TQ_SLOT(updateHotkeyCombo()));

  //Create controllers for all options
  TQDictIterator<char> it(m_rules->options());
  OptionListItem *parent;
  for (; it.current(); ++it)
  {
    if (!it.currentKey().contains(':'))
    {
      if( it.currentKey() == "ctrl" || it.currentKey() == "caps"
          || it.currentKey() == "altwin") {
        parent = new OptionListItem(listView, XkbRules::trOpt( it.current() ),
            TQCheckListItem::RadioButtonController, it.currentKey());
        OptionListItem *item = new OptionListItem(parent, i18n( "None" ),
            TQCheckListItem::RadioButton, "none");
        item->setState(TQCheckListItem::On);
      }
      else if (it.currentKey() == "grp") {
        parent = new OptionListItem(listView, XkbRules::trOpt(it.current()),
            TQCheckListItem::RadioButtonController, it.currentKey());
        parent->setSelectable(false);
        OptionListItem *item = new OptionListItem(parent, i18n("None"),
            TQCheckListItem::CheckBox, "grp:none");
      }
      else {
        parent = new OptionListItem(listView, XkbRules::trOpt( it.current() ),
            TQCheckListItem::CheckBoxController, it.currentKey());
      }
      parent->setOpen(true);
      m_optionGroups.insert(it.currentKey(), parent);
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
      if (parent == NULL ) { // All unparanted options go into "custom" group
        parent = m_optionGroups["custom"];
        if (parent == NULL ) {
          parent = new OptionListItem(listView, XkbRules::trOpt( I18N_NOOP("Miscellaneous options") ),
              TQCheckListItem::CheckBoxController, "custom");
        }
      }
      // workaroung for mistake in rules file for xkb options in XFree 4.2.0
      TQString text(it.current());
      text = text.replace( "Cap$", "Caps." );
      if ( parent->type() == TQCheckListItem::CheckBoxController
           || key.startsWith("grp:"))
          new OptionListItem(parent, XkbRules::trOpt(text),
              TQCheckListItem::CheckBox, key);
      else
          new OptionListItem(parent, XkbRules::trOpt(text),
              TQCheckListItem::RadioButton, key);
    }
  }

  //scroll->setMinimumSize(450, 330);

  return listView;
}

TQWidget* LayoutConfig::makeShortcutsTab() {
  m_keyChooser = new KKeyChooser(keys, widget->tabShortcuts, false, false);
  connect(m_keyChooser, TQ_SIGNAL(keyChange()), this, TQ_SLOT(changed()));
  widget->tabShortcuts->layout()->add(m_keyChooser);
  return m_keyChooser;
}

void LayoutConfig::updateOptionsCommand()
{
  TQString setxkbmap;
  TQString options = createOptionString();
  bool overwrite = widget->radXkbOverwrite->isOn();

  if( !options.isEmpty() ) {
    setxkbmap = "setxkbmap -option "; //-rules " + m_rule
    if (overwrite)
      setxkbmap += "-option ";
    setxkbmap += options;
  }
  else if (overwrite) {
    setxkbmap = "setxkbmap -option";
  }
  widget->editCmdLineOpt->setText(setxkbmap);
  widget->editCmdLineOpt->setDisabled(setxkbmap.isEmpty());
}

void LayoutConfig::updateLayoutCommand()
{
  TQString setxkbmap = "setxkbmap";
  setxkbmap += " -model " + lookupLocalized(m_rules->models(),
                                            widget->comboModel->currentText());
  TQStringList layoutCodes;
  TQStringList layoutVariants;
  TQListViewItem *item = widget->listLayoutsDst->firstChild();
  while (item) {
    layoutCodes << item->text(LAYOUT_COLUMN_MAP);

    TQString layoutVariant = item->text(LAYOUT_COLUMN_VARIANT);
    if (layoutVariant == DEFAULT_VARIANT_NAME) {
      layoutVariant = "";
    }
    layoutVariants << layoutVariant;

    item = item->nextSibling();
  }

  setxkbmap += " -layout " + layoutCodes.join(",");

  if( !layoutVariants.isEmpty() ) {
    setxkbmap += " -variant " + layoutVariants.join(",");
  }

  widget->editCmdLine->setText(setxkbmap);

  /* update display name field */
  TQListViewItem *sel = widget->listLayoutsDst->selectedItem();
  if (!sel) {
    return;
  }
  TQString selLayoutCode = sel->text(LAYOUT_COLUMN_MAP);
  TQString selLayoutVariant = widget->comboVariant->currentText();
  TQString selDisplayName = sel->text(LAYOUT_COLUMN_DISPLAY_NAME);
  if (selDisplayName.isEmpty()) {
    int count = 0;
    TQListViewItem *item = widget->listLayoutsDst->firstChild();
    while (item) {
      TQString layoutCode_ = item->text(LAYOUT_COLUMN_MAP);
      if (layoutCode_ == selLayoutCode) {
        ++count;
      }
      item = item->nextSibling();
    }
    bool single = count < 2;
    selDisplayName = m_kxkbConfig.getDefaultDisplayName(LayoutUnit(selLayoutCode, selLayoutVariant), single);
  }

  widget->editDisplayName->setEnabled( sel != NULL );
  widget->editDisplayName->setText(selDisplayName);
}

void LayoutConfig::checkConflicts(OptionListItem *current,
                                  TQStringList conflicting,
                                  TQStringList &conflicts)
{
    if (!current || conflicting.count() < 2 ||
        !conflicting.contains(current->optionName()))
    {
        return;
    }
    TQStringList::Iterator it;
    for (it = conflicting.begin(); it != conflicting.end(); ++it) {
        TQString option(*it);
        if (option == current->optionName()) {
            continue;
        }

        OptionListItem *item = itemForOption(option);
        if (item && item->isOn()) {
            conflicts << item->text();
        }
    }
}

void LayoutConfig::resolveConflicts(TQListViewItem *lvi) {
    OptionListItem *current = (OptionListItem*)lvi;

    kdDebug() << "resolveConflicts : " << current->optionName() << endl;

    if (current->optionName().startsWith("grp:")) {
        OptionListItem *grpItem = m_optionGroups["grp"];
        if (grpItem == NULL) {
            kdWarning() << "LayoutConfig: cannot find grp item group" << endl;
            return;
        }

        OptionListItem *noneItem = grpItem->findChildItem("grp:none");
        if (!noneItem) {
            kdDebug() << "LayoutConfig: unable to find None item for grp!" << endl;
        }
        else {
            // Option "none" selected, uncheck all other options immediately
            if (current->optionName() == "grp:none") {
                if (current->isOn()) {
                    OptionListItem *child = (OptionListItem*)grpItem->firstChild();
                    while (child) {
                        if (child != current) {
                            child->setOn(false);
                        }
                        child = (OptionListItem*)child->nextSibling();
                    }
                }
                else {
                    current->setOn(true);
                }
                updateOptionsCommand();
                return;
            }

            // If no options are selected then select "none"
            bool notNone = false;
            OptionListItem *child = (OptionListItem*)grpItem->firstChild();
            while (child) {
                if (child->isOn() && child->optionName() != "none") {
                    notNone = true;
                    break;
                }
                child = (OptionListItem*)child->nextSibling();
            }

            noneItem->setOn(!notNone);
        }
    }

    TQStringList conflicts;
    OptionListItem *conflict;

    TQStringList conflicting;
    /* Might be incomplete */
    // Space
    conflicting << "grp:win_space_toggle"
                << "grp:alt_space_toggle"
                << "grp:ctrl_space_toggle";
    checkConflicts(current, conflicting, conflicts);

    // Shift
    conflicting.clear();
    conflicting << "grp:ctrl_shift_toggle"
                << "grp:alt_shift_toggle";
    checkConflicts(current, conflicting, conflicts);

    // Control
    conflicting.clear();
    conflicting << "grp:ctrl_select"
                << "grp:ctrl_alt_toggle"
                << "grp:ctrl_shift_toggle";
    checkConflicts(current, conflicting, conflicts);

    // Left Ctrl
    conflicting.clear();
    conflicting << "grp:lctrl_toggle"
                << "grp:lctrl_lshift_toggle";
    checkConflicts(current, conflicting, conflicts);

    // Right Ctrl
    conflicting.clear();
    conflicting << "grp:rctrl_toggle"
                << "grp:rctrl_rshift_toggle";
    checkConflicts(current, conflicting, conflicts);

    // Win
    conflicting.clear();
    conflicting << "grp:win_space_toggle"
                << "grp:win_switch"
                << "win_menu_select";
    checkConflicts(current, conflicting, conflicts);

    // Left Alt
    conflicting.clear();
    conflicting << "grp:lalt_toggle"
                << "grp:lalt_lshift_toggle";
    checkConflicts(current, conflicting, conflicts);

    // Right Alt
    conflicting.clear();
    conflicting << "grp:ralt_toggle"
                << "grp:ralt_rshift_toggle";
    checkConflicts(current, conflicting, conflicts);

    // Caps Lock
    conflicting.clear();
    conflicting << "grp:caps_toggle"
                << "grp:caps_select"
                << "grp:caps_switch"
                << "grp:alt_caps_toggle";
    checkConflicts(current, conflicting, conflicts);

    if (conflicts.count()) {
        TQString curText = current->text();
        int confirm = KMessageBox::warningYesNoList(this,
                          i18n("<qt>The option <b>%1</b> might conflict with "
                               "other options that you have already enabled.<br>"
                               "Are you sure that you really want to enable "
                               "<b>%2</b>?</qt>")
                               .arg(curText).arg(curText),
                          conflicts,
                          i18n("Conflicting options"));
        if (confirm == KMessageBox::No) {
            current->setOn(false);
        }
    }
    updateOptionsCommand();
}

// Synchronizes Xkb grp options --> hotkeys combobox
void LayoutConfig::updateHotkeyCombo() {
    updateHotkeyCombo(false);
}

void LayoutConfig::updateHotkeyCombo(bool initial) {
    OptionListItem *grpItem = m_optionGroups["grp"];
    if (grpItem == NULL) {
        kdWarning() << "LayoutConfig: cannot find grp item group" << endl;
        return;
    }

    TQStringList hotkeys;

    // Get server options first
    if (initial || widget->xkbOptsMode->selectedId() == 1)
    {
        XkbOptions _opt = XKBExtension::the()->getServerOptions();
		TQStringList opts = TQStringList::split(",", _opt.options);
        for (TQStringList::Iterator it = opts.begin(); it != opts.end(); ++it)
        {
            TQString option(*it);

            if (!option.startsWith("grp:"))
            {
                continue;
            }

            // Get description from existing item
            // This spares us the trouble of querying Xkb rules second time
            OptionListItem *item = itemForOption(option);
            if (!item)
            {
                kdWarning() << "[updateHotkeyCombo] server has set unknown option: "
                            << option << endl;
                continue;
            }

            TQString optionName = item->text();
            if (!hotkeys.contains(optionName) && option != "grp:none")
            {
                hotkeys << optionName;
            }
        }
    }

    OptionListItem *child = (OptionListItem*)grpItem->firstChild();
    while (child) {
        TQString optionText = child->text();
        if (child->isOn() && !hotkeys.contains(optionText) && child->optionName() != "grp:none") {
            hotkeys << optionText;
        }
        child = (OptionListItem*)child->nextSibling();
    }

    if (!hotkeys.count()) {
        OptionListItem *noneItem = itemForOption("grp:none");
        if (noneItem)
        {
            hotkeys << noneItem->text();
        }
        else
        {
            kdWarning() << "[updateHotkeyCombo] cannot find grp:none item!" << endl;
            hotkeys << widget->comboHotkey->text(0); // fallback
        }
    }

    int other = widget->comboHotkey->count() - 1;
    widget->comboHotkey->changeItem(i18n("Custom..."), other);
    if (hotkeys.count() < 2) {
        bool found = false;
        for (int i = 0; i < widget->comboHotkey->count(); ++i) {
            if (hotkeys[0] == widget->comboHotkey->text(i)) {
                widget->comboHotkey->setCurrentItem(i);
                found = true;
            }
        }
        if (!found) {
            widget->comboHotkey->changeItem(i18n("Other (%1)").arg(hotkeys[0]),
                                            other);
            widget->comboHotkey->setCurrentItem(other);
        }
    }
    else {
        widget->comboHotkey->changeItem(i18n("Multiple (%1)").arg(hotkeys.join("; ")),
                                        other);
        widget->comboHotkey->setCurrentItem(other);
    }
}

// Synchronizes hotkeys combobox --> Xkb grp options
void LayoutConfig::hotkeyComboChanged() {
    TQStringList hotkeys;
    int other = widget->comboHotkey->count() - 1;

    if (widget->comboHotkey->currentItem() != other) {
        hotkeys << widget->comboHotkey->currentText();
    }
    else {
        TQString otherStr = widget->comboHotkey->text(other);
        int i1 = otherStr.find("(");
        if (i1 != -1) { // custom hotkey(s) set
            ++i1;
            int i2 = otherStr.findRev(")");
            if (i2 != -1) {
                hotkeys = TQStringList::split("; ", otherStr.mid(i1, i2-i1));
            }
        }
    }

    OptionListItem *grpItem = m_optionGroups["grp"];
    if (grpItem == NULL) {
        kdWarning() << "LayoutConfig: cannot find grp item group" << endl;
        return;
    }

    OptionListItem *child = (OptionListItem*)grpItem->firstChild();
    while (child) {
        child->setOn(hotkeys.contains(child->text()));
        child = (OptionListItem*)child->nextSibling();
    }

    if (widget->comboHotkey->currentItem() == other) {
        widget->tabWidget->setCurrentPage(4);
        widget->listOptions->ensureItemVisible(grpItem);
        widget->listOptions->setFocus();
    }

    m_forceGrpOverwrite = true;
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

		item->setPixmap(LAYOUT_COLUMN_FLAG, m_icoMgr->find(layout, PIXMAP_STYLE_CONTEXTMENU));
		item->setText(LAYOUT_COLUMN_NAME, i18n(layoutName.latin1()));
		item->setText(LAYOUT_COLUMN_MAP, layout);
		++it2;
	}
	widget->listLayoutsSrc->setSorting(LAYOUT_COLUMN_NAME);	// from Qt3 TQListView sorts by language

	//TODO: reset options and xkb options
}

OptionListItem* LayoutConfig::itemForOption(TQString option) {
    if (!option.contains(':')) {
        return nullptr;
    }

    TQString optionKey = option.mid(0, option.find(':'));
    OptionListItem *item = m_optionGroups[optionKey];

    if( !item ) {
        item = m_optionGroups["custom"];
        if( !item ) {
            kdDebug() << "WARNING: no group for item '"<< option << "' and 'custom' group does not exists" << endl;
            return nullptr;
        }
    }
    OptionListItem *rv = item->findChildItem(option);
    if( !rv ) {
        kdDebug() << "WARNING: no group for item '"<< option << "' and it's not in group 'custom'" << endl;
    }
    return rv;
}

TQString LayoutConfig::createOptionString()
{
  TQString options;
  for (TQDictIterator<char> it(m_rules->options()); it.current(); ++it)
  {
    TQString option(it.currentKey());
    OptionListItem *child = itemForOption(option);
    if (!child) {
      continue;
    }
    if ( child->state() == TQCheckListItem::On ) {
      TQString selectedName = child->optionName();
      if ( !selectedName.isEmpty() && selectedName != "none" ) {
        if (!options.isEmpty())
          options.append(',');
        options.append(selectedName);
      }
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
	TDE_EXPORT TDECModule *create_keyboard_layout(TQWidget *parent, const char *)
	{
		return new LayoutConfig(parent, "kcmlayout");
	}

	TDE_EXPORT TDECModule *create_keyboard(TQWidget *parent, const char *)
	{
		return new KeyboardConfig(parent, "kcmlayout");
	}

	TDE_EXPORT void init_keyboard()
	{
		KeyboardConfig::init_keyboard();

		KxkbConfig m_kxkbConfig;
		m_kxkbConfig.load(KxkbConfig::LOAD_INIT_OPTIONS);

		if( m_kxkbConfig.m_useKxkb == true ) {
			kapp->startServiceByDesktopName("kxkb");
		}
		else {
			if (!XKBExtension::the()->setXkbOptions(m_kxkbConfig.getKXkbOptions())) {
				kdDebug() << "Setting XKB options failed!" << endl;
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
 I18N_NOOP("Special keys (Ctrl+Alt+&lt;key&gt;) handled in a server.");
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
