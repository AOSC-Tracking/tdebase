#ifndef KASCLOCKITEM_H
#define KASCLOCKITEM_H

#include "kasitem.h"

/**
 * An item that displays a clock.
 */
class KDE_EXPORT KasClockItem : public KasItem
{
    TQ_OBJECT

public:
    KasClockItem( KasBar *parent );
    virtual ~KasClockItem();

    void paint( TQPainter *p );

public slots:
    void updateTime();

    void showMenuAt( TQMouseEvent *ev );
    void showMenuAt( TQPoint p );

protected:
    /** Reimplemented from KasItem to create a date picker. */
    virtual KasPopup *createPopup();

private:
    class LCD *lcd;
};

#endif // KASCLOCKITEM_H

