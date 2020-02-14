SUBDIRS = konsole other doc fonts terminalhere

xdg_apps_DATA = konsole.desktop konsolesu.desktop

kde_services_DATA = konsolepart.desktop kwrited.desktop konsole-script.desktop
kde_servicetypes_DATA = terminalemulator.desktop

update_DATA = konsole.upd
update_SCRIPTS = schemaStrip.pl
updatedir = $(kde_datadir)/tdeconf_update

servicesdir = $(kde_servicesdir)/kded
services_DATA = kwrited.desktop

install-data-local:
	@-rm -f $(DESTDIR)$(datadir)/autostart/kwrited.desktop

terminalhere_DATA = terminalhere.desktop
terminalheredir = $(kde_datadir)/konqueror/servicemenus

misc_DATA = eventsrc tips
miscdir = $(kde_datadir)/konsole

EXTRA_DIST = CONTRIBUTORS LICENSE.readme README.bugs \
	README.linux.console tests $(xdg_apps_DATA)
