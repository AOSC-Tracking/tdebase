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

#ifndef __containerarealayout_h__
#define __containerarealayout_h__

#include <tqlayout.h>

class ContainerAreaLayout;

class ContainerAreaLayoutItem : public TQt
{
    public:
        ContainerAreaLayoutItem(TQLayoutItem* i, ContainerAreaLayout* layout)
            : item(i),
              m_freeSpaceRatio(0.0),
              m_layout(layout)
        {}

        ~ContainerAreaLayoutItem()
        { delete item; }

        int heightForWidth(int w) const;
        int widthForHeight(int h) const;

        bool isStretch() const;

        TQRect geometry() const
        { return item->geometry(); }
        void setGeometry(const TQRect& geometry)
        { item->setGeometry(geometry); }

        double freeSpaceRatio() const;
        void setFreeSpaceRatio(double ratio);

        Orientation orientation() const;

        // Relative geometry
        TQRect geometryR() const;
        void setGeometryR(const TQRect&);
        int widthForHeightR(int w) const;
        int widthR() const;
        int heightR() const;
        int leftR() const;
        int rightR() const;

        TQLayoutItem* item;

    private:
        double m_freeSpaceRatio;
        ContainerAreaLayout* m_layout;
};

class ContainerAreaLayout : public TQLayout
{
    public:
        typedef ContainerAreaLayoutItem Item;
        typedef TQValueList<Item*> ItemList;

        ContainerAreaLayout(TQWidget* parent);

        void addItem(TQLayoutItem* item);
        void insertIntoFreeSpace(TQWidget* item, TQPoint insertionPoint);
        TQStringList listItems() const;
        TQWidget* widgetAt(int index) const;
        TQSize sizeHint() const;
        TQSize minimumSize() const;
        TQLayoutIterator iterator();
        void setGeometry(const TQRect& rect);

        Orientation orientation() const { return m_orientation; }
        void setOrientation(Orientation o) { m_orientation = o; }
        int heightForWidth(int w) const;
        int widthForHeight(int h) const;
        void updateFreeSpaceValues();
        void moveToFirstFreePosition(BaseContainer* a);

        void setStretchEnabled(bool enable);

        void moveContainerSwitch(TQWidget* container, int distance);
        int moveContainerPush(TQWidget* container, int distance);

        // Relative geometry
        TQRect transform(const TQRect&) const;
        int widthForHeightR(int w) const;
        int widthR() const;
        int heightR() const;
        int leftR() const;
        int rightR() const;

#ifdef USE_QT4

        QLAYOUT_REQUIRED_METHOD_DECLARATIONS

#endif // USE_QT4

    private:
        int moveContainerPushRecursive(ItemList::const_iterator it, int distance);
        int distanceToPreviousItem(ItemList::const_iterator it) const;

        Orientation m_orientation;
        bool m_stretchEnabled;
        ItemList m_items;
};

#endif
