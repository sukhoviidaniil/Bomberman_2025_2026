add_executable(bomberman main.cpp)
target_link_libraries(bomberman PRIVATE bomberman_view)

# ---------------------------------------------------------------------------
# Asset registry
#
# Generated at build time by sif's own tool rather than committed, so it
# can never drift from the *.asset.json descriptors next to the data. The
# tool is one of the targets sif brings with it, hence $<TARGET_FILE:...>
# instead of a hard-coded path.
# ---------------------------------------------------------------------------
add_custom_command(
        OUTPUT "${CMAKE_SOURCE_DIR}/assets/bin/registry.rgst.json"
        COMMAND Asset_GUID_Assignment
                "${CMAKE_SOURCE_DIR}/assets/"
                "${CMAKE_SOURCE_DIR}/assets/bin/registry.rgst.json"
        DEPENDS Asset_GUID_Assignment
        COMMENT "Generating the Bomberman asset registry"
        VERBATIM
)
add_custom_target(bomberman_assets ALL
        DEPENDS "${CMAKE_SOURCE_DIR}/assets/bin/registry.rgst.json")
add_dependencies(bomberman bomberman_assets)
