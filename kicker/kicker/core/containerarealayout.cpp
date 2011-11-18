/*****************************************************************

Copyright (c) 1996-2004 the kicker authors. See file AUTHORS.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include <assert.h>
#include <math.h>

#include <tqapplication.h>
#include <kdebug.h>
#include <kglobal.h>

#include "containerarea.h"
#include "containerarealayout.h"
#include "container_applet.h"
#include "container_button.h"

class ContainerAreaLayoutIterator : public TQGLayoutIterator
{
    public:
        ContainerAreaLayoutIterator(ContainerAreaLayout::ItemList *l)
            : m_idx(0), m_list(l)
        {
        }

        TQLayoutItem* current()
        {
            return m_idx < int(m_list->count()) ? (*m_list->tqat(m_idx))->item : 0;
        }

        TQLayoutItem* next()
        {
            m_idx++;
            return current();
        }

        TQLayoutItem* takeCurrent()
        {
            TQLayoutItem* item = 0;
            ContainerAreaLayout::ItemList::iterator b = m_list->tqat(m_idx);
            if (b != m_list->end())
            {
                ContainerAreaLayoutItem* layoutItem = *b;
                item = layoutItem->item;
                layoutItem->item = 0;
                m_list->erase(b);
                delete layoutItem;
            }
            return item;
        }

    private:
        int m_idx;
        ContainerAreaLayout::ItemList* m_list;
};

int ContainerAreaLayoutItem::heightForWidth(int w) const
{
    BaseContainer* container = dynamic_cast<BaseContainer*>(item->widget());
    if (container)
    {
        return container->heightForWidth(w);
    }
    else
    {
        return item->tqsizeHint().height();
    }
}

int ContainerAreaLayoutItem::widthForHeight(int h) const
{
    BaseContainer* container = dynamic_cast<BaseContainer*>(item->widget());
    if (container)
    {
        return container->widthForHeight(h);
    }
    else
    {
        return item->tqsizeHint().width();
    }
}

bool ContainerAreaLayoutItem::isStretch() const
{
    BaseContainer* container = dynamic_cast<BaseContainer*>(item->widget());
    return container ? container->isStretch() : false;
}

double ContainerAreaLayoutItem::freeSpaceRatio() const
{
    BaseContainer* container = dynamic_cast<BaseContainer*>(item->widget());
    if (container)
        return kClamp(container->freeSpace(), 0.0, 1.0);
    else
        return m_freeSpaceRatio;
}

void ContainerAreaLayoutItem::setFreeSpaceRatio(double ratio)
{
    BaseContainer* container = dynamic_cast<BaseContainer*>(item->widget());
    if (container)
        container->setFreeSpace(ratio);
    else
        m_freeSpaceRatio = ratio;
}

Qt::Orientation ContainerAreaLayoutItem::orientation() const
{
    return m_layout->orientation();
}

TQRect ContainerAreaLayoutItem::geometryR() const
{
    return m_layout->transform(geometry());
}

void ContainerAreaLayoutItem::setGeometryR(const TQRect& r)
{
    setGeometry(m_layout->transform(r));
}

int ContainerAreaLayoutItem::widthForHeightR(int h) const
{
    if (orientation() == Qt::Horizontal)
    {
        return widthForHeight(h);
    }
    else
    {
        return heightForWidth(h);
    }
}

int ContainerAreaLayoutItem::widthR() const
{
    if (orientation() == Qt::Horizontal)
    {
        return geometry().width();
    }
    else
    {
        return geometry().height();
    }
}

int ContainerAreaLayoutItem::heightR() const
{
    if (orientation() == Qt::Horizontal)
    {
        return geometry().height();
    }
    else
    {
        return geometry().width();
    }
}

int ContainerAreaLayoutItem::leftR() const
{
    if (orientation() == Qt::Horizontal)
    {
        if (TQApplication::reverseLayout())
            return m_layout->geometry().right() - geometry().right();
        else
            return geometry().left();
    }
    else
    {
        return geometry().top();
    }
}

int ContainerAreaLayoutItem::rightR() const
{
    if (orientation() == Qt::Horizontal)
    {
        if (TQApplication::reverseLayout())
            return m_layout->geometry().right() - geometry().left();
        else
            return geometry().right();
    }
    else
    {
        return geometry().bottom();
    }
}


ContainerAreaLayout::ContainerAreaLayout(TQWidget* parent)
    : TQLayout(parent),
      m_orientation(Qt::Horizontal),
      m_stretchEnabled(true)
{
}

#ifdef USE_QT4
/*!
    \reimp
*/
int ContainerAreaLayout::count() const {
	return m_items.count();
}

/*!
    \reimp
*/
TQLayoutItem* ContainerAreaLayout::itemAt(int index) const {
	return index >= 0 && index < m_items.count() ? (*m_items.tqat(index))->item : 0;
}

/*!
    \reimp
*/
TQLayoutItem* ContainerAreaLayout::takeAt(int index) {
	if (index < 0 || index >= m_items.count())
		return 0;
	ContainerAreaLayoutItem *b = *m_items.tqat(index);
	m_items.remove(m_items.tqat(index));
	TQLayoutItem *item = b->item;
	b->item = 0;
	delete b;

	invalidate();
	return item;
}
#endif // USE_QT4

void ContainerAreaLayout::addItem(TQLayoutItem* item)
{
    m_items.append(new ContainerAreaLayoutItem(static_cast<TQLayoutItem*>(item), this));
}

void ContainerAreaLayout::insertIntoFreeSpace(TQWidget* widget, TQPoint insertionPoint)
{
    if (!widget)
    {
        return;
    }

    add(widget);
    ContainerAreaLayoutItem* item = m_items.last();

    if (!item)
    {
        // this should never happen as we just added the item above
        // but we do this to be safe.
        return;
    }

    ItemList::iterator currentIt = m_items.begin();
    if (currentIt == m_items.end())
    {
        // this shouldn't happen either, but again... we try and be safe
        return;
    }

    ItemList::iterator nextIt = m_items.begin();
    ++nextIt;

    if (nextIt == m_items.end())
    {
        // first item in!
        item->setGeometryR(TQRect(insertionPoint.x(), insertionPoint.y(), widget->width(), widget->height()));
        updateFreeSpaceValues();
        return;
    }

    int insPos = (orientation() == Qt::Horizontal) ? insertionPoint.x(): insertionPoint.y();
    Item* current = *currentIt;
    Item* next = *nextIt;

    for (; nextIt != m_items.end(); ++currentIt, ++nextIt)
    {
        next = *nextIt;
        current = *currentIt;
        if (current == item || next == item)
        {
            continue;
        }

        if (insPos == 0)
        {
            if (current->rightR() + 3 < next->leftR())
            {
                insPos = current->rightR();
                break;
            }
        }
        else
        {
            if (currentIt == m_items.begin() &&
                (current->leftR() > insPos ||
                 (current->leftR() <= insPos) &&
                 (current->rightR() > insPos)))
            {
                break;
            }

            if ((current->rightR() < insPos) && (next->leftR() > insPos))
            {
                // Free space available at insertion point!
                if (insPos + item->widthR() > next->leftR())
                {
                    // We have overlap on the right, move to the left
                    insPos = next->leftR() - item->widthR();
                    if (insPos < current->rightR())
                    {
                        // We have overlap on the left as well, move to the right
                        insPos = current->rightR();
                        // We don't fit entirely, let updateFreeSpaceValues sort it out
                    }
                }
                current = next;
                break;
            }

            if ((next->leftR() <= insPos) && (next->rightR() > insPos))
            {
                // Insert at the location of next
                current = next;
                insPos = next->leftR();
                break;
            }
        }
    }

    TQRect geom = item->geometryR();
    geom.moveLeft(insPos);
    item->setGeometryR(geom);
    widget->setGeometry(transform(geom)); // widget isn't shown, layout not active yet

    if (current)
    {
        m_items.erase(m_items.fromLast());
        ItemList::iterator insertIt = m_items.find(current);

        if (insertIt == m_items.begin())
        {
            m_items.push_front(item);
        }
        else if (insertIt == m_items.end())
        {
            // yes, we just removed it from the end, but
            // if we remove it afterwards and it insertIt
            // was our item then we end up with a bad iterator
            m_items.append(item);
        }
        else
        {
            m_items.insert(insertIt, item);
        }
    }

    updateFreeSpaceValues();
}

TQStringList ContainerAreaLayout::listItems() const
{
    TQStringList items;
    for (ItemList::const_iterator it = m_items.constBegin();
         it != m_items.constEnd(); ++it)
    {
        TQLayoutItem* item = (*it)->item;
        BaseContainer* container = dynamic_cast<BaseContainer*>(item->widget());

        if (!container)
        {
            continue;
        }

        AppletContainer* applet = dynamic_cast<AppletContainer*>(container);
        if (applet)
        {
            items.append(applet->info().desktopFile());
        }
        else
        {
            // button containers don't give us anything useful that isn't
            // i18n'd (e.g. all service buttons and url buttons simply report
            // "URL" as their tileName()) which would require that we
            // extend PanelButton to be more expressive to get something more
            // instead i just cop'd out and use the visible name. good enough.
            items.append(container->visibleName());
        }
    }

    return items;
}

TQWidget* ContainerAreaLayout::widgetAt(int index) const
{
    if (index < 0 || index >= (int)m_items.count())
    {
        return 0;
    }

    return m_items[index]->item->widget();
}

TQSize ContainerAreaLayout::tqsizeHint() const
{
    const int size = KickerLib::sizeValue(KPanelExtension::SizeSmall);

    if (orientation() == Qt::Horizontal)
    {
        return TQSize(widthForHeight(size), size);
    }
    else
    {
        return TQSize(size, heightForWidth(size));
    }
}

TQSize ContainerAreaLayout::tqminimumSize() const
{
    const int size = KickerLib::sizeValue(KPanelExtension::SizeTiny);

    if (orientation() == Qt::Horizontal)
    {
        return TQSize(widthForHeight(size), size);
    }
    else
    {
        return TQSize(size, heightForWidth(size));
    }
}

TQLayoutIterator ContainerAreaLayout::iterator()
{
    // [FIXME]
#ifdef USE_QT4
    #warning [FIXME] ContainerAreaLayout iterators may not function correctly under Qt4
    return TQLayoutIterator(this);	    	// [FIXME]
#else // USE_QT4
    return TQLayoutIterator(new ContainerAreaLayoutIterator(&m_items));
#endif // USE_QT4
}

void ContainerAreaLayout::setGeometry(const TQRect& rect)
{
    //RESEARCH: when can we short circuit this?
    //          maybe a dirty flag to be set when we have containers
    //          that needs laying out?

    TQLayout::setGeometry(rect);

    float totalFreeSpace = kMax(0, widthR() - widthForHeightR(heightR()));
    int occupiedSpace = 0;

    ItemList::const_iterator it = m_items.constBegin();
    while (it != m_items.constEnd())
    {
        ContainerAreaLayoutItem* cur  = *it;
        ++it;
        ContainerAreaLayoutItem* next = (it != m_items.constEnd()) ? *it : 0;

        double fs = cur->freeSpaceRatio();
        double freeSpace = fs * totalFreeSpace;
        int pos = int(rint(freeSpace)) + occupiedSpace;

        int w = cur->widthForHeightR(heightR());
        occupiedSpace += w;
        if (m_stretchEnabled && cur->isStretch())
        {
            if (next)
            {
                double nfs = next->freeSpaceRatio();
                w += int((nfs - fs)*totalFreeSpace);
            }
            else
            {
                w = widthR() - pos;
            }
        }
        cur->setGeometryR(TQRect(pos, 0, w, heightR()));
    }
}

int ContainerAreaLayout::widthForHeight(int h) const
{
    int width = 0;
    ItemList::const_iterator it = m_items.constBegin();
    for (; it != m_items.constEnd(); ++it)
    {
        width += kMax(0, (*it)->widthForHeight(h));
    }
    return width;
}

int ContainerAreaLayout::heightForWidth(int w) const
{
    int height = 0;
    ItemList::const_iterator it = m_items.constBegin();
    for (; it != m_items.constEnd(); ++it)
    {
        height += kMax(0, (*it)->heightForWidth(w));
    }
    return height;
}

void ContainerAreaLayout::setStretchEnabled(bool enable)
{
    if (m_stretchEnabled != enable)
    {
        m_stretchEnabled = enable;
        activate();
    }
}

void ContainerAreaLayout::updateFreeSpaceValues()
{
    int freeSpace =
        kMax(0, widthR() - widthForHeightR(heightR()));

    double fspace = 0;
    for (ItemList::const_iterator it = m_items.constBegin();
         it != m_items.constEnd();
         ++it)
    {
        int distance = distanceToPreviousItem(it);
        if (distance < 0) distance = 0;
        fspace += distance;
        double ssf = ( freeSpace == 0 ? 0 : fspace/freeSpace );
        if (ssf > 1) ssf = 1;
        if (ssf < 0) ssf = 0;
        (*it)->setFreeSpaceRatio(ssf);
    }
}

int ContainerAreaLayout::distanceToPreviousItem(ItemList::const_iterator it) const
{
    assert(it != m_items.constEnd());

    ContainerAreaLayoutItem* cur  = *it;
    --it;
    ContainerAreaLayoutItem* prev = (it != m_items.constEnd()) ? *it : 0;

    return prev ? cur->leftR() - prev->leftR() - prev->widthForHeightR(heightR()) :
                  cur->leftR() - leftR();
}

void ContainerAreaLayout::moveContainerSwitch(TQWidget* container, int distance)
{
    const bool horizontal    = orientation() == Qt::Horizontal;
    const bool reverseLayout = TQApplication::reverseLayout();

    if (horizontal && reverseLayout)
        distance = - distance;

    const bool forward = distance > 0;

    // Get the iterator 'it' pointing to 'moving'.
    ItemList::const_iterator it = m_items.constBegin();
    while (it != m_items.constEnd() && (*it)->item->widget() != container)
    {
        ++it;
    }

    if (it == m_items.constEnd())
    {
        return;
    }

    ContainerAreaLayoutItem* moving = *it;
    forward ? ++it : --it;
    ContainerAreaLayoutItem* next = (it != m_items.constEnd()) ? *it : 0;
    ContainerAreaLayoutItem* last = moving;

    while (next)
    {
        // Calculate the position and width of the virtual container
        // containing 'moving' and 'next'.
        int tpos = forward ? next->leftR() - moving->widthR()
                           : next->leftR();
        int tsize = moving->widthR() + next->widthR();

        // Determine the middle of the containers.
        int tmiddle = tpos + tsize / 2;
        int movingMiddle = moving->leftR() + distance + moving->widthR() / 2;

        // Check if the middle of 'moving' has moved past the middle of the
        // virtual container.
        if (!forward && movingMiddle > tmiddle
          || forward && movingMiddle < tmiddle)
            break;

        // Move 'next' to the other side of 'moving'.
        TQRect geom = next->geometryR();
        if (forward)
            geom.moveLeft(geom.left() - moving->widthR());
        else
            geom.moveLeft(geom.left() + moving->widthR());
        next->setGeometryR(geom);

        // Store 'next'. It may become null in the next iteration, but we
        // need its value later.
        last = next;
        forward ? ++it : --it;
        next = (it != m_items.constEnd()) ? *it : 0;
    }

    int newPos = moving->leftR() + distance;
    if (last != moving)
    {
        // The case that moving has switched position with at least one other container.
        newPos = forward ? kMax(newPos, last->rightR() + 1)
                         : kMin(newPos, last->leftR() - moving->widthR());

        // Move 'moving' to its new position in the container list.
        ItemList::iterator itMoving = m_items.find(moving);

        if (itMoving != m_items.end())
        {
            ItemList::iterator itLast = itMoving;
            if (forward)
            {
                ++itLast;
                ++itLast;
            }
            else
            {
                --itLast;
            }

            m_items.erase(itMoving);

            if (itLast == m_items.end())
            {
                if (forward)
                {
                    m_items.append(moving);
                }
                else
                {
                    m_items.push_front(moving);
                }
            }
            else
            {
                m_items.insert(itLast, moving);
            }
        }
    }
    else if (next)
    {
        // Make sure that the moving container will not overlap the next one.
        newPos = forward ? kMin(newPos, next->leftR() - moving->widthR())
                         : kMax(newPos, next->rightR() + 1);
    }

    // Move the container to its new position and prevent it from moving outside the panel.
    TQRect geom = moving->geometryR();
    distance = kClamp(newPos, 0, widthR() - moving->widthR());
    geom.moveLeft(distance);
    moving->setGeometryR(geom);

    // HACK - since the menuapplet is not movable by the user, make sure it's always left-aligned
    ItemList::const_iterator prev = m_items.constEnd();
    for( ItemList::const_iterator it = m_items.constBegin();
         it != m_items.constEnd();
         ( prev = it ), ++it )
    {
        if( BaseContainer* container = dynamic_cast<BaseContainer*>((*it)->item->widget()))
            if(AppletContainer* applet = dynamic_cast<AppletContainer*>(container))
                if( applet->info().desktopFile() == "menuapplet.desktop" )
                {
                    TQRect geom = (*it)->geometryR();
                    if( prev != m_items.constEnd())
                        geom.moveLeft( (*prev)->rightR() + 1 );
                    else
                        geom.moveLeft( 0 );
                    (*it)->setGeometryR( geom );
                }
    }

    updateFreeSpaceValues();
}

int ContainerAreaLayout::moveContainerPush(TQWidget* a, int distance)
{
    const bool horizontal    = orientation() == Qt::Horizontal;
    const bool reverseLayout = TQApplication::reverseLayout();

    // Get the iterator 'it' pointing to the layoutitem representing 'a'.
    ItemList::const_iterator it = m_items.constBegin();
    while (it != m_items.constEnd() && (*it)->item->widget() != a)
    {
        ++it;
    }

    if (it != m_items.constEnd())
    {
        if (horizontal && reverseLayout)
        {
            distance = -distance;
        }

        int retVal = moveContainerPushRecursive(it, distance);
        updateFreeSpaceValues();
        if (horizontal && reverseLayout)
        {
            retVal = -retVal;
        }
        return retVal;
    }
    else
    {
        return 0;
    }
}

int ContainerAreaLayout::moveContainerPushRecursive(ItemList::const_iterator it,
                                                    int distance)
{
    if (distance == 0)
        return 0;

    const bool forward = distance > 0;

    int available; // Space available for the container to move.
    int moved;     // The actual distance the container will move.
    ContainerAreaLayoutItem* cur  = *it;
    forward ? ++it : --it;
    ContainerAreaLayoutItem* next = (it != m_items.constEnd()) ? *it : 0;

    if (!next)
    {
        available = forward ? rightR() - cur->rightR()
                            : -cur->leftR();
    }
    else
    {
        available = forward ? next->leftR()  - cur->rightR() - 1
                            : next->rightR() - cur->leftR()  + 1;

        if (!forward && distance < available
          || forward && distance > available)
            available += moveContainerPushRecursive(it, distance - available);
    }
    moved = forward ? kMin(distance, available)
                    : kMax(distance, available);

    TQRect geom = cur->geometryR();
    geom.moveLeft(geom.left() + moved);
    cur->setGeometryR(geom);

    return moved;
}

TQRect ContainerAreaLayout::transform(const TQRect& r) const
{
    if (orientation() == Qt::Horizontal)
    {
        if (TQApplication::reverseLayout())
        {
            TQRect t = r;
            t.moveLeft(geometry().right() - r.right());
            return t;
        }
        else
        {
            return r;
        }
    }
    else
    {
        return TQRect(r.y(), r.x(), r.height(), r.width());
    }
}

int ContainerAreaLayout::widthForHeightR(int h) const
{
    if (orientation() == Qt::Horizontal)
    {
        return widthForHeight(h);
    }
    else
    {
        return heightForWidth(h);
    }
}

int ContainerAreaLayout::widthR() const
{
    if (orientation() == Qt::Horizontal)
    {
        return geometry().width();
    }
    else
    {
        return geometry().height();
    }
}

int ContainerAreaLayout::heightR() const
{
    if (orientation() == Qt::Horizontal)
    {
        return geometry().height();
    }
    else
    {
        return geometry().width();
    }
}

int ContainerAreaLayout::leftR() const
{
    if (orientation() == Qt::Horizontal)
        return geometry().left();
    else
        return geometry().top();
}

int ContainerAreaLayout::rightR() const
{
    if (orientation() == Qt::Horizontal)
        return geometry().right();
    else
        return geometry().bottom();
}

