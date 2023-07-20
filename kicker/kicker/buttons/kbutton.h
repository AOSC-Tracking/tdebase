/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.

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

#ifndef __kbutton_h__
#define __kbutton_h__

#include "panelbutton.h"

/**
 * Button that contains the PanelKMenu and client menu manager.
 */
class KButton : public PanelPopupButton
{
    TQ_OBJECT

public:
    KButton( TQWidget *parent );
    ~KButton();

    void loadConfig( const TDEConfigGroup& config );

    virtual void properties();

    /**
     * Reimplement this to give Kicker a hint for the width of the button
     * given a certain height.
     */
    virtual int widthForHeight(int height) const;

    /**
     * Reimplement this to give Kicker a hint for the height of the button
     * given a certain width.
     */
    virtual int heightForWidth(int width) const;

protected:
    virtual TQString tileName() { return "KMenu"; }
    virtual void initPopup();
    virtual TQString defaultIcon() const { return "go"; }
    virtual void drawButton(TQPainter *);
};

#endif
