#################################################
#
#  (C) 2010-2011 Serghei Amelian
#  serghei (DOT) amelian (AT) gmail.com
#
#  (C) 2020 Slávek Banko
#  slavek.banko (AT) axis.cz
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

foreach( _wallpaper
    aghi.jpg
    airplane.jpg
    All-Good-People-1.jpg
    alta-badia.jpg
    andes-venezolanos.svgz
    another-view.jpg
    aurora.svgz
    blue-bend.jpg
    celtic.svgz
    Chicken-Songs-2.jpg
    default_blue.jpg
    default_gears.jpg
    floating-leaves.jpg
    globe.svgz
    green_curtain.jpg
    here-gear.svgz
    horse-head-nebula.png
    isadora.png
    konqui.svgz
    lineart.svgz
    moon.svgz
    mountain-lake.jpg
    No-Ones-Laughing-3.jpg
    seaofconero.jpg
    soft-green.jpg
    stelvio.jpg
    stripes.jpg
    sunshine-after-the-rain.jpg
    Time-For-Lunch-2.jpg
    Totally-New-Product-1.jpg
    Trinity4.png
    Trinity-lineart.svg
    triplegears.jpg
    Won-Ton-Soup-3.jpg
    world-desktop.jpg
)
  if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_wallpaper} )
    install(
      FILES ${_wallpaper}
      DESTINATION ${WALLPAPER_INSTALL_DIR}
    )
    if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_wallpaper}.desktop )
      tde_create_translated_desktop(
        SOURCE ${_wallpaper}.desktop
        DESTINATION ${WALLPAPER_INSTALL_DIR}
        PO_DIR wallpapers-desktops
      )
    endif()
  endif()
endforeach()
