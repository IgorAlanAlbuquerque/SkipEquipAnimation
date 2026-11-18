# Contract: branch de contribuição `contrib/build-improvements`

Invariantes da branch que será proposta ao PhiloSocio/SkipEquipAnimation (FR-012, FR-012a, FR-013, FR-014).

## C-PR-1: Base

- `git merge-base contrib/build-improvements upstream/main` é igual a `upstream/main` (`22f1e3b`). A branch parte do HEAD do upstream, então o merge no upstream é fast-forward ou não tem conflito.

## C-PR-2: Allowlist de arquivos

`git diff --name-status upstream/main contrib/build-improvements` contém **só**:

| Status | Caminho | Commit de origem |
| --- | --- | --- |
| M | `.gitignore` | cherry-pick de `ba679c4` |
| M | `CMakeLists.txt` | cherry-pick de `ba679c4` |
| M | `CMakePresets.json` | cherry-pick de `ba679c4` |
| M | `src/event.h` | `refactor(event)` |
| M | `SKSE/Plugins/BehaviorDataInjector/SkipEquipAnimation_BDI.json` | `fix(bdi)` (item 14) |
| D | `cmake/headerlist.cmake` | `chore(cmake)` |
| D | `cmake/sourcelist.cmake` | `chore(cmake)` |
| A | `.gitattributes` | opcional |

**Proibido**: `TASKS.md`, `.claude/**`, `.specify/**`, `specs/**`, qualquer `src/*` além de `event.h`, e o `cmake/x64-windows-skse.cmake`, que já existe no upstream com conteúdo idêntico.

## C-PR-3: Commits e autoria

| Ordem | Mensagem | Autor |
| --- | --- | --- |
| 1 | `Improved the new build system.` + `(cherry picked from commit ba679c4…)` | AnArchos (original, preservado) |
| 2 | `refactor(event): drop redundant virtual on ProcessEvent override` | usuário |
| 3 | `chore(cmake): remove unused headerlist/sourcelist (sources are globbed)` | usuário |
| 4 | `fix(bdi): add missing colon in InstantEquipAnim entry` | usuário |
| 5 (opcional) | `chore(git): keep CRLF blobs as-is via .gitattributes` | usuário |

Os commits 2 a 5 terminam com `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

## C-PR-4: Fim de linha

- `git diff --stat upstream/main contrib/build-improvements` e a mesma comparação com `--ignore-cr-at-eol` listam os mesmos arquivos. Nenhum diff é só de EOL.

## C-PR-5: Build

- Com a branch em checkout, o build de `build/release-msvc` conclui com exit 0.

## C-PR-6: Descrição do PR (esboço)

```markdown
## Summary
- Brings over the build improvements from ba679c4 (custom `x64-windows-skse` overlay triplet,
  Ninja generator, `/OPT:REF` for Release/RelWithDebInfo, stop ignoring `*.cmake` so the triplet
  stays tracked). These were authored by AnArchos on the fork right after the 1.7.99
  build-system squash but never landed on upstream (authorship preserved via cherry-pick -x).
- Drops a redundant `virtual` on `ProcessEvent ... override` in `event.h`.
- Removes `cmake/headerlist.cmake` / `cmake/sourcelist.cmake`, unused since sources are globbed.
- (optional) Adds `.gitattributes` so CRLF blobs are stored as-is and `core.autocrlf=true`
  clients stop producing whole-file LF diffs.

## Test plan
- [ ] Configure + build `build-release-msvc` preset (Ninja, x64-windows-skse)
- [ ] In-game: skip / instant / normal equip, draw & sheath

🤖 Generated with [Claude Code](https://claude.com/claude-code)
```

A descrição não menciona nenhum `@login`, para evitar notificações indevidas. Se o usuário quiser marcar o mantenedor, isso é feito no momento de abrir o PR.

## C-PR-7: Publicação

- Nenhum `git push` da branch e nenhum `gh pr create` sem confirmação explícita do usuário no momento da execução.
