#
# MegaMol™ Core_gl
# Copyright 2017, by MegaMol Team
# Alle Rechte vorbehalten. All rights reserved.
#

if(BUILD_CORE)
  project(core_gl)

  # Add externals.
  require_external(glowl)
  require_external(megamol-shader-factory)

  # Collect source files
  file(GLOB_RECURSE public_header_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "include/*.h" "include/*.inl")
  file(GLOB_RECURSE source_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "src/*.cpp")
  file(GLOB_RECURSE header_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "src/*.h")
  file(GLOB_RECURSE shader_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "shaders/*")

  # Target definition
  add_library(${PROJECT_NAME} OBJECT ${public_header_files} ${header_files} ${source_files} ${shader_files})
  target_compile_definitions(${PROJECT_NAME} PRIVATE MEGAMOLCORE_EXPORTS)
  target_include_directories(${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
  target_link_libraries(${PROJECT_NAME} 
    PUBLIC core vislib_gl glowl megamol-shader-factory frontend_resources  
    PRIVATE abstract_frontend_service) # abstract render api used by MegaMolGraph

  if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
      LINK_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DYNAMICBASE:NO")
  endif()

  # Install shaders
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ DESTINATION "share/shaders")

  # Source groups in Visual Studio
  set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER base)
  source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include/mmcore_gl" PREFIX "Public Header Files" FILES ${public_header_files})
  source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Header Files" FILES ${header_files})
  source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${source_files})
  source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/shaders" PREFIX "Shader Files" FILES ${shader_files})

endif(BUILD_CORE)
