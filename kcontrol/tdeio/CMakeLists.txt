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

add_subdirectory( uasproviders )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### other data ################################

tde_create_translated_desktop(
  SOURCE
    cookies.desktop useragent.desktop cache.desktop
    lanbrowser.desktop netpref.desktop proxy.desktop
  PO_DIR kcontrol-desktops
)

install( FILES uninstall.desktop
  RENAME smb.desktop
  DESTINATION ${APPS_INSTALL_DIR}/Settings/WebBrowsing )

install( FILES socks.upd DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )

tde_create_translated_desktop(
  SOURCE uasprovider.desktop
  DESTINATION ${SERVICETYPES_INSTALL_DIR}
  PO_DIR kcontrol-desktops
)

tde_create_translated_desktop(
  SOURCE smb.desktop
  DESTINATION ${APPS_INSTALL_DIR}/.hidden
  PO_DIR kcontrol-desktops
)


##### kcm_tdeio (module) ##########################

tde_add_kpart( kcm_tdeio AUTOMOC
  SOURCES
    policydlg.cpp kcookiesmain.cpp kcookiespolicies.cpp
      main.cpp smbrodlg.cpp useragentdlg.cpp kproxydlgbase.cpp
      kmanualproxydlg.cpp kenvvarproxydlg.cpp kproxydlg.cpp
      kcookiesmanagement.cpp fakeuaprovider.cpp uagentproviderdlg.cpp
      netpref.cpp cache.cpp ksaveioconfig.cpp socks.cpp socksbase.ui
      kproxydlg_ui.ui kcookiesmanagementdlg_ui.ui kcookiespoliciesdlg_ui.ui
      policydlg_ui.ui useragentdlg_ui.ui uagentproviderdlg_ui.ui
      manualproxy_ui.ui envvarproxy_ui.ui cache_ui.ui
  LINK tdeio-shared tdeutils-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
