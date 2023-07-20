/*
 * Copyright (C) 2000 by Ian Reinhart Geiser <geiseri@kde.org>
 *
 * Licensed under the Artistic License.
 */

#ifndef __KDCOPLISTVIEW_H__
#define __KDCOPLISTVIEW_H__


#include <tdelistview.h>
class TQDragObject;

class KDCOPListView : public TDEListView
{
  TQ_OBJECT

  public:
  	KDCOPListView ( TQWidget * parent = 0, const char * name = 0 );
	virtual ~KDCOPListView();
  	TQDragObject *dragObject();
	void setMode(const TQString &mode);
	TQString getCurrentCode() const;

  private:
  	TQString encode(TQListViewItem *code);
	TQString mode;

};

class DCOPBrowserItem : public TQListViewItem
{
  public:

    enum Type { Application, Interface, Function };

    DCOPBrowserItem(TQListView * parent, Type type);
    DCOPBrowserItem(TQListViewItem * parent, Type type);

    virtual ~DCOPBrowserItem() {}

    Type type() const;

  private:

    Type type_;
};

class DCOPBrowserApplicationItem : public TQObject, public DCOPBrowserItem
{
 TQ_OBJECT
  public:

    DCOPBrowserApplicationItem(TQListView * parent, const TQCString & app);
    virtual ~DCOPBrowserApplicationItem() {}

    TQCString app() const { return app_; }

    virtual void setOpen(bool o);

  protected:

    virtual void populate();

  private:

    TQCString app_;
  private slots:
  /**
   * Theses two slots are used to get the icon of the application
   */
    void retreiveIcon(int callId, const TQCString& replyType, const TQByteArray &replyData);
	void slotGotWindowName(int callId, const TQCString& replyType, const TQByteArray &replyData);
};

class DCOPBrowserInterfaceItem : public TQObject, public DCOPBrowserItem
{
  public:

    DCOPBrowserInterfaceItem
    (
     DCOPBrowserApplicationItem * parent,
     const TQCString & app,
     const TQCString & object,
     bool def
    );

    virtual ~DCOPBrowserInterfaceItem() {}

    TQCString app() const { return app_; }
    TQCString object() const { return object_; }

    virtual void setOpen(bool o);

  protected:

    virtual void populate();

  private:

    TQCString app_;
    TQCString object_;
};


class DCOPBrowserFunctionItem : public DCOPBrowserItem
{
  public:

    DCOPBrowserFunctionItem
    (
     DCOPBrowserInterfaceItem * parent,
     const TQCString & app,
     const TQCString & object,
     const TQCString & function
    );

    virtual ~DCOPBrowserFunctionItem() {}

    TQCString app() const { return app_; }
    TQCString object() const { return object_; }
    TQCString function() const { return function_; }

    virtual void setOpen(bool o);

  private:

    TQCString app_;
    TQCString object_;
    TQCString function_;
};

#endif
