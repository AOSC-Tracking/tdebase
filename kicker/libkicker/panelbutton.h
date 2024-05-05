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

#ifndef __panelbutton_h__
#define __panelbutton_h__

#include <algorithm>

#include <tqbutton.h>

#include <kpanelapplet.h>
#include <kpanelextension.h>
#include <kurldrag.h>

#include "kickertip.h"

class TQPopupMenu;
class TDEConfigGroup;
class KShadowEngine;

/**
 * PanelButton is the base class for all buttons to be
 * placed in Kicker's panels. It inherits TQButton, and
 * KickerTip::Client.
 */
class TDE_EXPORT PanelButton: public TQButton, public KickerTip::Client
{
    TQ_OBJECT

public:
    /**
     * Create a panel button
     * @param parent the parent widget
     * @param name the widget's name
     */
    PanelButton( TQWidget* parent, const char* name, bool forceStandardCursor = FALSE );

    /**
     * Configures this button according to the user's preferences for
     * button tiles/colors/etc. This must be called by the container
     * embedding the button after consturction and thereafter whenever
     * the configuration changes to ensure it remains properly styled.
     * Note that it is not used for configuration specific to the subclass.
     */
    void configure();

    /**
     * Prompts the button to save it's configuration. Subclass specific
     * settings should be saved in this method to the TDEConfigGroup passed in.
     */
    virtual void saveConfig(TDEConfigGroup&) const {}

    /**
     * Reimplement this to display a properties dialog for your button.
     */
    virtual void properties() {}

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

    /**
     * @return the button's current icon
     */
    virtual const TQPixmap& labelIcon() const;

    /**
     * @return the button's zoom icon
     */
    virtual const TQPixmap& zoomIcon() const;

     /**
     * @return true if this button is valid.
     */
    bool isValid() const;

    /**
     * Changes the title for the panel button.
     * @param t the button's title
     */
    void setTitle(const TQString& t);

    /**
     * @return the title of the button.
     */
    TQString title() const;

    /**
     * Changes the name of the panel button's tile, with
     * optional color.
     * @param tile the button's tile name
     * @param color the button's tile color
     */
    void setTile(const TQString& tile, const TQColor& color = TQColor());

    /**
     * Set to true to draw an arrow on the button.
     */
    void setDrawArrow(bool drawArrow);

    /**
     * Used to set the icon for this panel button.
     * @param icon the path to the button's icon
     */
    void setIcon(const TQString& icon);

    /**
     * @return the button's icon
     */
    TQString icon() const;

    /**
     * @return whether this button has a text label or not
     */
    bool hasText() const;

    /**
     * Change the button's text label
     * @param text text for button's label
     */
    void setButtonText(const TQString& text);

    /**
     * @return button's text label
     */
    TQString buttonText() const;

    /**
     * Change the button's text label color
     * @param c the new text label color
     */
    void setTextColor(const TQColor& c);

    /**
     * @return the button's text label color
     */
    TQColor textColor() const;

    /**
     * Change the button's text scale
     * @param p font scale (in percent)
     */
    void setFontPercent(double p);

    /**
     * @return the button's text scale (in percent)
     */
    double fontPercent() const;

    /**
     * @return the orientation of the button
     */
    Orientation orientation() const;

    /**
     * @return the button's popup direction (read from parent KPanelApplet)
     */
    KPanelApplet::Direction popupDirection() const;

    /**
     * @return global position of the center of the button
     */
    TQPoint center() const;

    /**
     * Used to load the graphical tile of the button
     * @param name path/name of button's tile
     * @param size size of the tile
     * @param state used if button has multiple states (null by default)
     */
    static TQImage loadTile(const TQString& name, const TQSize&,
                           const TQString& state = TQString::null);

    /**
     * Update the contents of the button's KickerTip
     * @param data new KickerTip data
     */
    void updateKickerTip(KickerTip::Data& data);

    /**
     * @return true if the button should be centered in its parent container, false if not
     */
    bool centerButtonInContainer();

signals:
    /**
     * Emitted when the button's icon is changed.
     */
    void iconChanged();

    /**
     * Emitted to notify parent containers to save config
     */
    void requestSave();

    /**
     * Emitted when the button needs to be removed from it's container
     * @see KickerSettings::removeButtonsWhenBroken()
     */
    void removeme();

    /**
     * Emitted when the button may need to be removed, but that removal depends
     * on as-yet-uncertain future events and therefore ought to be hidden from
     * view, though not deleted quite yet.
     * @see KickerSettings::removeButtonsWhenBroken()
     */
    void hideme(bool hide);

    /**
     * Emitted when button initiates a drag
     */
    void dragme(const TQPixmap);

    /**
     * Overloads dragme to support panel button's with a list of KURL's ([url/servicemenu/browser]button)
     */
    void dragme(const KURL::List, const TQPixmap);

public slots:
    /**
     * Set to true to enable the button.
     */
    void setEnabled(bool enable);

    /**
     * Sets the orientation of the button (ie. which direction the icon will rotate).
     */
    void setOrientation(Orientation o);

    /**
     * Sets the direction to pop up the contents of the button.
     */
    virtual void setPopupDirection(KPanelApplet::Direction d);

protected:

    void setIconAlignment(TQt::AlignmentFlags align);
    /**
     * Subclasses must implement this to define the name of the button which is
     * used to identify this button for saving and loading. It must be unique
     * to the subclass, should not be i18n'd and is never made user visible.
     * KDE4: remove this and use the classname directly instead.
     */
    virtual TQString tileName() = 0;

    /**
     * @return the default icon for the button
     */
    virtual TQString defaultIcon() const { return "unknown"; };

    /**
     * Called right before drag occurs.
     */
    virtual void triggerDrag();

    /**
     * Emits a signal to drag the button. Reimplement this if, for example,
     * if you need the button to call dragme(KURL::List, const TQPixmap)
     * instead of dragme(const TQPixmap)
     */
    virtual void startDrag();

    virtual void enterEvent(TQEvent *);
    virtual void leaveEvent(TQEvent *);
    virtual void dragEnterEvent(TQDragEnterEvent *);
    virtual void dragLeaveEvent(TQDragLeaveEvent *);
    virtual void dropEvent(TQDropEvent *);
    virtual void mouseMoveEvent(TQMouseEvent *);
    virtual void mousePressEvent(TQMouseEvent *);
    virtual void mouseReleaseEvent(TQMouseEvent *);
    virtual void resizeEvent(TQResizeEvent*);
    virtual void drawButton(TQPainter *);
    virtual void drawDeepButton(TQPainter *);
    virtual void drawButtonLabel(TQPainter *);
    virtual void drawButtonLabel(TQPainter *, int voffset, bool drawArrow);

    /**
     * @return the preferred icon size.
     */
    virtual int preferredIconSize(int proposed_size = -1) const;

    /**
     * @return the preferred dimensions for the button
     */
    virtual int preferredDimension(int panelDim) const;

    /**
     * if the button represents a local file, it tells PanelButton
     * what file that is and it starts to watch it. if the file is
     * deleted, it is disabled and then checked for one second later
     * to see if has returned (e.g. a reinstall occurred) by calling
     * checkForBackingFile(). if that returns false, then the button
     * is removed from kicker.
     * TODO: implement a heuristic that checks back in intervals for
     * the reappearance of the file and returns the button to the panel
     */
    virtual bool checkForBackingFile();

    /**
     * Set the file backing this button (See @ref checkForBackingFile()),
     * you shouldn't need to use this, currently it's only used in [url/service]button
     */
    void backedByFile(const TQString& localFilePath);

    /**
     * Sets the button's arrow direction.
     * @param dir the arrow direction
     */
    void setArrowDirection(KPanelExtension::Position dir);

    /**
     * Loads the tiles for the button
     */
    void loadTiles();

    /**
     * Loads the icons for the button
     */
    void loadIcons();

    /**
     * (Re)Calculate icon sizes and return true if they have changed.
     */
    bool calculateIconSize();

    /**
     * @param center true if the button should be centered in its parent container, false if not
     */
    void setCenterButtonInContainer(bool center);

    bool m_valid;
    TQPixmap m_icon;

protected slots:
    /**
     * Called from TDEApplication when global icon settings have changed.
     * @param group the new group
     */
    void updateIcon(int group);

    /**
     * Called from TDEApplication when global settings have changed.
     * @param category the settings category, see TDEApplication::SettingsCategory
     */
    void updateSettings(int category);

    /**
     * Used for backedByFile, to check if the file backing this button
     * has been deleted.
     * @param path path to backing file
     */
    void checkForDeletion(const TQString& path);

    /**
     * Called to prepare the button for removal from the Kicker
     */
    void scheduleForRemoval();

private:
    TQPoint m_lastLeftMouseButtonPress;
    bool m_isLeftMouseButtonDown;
    bool m_drawArrow;
    bool m_highlight;
    bool m_changeCursorOverItem;
    bool m_hasAcceptedDrag;
    bool m_centerInContainer;
    TQColor m_textColor;
    TQColor m_tileColor;
    TQString m_buttonText;
    TQString m_tile;
    TQString m_title;
    TQString m_iconName;
    TQString m_backingFile;
    TQPixmap m_up;
    TQPixmap m_down;
    TQPixmap m_iconh; // hover
    TQPixmap m_iconz; // mouse over
    KPanelExtension::Position m_arrowDirection;
    KPanelApplet::Direction m_popupDirection;
    TQt::AlignmentFlags m_iconAlignment;
    Orientation m_orientation;
    int m_size;
    double m_fontPercent;
    bool m_forceStandardCursor;
    static KShadowEngine* s_textShadowEngine;

    class PanelPopupPrivate;
    PanelPopupPrivate* d;
};

/**
 * Base class for panelbuttons which popup a menu
 */
class TDE_EXPORT PanelPopupButton : public PanelButton
{
    TQ_OBJECT

public:
   /**
    * Create a panel button that pops up a menu.
    * @param parent the parent widget
    * @param name the widget's name
    */
    PanelPopupButton(TQWidget *parent=0, const char *name=0, bool forceStandardCursor = FALSE);

    /**
     * Sets the button's popup menu.
     * @param popup the menu to pop up
     */
    void setPopup(TQWidget *popup);

    /**
     * @return the button's popup menu
     */
    TQWidget *popup() const;

    bool eventFilter(TQObject *, TQEvent *);
    virtual void showMenu();

protected:
    /**
     * Called each time the button is clicked and the popup
     * is displayed. Reimplement for dynamic popup menus.
     */
    virtual void initPopup() {};

    /**
     * Called before drag occurs. Reimplement to do any
     * necessary setup before the button is dragged.
     */
    virtual void triggerDrag();

    /**
     * Marks the menu as initialized.
     */
    void setInitialized(bool initialized);

protected slots:
    /**
     * Connected to the button's pressed() signal, this is
     * the code that actually displays the menu. Reimplement if
     * you need to take care of any tasks before the popup is
     * displayed (eg. KickerTip)
     */
    virtual void slotExecMenu();

private slots:
    void menuAboutToHide();

protected:
    TQWidget *m_popup;
    bool m_pressedDuringPopup;
    bool m_initialized;

    class PanelPopupButtonPrivate;
    PanelPopupButtonPrivate* d;
};

#endif // __panelbutton_h__
