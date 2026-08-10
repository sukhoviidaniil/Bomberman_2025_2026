add_executable(bomberman main.cpp)
target_link_libraries(bomberman PRIVATE bomberman_view)

# ---------------------------------------------------------------------------
# Asset pipeline
#
# Two steps, both using tools that ship with sif, both run by the build so the
# generated files can never drift from their inputs:
#
#   sif_sprite_packer      packs the per-frame artwork listed in
#                          assets/sprites.pack.json into sprite strips and
#                          writes their *.asset.json descriptors
#   Asset_GUID_Assignment  scans every *.asset.json and writes the registry
#
# Neither output is committed: both are reproducible from their inputs, and a
# generated file in version control is a file that will eventually be edited by
# hand and then silently overwritten.
# ---------------------------------------------------------------------------
set(BOMBERMAN_ASSET_DIR "${CMAKE_SOURCE_DIR}/assets")
set(BOMBERMAN_SPRITE_PACK "${BOMBERMAN_ASSET_DIR}/sprites.pack.json")
set(BOMBERMAN_REGISTRY "${BOMBERMAN_ASSET_DIR}/bin/registry.rgst.json")

if(TARGET sif_sprite_packer)
    add_custom_command(
            OUTPUT "${BOMBERMAN_REGISTRY}"
            COMMAND sif_sprite_packer "${BOMBERMAN_ASSET_DIR}/" "${BOMBERMAN_SPRITE_PACK}"
            COMMAND Asset_GUID_Assignment "${BOMBERMAN_ASSET_DIR}/" "${BOMBERMAN_REGISTRY}"
            DEPENDS sif_sprite_packer Asset_GUID_Assignment "${BOMBERMAN_SPRITE_PACK}"
            COMMENT "Packing sprite strips and generating the asset registry"
            VERBATIM
    )
else()
    # sif skips the packer when SFML is unavailable. The strips cannot be
    # rebuilt then, but the registry still can - and saying so beats a build
    # that fails for a reason three directories away.
    message(WARNING
            "sif_sprite_packer is unavailable, so sprite strips will not be rebuilt. "
            "The asset registry is still generated from the existing descriptors.")
    add_custom_command(
            OUTPUT "${BOMBERMAN_REGISTRY}"
            COMMAND Asset_GUID_Assignment "${BOMBERMAN_ASSET_DIR}/" "${BOMBERMAN_REGISTRY}"
            DEPENDS Asset_GUID_Assignment
            COMMENT "Generating the asset registry"
            VERBATIM
    )
endif()

add_custom_target(bomberman_assets ALL DEPENDS "${BOMBERMAN_REGISTRY}")
add_dependencies(bomberman bomberman_assets)
