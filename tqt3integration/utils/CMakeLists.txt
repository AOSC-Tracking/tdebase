#################################################
#
#  (C) 2011 Timothy Pearson
#  kb9vqf (AT) pearsoncomputing.net
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

add_subdirectory( tqt )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### gen (internal executable) ########################

tde_add_executable( gen AUTOMOC
  SOURCES
    gen.cpp
  LINK tdecore-shared
)

add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/tqttde_functions.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/module_functions.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/module_functions.h
  COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/tqt
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/gen ${CMAKE_CURRENT_SOURCE_DIR}/gen.txt
  COMMAND cat ${CMAKE_CURRENT_BINARY_DIR}/tqt/in/tqttdeintegration_x11_0.cpp ${CMAKE_CURRENT_SOURCE_DIR}/tqt/in/tqttdeintegration_x11_1.cpp ${CMAKE_CURRENT_BINARY_DIR}/tqttdeintegration_x11.cpp.gen ${CMAKE_CURRENT_SOURCE_DIR}/tqt/in/tqttdeintegration_x11_2.cpp > ${CMAKE_CURRENT_BINARY_DIR}/tqt/tqttdeintegration_x11.cpp
  COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/tqt/in/tqttdeintegration_x11_p_1.h ${CMAKE_CURRENT_BINARY_DIR}/tqttdeintegration_x11_p.h.gen ${CMAKE_CURRENT_SOURCE_DIR}/tqt/in/tqttdeintegration_x11_p_2.h > ${CMAKE_CURRENT_BINARY_DIR}/tqt/tqttdeintegration_x11_p.h
  COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/tqttdeintegration_x11.cpp.gen ${CMAKE_CURRENT_BINARY_DIR}/tqttdeintegration_x11_p.h.gen
  DEPENDS gen
)

add_custom_target( generate_tqt3_bindings
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/tqttde_functions.cpp
)
