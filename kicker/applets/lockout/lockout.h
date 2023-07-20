#ifndef LOCKOUT_H
#define LOCKOUT_H

#include <tqevent.h>
#include <tqstring.h>
#include <kpanelapplet.h>

#include "simplebutton.h"

class TQBoxLayout;
class TQToolButton;

class Lockout : public KPanelApplet
{
    TQ_OBJECT

public:
    Lockout( const TQString& configFile,
	     TQWidget *parent = 0, const char *name = 0 );
    ~Lockout();

    int widthForHeight(int height) const;
    int heightForWidth(int width) const;

protected:
    virtual void mousePressEvent( TQMouseEvent * );
    virtual void mouseMoveEvent( TQMouseEvent * );
    virtual void mouseReleaseEvent( TQMouseEvent * );
    virtual void mouseDoubleClickEvent( TQMouseEvent * );

    virtual bool eventFilter( TQObject *, TQEvent * );

private slots:
    void lock();
    void logout();

    void slotLockPrefs();
    void slotLogoutPrefs();
    void slotTransparent();
    void slotIconChanged();

private:
    void propagateMouseEvent( TQMouseEvent * );
    void checkLayout( int height ) const;

    SimpleButton *lockButton, *logoutButton;
    TQBoxLayout *layout;

    bool bTransparent;
};

#endif // LOCKOUT_H
