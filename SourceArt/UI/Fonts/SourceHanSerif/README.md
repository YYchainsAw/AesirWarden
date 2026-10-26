# Source Han Serif SC Regular / 思源宋体

This folder preserves the original Adobe font and its SIL Open Font License 1.1 for the AesirWarden menu.

## Upstream and provenance

- Author: Adobe, with contributions from Google and the Source Han project.
- Official repository: https://github.com/adobe-fonts/source-han-serif
- Official release branch revision: `7889f11bf31170b5d092a083b357c8c8130f89e0`
- Font version: `2.003`.
- Font download: https://raw.githubusercontent.com/adobe-fonts/source-han-serif/7889f11bf31170b5d092a083b357c8c8130f89e0/OTF/SimplifiedChinese/SourceHanSerifSC-Regular.otf
- License download: https://raw.githubusercontent.com/adobe-fonts/source-han-serif/7889f11bf31170b5d092a083b357c8c8130f89e0/LICENSE.txt
- Retrieved: 2026-10-20.
- SHA-256: `78AA7A328FD974DF2D688C8A9FD74A33D8334DFA84AB24D9D11EFB2FFC464117`.
- Original file size: `24,543,332` bytes.

## License

See [OFL.txt](OFL.txt) for the original copyright notice and complete SIL Open Font License 1.1. The font can be embedded and redistributed with the game under that license. Retain the license alongside font source redistribution and in the game's third-party notices. The font file has not been modified, subsetted, or renamed.

## Menu usage

The original file is a static OpenType/CFF font, not a variable font. It contains simplified Chinese and Latin serif glyphs; use it for all menu button labels for a consistent classical style. The intended initial button size is 55 UMG units. Font coverage has been checked for `开始游戏`, `设置`, `制作人员`, `退出游戏`, common Chinese punctuation, digits, and `AESIR WARDEN`.

Import as a runtime Font Face and reference it through a runtime composite Font asset for UMG. The existing logo remains an image; this typeface provides a compatible serif direction for readable Chinese text, rather than a claim to reproduce its custom lettering.
