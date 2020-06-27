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

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/libkonq
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### other data ################################

tde_create_translated_desktop(
  SOURCE
    tdehtml_behavior.desktop tdehtml_java_js.desktop
    tdehtml_fonts.desktop tdehtml_filter.desktop
    tdehtml_userinterface.desktop
  PO_DIR kcontrol-desktops
)

install(
  FILES uninstall.desktop
  DESTINATION ${APPS_INSTALL_DIR}/Settings/WebBrowsing
  RENAME nsplugin.desktop )

install(
  FILES uninstall.desktop
  DESTINATION ${APPS_INSTALL_DIR}/Settings/WebBrowsing
  RENAME tdehtml_appearance.desktop )


##### kcm_konqhtml (module) #####################

tde_add_kpart( kcm_konqhtml AUTOMOC
  SOURCES
    userInterOpts.ui userInterOpts_impl.cpp userInterConfig.cpp
    htmlopts.cpp jsopts.cpp javaopts.cpp pluginopts.cpp
    appearance.cpp khttpoptdlg.cpp policydlg.cpp main.cpp
    jspolicies.cpp nsconfigwidget.ui policies.cpp
    domainlistview.cpp filteropts.cpp
  LINK tdehtml-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
