if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/rapidutf-${PROJECT_VERSION}"
      CACHE PATH ""
  )
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package rapidutf)

install(
    DIRECTORY
    include/
    "${PROJECT_BINARY_DIR}/export/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT rapidutf_Development
)

install(
    TARGETS rapidutf_rapidutf
    EXPORT rapidutfTargets
    RUNTIME #
    COMPONENT rapidutf_Runtime
    LIBRARY #
    COMPONENT rapidutf_Runtime
    NAMELINK_COMPONENT rapidutf_Development
    ARCHIVE #
    COMPONENT rapidutf_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    rapidutf_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(rapidutf_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${rapidutf_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT rapidutf_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${rapidutf_INSTALL_CMAKEDIR}"
    COMPONENT rapidutf_Development
)

install(
    EXPORT rapidutfTargets
    NAMESPACE rapidutf::
    DESTINATION "${rapidutf_INSTALL_CMAKEDIR}"
    COMPONENT rapidutf_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
