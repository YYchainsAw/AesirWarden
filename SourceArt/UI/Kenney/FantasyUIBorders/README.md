# Kenney Fantasy UI Borders

- Original author: Kenney, https://kenney.nl/
- Source: https://kenney.nl/assets/fantasy-ui-borders
- Version: 1.0 (2023-12-03)
- Downloaded: 2026-10-20
- License: CC0; personal, educational and commercial use permitted. See [License.txt](Assets/License.txt).
- Original archive: `kenney_fantasy-ui-borders.zip`
- Archive SHA-256: `59532DA3BD61195A425585455B40BAA6CBF1EDA8227C42ADCEF31A535A737769`

`Assets/` contains the unmodified extracted pack: 280 PNG assets in Default/Double variants, one SVG source sheet, preview/sample images, and the original license and links. The archive is retained for recovery.

## Unreal Engine textures

The 280 PNGs are imported as Texture2D assets under `/Game/Aesir/UI/Art/Kenney/FantasyUIBorders`. `Default` and `Double` each contain 140 textures, grouped into Border, Divider, DividerFade, Panel, TransparentBorder and TransparentCenter. The SVG source and sample sheets remain here as source art.

The import uses UI texture group, uncompressed RGBA8, no mipmaps, bilinear filtering, sRGB and preserved alpha. These white assets can be tinted in UMG. In a Widget Designer, add an Image and select a texture in Brush > Image; use Draw As > Box with margins adjusted to the actual corner width when stretching panel frames. Double provides the larger source size.

The repeatable importer is [import_kenney_ui.py](../../../../Scripts/Editor/import_kenney_ui.py). Existing assets are validated without overwriting edits. Run it through Unreal Editor Python, or use:

```powershell
& 'D:/Program Files/Epic/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' 'D:/YYchainsaw/Unreal-Projects/AesirWarden/AesirWarden.uproject' -run=pythonscript -EnablePlugins=PythonScriptPlugin -script='D:/YYchainsaw/Unreal-Projects/AesirWarden/Scripts/Editor/import_kenney_ui.py' -unattended -NullRHI -nosound
```

Append `-KenneyUIVerifyOnly` to verify the saved assets without importing. Reports are written under `Saved/KenneyUIImport/`. Importing does not change menus or levels.
