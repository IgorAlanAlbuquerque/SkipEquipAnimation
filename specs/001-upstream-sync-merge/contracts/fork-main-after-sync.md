# Contract: estado do `main` do fork após a sincronização

Invariantes que o `main` precisa satisfazer ao final das fases de sincronização e de registro da convenção de EOL. A verificação está no [quickstart.md](../quickstart.md).

## C-MAIN-1: Ancestralidade

- `git merge-base --is-ancestor upstream/main main` retorna 0.
- `git merge-base --is-ancestor backup/pre-upstream-sync main` retorna 0. Os 16 commits anteriores continuam acessíveis (SC-009).
- O commit de merge tem dois pais: o HEAD do fork anterior ao merge e o `22f1e3b`.

## C-MAIN-2: Zero conflitos

- `git grep -nE '^(<<<<<<<|>>>>>>>)( |$)'` não encontra nada (SC-002).
- `git merge-tree --write-tree main upstream/main` retorna exit 0 (SC-003).

## C-MAIN-3: Diff permitido contra o `upstream/main`

`git diff --stat upstream/main main` lista **exatamente** estes caminhos e nenhum outro:

| Caminho | Diferença esperada | Inventário |
| --- | --- | --- |
| `.gitattributes` | arquivo novo (regras `-text` do research R3) | 10 |
| `.gitignore` | remove a linha `*.cmake` | 7 |
| `CMakeLists.txt` | adiciona o bloco `if(MSVC) target_link_options(... /OPT:REF ...)` | 6 |
| `CMakePresets.json` | triplet `x64-windows-skse` (target e host) + `VCPKG_OVERLAY_TRIPLETS`; gerador `Ninja` nos presets Debug e Release | 5 |
| `cmake/headerlist.cmake` | removido | 9 |
| `cmake/sourcelist.cmake` | removido | 9 |
| `src/event.h` | 1 linha: sem `virtual ` antes de `EventChecker ProcessEvent(` | 4 |
| `SKSE/Plugins/BehaviorDataInjector/SkipEquipAnimation_BDI.json` | 1 linha: `"name": "InstantEquipAnim"`, com o `:` que faltava | 14 |
| `TASKS.md` | só no fork | 11 |
| `.claude/settings.json`, `.claude/skills/**` | só no fork | 12 |
| `.specify/**` | só no fork | 12 |
| `specs/**` | só no fork | 12 |

**Proibido no diff**:
- `src/hook.cpp`, `src/hook.h`, `src/util.h`, `src/event.cpp`, `src/plugin.cpp`, `src/PCH.h`;
- `cmake/x64-windows-skse.cmake`, `SKSE/**` (exceto a linha do item 14), `README.md`, `vcpkg*.json`.

Qualquer um destes no diff indica que a resolução não seguiu o research R2.

## C-MAIN-4: Fim de linha

- `git diff --ignore-cr-at-eol --stat upstream/main main` e `git diff --stat upstream/main main` listam **os mesmos** arquivos. Ou seja, não existe diff só de EOL (SC-005).
- Os blobs de todos os caminhos compartilhados com o upstream estão em CRLF. O `.gitignore` está em LF.
- Depois do commit do `.gitattributes`, `git status --porcelain` volta vazio.

## C-MAIN-5: Conteúdo ausente

- Nenhuma ocorrência de `LogGraphState`, `g_equipId`, `NotifyAnimationGraph_Hook` ou `SM_Update_Hook` em `src/` (FR-007).
- Nenhuma ocorrência de `vtbl.write_vfunc` em `src/hook.h` (FR-008).

## C-MAIN-6: Build

- O build de `build/release-msvc` conclui com exit 0 e gera `SkipEquipAnimation.dll` (SC-004).
