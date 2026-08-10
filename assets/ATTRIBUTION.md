# Asset attribution

Everything in `assets/` that this project did not create itself, and where it
came from. All of it is freely licensed; nothing here is commercial art lifted
from another game.

| Asset | Source | Author | Licence |
|---|---|---|---|
| `graphics/bombman/**` | [Bombman 2D resources](https://opengameart.org/content/bombman-2d-resources) (OpenGameArt) | drummyfish | CC0 / public domain (see `graphics/bombman/readme.txt`) |
| `sfx/SFX- The Ultimate 2017 8 bit sound Mini pack/**` | [SFX: The Ultimate 2017 8-bit mini pack](https://opengameart.org/content/sfx-the-ultimate-2017-8-bit-mini-pack) (OpenGameArt) | phoenix1291 / SwissArcadeGameEntertainment | see the pack's `readme.txt` |
| `graphics/BomberBots_Free_Ver/**` | Bomber Bots — Game Asset Pack | Yahallo Games | see `graphics/BomberBots_Free_Ver/readme.txt`; **currently unused** |
| `graphics/font/DejaVuSans.ttf` | [DejaVu Fonts](https://dejavu-fonts.github.io/) | DejaVu project | DejaVu Fonts License |

## What is used as-is, and what is derived

The packs above are kept **unmodified**. Nothing is copied out of them:
`assets/descriptors/*.asset.json` names the original file, which is exactly
what an asset descriptor is for.

The only derived files are the seven animation strips in `graphics/sprites/`,
packed from the per-frame PNGs by `sif_sprite_packer` (a tool that ships with
the engine) because sif addresses animation frames as rectangles inside one
texture. They are not committed; the build regenerates them:

```bash
sif_sprite_packer assets/ assets/sprites.pack.json
```
