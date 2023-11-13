/*  Copyright 2004, Daniel Woods Bullok <dan.devel@bullok.com>
    distributed under the terms of the
    GNU GENERAL PUBLIC LICENSE Version 2 -
    See the file tdebase/COPYING for details
*/

#include "flowgridmanager.h"
#include <kdebug.h>
#ifdef DEBUG
   #define DEBUGSTR kdDebug()
#else
   #define DEBUGSTR kndDebug()
#endif


FlowGridManager::FlowGridManager(TQSize p_item_size,
                                 TQSize p_space_size,
                                 TQSize p_border_size,
                                 TQSize p_frame_size,
                                 TQt::Orientation orient,
                                 int num_items,
                                 Slack slack_x,Slack slack_y)
{
    _pItemSize=p_item_size;
    _pSpaceSize=p_space_size;
    _pBorderSize=p_border_size;
    _pFrameSize=p_frame_size;
    _orientation=orient;
    _numItems=num_items;
    _slackX=slack_x;
    _slackY=slack_y;
    _conserveSpace=false;

    _dirty=true;
    _valid=false;
}

// set members.
// These all set the _dirty flag if the new value is different.
void FlowGridManager::setNumItems(int num_items)
{   if (_numItems==num_items)
        return;
    _numItems=num_items; _dirty=true;
}
void FlowGridManager::setItemSize(TQSize p_item_size)
{   if (_pItemSize==p_item_size)
        return;
    _pItemSize=p_item_size; _dirty=true;
}

void FlowGridManager::setSpaceSize(TQSize p_space_size)
{   if (_pSpaceSize==p_space_size)
        return;
    _pSpaceSize=p_space_size; _dirty=true;
}

void FlowGridManager::setBorderSize(TQSize p_border_size)
{   if (_pBorderSize==p_border_size)
        return;
    _pBorderSize=p_border_size; _dirty=true;
}

void FlowGridManager::setFrameSize(TQSize p_frame_size)
{   if (_pFrameSize==p_frame_size)
        return;
    _pFrameSize=p_frame_size;
    if (_pFrameSize.width()<=0) {
        _orientation=TQt::Vertical;
    }
    if (_pFrameSize.height()<=0) {
        _orientation=TQt::Horizontal;
    }
    _dirty=true;
}

void FlowGridManager::setOrientation(TQt::Orientation orient)
{   if (orient==_orientation)
        return;
    _orientation=orient; _dirty=true;
}

void FlowGridManager::setSlack(Slack slack_x, Slack slack_y)
{   if (slack_x==_slackX && slack_y==_slackY) return;
     _slackX=slack_x; _slackY=slack_y; _dirty=true;}


void FlowGridManager::setConserveSpace(bool conserve)
{   if (_conserveSpace==conserve)
        return;
    _conserveSpace=conserve; _dirty=true;
}



// get members
TQSize  FlowGridManager::itemSize() const
{   _checkReconfigure();  return _itemSize;}

TQSize  FlowGridManager::spaceSize() const
{   _checkReconfigure();  return _spaceSize;}

TQSize  FlowGridManager::borderSize() const
{   _checkReconfigure();  return _borderSize;}

TQSize  FlowGridManager::gridDim() const
{   _checkReconfigure();  return _gridDim;}

TQSize  FlowGridManager::gridSpacing() const
{   _checkReconfigure();  return _gridSpacing;}

TQSize  FlowGridManager::frameSize() const
{   _checkReconfigure();  return _frameSize;}

TQPoint FlowGridManager::origin() const
{   _checkReconfigure();  return _origin;}

TQt::Orientation FlowGridManager::orientation() const
{   _checkReconfigure();  return _orientation;}

/*Slack FlowGridManager::slackX() const
{   return _slackY;}

Slack FlowGridManager::slackY() const
{   return _slackY;}
*/

bool FlowGridManager::conserveSpace() const
{        return _conserveSpace; }


bool FlowGridManager::isValid() const
{   _checkReconfigure();  return _valid;}

TQPoint FlowGridManager::posAtCell(int x,int y) const
{  _checkReconfigure();
    return _origin+TQPoint(_gridSpacing.width()*x,_gridSpacing.height()*y);
}

TQPoint FlowGridManager::pos(int i) const
{   return posAtCell(cell(i).x(),cell(i).y());
}

TQPoint FlowGridManager::cell(int index) const
{   _checkReconfigure();
    //assert((index>=0) && (index<_gridDim.width()*_gridDim.height()));
    int x=index % _gridDim.width(),
        y=index / _gridDim.width();
    return TQPoint(x,y);
}




// return height if orientation is Horizontal
// return width if orientation is Vertical
int FlowGridManager::_getHH(TQSize size) const
{  if (_orientation==TQt::Horizontal)
        return size.height();
    return size.width();
}

// return height if orientation is Vertical
// return width if orientation is Horizontal
int FlowGridManager::_getWH(TQSize size) const
{  if (_orientation==TQt::Horizontal)
        return size.width();
    return size.height();
}

// swap horizontal and vertical if orientation is Vertical, otherwise return arg
TQSize FlowGridManager::_swapHV(TQSize hv) const
{  if (_orientation==TQt::Horizontal)
        return hv;
    TQSize temp=hv;
    temp.transpose();
    return temp;
}


// return the amount of slack when:
//    nitems = # of items
//    length = total length of space where items will be placed
//    item, space, border = length of respective entities
int FlowGridManager::_slack(int nitems,int length,int item,int space,int border) const
{  return length-(2*border)-(nitems-1)*space-nitems*item;}


void FlowGridManager::_clear() const
{
    _borderSize=TQSize(0,0);
    _spaceSize=TQSize(0,0);
    _itemSize=TQSize(0,0);
    _gridDim=TQSize(0,0);
    _gridSpacing=TQSize(0,0);
    _origin=TQPoint(0,0);
    _frameSize=TQSize(0,0);

    _dirty=false;
    _valid=false;
}


int FlowGridManager::indexNearest(TQPoint p) const
{  if (!isValid()) return -1;
   TQPoint c=(p-_origin)-TQPoint(_spaceSize.width(),_spaceSize.height())/2;
   int x=c.x()/_gridSpacing.width(),
       y=c.y()/_gridSpacing.height();
   int i= x+y*_gridDim.width();
   if (i>_numItems) return -1;
   return i;
}



// Redistribute the boxes
void FlowGridManager::_reconfigure() const
{   if ((!_pFrameSize.isValid()) ||
        (!_pItemSize.isValid()) ||
         _numItems==0 ) {
        _clear();
        return;
    }
    int height=_getHH(_pFrameSize),
        pItemHeight=_getHH(_pItemSize),
        pSpaceHeight=_getHH(_pSpaceSize),
        pBorderHeight=_getHH(_pBorderSize),
        spanlen=(height-2*pBorderHeight+pSpaceHeight)/(pItemHeight+pSpaceHeight);
    int slack,iSlack;

    if (spanlen==0) {
        _dirty=false;
        _valid=false;
        return;
    }
    // figure out the number of spans required for all items
    int numspans=_numItems/spanlen;
    if (numspans*spanlen<_numItems) {
        numspans++;
    }

    slack=_slack(spanlen,height,pItemHeight,pSpaceHeight,pBorderHeight);  // total slack
    iSlack=slack/spanlen;                // slack per item
    // Items pick up extra slack
    if (_slackX==ItemSlack) pItemHeight+=iSlack;
    slack=_slack(spanlen,height,pItemHeight,pSpaceHeight,pBorderHeight);

    // space picks up extra slack
    if (spanlen>1) {
        iSlack=slack/(spanlen+1);
        pSpaceHeight+=iSlack;
    }

    slack=_slack(spanlen,height,pItemHeight,pSpaceHeight,pBorderHeight);
    iSlack=slack/2;
    pBorderHeight+=iSlack;
    if (_conserveSpace) {
        _itemSize=_swapHV(TQSize(_getWH(_pItemSize),pItemHeight));
        _spaceSize=_swapHV(TQSize(_getWH(_pSpaceSize),pSpaceHeight));
        _borderSize=_swapHV(TQSize(_getWH(_pBorderSize),pBorderHeight));
    }
    else {
        _itemSize=_swapHV(TQSize(pItemHeight,pItemHeight));
        _spaceSize=_swapHV(TQSize(pSpaceHeight,pSpaceHeight));
        _borderSize=_swapHV(TQSize(pBorderHeight,pBorderHeight));
    }
    _gridDim=_swapHV(TQSize(numspans,spanlen));

    _gridSpacing=_itemSize+_spaceSize;
    _origin=TQPoint(_borderSize.width(),_borderSize.height());
    _frameSize=2*_borderSize+TQSize(_gridDim.width()*_gridSpacing.width()-_spaceSize.width(),
                              _gridDim.height()*_gridSpacing.height()-_spaceSize.height());

    _dirty=false;
    _valid=true;
}


void FlowGridManager::dump()
{
    DEBUGSTR<<endl<<flush;

    DEBUGSTR<<"_pItemSize=("<<_pItemSize.width()<<","<<_pItemSize.height()<<")"<<endl<<flush;
    DEBUGSTR<<"_pSpaceSize=("<<_pSpaceSize.width()<<","<<_pSpaceSize.height()<<")"<<endl<<flush;
    DEBUGSTR<<"_pBorderSize=("<<_pBorderSize.width()<<","<<_pBorderSize.height()<<")"<<endl<<flush;
    DEBUGSTR<<"_pFrameSize=("<<_pFrameSize.width()<<","<<_pFrameSize.height()<<")"<<endl<<flush;
    DEBUGSTR<<"_borderSize=("<<_borderSize.width()<<","<<_borderSize.height()<<")"<<endl<<flush;
    DEBUGSTR<<"_spaceSize=("<<_spaceSize.width()<<","<<_spaceSize.height()<<")"<<endl<<flush;
    DEBUGSTR<<"_itemSize=("<<_itemSize.width()<<","<<_itemSize.height()<<")"<<endl<<flush;
    DEBUGSTR<<"_gridDim=("<<_gridDim.width()<<","<<_gridDim.height()<<")"<<endl<<flush;
    DEBUGSTR<<"_gridSpacing=("<<_gridSpacing.width()<<","<<_gridSpacing.height()<<")"<<endl<<flush;
    DEBUGSTR<<"_origin=("<<_origin.x()<<","<<_origin.y()<<")"<<endl<<flush;
    DEBUGSTR<<"_frameSize=("<<_frameSize.width()<<","<<_frameSize.height()<<")"<<endl<<flush;
    DEBUGSTR<<"_conserveSpace="<<_conserveSpace<<endl<<flush;

    DEBUGSTR<<"_orientation="<<_orientation<<endl<<flush;
    DEBUGSTR<<"_numItems="<<_numItems<<endl<<flush;
    DEBUGSTR<<"_slackX="<<_slackX<<endl<<flush;
    DEBUGSTR<<"_slackY="<<_slackY<<endl<<flush;
    DEBUGSTR<<"_dirty="<<_dirty<<endl<<flush;
    DEBUGSTR<<"_valid="<<_valid<<endl<<flush;
    DEBUGSTR<<endl<<flush;
}



bool operator== ( const FlowGridManager & csg1, const FlowGridManager & csg2 )
{
    return csg1.gridDim()==csg2.gridDim() &&
    csg1.origin()==csg2.origin() &&
    csg1.gridSpacing()==csg2.gridSpacing() &&
    csg1.frameSize()==csg2.frameSize();
}




