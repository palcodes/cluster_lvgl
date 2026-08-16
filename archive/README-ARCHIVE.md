# Archive

The previous cluster, kept whole and untouched.

It was a different design: a speed dial with a multi-segment gradient ring,
three counter-rotating halo arcs, shadow blooms and a staggered power-on
reveal, set in Outfit at six weights. Its own README is `README.md` in this
directory and still describes it accurately — the paths in it are relative
to this folder, not to the repository root.

```
archive/
  README.md       the old project's documentation
  ui/             ui_theme.h, ui_fonts.h, ui_dash.[ch], ui_events.c
  app/            ev_data.[ch]
  fonts/          Outfit + FontAwesome faces, and their generate.sh
  docs/           its rendered previews
  sim/            Win32 runner and build scripts
  test/           headless preview
  mcux/           i.MX RT1170 seam and bring-up guide
```

Nothing in the live tree references any of it. It is here because the
Win32 runner, the headless preview harness and the RT1170 bring-up notes
were worth keeping a copy of — the current versions of those three were
rewritten from these rather than from scratch.

Superseded by the Figma frame **Dashboard - v2**
(`u7aoLFxLjCtp1zq2fgMUdD`, node 1:2), which the root README covers.
