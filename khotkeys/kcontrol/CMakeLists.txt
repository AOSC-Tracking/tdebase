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

add_subdirectory( ui )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/ui
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/khotkeys/shared
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_install_icons( khotkeys )
tde_create_translated_desktop( khotkeys.desktop )


##### kcm_khotkeys (module) #####################

tde_add_kpart( kcm_khotkeys AUTOMOC
  SOURCES
    menuedit.cpp window_trigger_widget.cpp tab_widget.cpp main_buttons_widget.cpp
    actions_listview_widget.cpp menuentry_widget.cpp general_tab.cpp waiting_widget.cpp
    action_group_tab.cpp kcmkhotkeys.cpp command_url_widget.cpp windowdef_list_widget.cpp
    windowdef_simple_widget.cpp triggers_tab.cpp dcop_widget.cpp info_tab.cpp
    action_list_widget.cpp keyboard_input_widget.cpp condition_list_widget.cpp
    gesturedrawer.cpp gesturerecorder.cpp gesturerecordpage.cpp gesture_triggers_tab.cpp
    windowselector.cpp gestures_settings_tab.cpp general_settings_tab.cpp
    voicerecorder.cpp voicerecordpage.cpp voice_settings_tab.cpp
  LINK ui-static
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### kcm_khotkeys_init (module) ################

tde_add_kpart( kcm_khotkeys_init AUTOMOC
  SOURCES init.cpp
  LINK tdecore-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
