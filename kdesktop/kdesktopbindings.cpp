#ifndef NOSLOTS
# define DEF( name, key3, key4, fnSlot ) \
   keys->insert( name, i18n(name), TQString(), key3, key4, this, TQ_SLOT(fnSlot) )
# define DEF2( name, key3, key4, receiver, slot ) \
   keys->insert( name, i18n(name), TQString(), key3, key4, receiver, slot );
#else
# define DEF( name, key3, key4, fnSlot ) \
   keys->insert( name, i18n(name), TQString(), key3, key4 )
# define DEF2( name, key3, key4, receiver, slot ) \
   keys->insert( name, i18n(name), TQString(), key3, key4 )
#endif
#define WIN KKey::QtWIN

	keys->insert( "Program:kdesktop", i18n("Desktop") );

#ifndef NOSLOTS
	if (tdeApp->authorize("run_command"))
	{
#endif
		DEF( I18N_NOOP("Run Command"), ALT+TQt::Key_F2, WIN+TQt::Key_Return, slotExecuteCommand() );
#ifndef NOSLOTS
	}
#endif
	DEF( I18N_NOOP("Show Taskmanager"), CTRL+TQt::Key_Escape, WIN+CTRL+TQt::Key_Pause, slotShowTaskManager() );
	DEF( I18N_NOOP("Show Window List"), ALT+TQt::Key_F5, WIN+TQt::Key_0, slotShowWindowList() );
	DEF( I18N_NOOP("Switch User"), ALT+CTRL+TQt::Key_Insert, WIN+TQt::Key_Insert, slotSwitchUser() );
#ifndef NOSLOTS
	if (tdeApp->authorize("lock_screen"))
	{
#endif
		DEF2( I18N_NOOP("Lock Session"), ALT+CTRL+TQt::Key_L, WIN+TQt::Key_ScrollLock, KRootWm::self(), TQ_SLOT(slotLock()) );
		DEF2( I18N_NOOP("Lock Session (Hotkey)"), TDEShortcut(TQString("XF86ScreenSaver")), TDEShortcut(TQString("XF86ScreenSaver")), KRootWm::self(), TQ_SLOT(slotLock()) );
#ifndef NOSLOTS
	}
	if (tdeApp->authorize("start_screensaver"))
	{
#endif
		DEF2( I18N_NOOP("Start Screen Saver"), ALT+CTRL+TQt::Key_S, WIN+TQt::Key_S, KRootWm::self(), TQ_SLOT(slotSave()) );
#ifndef NOSLOTS
	}
	if (tdeApp->authorize("logout"))
	{
#endif
		DEF( I18N_NOOP("Log Out"), ALT+CTRL+TQt::Key_Delete, WIN+TQt::Key_Escape, slotLogout() );
		DEF( I18N_NOOP("Log Out Without Confirmation"), ALT+CTRL+SHIFT+TQt::Key_Delete, WIN+SHIFT+TQt::Key_Escape, slotLogoutNoCnf() );
		DEF( I18N_NOOP("Halt without Confirmation"), ALT+CTRL+SHIFT+TQt::Key_PageDown, WIN+CTRL+SHIFT+TQt::Key_PageDown, slotHaltNoCnf() );
		DEF( I18N_NOOP("Reboot without Confirmation"), ALT+CTRL+SHIFT+TQt::Key_PageUp, WIN+CTRL+SHIFT+TQt::Key_PageUp, slotRebootNoCnf() );
#ifndef NOSLOTS
	}
#endif

    // Only add these options if supported by ksmserver
    DCOPRef ksmref("ksmserver", "ksmserver");
    DCOPReply reply = ksmref.call("suspendOptions");

    TQStringList suspendOptions;
    if (reply.isValid()) {
        reply.get(suspendOptions);
    }

    if (suspendOptions.contains("freeze"))
        DEF( I18N_NOOP("Freeze"), TDEShortcut(), TDEShortcut(), slotFreeze() );

    if (suspendOptions.contains("suspend"))
        DEF( I18N_NOOP("Suspend"), TDEShortcut(TQString("XF86Sleep")), TDEShortcut(TQString("XF86Sleep")), slotSuspend() );

    if (suspendOptions.contains("hibernate"))
        DEF( I18N_NOOP("Hibernate"), TDEShortcut(), TDEShortcut(), slotHibernate() );

    if (suspendOptions.contains("hybridSuspend"))
        DEF( I18N_NOOP("Hybrid Suspend"), TDEShortcut(), TDEShortcut(), slotHybridSuspend() );

#undef DEF
#undef DEF2
#undef WIN
