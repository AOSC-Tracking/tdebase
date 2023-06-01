#ifndef NOSLOTS
# define DEF( name, key3, key4, fnSlot ) \
   keys->insert( name, i18n(name), TQString(), key3, key4, TQT_TQOBJECT(this), TQT_SLOT(fnSlot) )
#else
# define DEF( name, key3, key4, fnSlot ) \
   keys->insert( name, i18n(name), TQString(), key3, key4, 0, 0 )
#endif

    keys->insert( "Program:kxkb", i18n("Keyboard") );
    DEF( I18N_NOOP("Switch to Next Keyboard Layout"), TDEShortcut(), TDEShortcut(), nextLayout() );
    DEF( I18N_NOOP("Switch to Previous Keyboard Layout"), TDEShortcut(), TDEShortcut(), prevLayout() );

#undef DEF