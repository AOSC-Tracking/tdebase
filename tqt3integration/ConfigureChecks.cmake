#################################################
#
#  (C) 2019 Slávek Banko
#  slavek (DOT) banko (AT) axis.cz
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################


##### check for tqt plugins dir #################

execute_process(
  COMMAND ${PKG_CONFIG_EXECUTABLE}
    tqt-mt --variable=pluginsdir
  OUTPUT_VARIABLE TQT_PLUGINS_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
