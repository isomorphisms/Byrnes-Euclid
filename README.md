# Byrne's Euclid

Touch-first, draggable applets and small Android packages for individual propositions from Oliver Byrne's edition of Euclid.

First specimen: Book I, Proposition 11 — construct a perpendicular at a given point on a straight line.

## Current sketch

- [Touch interaction](docs/touch-interaction.md) — engine-neutral interaction contract adapted from the working Wegert phone UI.
- [Touch contract](code/touch_contract.h) — small C vocabulary for capture, hit radius, and drag threshold.
- [Wegert mirror](mirrors/wegert/PROVENANCE.md) — pinned local copies of the NDK touch, JNI, and direct-DEX examples so a fork does not need to reconstruct the Android path from another repository.

The mirror is reference material, not a dependency. Proposition geometry should remain separate from platform input and packaging.
