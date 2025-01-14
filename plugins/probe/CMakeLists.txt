# MegaMol
# Copyright (c) 2021, MegaMol Dev Team
# All rights reserved.
#

megamol_plugin(probe
  BUILD_DEFAULT OFF
  DEPENDS_PLUGINS
    mmadios
    mesh
    datatools
    geometry_calls
  DEPENDS_EXTERNALS
    libigl
    asmjit
    blend2d
    qhull
    PUBLIC Eigen
    nanoflann)

if (probe_PLUGIN_ENABLED)
  file(GLOB_RECURSE third RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "3rd/*")
  file(GLOB_RECURSE ospray_sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "probe_ospray/src/*.cpp")
  file(GLOB_RECURSE ospray_headers RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "probe_ospray/src/*.h")
  file(GLOB_RECURSE ospray_public_headers RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "probe_ospray/include/*.h")

  target_sources(probe PRIVATE ${third})
  target_include_directories(probe PUBLIC "3rd")
  
  find_package(CGAL 5.3 CONFIG REQUIRED)
  target_link_libraries(probe PUBLIC CGAL::CGAL)
  if(WIN32)
    install(FILES "${CGAL_DIR}/../../bin/gmp.dll" "${CGAL_DIR}/../../bin/mpfr-6.dll" DESTINATION "bin")
  endif()
  if (BUILD_MMOSPRAY_PLUGIN_PLUGIN)
    target_sources(probe PRIVATE ${ospray_sources} ${ospray_headers} ${ospray_public_headers})
    target_compile_definitions(probe PRIVATE ${EXPORT_NAME}_HAS_OSPRAY)
    target_include_directories(probe PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/probe_ospray/include> "probe_ospray/src")
    target_link_libraries(probe PUBLIC mmospray)
  endif ()

endif ()
