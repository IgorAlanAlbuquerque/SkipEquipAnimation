# Implementation Plan: Sincronização do fork com o upstream (PhiloSocio/SkipEquipAnimation)

**Branch**: `001-upstream-sync-merge` (diretório da feature; a execução acontece no `main`, por merge commit) | **Date**: 2026-11-17 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/001-upstream-sync-merge/spec.md`

**Note**: This template is filled in by the `/speckit-plan` command; its definition describes the execution workflow.

## Summary

O `upstream/main` (`22f1e3b`) é um squash do próprio fork até o `9866463`, gravado em CRLF e mantendo dois arquivos cmake mortos. Os 7 conflitos (4 ignorando EOL) vêm só do trabalho do fork posterior ao squash colidindo com ele.

**Abordagem**:

1. **Proteger**: criar um ponto de restauração do `main`, do staged e da DLL instalada no MO2.
2. **Separar o staged** em dois commits: a correção do hook (C1) e o tooling do Spec Kit (C2). Validar que o C1 compila.
3. **Fazer o merge** `--no-ff --no-commit` do `upstream/main` e resolver cada arquivo a partir de uma árvore conhecida:
   - adotar o upstream em `util.h`, `event.cpp`, BDI, `hook.*` e no triplet;
   - adotar o fork em `CMakePresets.json`;
   - `event.h` = upstream sem `virtual`;
   - aceitar o auto-merge em `CMakeLists.txt` e `.gitignore`.
4. **Registrar a convenção CRLF** com um `.gitattributes` `-text` restrito aos caminhos compartilhados.
5. **Preparar** `contrib/build-improvements` a partir do `upstream/main`: cherry-pick `-x ba679c4` (autor preservado) + `event.h` + remoção dos cmake mortos.
6. **Publicar** (push e PR) só com confirmação.

Os detalhes e as alternativas estão em [research.md](./research.md).

## Technical Context

**Language/Version**: C++23 (`cxx_std_23`), MSVC do Visual Studio 18 (via `vcvars64.bat`). Git 2.56.1 (Windows) com `core.autocrlf=true` na config de sistema.

**Primary Dependencies**: CommonLibSSE-NG (via `CommonLibSSEPath`), vcpkg (`D:\vcpkg`) com `directxtk`, `fmt`, `nlohmann-json`, `rapidcsv`, `simpleini`, `spdlog` e `xbyak`. CMake 3.21+ com presets e Ninja. Triplet overlay `x64-windows-skse`.

**Storage**: N/A. Os objetos manipulados são commits, blobs e refs do git, além da DLL e do PDB instalados no MO2.

**Testing**: não há testes automatizados no projeto. A validação combina:

- invariantes git verificáveis por comando ([quickstart.md](./quickstart.md) V0 a V6 e V8);
- build Release;
- teste manual no jogo (V7).

**Target Platform**: Skyrim SE/AE 1.7.99 (SKSE plugin `.dll`, Windows x64), carregado via Mod Organizer 2.

**Project Type**: SKSE plugin (DLL nativa). Esta feature é manutenção de repositório (merge e contribuição), sem código novo.

**Performance Goals**: N/A. Requisito: o rollback completo leva menos de 1 minuto (SC-006).

**Constraints**:

- sem reescrever o histórico do `main` e sem force-push (FR-010);
- nenhum push ou PR sem confirmação (FR-014);
- EOL CRLF byte a byte igual ao upstream (FR-009);
- cada build faz deploy automático para `D:\ModdingSkyrim\MO2\mods\Skip Equip Animations\SKSE\Plugins\` (research R6).

**Scale/Scope**: 1 merge (7 arquivos em conflito, 13 itens de inventário), 1 branch de contribuição com 3 ou 4 commits, cerca de 20 arquivos de produto envolvidos.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

`.specify/memory/constitution.md` **ainda não foi ratificada**: é o template com placeholders. Não há princípios formais para violar. Sem eles, os gates abaixo vêm dos requisitos da spec:

| Gate | Origem | Pré-pesquisa | Pós-design |
| --- | --- | --- | --- |
| G1. Nenhum trabalho perdido (staged + histórico) | FR-004, FR-005, SC-009 | ✅ planejado (R4) | ✅ PontoDeRestauração + rollback ([data-model.md](./data-model.md)) |
| G2. Sem reescrita de histórico | FR-010 | ✅ merge commit | ✅ C-MAIN-1 |
| G3. Nada publicado sem confirmação | FR-014 | ✅ | ✅ C-PR-7, transição `[Publicado]` só com confirmação |
| G4. O resultado compila | FR-008, SC-004 | ✅ | ✅ builds em 3 checkpoints (R6), C-MAIN-6 e C-PR-5 |
| G5. Sem diff só de EOL | FR-009, SC-005 | ✅ | ✅ C-MAIN-4, C-PR-4 e `.gitattributes` (R3) |
| G6. Artefatos do fork fora do PR | FR-013 | ✅ | ✅ allowlist C-PR-2 |
| G7. Efeitos externos reversíveis (deploy no MO2) | Extra, derivado do R6 | ⚠️ identificado na pesquisa | ✅ backup da DLL/PDB + confirmação antes de cada build |

**Resultado**: todos os gates passam e não há violações a justificar. Recomenda-se rodar `/speckit-constitution` depois desta feature para formalizar os princípios do projeto.

## Project Structure

### Documentation (this feature)

```text
specs/001-upstream-sync-merge/
├── spec.md                          # /speckit-specify (+ clarificações 2026-11-17)
├── plan.md                          # Este arquivo
├── research.md                      # Fase 0: R1 a R9
├── data-model.md                    # Fase 1: entidades + transições de estado + rollback
├── quickstart.md                    # Fase 1: validação V0 a V9
├── contracts/
│   ├── fork-main-after-sync.md      # Invariantes do main sincronizado (C-MAIN-1 a 6)
│   └── contribution-branch.md       # Invariantes da branch de PR (C-PR-1 a 7)
├── checklists/
│   └── requirements.md              # Qualidade da spec
└── tasks.md                         # Fase 2 (/speckit-tasks; não é criado aqui)
```

### Source Code (repository root)

Arquivos tocados pela sincronização. Nenhum arquivo novo de código.

```text
.gitattributes                         # NOVO: regras -text por caminho (R3)
.gitignore                             # auto-merge: sem *.cmake (ba679c4)
CMakeLists.txt                         # auto-merge: + /OPT:REF (ba679c4)
CMakePresets.json                      # fork: x64-windows-skse + Ninja (ba679c4, Q2 = A)
cmake/
├── x64-windows-skse.cmake             # blob do upstream (CRLF, conteúdo idêntico)
├── headerlist.cmake                   # removido (morto)
└── sourcelist.cmake                   # removido (morto)
SKSE/Plugins/BehaviorDataInjector/
└── SkipEquipAnimation_BDI.json        # blob do upstream (só EOL)
src/
├── event.h                            # blob do upstream − "virtual " (1 linha)
├── event.cpp                          # blob do upstream (só EOL)
├── hook.cpp                           # C1 (staged) → no merge, blob do upstream
├── hook.h                             # C1 (staged) → no merge, blob do upstream
└── util.h                             # blob do upstream (só EOL)
TASKS.md, .claude/**, .specify/**, specs/**   # só no fork, inalterados pelo merge
```

**Structure Decision**: o layout atual do repositório é mantido sem nenhuma reorganização. A tabela de resolução por arquivo do [research.md R2](./research.md#r2-método-de-resolução-do-merge) é a referência para a execução.

## Execution Phases (insumo para `/speckit-tasks`)

| Fase | Objetivo | Saída verificável | Stories |
| --- | --- | --- | --- |
| P0. Pré-condições | fetch, confirmar `22f1e3b`, working tree só com o staged esperado | V0 | — |
| P1. Ponto de restauração | `backup/pre-upstream-sync`, `backup/pre-upstream-sync-staged`, cópia da DLL/PDB do MO2 | V1 | US2 |
| P2. Separar o staged | C1 (`hook.*`) e C2 (Spec Kit + specs), com `core.autocrlf=false` nos adds | `git status` limpo | US2 |
| P3. Build de checkpoint do C1 | confirmar com o usuário (deploy no MO2), depois compilar | V6 no C1 | US3 |
| P4. Merge e resolução | `merge --no-ff --no-commit`, tabela R2, edição do `event.h`, commit do merge | V2, V3, V5 | US1, US2 |
| P5. Build pós-merge | confirmar, compilar, teste no jogo pelo usuário | V6, V7 | US3 |
| P6. Registrar EOL | commit do `.gitattributes` | V4 | US2 |
| P7. Branch de contribuição | `contrib/build-improvements`: cherry-pick `-x`, 2 commits (+1 opcional), build | V8 | US4 |
| P8. Publicação | **com confirmação**: push do `main` e do `contrib/*` para `origin`, `gh pr create` contra o upstream | link do PR | US4 |

**Pontos de parada obrigatórios**:

- P0, se o upstream tiver avançado;
- P3 e P5, se o build falhar;
- P8, que sempre exige confirmação.

## Complexity Tracking

Nenhuma violação a justificar. A constituição não foi ratificada e todos os gates derivados da spec passam.
