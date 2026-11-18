---

description: "Tarefas executáveis para sincronizar o fork com PhiloSocio/SkipEquipAnimation:main"
---

# Tasks: Sincronização do fork com o upstream (PhiloSocio/SkipEquipAnimation)

**Input**: Design documents from `/specs/001-upstream-sync-merge/`

**Prerequisites**: [plan.md](./plan.md), [spec.md](./spec.md), [research.md](./research.md), [data-model.md](./data-model.md), [contracts/](./contracts/), [quickstart.md](./quickstart.md)

**Tests**: a spec não pede testes automatizados e o projeto não tem nenhum. A validação é feita pelos comandos de verificação do [quickstart.md](./quickstart.md) (V0 a V9), embutidos nas tarefas, mais o teste manual no jogo.

**Organization**: as tarefas estão agrupadas por user story. Como tudo opera sobre o mesmo índice git, **nenhuma operação git roda em paralelo**. `[P]` só marca tarefas fora do git (cópia de arquivos, escrita de docs) ou leituras puras.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: pode rodar em paralelo (arquivos diferentes, sem dependência e sem escrita no índice git)
- **[Story]**: US1 a US4 da [spec.md](./spec.md)

## Convenções de execução (valem para todas as tarefas)

- **Shell**: PowerShell, a partir de `D:\repositorios\SkipEquipAnimation`.
- **Adicionar arquivos compartilhados com o upstream** (`src/**`, `cmake/**`, `SKSE/**`, `CMakeLists.txt`, `CMakePresets.json`, `vcpkg*.json`, `README.md`): sempre com `git -c core.autocrlf=false add <arquivo>` ([research.md R3](./research.md)).
- **Mensagens de commit**: todo commit criado aqui termina com uma linha em branco seguida de `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Para mensagens de várias linhas, use um here-string `@'…'@` com o `'@` de fechamento na coluna 0.
- **Build**: `cmd /c build_temp.bat` (vcvars64 do VS 18 + `cmake --build build\release-msvc`). **Ele faz deploy automático** em `D:\ModdingSkyrim\MO2\mods\Skip Equip Animations\SKSE\Plugins\`, então peça confirmação ao usuário antes de cada build.
  - **Ajuste descoberto no T003 (2026-11-17)**: o VS 18 está em `D:\Program Files\Microsoft Visual Studio\18\Community`, mas o `build_temp.bat` (local e não versionado) aponta para `C:\`. Nos builds, use o comando equivalente:
    ```powershell
    cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul && cmake --build build\release-msvc'
    ```
- **⛔ STOP**: na falha de uma verificação marcada com ⛔, pare a execução, relate a saída ao usuário e não prossiga.
- **Publicação**: nenhum `git push` e nenhuma abertura de PR sem confirmação explícita do usuário **no momento** da tarefa (FR-014).

---

## Phase 1: Setup (pré-condições, P0)

**Purpose**: garantir que o estado analisado na spec ainda é o estado real.

- [X] T001 Atualizar o upstream e confirmar o HEAD analisado: rodar `git fetch upstream` e depois `git rev-parse upstream/main`. ⛔ O resultado precisa começar com `22f1e3b`. Se não começar, o upstream avançou e o inventário em `specs/001-upstream-sync-merge/spec.md` precisa ser refeito antes de qualquer outra tarefa (research R9).
- [X] T002 Confirmar o HEAD e o working tree: `git rev-parse --short main` precisa ser `25b0006`, e `git branch --show-current` precisa ser `main`. ⛔ Em `git status --porcelain=v1`, toda linha precisa ser uma destas:
  - `M  src/hook.cpp` ou `M  src/hook.h`, ambos staged e sem modificação no working tree (a 2ª coluna em branco);
  - um caminho sob `.claude/skills/`, `.specify/` ou `specs/001-upstream-sync-merge/`, em qualquer status.

  Qualquer outra linha indica trabalho não previsto.
- [X] T003 [P] Confirmar o ambiente de build (só leitura):
  - `Test-Path "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"` precisa retornar `True`;
  - `build\release-msvc\CMakeCache.txt` precisa conter `CMAKE_GENERATOR:INTERNAL=Ninja` e `VCPKG_TARGET_TRIPLET:STRING=x64-windows-skse`;
  - registrar o valor de `$env:SKYRIM_MODS_FOLDER`. O esperado é `D:\ModdingSkyrim\MO2\mods`.

---

## Phase 2: Foundational (backup + separar o staged + checkpoint do C1)

**Purpose**: ponto de restauração (FR-004, FR-005) e staged commitado de forma limpa. **Bloqueia todas as user stories.**

**⚠️ CRITICAL**: nenhuma tarefa de US1 a US4 começa antes do T010 passar.

- [X] T004 Criar a branch de backup do `main`: `git branch backup/pre-upstream-sync 25b0006`. ⛔ Se a branch já existir, pare e pergunte ao usuário (não sobrescreva). Verificar com `git rev-parse --short backup/pre-upstream-sync`, que deve retornar `25b0006`.
- [X] T005 Capturar o índice e o working tree sem alterá-los:
  - rodar `$s = git stash create "pre-upstream-sync: staged hook fix + Spec Kit"`. ⛔ `$s` não pode ser vazio;
  - rodar `git branch backup/pre-upstream-sync-staged $s`;
  - verificar que `git cat-file -t backup/pre-upstream-sync-staged` retorna `commit` e que `git status --porcelain=v1` continua idêntico ao do T002.
- [X] T006 [P] Fazer backup dos arquivos instalados no MO2. Criar `build\backup-pre-upstream-sync\` (ignorado pelo git via `/build*`) e copiar para lá estes três arquivos, de `D:\ModdingSkyrim\MO2\mods\Skip Equip Animations\SKSE\Plugins\`:
  - `SkipEquipAnimation.dll`
  - `SkipEquipAnimation.pdb`
  - `BehaviorDataInjector\SkipEquipAnimation_BDI.json`

  Verificar que os tamanhos batem com os originais (a DLL tem 559104 bytes). Avisar o usuário de que esse backup some se a pasta `build/` for apagada.
- [X] T007 Isolar o C1 no índice: rodar `git restore --staged .claude/skills .specify specs`. Depois, `git diff --cached --name-only` precisa listar **só** `src/hook.cpp` e `src/hook.h`.
- [X] T008 Criar o commit C1 com `src/hook.cpp` e `src/hook.h`. Mensagem:
  ```text
  fix(hook): remove investigation logging and fix undeclared vtbl

  - Drop LogGraphState, the equip ID counter and the NotifyAnimationGraph /
    hkbStateMachine::Update investigation hooks added in d242488
  - Install Activate/Update/Deactivate hooks through hkbClipGeneratorVtbl
    (d242488 referenced an undeclared `vtbl`, breaking the build)

  Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
  ```
  Verificar com `git show --stat HEAD`, que deve mostrar só os 2 arquivos.
- [X] T009 Criar o commit C2 com o tooling e as specs:
  - rodar `git add .claude/skills .specify specs`. O `.specify/feature.json` continua ignorado pelo `.specify/.gitignore`, o que é correto;
  - verificar que `git diff --cached --name-only` só contém caminhos sob `.claude/skills/`, `.specify/` e `specs/`;
  - commit com a mensagem:
  ```text
  chore(tooling): add Spec Kit scaffolding and upstream-sync spec

  - Spec Kit 1.4.0 (Claude integration, PowerShell scripts)
  - specs/001-upstream-sync-merge: spec, plan, research, contracts, tasks

  Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
  ```
  Depois, `git status --porcelain=v1` precisa estar vazio.
- [X] T010 Build de checkpoint do C1 (research R6, gate G4):
  1. **pedir confirmação ao usuário** para o deploy no MO2;
  2. rodar `cmd /c build_temp.bat`. ⛔ O exit code precisa ser 0;
  3. verificar que `build\release-msvc\SkipEquipAnimation.dll` tem `LastWriteTime` atual.

  Se falhar, a correção staged do `vtbl` não foi suficiente: pare e relate o erro de compilação.

  > **Execução 2026-11-17**:
  > - O usuário liberou os 3 builds (T010, T025, T033) com deploy no MO2.
  > - A primeira tentativa falhou **antes de compilar**. O `build/release-msvc` havia sido configurado com o VS 18 em `C:\`, mas o VS agora está em `D:\`, e o cache apontava para um ninja e um linker inexistentes.
  > - Correção: backup do cache antigo em `build/backup-pre-upstream-sync/CMakeCache.release-msvc.txt` e `cmake --preset build-release-msvc --fresh`, que levou 154 s, incluindo o rebuild das 20 dependências do vcpkg. O resultado foi exit 0, com MSVC 14.52 e o deploy continuando em `MO2/mods/Skip Equip Animations`.
  > - T011 e T012, que são só leitura, foram adiantados durante o configure. O T013 (commit) esperou o T010.

**Checkpoint**: backup completo, staged preservado em C1 e C2, e o C1 compila.

---

## Phase 3: User Story 1: análise classificada validada (Priority: P1) 🎯 MVP

**Goal**: confirmar no estado real (depois de C1 e C2) que o inventário de 13 itens da spec cobre toda a divergência (FR-001 a FR-003, SC-001).

**Independent Test**: todo caminho listado em `git diff --name-only upstream/main main` corresponde a uma linha do inventário em `specs/001-upstream-sync-merge/spec.md` (seção Key Entities).

- [X] T011 [US1] Simular o merge sem alterar nada: `git merge-tree --write-tree --name-only --messages main upstream/main`. ⛔ Os arquivos em conflito precisam ser exatamente estes 7 (`src/hook.*` continuam em conflito mesmo depois do C1, porque o histórico diverge):
  - `CMakePresets.json`
  - `SKSE/Plugins/BehaviorDataInjector/SkipEquipAnimation_BDI.json`
  - `src/event.cpp`
  - `src/event.h`
  - `src/hook.cpp`
  - `src/hook.h`
  - `src/util.h`
- [X] T012 [P] [US1] Levantar a divergência ignorando EOL: `git diff -w --ignore-cr-at-eol --name-status upstream/main main`. Mapear cada caminho para um item do inventário, usando o número da tabela da spec:
  - `src/hook.cpp` → item 3, só formatação do lambda de `std::jthread`;
  - `src/event.h` → item 4;
  - `CMakePresets.json` → item 5;
  - `CMakeLists.txt` → item 6;
  - `.gitignore` → item 7;
  - `cmake/headerlist.cmake` e `cmake/sourcelist.cmake` → item 9;
  - `TASKS.md` → item 11;
  - `.claude/**`, `.specify/**` e `specs/**` → item 12.

  ⛔ Se algum caminho não tiver mapeamento, é uma divergência nova: pare e adicione a linha ao inventário antes de prosseguir.
- [X] T013 [US1] Registrar a validação em `specs/001-upstream-sync-merge/spec.md`: adicionar, ao final da seção "Contexto e diagnóstico", o bullet `- **Inventário validado em <data>** contra main=<sha do C2> e upstream/main=22f1e3b: 7 conflitos confirmados; 100% dos caminhos mapeados.` Commitar só esse arquivo com a mensagem `docs(specs): record inventory validation` + a linha Co-Authored-By.

**Checkpoint**: a análise pedida pelo usuário está comprovada contra o estado real.

---

## Phase 4: User Story 2: fork sincronizado e sem conflitos (Priority: P1)

**Goal**: o `upstream/main` passa a ser ancestral do `main`, com a resolução da tabela do [research.md R2](./research.md) e o diff final igual a [contracts/fork-main-after-sync.md](./contracts/fork-main-after-sync.md) (FR-006, FR-007, FR-009, FR-010, FR-011).

**Independent Test**: [quickstart.md](./quickstart.md) V2, V3, V4 e V5 passam.

- [X] T014 [US2] Iniciar o merge sem commit: `git merge --no-ff --no-commit upstream/main`. O exit 1 com conflitos é **esperado**. Em seguida, `git diff --name-only --diff-filter=U` precisa listar os mesmos 7 arquivos do T011.
- [X] T015 [US2] Adotar os blobs do upstream (CRLF) nos arquivos de conteúdo idêntico e no `hook.*`:
  ```powershell
  git checkout upstream/main -- src/util.h src/event.cpp src/hook.cpp src/hook.h cmake/x64-windows-skse.cmake "SKSE/Plugins/BehaviorDataInjector/SkipEquipAnimation_BDI.json"
  ```
  Depois, `git diff --cached --name-only upstream/main -- src/util.h src/event.cpp src/hook.cpp src/hook.h cmake/x64-windows-skse.cmake SKSE` precisa retornar vazio.
- [X] T016 [US2] Manter o `CMakePresets.json` do fork (Q2 = A): `git checkout HEAD -- CMakePresets.json`. Em `git diff --cached upstream/main -- CMakePresets.json`, as únicas mudanças devem ser:
  - `VCPKG_TARGET_TRIPLET` e `VCPKG_HOST_TRIPLET`, de `x64-windows-static-md` para `x64-windows-skse`;
  - `VCPKG_OVERLAY_TRIPLETS: ${sourceDir}/cmake` e uma linha em branco, ambos acrescentados;
  - `"generator"`, de `Visual Studio 17 2022` para `Ninja`, nos presets Debug e Release.
- [X] T017 [US2] Resolver o `src/event.h` como blob do upstream menos `virtual `:
  1. rodar `git checkout upstream/main -- src/event.h`;
  2. editar `src/event.h` **preservando CRLF e BOM**. A edição é feita em bytes, via PowerShell `[IO.File]::ReadAllBytes` → substituir a sequência ASCII `virtual EventChecker ProcessEvent(` por `EventChecker ProcessEvent(` → `[IO.File]::WriteAllBytes`. Não use o editor de texto, que pode converter o EOL;
  3. rodar `git -c core.autocrlf=false add src/event.h`.

  ⛔ `git diff --cached --stat upstream/main -- src/event.h` precisa mostrar `1 insertion(+), 1 deletion(-)`.
- [X] T018 [US2] Conferir os auto-merges. `git diff --cached upstream/main -- CMakeLists.txt .gitignore` precisa mostrar só:
  - em `CMakeLists.txt`, o bloco acrescentado `if(MSVC) target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:RelWithDebInfo>:/OPT:REF>) endif()`;
  - em `.gitignore`, a remoção da linha `*.cmake`.

  Além disso, `git ls-files cmake/` precisa listar só `cmake/x64-windows-skse.cmake`, já que `headerlist.cmake` e `sourcelist.cmake` continuam removidos.
- [X] T019 [US2] Fechar a resolução:
  - `git diff --name-only --diff-filter=U` precisa estar vazio;
  - `git grep -nE '^(<<<<<<<|>>>>>>>)( |$)'` precisa ter exit 1;
  - ⛔ `git diff --cached --name-status upstream/main` precisa listar **só** os caminhos da tabela C-MAIN-3 de [contracts/fork-main-after-sync.md](./contracts/fork-main-after-sync.md), exceto `.gitattributes`, que ainda não existe.
- [X] T020 [US2] Criar o commit do merge:
  ```text
  Merge upstream/main (22f1e3b) into fork main

  upstream 22f1e3b is a squash of this fork up to 9866463 (build system
  for 1.7.99). Resolution, per specs/001-upstream-sync-merge/research.md R2:
  - adopt upstream blobs (CRLF) for util.h, event.cpp, hook.*, BDI json and
    cmake/x64-windows-skse.cmake (content identical or formatting-only)
  - keep fork CMakePresets.json: x64-windows-skse overlay triplet + Ninja (ba679c4)
  - keep fork event.h without the redundant `virtual`
  - keep /OPT:REF (CMakeLists.txt) and stop ignoring *.cmake (.gitignore)
  - keep cmake/headerlist.cmake and sourcelist.cmake removed (sources are globbed)

  Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
  ```
  Verificar que `git log -1 --format=%P` mostra 2 pais: o C2 (ou o commit do T013) e o `22f1e3b…`.
- [X] T021 [US2] Rodar as verificações V2 e V5 do [quickstart.md](./quickstart.md). ⛔ Todas precisam passar:
  - `git merge-base --is-ancestor upstream/main main` → exit 0;
  - `git merge-base --is-ancestor backup/pre-upstream-sync main` → exit 0;
  - `git merge-tree --write-tree main upstream/main` → exit 0;
  - `git grep -nE 'LogGraphState|g_equipId|NotifyAnimationGraph_Hook|SM_Update_Hook' -- src` → exit 1;
  - `git grep -n 'vtbl\.write_vfunc' -- src/hook.h` → exit 1.
- [X] T022 [US2] Pré-checagem de EOL antes do `.gitattributes`: rodar `git ls-files --eol src cmake SKSE CMakeLists.txt CMakePresets.json README.md vcpkg.json vcpkg-configuration.json`. ⛔ Todas as linhas precisam ter `i/crlf`. Uma linha `i/lf` aqui faria o `.gitattributes` marcar esse arquivo como modificado.
- [X] T023 [US2] Criar `.gitattributes` na raiz, com LF, com exatamente o conteúdo de [research.md R3](./research.md):
  ```gitattributes
  /src/**                   -text
  /cmake/**                 -text
  /SKSE/**                  -text
  /CMakeLists.txt           -text
  /CMakePresets.json        -text
  /vcpkg.json               -text
  /vcpkg-configuration.json -text
  /README.md                -text
  ```
  ⛔ `git status --porcelain=v1` precisa mostrar **só** `?? .gitattributes`. Depois:
  - rodar `git add .gitattributes`;
  - commitar com a mensagem `chore(git): store upstream-shared files as-is (CRLF) via .gitattributes` + a linha em branco + Co-Authored-By.
- [X] T024 [US2] Rodar as verificações finais V3 e V4 do [quickstart.md](./quickstart.md). ⛔ Tudo precisa bater:
  - `git diff --name-status upstream/main main` igual à tabela C-MAIN-3, agora incluindo `A .gitattributes`;
  - as contagens de `git diff --name-only upstream/main main` e `git diff --ignore-cr-at-eol --name-only upstream/main main` iguais;
  - `git ls-files --eol` com `attr/-text` nos caminhos compartilhados;
  - `git status --porcelain=v1` vazio.

**Checkpoint**: o `main` está sincronizado localmente. Nada foi publicado.

---

## Phase 5: User Story 3: resultado compila e mantém o comportamento (Priority: P2)

**Goal**: FR-008, SC-004 e SC-007 no `main` sincronizado.

**Independent Test**: V6 (build) e V7 (no jogo) do [quickstart.md](./quickstart.md).

- [X] T025 [US3] Build do `main` sincronizado:
  1. **pedir confirmação ao usuário** para o deploy no MO2;
  2. rodar `cmd /c build_temp.bat`. ⛔ O exit code precisa ser 0;
  3. confirmar que `D:\ModdingSkyrim\MO2\mods\Skip Equip Animations\SKSE\Plugins\SkipEquipAnimation.dll` tem `LastWriteTime` atual.
- [X] T026 [US3] Teste no jogo, manual e feito pelo usuário. Pedir que ele rode os 5 cenários da tabela V7 do [quickstart.md](./quickstart.md): skip, InstantEquipAnim, draw/sheath, magia e modo desativado. Ele deve responder OK ou FALHA para cada um. Registrar o resultado como uma linha em `specs/001-upstream-sync-merge/quickstart.md`, logo abaixo da tabela V7: `Resultado <data>: skip=…, instant=…, draw/sheath=…, magia=…, desativado=…`. ⛔ Qualquer FALHA interrompe a execução antes da US4. A restauração da DLL anterior usa os arquivos de `build\backup-pre-upstream-sync\` (T006).

  > **⏸ Pausado em 2026-11-17**: o usuário escolheu "testar depois". A DLL do `main` sincronizado (`6cf8719`, 590.848 bytes, build 21:59) já está instalada no MO2. **A execução retoma aqui**: coletar o resultado dos 5 cenários antes de iniciar o T027.
  >
  > **▶ Retomado em 2026-11-17**: o usuário confirmou os 5 cenários OK, e o resultado foi registrado no quickstart V7.

- [X] T042 [US3] **(adicionada em 2026-11-17, item 14 do inventário; executar logo após o T026)** Commitar no `main` a correção do `SKSE/Plugins/BehaviorDataInjector/SkipEquipAnimation_BDI.json`, que foi staged pelo usuário e troca `"name" "InstantEquipAnim"` por `"name": "InstantEquipAnim"`:
  - ⛔ `git diff --cached --stat` precisa mostrar só esse arquivo, com `1 insertion(+), 1 deletion(-)`;
  - o JSON precisa ser válido (`ConvertFrom-Json` sem erro), e `git ls-files --eol` precisa mostrar `i/crlf attr/-text`;
  - commitar com a mensagem `fix(bdi): add missing colon in InstantEquipAnim entry` + Co-Authored-By.

**Checkpoint**: o `main` sincronizado compila e foi validado no jogo.

---

## Phase 6: User Story 4: branch de contribuição ao upstream (Priority: P3)

**Goal**: `contrib/build-improvements` conforme [contracts/contribution-branch.md](./contracts/contribution-branch.md) (FR-012, FR-012a, FR-013).

**Independent Test**: V8 do [quickstart.md](./quickstart.md).

- [X] T027 [US4] Criar a branch a partir do upstream: `git switch -c contrib/build-improvements upstream/main`. ⛔ `git rev-parse HEAD` precisa ser igual a `git rev-parse upstream/main`.
- [X] T028 [US4] Fazer o cherry-pick do `ba679c4` preservando o autor: `git cherry-pick -x ba679c4`. Espera-se um conflito add/add em `cmake/x64-windows-skse.cmake`, onde só o EOL difere. Resolver assim:
  1. `git checkout HEAD -- cmake/x64-windows-skse.cmake`, que mantém o blob CRLF do upstream;
  2. se `CMakePresets.json`, `CMakeLists.txt` ou `.gitignore` também conflitarem, resolver cada um com `git checkout main -- <arquivo>`. A versão do `main` é exatamente upstream + `ba679c4`;
  3. `git cherry-pick --continue --no-edit`.

  ⛔ `git log -1 --format='%an'` precisa retornar `AnArchos`, e a mensagem precisa conter `(cherry picked from commit ba679c4`.
- [X] T029 [US4] Criar o commit do `event.h`:
  - rodar `git checkout main -- src/event.h`, que já é o blob do upstream menos `virtual`, em CRLF;
  - commitar com a mensagem `refactor(event): drop redundant virtual on ProcessEvent override` + Co-Authored-By;
  - ⛔ `git show --stat HEAD` precisa mostrar `1 insertion(+), 1 deletion(-)`.
- [X] T030 [US4] Criar o commit dos cmake mortos:
  - rodar `git rm cmake/headerlist.cmake cmake/sourcelist.cmake`;
  - commitar com a mensagem `chore(cmake): remove unused headerlist/sourcelist (sources are globbed)` + Co-Authored-By.
- [X] T043 [US4] **(adicionada em 2026-11-17, item 14; executar logo após o T030)** Levar a correção do BDI para a branch de contribuição:
  - rodar `git checkout main -- SKSE/Plugins/BehaviorDataInjector/SkipEquipAnimation_BDI.json`;
  - commitar com a mensagem `fix(bdi): add missing colon in InstantEquipAnim entry` + Co-Authored-By;
  - ⛔ `git show --stat HEAD` precisa mostrar `1 insertion(+), 1 deletion(-)`.
- [X] T031 [US4] Commit opcional: **perguntar ao usuário** se o `.gitattributes` entra no PR (**decidido em 2026-11-17: incluir**). Se sim:
  - rodar `git checkout main -- .gitattributes`;
  - commitar com a mensagem `chore(git): store CRLF blobs as-is via .gitattributes` + Co-Authored-By.
- [X] T032 [US4] Rodar as verificações V8 do [quickstart.md](./quickstart.md). ⛔ Tudo precisa bater:
  - `git diff --name-status upstream/main contrib/build-improvements` contém só a allowlist C-PR-2: `M .gitignore`, `M CMakeLists.txt`, `M CMakePresets.json`, `M src/event.h`, `M SKSE/Plugins/BehaviorDataInjector/SkipEquipAnimation_BDI.json`, `D cmake/headerlist.cmake`, `D cmake/sourcelist.cmake` e, se o T031 foi feito, `A .gitattributes`;
  - as contagens com e sem `--ignore-cr-at-eol` são iguais;
  - `git log --format='%h %an | %s' upstream/main..contrib/build-improvements` lista 4 commits (5 com o T031), com o primeiro de autoria AnArchos.
- [X] T033 [US4] Build na branch de contribuição:
  1. **pedir confirmação ao usuário** para o deploy;
  2. rodar `cmd /c build_temp.bat`. ⛔ O exit code precisa ser 0.

  O código em `src/` é idêntico ao do `main`, então a DLL instalada continua equivalente à validada no T026.
- [X] T034 [P] [US4] Criar `specs/001-upstream-sync-merge/pr-description.md` com o esboço da C-PR-6 de [contracts/contribution-branch.md](./contracts/contribution-branch.md):
  - remover o bullet do `.gitattributes` se o T031 não foi feito;
  - não incluir nenhum `@login`;
  - terminar com a linha `🤖 Generated with [Claude Code](https://claude.com/claude-code)`.
- [X] T035 [US4] Voltar para o `main` (`git switch main`) e commitar o `pr-description.md` com a mensagem `docs(specs): add upstream PR description` + Co-Authored-By. Depois, `git status --porcelain=v1` precisa estar vazio.

**Checkpoint**: a contribuição está pronta localmente e não foi publicada.

---

## Phase 7: Polish & Publicação (com confirmação)

**Purpose**: publicar (FR-014), registrar o estado final e validar o rollback.

- [X] T036 Validar o rollback (SC-006) sem tocar no `main`:
  1. `git worktree add ..\SkipEquipAnimation-rollback-test backup/pre-upstream-sync`;
  2. dentro dele, `git stash apply --index backup/pre-upstream-sync-staged`;
  3. `git status --porcelain=v1` precisa mostrar o staged original: `M  src/hook.cpp`, `M  src/hook.h`, mais os arquivos de `.claude/skills/`, `.specify/` e `specs/` do momento do T005;
  4. medir o tempo total, que precisa ficar abaixo de 1 minuto;
  5. remover com `git worktree remove --force ..\SkipEquipAnimation-rollback-test`.
- [X] T037 Atualizar a documentação da feature:
  - em `specs/001-upstream-sync-merge/spec.md`, trocar `**Status**: Draft` por `**Status**: Implemented (<data>)`;
  - marcar todas as tarefas concluídas neste `specs/001-upstream-sync-merge/tasks.md`;
  - commitar com a mensagem `docs(specs): mark upstream sync as implemented` + Co-Authored-By.
- [ ] T038 **Pedir confirmação explícita** e só então publicar o `main` no fork: `git push origin main`. É fast-forward a partir de `origin/main` = `25b0006` e **não** usa `--force`. ⛔ Se o push for rejeitado, pare. Não force.
- [ ] T039 **Pedir confirmação explícita** e só então publicar a branch de contribuição: `git push -u origin contrib/build-improvements`.
- [ ] T040 Abrir o PR. O `gh` CLI **não está instalado** neste ambiente, então:
  1. entregar ao usuário a URL de comparação `https://github.com/PhiloSocio/SkipEquipAnimation/compare/main...IgorAlanAlbuquerque:SkipEquipAnimation:contrib/build-improvements?expand=1`;
  2. sugerir o título `Build improvements: x64-windows-skse triplet, Ninja, /OPT:REF`;
  3. indicar `specs/001-upstream-sync-merge/pr-description.md` como corpo do PR, a ser colado no formulário.

  Se o usuário instalar e autenticar o `gh` antes, então, **só com confirmação**, rodar:
  ```powershell
  gh pr create --repo PhiloSocio/SkipEquipAnimation --base main --head IgorAlanAlbuquerque:contrib/build-improvements --title "<título>" --body-file specs/001-upstream-sync-merge/pr-description.md
  ```
- [ ] T041 [P] Lembrar o usuário dos backups, **sem apagá-los**:
  - as branches `backup/pre-upstream-sync` e `backup/pre-upstream-sync-staged`, que são locais e não foram publicadas;
  - a pasta `build\backup-pre-upstream-sync\`.

  Remover só se ele pedir explicitamente.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (T001 a T003)**: sem dependências. O T001 é gate de tudo.
- **Foundational (T004 a T010)**: depende do Setup e **bloqueia todas as stories**. A ordem interna é estrita: T004 → T005 → T007 → T008 → T009 → T010. O T006 pode rodar em paralelo com T004 e T005.
- **US1 (T011 a T013)**: depende do T010.
- **US2 (T014 a T024)**: depende do T013, porque o inventário precisa estar validado antes de resolver. A ordem interna é estrita.
- **US3 (T025 e T026)**: depende do T024.
- **US4 (T027 a T035)**: depende do T026, que garante que só se propõe ao upstream algo validado no jogo. O T034 pode rodar em paralelo com T027 a T033.
- **Polish (T036 a T041)**: depende do T035. A ordem é T036 → T037 → T038 → T039 → T040. O T041 vem por último ou em paralelo.

### User Story Dependencies

```text
Setup → Foundational → US1 → US2 → US3 → US4 → Publicação
```

As stories são **sequenciais por natureza**: todas operam sobre o mesmo repositório, e cada uma valida a anterior. A "independência" de cada uma está no teste, já que cada checkpoint tem verificações próprias no quickstart, e não na possibilidade de rodar em paralelo.

### Parallel Opportunities

- T003 (leitura do ambiente) ∥ T001 e T002.
- T006 (cópia de arquivos do MO2, fora do git) ∥ T004 e T005.
- T012 (diff só-leitura) ∥ T011.
- T034 (escrita do `pr-description.md`) ∥ T027 a T033.
- T041 ∥ T040.

---

## Parallel Example: Foundational + US4

```text
# Enquanto as refs de backup são criadas (T004, T005), em paralelo:
Task: "T006 Copiar DLL/PDB/BDI do MO2 para build\backup-pre-upstream-sync\"

# Enquanto a branch de contribuição é montada (T027–T033), em paralelo:
Task: "T034 Criar specs/001-upstream-sync-merge/pr-description.md a partir da C-PR-6"
```

---

## Implementation Strategy

### MVP First

1. Setup + Foundational (T001 a T010): backup e staged commitado. O C1 compila.
2. US1 (T011 a T013): inventário validado. **Essa é a análise pedida pelo usuário.**
3. **PARAR e VALIDAR** com o usuário antes de iniciar o merge, se ele quiser revisar.

### Incremental Delivery

1. MVP → US2 (merge local) → US3 (build e jogo) → US4 (branch de PR) → Publicação.
2. Cada fase termina num checkpoint verificável e **nada sai da máquina** antes do T038.
3. Rollback em qualquer ponto antes do T038 segue o procedimento descrito em [data-model.md](./data-model.md#transições-de-estado-da-sincronização).

---

## Notes

- `[P]` aqui **nunca** se aplica a duas escritas no índice git ao mesmo tempo. Isso evita conflitos de `index.lock`.
- O inventário e as justificativas estão em [spec.md](./spec.md) (Key Entities), e a tabela de resolução por arquivo está em [research.md R2](./research.md).
- Se uma verificação ⛔ falhar, **não improvise uma correção**: relate ao usuário com a saída do comando.
