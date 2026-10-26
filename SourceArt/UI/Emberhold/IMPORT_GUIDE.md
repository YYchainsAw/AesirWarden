# Emberhold Free - inspected Unreal HUD source guide

Downloaded from the author's free product page on 2026-10-21. See SOURCE_AND_LICENSE.md and the original bundled LICENSE.txt. Only the free edition was downloaded; no purchase was made.

Source root (absolute):
D:/YYchainsaw/Unreal-Projects/AesirWarden/SourceArt/UI/Emberhold/emberhold-free-v1.0/emberhold-free/

## Recommended files

| Relative file | Actual pixels | Use |
| --- | --- | --- |
| ui/png/bar_frame.png | 256 x 28 | Thin gold frame with dark interior |
| ui/png/bar_fill_hp.png | 248 x 20 | Red health fill |
| ui/png/bar_fill_mp.png | 248 x 20 | Blue mana fill |
| ui/png/bar_segments.png | 248 x 20 | Optional transparent notch overlay |
| ui/png/divider.png | 192 x 16 | Transparent gold line and central diamond |
| ui/png/slot_empty.png | 64 x 64 | Optional item/ability slot; dark opaque interior |
| icons/png/white/128px/heart.png | 128 x 128 | Health icon, tintable white |
| icons/png/white/128px/shield.png | 128 x 128 | Defense/shield icon, tintable white |
| icons/png/white/128px/boots.png | 128 x 128 | Movement icon |
| icons/png/white/128px/sword.png | 128 x 128 | Weapon/action icon |
| icons/png/white/128px/potion.png | 128 x 128 | Consumable icon |
| icons/png/white/128px/elixir.png | 128 x 128 | Alternative consumable icon |
| icons/png/white/128px/fire.png | 128 x 128 | Fire ability |
| icons/png/white/128px/lightning.png | 128 x 128 | Lightning ability |
| icons/png/white/128px/bag.png | 128 x 128 | Inventory |
| icons/png/white/128px/skull.png | 128 x 128 | Enemy/boss marker |
| icons/png/white/128px/coin.png | 128 x 128 | Currency |

The corresponding 32/64 px PNGs and SVG masters are retained upstream. Import only the chosen PNGs for the first UMG implementation; do not import the entire source tree or duplicate sound formats.

## Verified alpha

All 25 UI PNGs plus 12 selected 128 px white icons were decoded successfully as Format32bppArgb and inspected pixel by pixel. INSPECTED_PNGS.json preserves dimensions, alpha ranges, transparent/partial/opaque counts and visible bounds.

- White icons have alpha 0..255 with large transparent areas.
- divider.png has alpha 0..255 and 2252 completely transparent pixels out of 3072.
- bar_segments.png has alpha 0..147, no opaque pixels; use as an optional overlay.
- bar_frame.png has a dark opaque well: 5508 opaque pixels out of 7168 and only four completely transparent pixels. It is not a hollow border cutout.
- The panel, button and slot images also contain opaque interiors. To retain the project's transparent-panel look, use existing Kenney borders, omit these backgrounds, or use a Slate Border brush that skips the source's middle region. Keep a dark well only where it improves resource-bar readability.

## Upstream 9-slice data and normalized Slate margins

Values are copied from the bundled README.txt and divided by the actual texture width/height. Margin order: Left, Top, Right, Bottom.

| Asset | Upstream inset | Normalized Slate margin L,T,R,B |
| --- | --- | --- |
| bar_frame (256x28) | 12 px | 0.046875, 0.428571, 0.046875, 0.428571 |
| bar_fill_hp/mp (248x20) | 8 px | 0.032258, 0.4, 0.032258, 0.4 |
| panel_ornate (128x128) | 26 px | 0.203125, 0.203125, 0.203125, 0.203125 |
| panel_plain (96x96) | 18 px | 0.1875, 0.1875, 0.1875, 0.1875 |
| panel_inset (64x64) | 14 px | 0.21875, 0.21875, 0.21875, 0.21875 |
| tooltip (64x48) | 14 px | 0.21875, 0.291667, 0.21875, 0.291667 |
| header_banner (192x56) | 20 px | 0.104167, 0.357143, 0.104167, 0.357143 |
| btn_standard_* (160x56) | 18 px | 0.1125, 0.321429, 0.1125, 0.321429 |

Use Draw As Box for a complete bar frame and fill. Icons, icon buttons, slots, checkboxes and toggles are fixed-size art; do not 9-slice them. bar_segments should stay anchored across the full fill well and be clipped to the same resource fraction; do not squash it as the resource drains.

The upstream bar assembly prose says to inset the fill by 8 px horizontally and 4 px vertically, but its actual 248 px native fill is only 8 px narrower in total than the 256 px frame. Those instructions cannot both describe an unscaled native-size placement. Define the intended well in UMG explicitly: for an 8 px inset on each horizontal side, stretch the fill brush into a 240x20 destination at native frame size; alternatively a centered 248x20 fill has 4 px horizontal/vertical insets. Verify the chosen well visually in UMG rather than using native size plus a one-sided 8 px offset.

## Suggested Unreal texture import

- Texture Group: UI.
- Compression Settings: UserInterface2D (RGBA).
- Mip Gen Settings: NoMipmaps.
- Preserve the alpha channel; sRGB enabled for these colour UI PNGs.
- Use white icon source and UMG tint for ivory/gold. Author palette: gold #B8893C, light gold #E6C079, parchment #E8DCC0.
- The current game HUD uses health and guard pressure, with no mana or stamina bars.

The project importer is Scripts/Editor/setup_combat_hud.py. It imports only the six
selected white glyphs into /Game/Aesir/UI/Art/Emberhold and keeps these third-party
textures local; the HUD frames continue to use the existing transparent Kenney art.
