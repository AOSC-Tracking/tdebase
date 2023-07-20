#ifndef __konq_browseriface_h__
#define __konq_browseriface_h__

#include <tdeparts/browserinterface.h>

class KonqView;

class KonqBrowserInterface : public KParts::BrowserInterface
{
    TQ_OBJECT
    TQ_PROPERTY( uint historyLength READ historyLength )
public:
    KonqBrowserInterface( KonqView *view, const char *name );

    uint historyLength() const;

public slots:
    void goHistory( int );

private:
    KonqView *m_view;
};

#endif
