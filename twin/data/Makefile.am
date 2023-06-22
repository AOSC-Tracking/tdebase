tdeconf_PROGRAMS = twin_update_window_settings twin_update_default_rules
tdeconfdir = $(libdir)/tdeconf_update_bin

twin_update_window_settings_SOURCES = update_window_settings.cpp
twin_update_window_settings_LDADD   = $(LIB_TDECORE) $(KDE_RPATH)
twin_update_window_settings_LDFLAGS = $(all_libraries)

twin_update_default_rules_SOURCES = update_default_rules.cpp
twin_update_default_rules_LDADD   = $(LIB_TDECORE) $(KDE_RPATH)
twin_update_default_rules_LDFLAGS = $(all_libraries)

INCLUDES = $(all_includes)

METASOURCES = AUTO

sounddir= $(kde_sounddir)
sound_DATA = pop.wav

update_DATA = twin.upd twinsticky.upd twiniconify.upd twin_focus1.upd \
        twinupdatewindowsettings.upd twin_focus2.upd twin_fsp_workarounds_1.upd
update_SCRIPTS = pluginlibFix.pl twin_focus1.sh twin_focus2.sh
updatedir = $(kde_datadir)/tdeconf_update

twin_default_rules_DATA = fsp_workarounds_1.twinrules
twin_default_rulesdir = $(kde_datadir)/twin/default_rules
