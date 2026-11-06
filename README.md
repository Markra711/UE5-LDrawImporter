# LDraw Importer for Unreal Engine 5

An editor plugin that imports LDraw models (`.ldr`) into Unreal Engine 5 as ready-to-use Static Meshes, complete with materials, and keeps them reimportable.

![LDraw model imported into Unreal Engine 5](docs/screenshot.png)

LDraw is the open file format behind most digital brick-building tools such as Stud.io, LDCad and LeoCAD. This plugin lets you build a model in one of those tools and drag the file straight into the Content Browser.

## Features

- **Drag-and-drop import** of `.ldr` files into the Content Browser, plus **reimport** when the source file changes
- **Bundled part library:** over 10,000 pre-converted part and primitive meshes in 39 categories, with beveled edges
- **Bundled material library:** about 200 material instances covering the LDraw colour palette (solid, transparent, chrome, metallic, pearl, rubber, glitter, speckle and more), built on a set of master materials
- **Multi-part files (MPD):** handles the `0 FILE` blocks that Stud.io and LDCad write, as well as `0 Name:` and nested submodels
- **Colour inheritance:** resolves LDraw colour 16 ("use the colour of the parent"), so recoloured sub-assemblies come out right
- **Build steps are preserved:** `0 STEP` is parsed per assembly, as groundwork for importing a model up to a given step
- **Robust parsing:** malformed lines are skipped with a log message that names the line number, and self-referencing submodels are detected instead of recursing forever
- **Efficient output:** all parts are merged into a single Static Mesh with one material slot per colour

## Installation

1. Download `LDrawImporter-UE5.8.zip` from the [latest release](../../releases/latest).
2. Unzip it into your project's `Plugins/` folder, so that you end up with `Plugins/LDraw/LDraw.uplugin`.
3. Start the editor and enable **LDraw** under *Edit → Plugins* if it isn't enabled yet.

Tested with Unreal Engine 5.8 on Windows.

## Usage

Drag an `.ldr` file into the Content Browser, or use *Import* and pick the file. The plugin creates a Static Mesh named after the file. If you edit the model later, right-click the asset and choose *Reimport*.

## Architecture

The plugin is split into a runtime module and an editor module, so that the parsed data can later be used at runtime as well.

```
.ldr file
   │
   ▼
FLDrawParser          (runtime)  reads the file into FLDrawModelData: assemblies,
   │                             build steps, placements and unresolved colour IDs
   ▼
FLDrawResolver        (runtime)  flattens the hierarchy, applies colour inheritance
   │                             and converts LDraw coordinates to Unreal space once
   ▼
LDrawModelBuilder     (editor)   merges the part meshes into one MeshDescription,
   │                             one polygon group per material
   ▼
UStaticMesh asset
```

- `ULDrawAssetEditorSubsystem` caches part meshes and colour materials once the Asset Registry is ready, so lookups during import are cheap.
- `ULDrawFactory` registers the `.ldr` format with the editor and handles reimport.
- Parsing and resolving are deliberately kept apart: the parsed model is faithful to the source file (hierarchy, steps, unresolved colours), because flattening throws that information away.

## Known limitations

- Only `.ldr` files are registered for import so far (`.mpd` files can be renamed to `.ldr`).
- Parts that are missing from the bundled library are skipped.
- Mirrored parts (negative determinant in the LDraw matrix) are not mirrored yet.
- Edge lines (LDraw line types 2 and 5) are not imported.

## Roadmap

- Import dialog to choose the output type and a build-step range (the resolver already supports this through `FLDrawFlattenFilter`)
- Output as individual actors or instanced meshes instead of a single merged mesh
- Support for `.mpd` and `.dat` files directly

## Credits and licence

- Part geometry is converted from the [LDraw Parts Library](https://library.ldraw.org). Depending on the part, it is licensed under CC BY 4.0 or CC BY 2.0; see [CAreadme.txt](CAreadme.txt) for details. Thanks to all LDraw part authors.
- Plugin code: see [LICENSE](LICENSE).

LEGO® is a trademark of the LEGO Group, which does not sponsor, authorise or endorse this project.
