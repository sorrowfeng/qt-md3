# Prompts

Reusable AI prompts for driving `qt-md3` development.

The full prompt pack lives in
[`../md3-qt-porting-prompts.md`](../md3-qt-porting-prompts.md). It contains:

- **Prompt A — Bootstrap**: build the base modules before any component.
- **Prompt B — Single-component loop**: the everyday design → implement → audit →
  document cycle.
- **Prompt C — Visual audit**: batch or targeted screenshot comparison.
- **Prompt D — Status sync**: refresh the coverage matrix and status docs.

Core rule, repeated everywhere: **finish all 36 official MD3 families before any
Qt extension component.** The coverage matrix in [`../md3-coverage.md`](../md3-coverage.md)
is the only source of truth for that switch.
