/*  Copyright 2004, Daniel Woods Bullok <dan.devel@bullok.com>
    distributed under the terms of the
    GNU GENERAL PUBLIC LICENSE Version 2 -
    See the file tdebase/COPYING for details
*/

#ifndef __quickbuttongroup_h__
#define __quickbuttongroup_h__

#include <tqstring.h>
#include <functional>
#include "easyvector.h"
#include "quickbutton.h"


class QuickButtonGroup: virtual public EasyVector< QuickButton* > {
public:
    QuickButtonGroup(const EasyVector< QuickButton* > &kv):EasyVector< QuickButton* >(kv){};
    QuickButtonGroup():EasyVector< QuickButton* >(){};
    Index findDescriptor(const TQString &desc);

    void show();
    void hide();
    void setDragging(bool drag);
    void setEnableDrag(bool enable);
    void deleteContents();
    void setUpdatesEnabled(bool enable);
};

QuickButtonGroup::Index QuickButtonGroup::findDescriptor(const TQString &desc)
{   return findProperty(desc, std::mem_fn(&QuickButton::url));}

inline void QuickButtonGroup::setUpdatesEnabled(bool enable)
{   for (QuickButtonGroup::iterator i=begin();i!=end();++i) {
        (*i)->setUpdatesEnabled(enable);
        if (enable) { (*i)->update();}
    }
}

inline void QuickButtonGroup::show()
{   std::for_each(begin(),end(),std::mem_fn(&TQWidget::show));}

inline void QuickButtonGroup::hide()
{   std::for_each(begin(),end(),std::mem_fn(&TQWidget::hide));}

inline void QuickButtonGroup::setDragging(bool drag)
{   std::for_each(begin(),end(),std::bind(std::mem_fn(&QuickButton::setDragging),std::placeholders::_1,drag));}

inline void QuickButtonGroup::setEnableDrag(bool enable)
{   std::for_each(begin(),end(),std::bind(std::mem_fn(&QuickButton::setEnableDrag),std::placeholders::_1,enable));}

inline void QuickButtonGroup::deleteContents()
{   for (QuickButtonGroup::iterator i=begin();i!=end();++i) {
        delete (*i);
        (*i)=0;
    }
}

#endif

