file( GLOB _man_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.1 )

install(
    FILES ${_man_files}
    DESTINATION ${MAN_INSTALL_DIR}/man1
    COMPONENT doc
)
