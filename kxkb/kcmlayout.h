#ifndef __KCM_LAYOUT_H__
#define __KCM_LAYOUT_H__


#include <tdecmodule.h>

#include <tqstring.h>
#include <tqlistview.h>

#include "kxkbconfig.h"


class OptionListItem;
class LayoutConfigWidget;
class XkbRules;

class LayoutConfig : public TDECModule
{
  TQ_OBJECT

public:
  LayoutConfig(TQWidget *parent = 0L, const char *name = 0L);
  virtual ~LayoutConfig();

  void load();
  void save();
  void defaults();
  void initUI();
  virtual TQString handbookDocPath() const;

protected:
  TQString createOptionString();
  void updateIndicator(TQListViewItem* selLayout);
  OptionListItem* itemForOption(TQString option);

protected slots:
  void moveUp();
  void moveDown();
  void hotkeyComboChanged();
  void variantChanged();
  void displayNameChanged(const TQString& name);
  void layoutSelChanged(TQListViewItem *);
  void loadRules();
  void updateLayoutCommand();
  void updateOptionsCommand();
  void updateHotkeyCombo();
  void updateHotkeyCombo(bool initial);
  void add();
  void remove();
  void resolveConflicts(TQListViewItem *lvi);

  void changed();

private:
  LayoutConfigWidget* widget;

  XkbRules *m_rules;
  KxkbConfig m_kxkbConfig;
  TQDict<OptionListItem> m_optionGroups;
  bool m_forceGrpOverwrite;
  KKeyChooser *m_keyChooser;
  TDEGlobalAccel *keys;

  TQWidget* makeOptionsTab();
  TQWidget* makeShortcutsTab();
  void updateStickyLimit();
  static LayoutUnit getLayoutUnitKey(TQListViewItem *sel);
  void checkConflicts(OptionListItem *current, TQStringList conflicting,
                      TQStringList &conflicts);
};


#endif
