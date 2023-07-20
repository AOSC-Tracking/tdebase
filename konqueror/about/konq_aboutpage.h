#ifndef __konq_aboutpage_h__
#define __konq_aboutpage_h__

#include <tdeparts/factory.h>
#include <tdehtml_part.h>

class TDEHTMLPart;
class TDEInstance;

class KonqAboutPageFactory : public KParts::Factory
{
public:
    KonqAboutPageFactory( TQObject *parent = 0, const char *name = 0 );
    virtual ~KonqAboutPageFactory();

    virtual KParts::Part *createPartObject( TQWidget *parentWidget, const char *widgetName,
                                            TQObject *parent, const char *name,
                                            const char *classname, const TQStringList &args );

    static TDEInstance *instance() { return s_instance; }

    static TQString launch();
    static TQString intro();
    static TQString specs();
    static TQString tips();
    static TQString plugins();

private:
    static TQString loadFile( const TQString& file );

    static TDEInstance *s_instance;
    static TQString *s_launch_html, *s_intro_html, *s_specs_html, *s_tips_html, *s_plugins_html;
};

class KonqAboutPage : public TDEHTMLPart
{
    TQ_OBJECT
public:
    KonqAboutPage( /*KonqMainWindow *mainWindow,*/
                   TQWidget *parentWidget, const char *widgetName,
                   TQObject *parent, const char *name );
    ~KonqAboutPage();

    virtual bool openURL( const KURL &url );

    virtual bool openFile();

    virtual void saveState( TQDataStream &stream );
    virtual void restoreState( TQDataStream &stream );

protected:
    virtual void urlSelected( const TQString &url, int button, int state, const TQString &target, KParts::URLArgs args = KParts::URLArgs() );

private:
    void serve( const TQString&, const TQString& );

    TDEHTMLPart *m_doc;
    //KonqMainWindow *m_mainWindow;
    TQString m_htmlDoc;
    TQString m_what;
};

#endif
