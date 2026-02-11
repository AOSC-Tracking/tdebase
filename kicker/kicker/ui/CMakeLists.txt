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
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/kicker/kicker/core
  ${CMAKE_BINARY_DIR}/kicker/libkicker
  ${CMAKE_SOURCE_DIR}/kicker/libkicker
  ${CMAKE_SOURCE_DIR}/kicker/kicker/core
  ${CMAKE_SOURCE_DIR}/kicker/kicker/buttons
  ${CMAKE_SOURCE_DIR}/tdmlib
  ${CMAKE_SOURCE_DIR}/libkonq
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES default-favs DESTINATION ${DATA_INSTALL_DIR}/kicker )


##### kicker_ui (static) ########################

set( target kicker_ui )

set( ${target}_SRCS
  addbutton_mnu.cpp appletitem.ui appletview.ui addapplet.cpp
  addapplet_mnu.cpp appletop_mnu.cpp browser_mnu.cpp
  client_mnu.cpp dirdrop_mnu.cpp nonKDEButtonSettings.ui
  exe_dlg.cpp k_new_mnu.cpp k_mnu.cpp k_mnu.skel quickbrowser_mnu.cpp
  service_mnu.cpp kmenuitembase.ui addextension_mnu.cpp
  extensionop_mnu.cpp k_mnu_stub.cpp recentapps.cpp
  browser_dlg.cpp itemview.cpp kickoff_bar.cpp removeapplet_mnu.cpp
  removeextension_mnu.cpp removecontainer_mnu.cpp removebutton_mnu.cpp
  popupmenutitle.cpp hidebutton.cpp popupmenutop.cpp addappletvisualfeedback.cpp
  clicklineedit.cpp flipscrollview.cpp media_watcher.cpp
  media_watcher.skel mykickoffsearchinterface.cpp query.cpp
)

tde_add_library( ${target} STATIC_PIC AUTOMOC
  SOURCES ${${target}_SRCS}
  DEPENDENCIES kicker_core-static kicker+libkicker+kickerSettings.cpp
  LINK dmctl-static kickoffsearch_interfaces-shared ${TDEHW_LIBRARIES}
)
