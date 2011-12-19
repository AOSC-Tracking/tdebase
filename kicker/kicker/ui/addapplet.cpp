/*****************************************************************

Copyright (c) 2005 Marc Cramdal
Copyright (c) 2005 Aaron Seigo <aseigo@kde.org>

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

#include <tqapplication.h>
#include <tqcombobox.h>
#include <tqdir.h>
#include <tqframe.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlineedit.h>
#include <tqpalette.h>
#include <tqscrollview.h>
#include <tqtimer.h>
#include <tqsizepolicy.h>

#include <kiconloader.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kpushbutton.h>
#include <kstandarddirs.h>
#include <kstdguiitem.h>

#include <paneldrag.h>

#include "addapplet.h"
#include "addappletvisualfeedback.h"
#include "appletwidget.h"
#include "appletview.h"
#include "container_applet.h"
#include "container_extension.h"
#include "containerarea.h"
#include "kicker.h"
#include "kickerSettings.h"
#include "menuinfo.h"
#include "pluginmanager.h"

AppletWidget::AppletWidget(const AppletInfo& info, bool odd, TQWidget *parent)
    : AppletItem(parent),
      m_appletInfo(info),
      m_odd(odd),
      m_selected(false)
{
    setFocusPolicy(TQ_StrongFocus);
    setSelected(m_selected);
    
    itemTitle->setText("<h3>" + info.name() + "</h3>");
    itemTitle->installEventFilter(this);

    if (info.comment() != info.name())
    {
        itemDescription->setText(info.comment());
    }

    itemDescription->installEventFilter(this);

    KIconLoader * ldr = KGlobal::iconLoader();
    TQPixmap icon = ldr->loadIcon(info.icon(), KIcon::Panel, KIcon::SizeLarge);
    itemPixmap->setPixmap(icon);
    itemPixmap->installEventFilter(this);
}

bool AppletWidget::eventFilter(TQObject*, TQEvent* e)
{
    if (e->type() == TQEvent::MouseButtonPress)
    {
        TQMouseEvent* me = TQT_TQMOUSEEVENT(e);
        if (me->button() & Qt::LeftButton)
        {
            m_dragStart = me->pos();
        }
    }
    else if (m_dragStart.isNull())
    {
        return false;
    }

    if (e->type() == TQEvent::MouseMove)
    {
        TQMouseEvent* me = TQT_TQMOUSEEVENT(e);
        if ((me->pos() - m_dragStart).manhattanLength() >
            KGlobalSettings::dndEventDelay())
        {
            AppletInfoDrag* drag = new AppletInfoDrag(m_appletInfo, this);

            if (itemPixmap->pixmap())
            {
                drag->setPixmap(*itemPixmap->pixmap());
            }

            drag->dragCopy();
            
            return true;
        }
    }
    else if (e->type() == TQEvent::MouseButtonRelease)
    {
        m_dragStart = TQPoint();
    }

    return false;
}

void AppletWidget::keyPressEvent(TQKeyEvent *e)
{
    if (e->key() == Qt::Key_Enter ||
        e->key() == Qt::Key_Return)
    {
        emit doubleClicked(this);
    }
    else if (e->key() == Qt::Key_Up)
    {
        TQKeyEvent fakedKeyPress(TQEvent::KeyPress, TQt::Key_BackTab, 0, 0);
        TQKeyEvent fakedKeyRelease(TQEvent::KeyRelease, Key_BackTab, 0, 0);
        TQApplication::sendEvent(this, &fakedKeyPress);
        TQApplication::sendEvent(this, &fakedKeyRelease);
    }
    else if (e->key() == Qt::Key_Down)
    {
        TQKeyEvent fakedKeyPress(TQEvent::KeyPress, Qt::Key_Tab, 0, 0);
        TQKeyEvent fakedKeyRelease(TQEvent::KeyRelease, Key_Escape, 0, 0);
        TQApplication::sendEvent(this, &fakedKeyPress);
        TQApplication::sendEvent(this, &fakedKeyRelease);
    }
    else
    {
        AppletItem::keyPressEvent(e);
    }
}

void AppletWidget::mousePressEvent(TQMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        emit clicked(this);
        m_dragStart = e->pos();
    }

    setFocus();
    TQWidget::mousePressEvent(e);
}

void AppletWidget::mouseMoveEvent(TQMouseEvent *e)
{
    if (e->button() == Qt::LeftButton &&
        !m_dragStart.isNull() &&
        (e->pos() - m_dragStart).manhattanLength() >
         KGlobalSettings::dndEventDelay())
    {
        AppletInfoDrag* drag = new AppletInfoDrag(m_appletInfo, this);

        if (itemPixmap->pixmap())
        {
            drag->setPixmap(*itemPixmap->pixmap());
        }

        drag->dragCopy();
    }
}

void AppletWidget::mouseReleaseEvent(TQMouseEvent *e)
{
    m_dragStart = TQPoint();
    TQWidget::mouseReleaseEvent(e);
}

void AppletWidget::mouseDoubleClickEvent(TQMouseEvent *e)
{
    if (!e->button() == Qt::LeftButton)
    {
        AppletItem::mouseDoubleClickEvent(e);
        return;
    }

    emit doubleClicked(this);
}

void AppletWidget::setSelected(bool selected)
{
    m_selected = selected;

    // for now just used to switch colours around =)
    if (m_selected)
    {
        setPaletteBackgroundColor(KGlobalSettings::highlightColor());
        setPaletteForegroundColor(KGlobalSettings::highlightedTextColor());
    }
    else if (m_odd)
    {
        setPaletteBackgroundColor(KGlobalSettings::baseColor());
        setPaletteForegroundColor(KGlobalSettings::textColor());
    }
    else
    {
        setPaletteBackgroundColor(KGlobalSettings::alternateBackgroundColor());
        setPaletteForegroundColor(KGlobalSettings::textColor());
    }
}

void AppletWidget::setOdd(bool odd)
{
    m_odd = odd;
    setSelected(m_selected);
}

void AppletWidget::focusInEvent(TQFocusEvent*)
{
    emit clicked(this);
}

AddAppletDialog::AddAppletDialog(ContainerArea* cArea,
                                 TQWidget* parent,
                                 const char* name)
    : KDialogBase(parent, name, false, i18n("Add Applet"), 0),
      m_selectedApplet(0),
      m_containerArea(cArea),
      m_insertionPoint(Kicker::the()->insertionPoint()),
      m_closing(false),
      m_searchDelay(new TQTimer(this))
{
    m_mainWidget = new AppletView(this, "AddAppletDialog::m_mainWidget");
    m_mainWidget->appletScrollView->setResizePolicy(TQScrollView::Manual);
    m_mainWidget->appletScrollView->setHScrollBarMode(TQScrollView::AlwaysOff);
    m_mainWidget->appletScrollView->viewport()->setPaletteBackgroundColor(KGlobalSettings::baseColor());

    setMainWidget(m_mainWidget);

    resize(configDialogSize("AddAppletDialog Settings"));
    centerOnScreen(this);

    KGuiItem addGuiItem = KStdGuiItem::add();
    addGuiItem.setText(m_mainWidget->appletInstall->text());
    m_mainWidget->appletInstall->setEnabled(false);
    m_mainWidget->appletInstall->setGuiItem(addGuiItem);
    m_mainWidget->closeButton->setGuiItem(KStdGuiItem::close());

    connect(m_mainWidget->appletSearch, TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(delayedSearch()));
    connect(m_searchDelay, TQT_SIGNAL(timeout()), this, TQT_SLOT(search()));
    connect(m_mainWidget->appletFilter, TQT_SIGNAL(activated(int)), this, TQT_SLOT(filter(int)));
    connect(m_mainWidget->appletInstall, TQT_SIGNAL(clicked()), this, TQT_SLOT(addCurrentApplet()));
    connect(m_mainWidget->closeButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(close()));

    m_selectedType = AppletInfo::Undefined;
    m_appletBox = 0;

    TQTimer::singleShot(0, this, TQT_SLOT(populateApplets()));
}

void AddAppletDialog::updateInsertionPoint()
{
    m_insertionPoint = Kicker::the()->insertionPoint();
}


void AddAppletDialog::closeEvent(TQCloseEvent* e)
{
    m_closing = true;
    saveDialogSize("AddAppletDialog Settings");
    KDialogBase::closeEvent(e);
}

void AddAppletDialog::resizeAppletView()
{
    int w, h;
    TQScrollView *v = m_mainWidget->appletScrollView;
    
    if (m_closing)
        return;
    
    for (int i = 0; i < 3; i++)
    {
        m_appletBox->layout()->activate();
        w = v->visibleWidth();
        h = m_appletBox->tqlayout()->minimumSize().height();
        v->resizeContents(w, QMAX(h, v->visibleHeight()));
        if (w == m_appletBox->width() && h == m_appletBox->height())
        break;
        m_appletBox->resize(w, h);
        v->updateScrollBars();
    }
}

bool AddAppletDialog::eventFilter(TQObject *o, TQEvent *e)
{
    if (e->type() == TQEvent::Resize)
        TQTimer::singleShot(0, this, TQT_SLOT(resizeAppletView()));
    
    return TQT_TQOBJECT(this)->TQObject::eventFilter(o, e);
}

void AddAppletDialog::populateApplets()
{
    m_appletBox = new TQWidget(m_mainWidget->appletScrollView->viewport());
    m_appletBox->setPaletteBackgroundColor(KGlobalSettings::baseColor());
    m_mainWidget->appletScrollView->addChild(m_appletBox, 0, 0);
    m_appletBox->show();
    TQVBoxLayout* layout = new TQVBoxLayout(m_appletBox);
    layout->setMargin(0);
    
    m_mainWidget->appletScrollView->installEventFilter(this);

    /* Three steps
     * - First we load the applets
     * - We load the special buttons
     * - Then we begin to populate the scrollview with the AppletWidget(s)
     */
    AppletInfo::List appletInfoList;

    // Loading applets
    appletInfoList = PluginManager::applets(false, &appletInfoList);

    // Loading built in buttons
    appletInfoList = PluginManager::builtinButtons(false, &appletInfoList);

    // Loading special buttons
    appletInfoList = PluginManager::specialButtons(false, &appletInfoList);

    qHeapSort(appletInfoList);

    int i = 0;
    bool odd = true;
    TQWidget* prevTabWidget = m_mainWidget->appletFilter;

    for (AppletInfo::List::iterator it = appletInfoList.begin();
         !m_closing && it != appletInfoList.end();
         ++i)
    {
        if ((*it).isHidden() || (*it).name().isEmpty() ||
            ((*it).isUniqueApplet() &&
             PluginManager::the()->hasInstance(*it)))
        {
            it = appletInfoList.erase(it);
            --i;
            continue;
        }

        AppletWidget *itemWidget = new AppletWidget(*it, odd, m_appletBox);

        if (m_mainWidget->appletSearch->text().isEmpty() ||
            appletMatchesSearch(itemWidget, m_mainWidget->appletSearch->text()))
        {
            itemWidget->show();
            odd = !odd;
        }
        else
        {
            itemWidget->hide();
        }

        layout->insertWidget(i, itemWidget);
        m_appletWidgetList.append(itemWidget);
        setTabOrder(prevTabWidget, itemWidget);
        prevTabWidget = itemWidget;

        connect(itemWidget, TQT_SIGNAL(clicked(AppletWidget*)),
                this, TQT_SLOT(selectApplet(AppletWidget*)));
        connect(itemWidget, TQT_SIGNAL(doubleClicked(AppletWidget*)),
                this, TQT_SLOT(addApplet(AppletWidget*)));

        if (m_closing)
        {
            return;
        }

        ++it;
    }
    
    resizeAppletView();
    
    m_mainWidget->closeButton->setEnabled(true);
}

void AddAppletDialog::selectApplet(AppletWidget *applet)
{
    m_mainWidget->appletInstall->setEnabled(true);

    if (m_selectedApplet)
    {
        m_selectedApplet->setSelected(false);
    }

    m_selectedApplet = applet;

    if (m_selectedApplet)
    {
        m_selectedApplet->setSelected(true);
    }
}

void AddAppletDialog::addCurrentApplet()
{
    addApplet(m_selectedApplet);
}

void AddAppletDialog::addApplet(AppletWidget* applet)
{
    if (!applet)
    {
        return;
    }

    TQPoint prevInsertionPoint = Kicker::the()->insertionPoint();
    Kicker::the()->setInsertionPoint(m_insertionPoint);

    const TQWidget* appletContainer = 0;

    if (applet->info().type() == AppletInfo::Applet)
    {
        appletContainer = m_containerArea->addApplet(applet->info());

        if (applet->info().isUniqueApplet() &&
            PluginManager::the()->hasInstance(applet->info()))
        {
            applet->hide();

            // reset the odd/even colouring from this item on down in the list
            bool odd = applet->odd();
            AppletWidget::List::const_iterator it = m_appletWidgetList.find(applet);
            for (; it != m_appletWidgetList.constEnd(); ++it)
            {
                if ((*it)->isHidden())
                {
                    continue;
                }

                (*it)->setOdd(odd);
                odd = !odd;
            }
        }
    }
    else if (applet->info().type() & AppletInfo::Button)
    {
        appletContainer = m_containerArea->addButton(applet->info());
    }

    if (appletContainer)
    {
        ExtensionContainer* ec =
           dynamic_cast<ExtensionContainer*>(m_containerArea->topLevelWidget());

        if (ec)
        {
            // unhide the panel and keep it unhidden for at least the time the
            // helper tip will be there
            ec->unhideIfHidden(KickerSettings::mouseOversSpeed() + 2500);
        }

        new AddAppletVisualFeedback(applet, appletContainer,
                                    m_containerArea->popupDirection());
    }

    Kicker::the()->setInsertionPoint(prevInsertionPoint);
}

bool AddAppletDialog::appletMatchesSearch(const AppletWidget* w,
                                          const TQString& s)
{
    if (w->info().type() == AppletInfo::Applet &&
        w->info().isUniqueApplet() &&
        PluginManager::the()->hasInstance(w->info()))
    {
        return false;
    }

    return (m_selectedType == AppletInfo::Undefined ||
            w->info().type() & m_selectedType) &&
           (w->info().name().contains(s, false) ||
            w->info().comment().contains(s, false));
}

void AddAppletDialog::delayedSearch()
{
    if (!m_searchDelay->isActive())
    {
        m_searchDelay->start(300, true);
    }
}

void AddAppletDialog::search()
{
    TQString s = m_mainWidget->appletSearch->text();
    bool odd = true;
    AppletWidget::List::const_iterator it = m_appletWidgetList.constBegin();
    AppletWidget::List::const_iterator itEnd = m_appletWidgetList.constEnd();

    for (; it != itEnd; ++it)
    {
        AppletWidget* w = *it;
        if (appletMatchesSearch(w, s))
        {
            w->setOdd(odd);
            w->show();
            odd = !odd;
        }
        else
        {
            w->hide();
        }
    }
    
    TQTimer::singleShot(0, this, TQT_SLOT(resizeAppletView()));
}

void AddAppletDialog::filter(int i)
{
    m_selectedType = AppletInfo::Undefined;

    if (i == 1)
    {
        m_selectedType = AppletInfo::Applet;
    }
    else if (i == 2)
    {
        m_selectedType = AppletInfo::Button;
    }

    search();
}

#include "addapplet.moc"
#include "appletwidget.moc"

