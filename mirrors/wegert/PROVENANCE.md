# Wegert mirror provenance

This directory is a local reference snapshot from:

- repository: `isomorphismes/wegert`
- commit: `a2be5077ad562c2ecfa505840428600c05bad90c`
- source license: GPL-3.0-or-later

The copied files are deliberately pinned rather than treated as live dependencies.

Included here because they preserve three Android boundaries that are easy to lose when instructions are repeatedly summarized:

- NDK touch/gesture behavior;
- the JNI bridge example;
- the direct DEX launcher/build path.

`code/touch_input_excerpt.c` is an intentionally non-standalone excerpt from `code/wegert.c`; the other mirrored source/build files are copied whole.
