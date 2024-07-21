@echo off

mkdir cache

build\lbasset "cache/" "data/glTF-Sample-Assets/Models/Sponza/glTF/Sponza.gltf"

build\lbasset "cache/" "data/orca/SpeedTree/azalea/azalea.gltf"
build\lbasset "cache/" "data/orca/SpeedTree/backyard-grass/backyard-grass.gltf"
build\lbasset "cache/" "data/orca/SpeedTree/boston-fern/boston-fern.gltf"
build\lbasset "cache/" "data/orca/SpeedTree/european-linden/european-linden.gltf"
build\lbasset "cache/" "data/orca/SpeedTree/hedge/hedge.gltf"
build\lbasset "cache/" "data/orca/SpeedTree/japanese-maple/japanese-maple.gltf"
build\lbasset "cache/" "data/orca/SpeedTree/red-maple-young/red-maple-young.gltf"
build\lbasset "cache/" "data/orca/SpeedTree/white-oak/white-oak.gltf"

build\lbasset --albedo "cache/" "data/texture/TCom_Sand_Muddy2_2x2_4K_albedo.tif"
build\lbasset --normal "cache/" "data/texture/TCom_Sand_Muddy2_2x2_4K_normal.tif"
build\lbasset --roughness "cache/" "data/texture/TCom_Sand_Muddy2_2x2_4K_roughness.tif"
build\lbasset --occlusion "cache/" "data/texture/TCom_Sand_Muddy2_2x2_4K_ao.tif"

build\lbasset --albedo "cache/" "data/texture/TCom_Rock_CliffLayered_1.5x1.5_4K_albedo.tif"
build\lbasset --normal "cache/" "data/texture/TCom_Rock_CliffLayered_1.5x1.5_4K_normal.tif"
build\lbasset --roughness "cache/" "data/texture/TCom_Rock_CliffLayered_1.5x1.5_4K_roughness.tif"
build\lbasset --occlusion "cache/" "data/texture/TCom_Rock_CliffLayered_1.5x1.5_4K_ao.tif"

build\lbasset --albedo "cache/" "data/texture/leafy-grass2-ue/leafy-grass2-albedo.png"
build\lbasset --normal "cache/" "data/texture/leafy-grass2-ue/leafy-grass2-normal-dx.png"
build\lbasset --roughness "cache/" "data/texture/leafy-grass2-ue/leafy-grass2-roughness.png"
build\lbasset --occlusion "cache/" "data/texture/leafy-grass2-ue/leafy-grass2-ao.png"

build\lbasset --albedo "cache/" "data/texture/mixedmoss-ue4/mixedmoss-albedo2.png"
build\lbasset --normal "cache/" "data/texture/mixedmoss-ue4/mixedmoss-normal2.png"
build\lbasset --roughness "cache/" "data/texture/mixedmoss-ue4/mixedmoss-roughness.png"
build\lbasset --occlusion "cache/" "data/texture/mixedmoss-ue4/mixedmoss-ao2.png"