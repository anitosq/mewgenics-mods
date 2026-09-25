# Research references and notices

Project code and documentation use the [MIT License](LICENSE). Credits and
licenses for other material are listed below. Game assets belong to their owners.

Inventory serialization research uses the format documented in
https://github.com/michael-trinity/mewgenics-savegame-editor,
`app/utils/parse/inventory.ts`. Our reader adds strict bounds/version/end checks,
uses read-only SQLite snapshots, and has no save writer. Retained MIT notice:

MIT License

Copyright (c) 2025 Nuxt UI Templates

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

The repositories in `work/modding-sources/` are local research references.
They aren't included in the mod. The cat-table repository had no license
file when inspected; no code or assets were copied from it.

## Native control lettering

Patrick Hand, Copyright (c) 2010-2012 Patrick Wagesreiter, is distributed under
the SIL Open Font License 1.1. The original font and full license are retained
in `assets/fonts/`. Source: https://github.com/google/fonts/tree/main/ofl/patrickhand.
This font was used by the earlier prototype. The current native control
generator loads the installed game's `Edmundm` font through `fonts.swf`.
The Patrick Hand files are historical references; include
`assets/fonts/OFL.txt` if distributing those files or the older generated assets.
fontTools under `work/python-libs/` is not required by the current UI generator.

The rarity tiles use the installed game's `HeadItemIcon` rarity frames.
Game fonts and artwork are loaded at runtime and aren't included in the mod archive.
