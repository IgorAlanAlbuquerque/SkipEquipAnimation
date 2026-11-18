## Summary

- Brings over the build improvements from `ba679c4` (custom `x64-windows-skse` overlay triplet,
  Ninja generator, `/OPT:REF` for Release/RelWithDebInfo, stop ignoring `*.cmake` so the triplet
  stays tracked). These were authored by AnArchos on the fork right after the 1.7.99
  build-system squash but never landed on upstream (authorship preserved via `cherry-pick -x`).
- Fixes invalid JSON in `SKSE/Plugins/BehaviorDataInjector/SkipEquipAnimation_BDI.json`: the
  `InstantEquipAnim` entry was missing the `:` after `"name"` (`"name" "InstantEquipAnim"`).
- Drops a redundant `virtual` on `ProcessEvent ... override` in `event.h`.
- Removes `cmake/headerlist.cmake` / `cmake/sourcelist.cmake`, unused since sources are globbed
  (`file(GLOB_RECURSE ...)` in `CMakeLists.txt`).
- Adds `.gitattributes` (`-text` on the shared source/build paths) so CRLF blobs are stored as-is
  and `core.autocrlf=true` clients stop producing whole-file LF diffs.

## Test plan

- [x] Configure + build `build-release-msvc` preset (Ninja, `x64-windows-skse`), MSVC 14.52: exit 0, no warnings in `src/`
- [x] In-game: skip / instant / normal equip, draw & sheath, spell equip

🤖 Generated with [Claude Code](https://claude.com/claude-code)
