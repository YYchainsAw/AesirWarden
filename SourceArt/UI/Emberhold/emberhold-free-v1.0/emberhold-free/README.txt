EMBERHOLD FREE
Dark fantasy UI kit - 52 icons, 25 interface elements, 24 matching sounds


WHAT THIS IS

A complete minimal interface, not a teaser. The selection is deliberate:
everything you need to build a working main menu, a HUD, an inventory screen
and an options screen - and nothing padded out to look bigger than it is.

If you are prototyping, this should be enough to finish. That is the point.

These are the same files as the paid kit. Same resolution, same formats, no
watermark, nothing downgraded.


WHAT IS IN IT

  icons/       52 icons, SVG plus PNG at 32 / 64 / 128 px,
               in parchment and pure white for engine tinting
  ui/          25 interface elements - panels, buttons in four states,
               health and mana bars, slots, checkboxes, toggles, tooltip
  sfx/         24 sounds in WAV (16-bit 44.1 kHz) and OGG Vorbis

  _hear_all_24_sounds.wav  plays the whole sound set back to back


THE SOUNDS ARE THE PART PEOPLE MISS

Most UI packs give you buttons and no audio. Finding sounds that fit together
- a positive click, a negative click, panel open, panel close, error, purchase,
level up - and cover everything an interface needs is harder than finding the
graphics.

Every sound here is synthesised from oscillators and filtered noise, run
through one shared convolution room and an identical filter chain, with all
melodic cues in D minor. Play any two next to each other and they agree. That
is not something you get by collecting sounds from twenty different authors.


9-SLICE

Panels, buttons, bars and the tooltip are built to stretch. Use these border
insets and the corners stay crisp at any size:

    panel_ornate          128x128     inset 26
    panel_plain            96x96      inset 18
    panel_inset            64x64      inset 14
    tooltip                64x48      inset 14
    header_banner         192x56      inset 20
    btn_standard_*        160x56      inset 18
    bar_frame             256x28      inset 12
    bar_fill_*            248x20      inset  8

Do not 9-slice the slots, checkboxes, toggles or icon buttons - they are fixed
size. Scale them whole, at integer multiples if you want crisp edges.

Building a bar: draw bar_frame first, then bar_fill_<type> inset 8 px
horizontally and 4 px vertically, cropped to the current value. bar_segments
goes on top of the fill, pitched across the full well and cropped to the same
value, so the notches do not move as the bar drains.


ENGINE NOTES

Godot   - panels and buttons as StyleBoxTexture, margins set to the inset above.
          OGG imports as AudioStreamOggVorbis; make sure Loop is off.
Unity   - Sprite (2D and UI), set the border in the Sprite Editor, then
          Image > Type: Sliced. Disable Generate Mip Maps for icons.
Unreal  - Slate Brush, Draw As: Box, margin = inset / texture size.
Web     - the SVGs carry fill on every shape; find-and-replace #E8DCC0 to
          recolour, or serve the white PNGs and tint with CSS filter.

Recolouring: gold trim is #B8893C, light trim #E6C079, panel body runs
#241E2E to #17131D, icon parchment is #E8DCC0. Swap the gold for steel blue or
blood red and you have a different faction in a few minutes.


LICENCE

Free for commercial and non-commercial projects, unlimited titles. Modify and
recolour freely. No attribution required - welcome, never obligatory.

The one thing you may not do is resell or redistribute these files as an asset
pack, on their own or inside another collection.


IF YOU NEED MORE

The full kit is 208 icons, 67 interface elements and 173 sounds - the same
design rules, four times the content. It adds combat statistics and status
effects, creatures, places, crafting materials, magic and ritual icons, five
inventory rarity frames, sliders, scrollbars, tabs, input fields, dropdowns,
and sound families this pack does not touch: drag and drop, keyboard, loot
tiers, forge and smelt, portals and runes, doors and books, countdowns, gongs.

    https://victorgabrielf.itch.io/emberhold-dark-fantasy-ui-kit-matching-sfx

And if an icon or a sound you need is missing from either pack, leave a comment
and ask. That is genuinely how the list grew from 44 sounds to 173.

- VinniBr


HOW THIS WAS MADE

The graphics are not AI-generated images. Every icon is declared once as vector
geometry and rendered to both SVG and PNG from that same declaration, which is
why the vector and raster versions never disagree. The interface elements are
drawn programmatically at 4x and downsampled. The sounds are synthesised - not
sampled, and not produced by a generative audio model.
