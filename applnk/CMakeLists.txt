#################################################
#
#  (C) 2010-2011 Serghei Amelian
#  serghei (DOT) amelian (AT) gmail.com
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

add_subdirectory( compat )

install( FILES
    tde-settings.menu tde-screensavers.menu
  DESTINATION ${XDG_MENU_INSTALL_DIR} )

install( FILES
    tde-essential.menu
  DESTINATION ${XDG_MENU_INSTALL_DIR}/applications-merged )

install( FILES
    hidden.directory
  DESTINATION ${APPS_INSTALL_DIR}/.hidden
  RENAME .directory )

install( FILES
    tde-main.directory tde-development.directory tde-editors.directory
    tde-edutainment.directory tde-games-arcade.directory
    tde-games-board.directory tde-games-card.directory
    tde-games-kids.directory tde-games-roguelikes.directory
    tde-games-strategy.directory tde-games.directory
    tde-graphics.directory tde-settings-information.directory
    tde-internet-terminal.directory tde-internet.directory
    tde-more.directory tde-multimedia.directory tde-office.directory
    tde-science.directory tde-settings-accessibility.directory
    tde-settings-components.directory tde-settings-desktop.directory
    tde-settings-looknfeel.directory tde-settings-network.directory
    tde-settings-peripherals.directory tde-settings-hardware.directory
    tde-settings-power.directory tde-settings-security.directory
    tde-settings-sound.directory tde-settings-system.directory
    tde-settings-webbrowsing.directory tde-settings.directory
    tde-settingsmenu.directory tde-system-screensavers.directory
    tde-system-terminal.directory tde-system.directory
    tde-toys.directory tde-unknown.directory
    tde-utilities-xutils.directory tde-utilities.directory
    tde-utilities-accessibility.directory tde-utilities-desktop.directory
    tde-utilities-peripherals.directory tde-utilities-file.directory
    tde-utilities-pim.directory tde-development-translation.directory
    tde-development-webdevelopment.directory
    kde-development-webdevelopment.directory kde-development.directory
    kde-editors.directory kde-edu-languages.directory kde-edu-mathematics.directory
    kde-edu-miscellaneous.directory kde-edu-science.directory kde-games-arcade.directory
    kde-games-board.directory kde-games-card.directory kde-games-kids.directory
    kde-games-strategy.directory kde-games.directory kde-graphics.directory
    kde-internet.directory kde-multimedia.directory kde-office.directory
    kde-settingsmenu.directory kde-system.directory kde-toys.directory
    kde-utilities-accessibility.directory kde-utilities-desktop.directory
    kde-utilities-pim.directory kde-utilities.directory tde-office-pim.directory
  DESTINATION ${XDG_DIRECTORY_INSTALL_DIR} )
