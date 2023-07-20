#ifndef _konq_sidebar_test_h_
#define _konq_sidebar_test_h_
#include <konqsidebarplugin.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tdeparts/part.h>
#include <tdeparts/factory.h>
#include <tdeparts/browserextension.h>
#include <kdialogbase.h>
#include <tqcombobox.h>
#include <tqstringlist.h>
#include <tdelocale.h>
#include <tqlineedit.h>
class KonqSidebarTree;
class TQVBox;

class KonqSidebar_Tree: public KonqSidebarPlugin
        {
                TQ_OBJECT
                public:
                KonqSidebar_Tree(TDEInstance *instance,TQObject *parent,TQWidget *widgetParent, TQString &desktopName_, const char* name=0);
                ~KonqSidebar_Tree();
                virtual void *provides(const TQString &);
//		void emitStatusBarText (const TQString &);
                virtual TQWidget *getWidget();
                protected:
                        class TQVBox *widget;
                        class KonqSidebarTree *tree;
                        virtual void handleURL(const KURL &url);
		protected slots:
			void copy();
			void cut();
			void paste();
			void trash();
			void del();
			void shred();
			void rename();
signals:
			void openURLRequest( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );
  			void createNewWindow( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );
			void popupMenu( const TQPoint &global, const KURL &url,
					const TQString &mimeType, mode_t mode = (mode_t)-1 );
			void popupMenu( const TQPoint &global, const KFileItemList &items );
			void enableAction( const char * name, bool enabled );
        };

#endif
