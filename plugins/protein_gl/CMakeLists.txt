# MegaMol
# Copyright (c) 2021, MegaMol Dev Team
# All rights reserved.
#

megamol_plugin(protein_gl
  BUILD_DEFAULT OFF
  DEPENDS_GL
  DEPENDS_PLUGINS
    protein
    trisoup
    protein_calls
    geometry_calls_gl
    compositing_gl
  DEPENDS_EXTERNALS
    glad
    nanoflann)

if (protein_gl_PLUGIN_ENABLED)
  if (WIN32)
    # special case for the MSMS-related files
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/msms/msms.exe DESTINATION "bin")
  endif ()
endif ()
