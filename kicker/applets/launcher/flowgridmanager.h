/*  Copyright 2004, Daniel Woods Bullok <dan.devel@bullok.com>
    distributed under the terms of the
    GNU GENERAL PUBLIC LICENSE Version 2 -
    See the file tdebase/COPYING for details
*/

#ifndef __const_space_grid_h__
#define __const_space_grid_h__

#include <tqnamespace.h>
#include <tqpoint.h>
#include <tqsize.h>


class FlowGridManager {
// Determine if two FlowGridManager objs have the same layout.  They may or
// may not have the same input parameters, but the resulting layout is identical.
    friend bool operator== ( const FlowGridManager & gp1, const FlowGridManager & gp2 );

public:
    typedef enum {
        ItemSlack,SpaceSlack,BorderSlack,NoSlack
    } Slack;

    FlowGridManager(TQSize p_item_size=TQSize(0,0),
                                     TQSize p_space_size=TQSize(0,0),
                                     TQSize p_border_size=TQSize(0,0),
                                     TQSize frame_size=TQSize(0,0),
                                     TQt::Orientation orient=TQt::Horizontal,
                                     int num_items=0,
                                     Slack slack_x=ItemSlack,
                                     Slack slack_y=ItemSlack);


    void setNumItems(int num_items);
    void setItemSize(TQSize item_size);
    void setSpaceSize(TQSize space_size);
    void setBorderSize(TQSize border_size);
    void setOrientation(TQt::Orientation orient);
    void setFrameSize(TQSize frame_size);
    void setSlack(Slack slack_x, Slack slack_y);
    void setConserveSpace(bool conserve);


    TQSize  itemSize() const;
    TQSize  spaceSize() const;
    TQSize  borderSize() const;
    TQSize  gridDim() const;
    TQSize  gridSpacing() const;
    TQSize  frameSize() const;
    TQPoint origin() const;
    TQt::Orientation orientation() const;
    bool   conserveSpace() const;

//    Slack  slackX() const;
//    Slack  slackY() const;

    TQPoint posAtCell(int x,int y) const;
    TQPoint pos(int i) const;
    TQPoint cell(int index) const;
    bool isValid() const;
    int indexNearest(TQPoint p) const;

    void dump();
protected:
    int _getHH(TQSize size) const;
    int _getWH(TQSize size) const;
    TQSize _swapHV(TQSize hv) const;
    inline void _checkReconfigure() const;
    int _slack(int nitems,int length,int item,int space,int border) const;
    void _reconfigure() const;
    void _clear() const;

protected:
    // user-definable data
    TQSize _pItemSize,_pSpaceSize,_pBorderSize,_pFrameSize;
    Slack _slackX, _slackY;
    bool _conserveSpace;
    TQt::Orientation _orientation;
    int _numItems;

    // results
    mutable TQSize _itemSize, _spaceSize, _borderSize, _gridDim, _gridSpacing, _frameSize;
    mutable TQPoint _origin;

    // status
    mutable bool _dirty, _valid;

};


// reconfigure the grid if necessary.
inline void FlowGridManager::_checkReconfigure() const
{   if (!_dirty) return;
    _reconfigure();
}

#endif

