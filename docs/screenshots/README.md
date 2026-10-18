# Screenshot gallery

Light and dark captures of the gallery, taken with
`qt-md3-example --language en --theme <light|dark> --screenshot …` on the
native Windows platform (so glyphs render).

| File | Page |
| --- | --- |
| `overview-light.png` / `overview-dark.png` | Gallery home — foundation modules at a glance |
| `showcase-mail-light.png` / `showcase-mail-dark.png` | Showcase · Mail / Inbox |
| `showcase-music-light.png` / `showcase-music-dark.png` | Showcase · Music player |
| `showcase-settings-light.png` / `showcase-settings-dark.png` | Showcase · Settings |
| `dynamic-colour-light.png` / `dynamic-colour-dark.png` | Dynamic colour — seed, variant, contrast |
| `buttons-light.png` / `buttons-dark.png` | Buttons (component page) |
| `chips-light.png` / `chips-dark.png` | Chips (component page) |

Regenerate the full set (every gallery page, window + full-height) with:

```bash
build/examples/qt-md3-example --language en --theme light --screenshot build/shots-light
build/examples/qt-md3-example --language en --theme dark  --screenshot build/shots-dark
```

Prefer `*-full.png` when auditing a page body: the window shot only shows the
first screenful. Capture from the native platform when text matters —
`QT_QPA_PLATFORM=offscreen` renders tofu boxes on Windows.
