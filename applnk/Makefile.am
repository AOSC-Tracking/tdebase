SUBDIRS = compat

xdg_menu_DATA = tde-settings.menu tde-screensavers.menu

xdg_mergedmenu_DATA = tde-essential.menu
xdg_mergedmenudir = $(xdg_menudir)/applications-merged

xdg_directory_DATA = \
tde-main.directory \
tde-development.directory \
tde-editors.directory \
tde-edutainment.directory \
tde-games-arcade.directory \
tde-games-board.directory \
tde-games-card.directory \
tde-games-kids.directory \
tde-games-roguelikes.directory \
tde-games-strategy.directory \
tde-games.directory \
tde-graphics.directory \
tde-internet-terminal.directory \
tde-internet.directory \
tde-more.directory \
tde-multimedia.directory \
tde-office.directory \
tde-science.directory \
tde-settings-accessibility.directory \
tde-settings-components.directory \
tde-settings-desktop.directory \
tde-settings-information.directory \
tde-settings-looknfeel.directory \
tde-settings-network.directory \
tde-settings-peripherals.directory \
tde-settings-hardware.directory \
tde-settings-power.directory \
tde-settings-security.directory \
tde-settings-sound.directory \
tde-settings-system.directory \
tde-settings-webbrowsing.directory \
tde-settings.directory \
tde-settingsmenu.directory \
tde-system-screensavers.directory \
tde-system-terminal.directory \
tde-system.directory \
tde-toys.directory \
tde-unknown.directory \
tde-utilities-xutils.directory \
tde-utilities.directory \
tde-utilities-accessibility.directory \
tde-utilities-desktop.directory \
tde-utilities-peripherals.directory \
tde-utilities-file.directory \
tde-utilities-pim.directory \
tde-development-translation.directory \
tde-development-webdevelopment.directory \
kde-development-webdevelopment.directory \
kde-development.directory \
kde-editors.directory \
kde-edu-languages.directory \
kde-edu-mathematics.directory \
kde-edu-miscellaneous.directory \
kde-edu-science.directory \
kde-games-arcade.directory \
kde-games-board.directory \
kde-games-card.directory \
kde-games-kids.directory \
kde-games-strategy.directory \
kde-games.directory \
kde-graphics.directory \
kde-internet.directory \
kde-multimedia.directory \
kde-office.directory \
kde-settingsmenu.directory \
kde-system.directory \
kde-toys.directory \
kde-utilities-accessibility.directory \
kde-utilities-desktop.directory \
kde-utilities-pim.directory \
kde-utilities.directory \
tde-office-pim.directory

install-data-local:
	for file in `cat $(srcdir)/old_desktop_files` ; do \
	    if test -f $(DESTDIR)$(xdg_appsdir)/$${file} ; then \
	        rm $(DESTDIR)$(xdg_appsdir)/$${file} ; \
		echo Deleting $(DESTDIR)$(xdg_appsdir)/$${file} ; \
	    fi; \
	done;

	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)
	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/.hidden
	$(INSTALL_DATA) $(srcdir)/hidden.directory $(DESTDIR)$(kde_appsdir)/.hidden/.directory

uninstall-local:
	-rm -f $(DESTDIR)$(kde_appsdir)/.hidden/.directory
