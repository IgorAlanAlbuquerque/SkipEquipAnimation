# Research: Sincronização do fork com o upstream

**Feature**: [spec.md](./spec.md) | **Plan**: [plan.md](./plan.md) | **Date**: 2026-11-17

Todas as conclusões abaixo foram verificadas no repositório local depois de `git fetch upstream` (HEAD do upstream = `22f1e3b`).

---

## R1. Natureza do commit do upstream

- **Decision**: tratar o `22f1e3b` (upstream) como **squash equivalente** ao `9866463` (fork), mais duas diferenças: (a) mantém `cmake/headerlist.cmake` e `cmake/sourcelist.cmake`; (b) grava os arquivos em CRLF.
- **Rationale**:
  - `git diff -w --ignore-cr-at-eol 9866463 22f1e3b` só lista os 3 arquivos `cmake/*`.
  - Os dois commits têm o mesmo autor (AnArchos `<…+PhiloSocio@users.noreply.github.com>`), o mesmo título e 2 minutos de diferença.
- **Consequence**: nenhum conteúdo de lógica precisa ser "trazido" do upstream. Os conflitos são todos causados pelos commits do fork **posteriores** ao `9866463` (`ba679c4`, `d242488`, `25b0006` e o staged) colidindo com o squash, que o git não reconhece como equivalente.
- **Alternatives considered**: tratar o upstream como trabalho independente e fazer uma revisão linha a linha completa. Foi descartado porque a equivalência foi comprovada e esse trabalho seria redundante.

## R2. Método de resolução do merge

- **Decision**: `git merge --no-ff --no-commit upstream/main` **sem** `-X ignore-space-at-eol`. Cada arquivo divergente é resolvido **explicitamente a partir de uma árvore conhecida** (`git checkout upstream/main -- <f>` ou `git checkout HEAD -- <f>`), com uma edição manual pontual só no `src/event.h`.
- **Rationale**:
  - O `git checkout <tree> -- <f>` grava no índice o blob exato da árvore escolhida, sem conversão de EOL, e resolve o conflito daquele caminho.
  - Com `-X ignore-space-at-eol`, o EOL do resultado fica indefinido e pode ficar misto dentro de um mesmo arquivo, o que viola o FR-009.
- **Alternatives considered**:
  - `-X ignore-space-at-eol`: reduz de 7 para 4 conflitos, mas tem o problema de EOL descrito acima.
  - `-X theirs` global: descartaria o `/OPT:REF`, o `.gitignore` e o preset do fork.
  - `-X ours` global: perderia o EOL do upstream nos arquivos de conteúdo idêntico.

### Tabela de resolução por arquivo

| Arquivo | Estado no merge simulado | Resolução | Fonte do blob |
| --- | --- | --- | --- |
| `src/util.h` | conflito (só EOL) | adotar upstream | `upstream/main` |
| `src/event.cpp` | conflito (só EOL) | adotar upstream | `upstream/main` |
| `SKSE/Plugins/BehaviorDataInjector/SkipEquipAnimation_BDI.json` | conflito (só EOL) | adotar upstream | `upstream/main` |
| `src/hook.cpp`, `src/hook.h` | conflito | adotar upstream (o fork, depois do C1, difere só em formatação) | `upstream/main` |
| `src/event.h` | conflito | blob do upstream + remoção de `virtual ` na linha de `ProcessEvent` | `upstream/main` + edição |
| `CMakePresets.json` | conflito | adotar fork (Q2 = A) | `HEAD` |
| `cmake/x64-windows-skse.cmake` | auto (EOL difere) | adotar upstream (conteúdo idêntico, CRLF) | `upstream/main` |
| `CMakeLists.txt` | auto-merge | manter resultado do merge (upstream + `/OPT:REF`) | merge |
| `.gitignore` | auto-merge | manter resultado do merge (sem `*.cmake`) | merge |
| `cmake/headerlist.cmake`, `cmake/sourcelist.cmake` | removidos (fork apagou, upstream não mexeu) | manter removidos | — |
| `TASKS.md`, `.claude/**`, `.specify/**`, `specs/**` | só no fork | manter | `HEAD` |

## R3. Convenção de fim de linha e como registrá-la (FR-009)

- **Observação**: EOL dos blobs medidos byte a byte.

  | Arquivo | fork (índice) | upstream |
  | --- | --- | --- |
  | `CMakeLists.txt`, `CMakePresets.json`, `README.md`, `vcpkg*.json` | CRLF | CRLF |
  | `src/hook.*`, `src/plugin.cpp`, `src/PCH.h` | CRLF | CRLF |
  | `src/util.h`, `src/event.h`, `src/event.cpp` | **LF** | CRLF |
  | `SKSE/.../SkipEquipAnimation_BDI.json`, `cmake/x64-windows-skse.cmake` | **LF** | CRLF |
  | `.gitignore` | LF | LF |

  O `core.autocrlf=true` vem da config de sistema do Git for Windows, e não existe `.gitattributes` em nenhum dos lados. Com `autocrlf=true`, arquivos novos ou recriados são gravados no índice como LF, o que é a causa provável da divergência.
- **Decision**:
  1. **Convenção**: CRLF em todos os arquivos compartilhados com o upstream. O `.gitignore` continua LF, como já é nos dois lados.
  2. **Durante a execução**: usar `git -c core.autocrlf=false add …` em todo `add` de arquivo compartilhado, para que nenhuma conversão aconteça.
  3. **Registro permanente**: um `.gitattributes` **restrito aos caminhos compartilhados com o upstream**, com `-text` (desliga a conversão de EOL; o diff e o merge continuam textuais):
     ```gitattributes
     /src/**                -text
     /cmake/**              -text
     /SKSE/**               -text
     /CMakeLists.txt        -text
     /CMakePresets.json     -text
     /vcpkg.json            -text
     /vcpkg-configuration.json -text
     /README.md             -text
     ```
     Ele entra num commit separado depois do merge. Na branch de contribuição, é um commit **opcional** e separado.
- **Rationale**:
  - Depois do merge, todos os arquivos compartilhados terão blobs CRLF iguais aos do upstream.
  - Com `-text`, o git grava os bytes como estão, então nenhuma ferramenta local com `autocrlf=true` volta a converter para LF.
  - O escopo por caminho evita que os arquivos só do fork (`.md`, Spec Kit) passem a aparecer como modificados.
- **Alternatives considered**:
  - `* text=auto` ou `* text eol=crlf`: normalizam o **índice para LF**. Isso recriaria o diff de arquivo inteiro contra os blobs CRLF do upstream a cada sincronização.
  - `* -text` global: afetaria arquivos só do fork que estão em LF no índice e CRLF no working tree, que passariam a aparecer como modificados.
  - Só `git config core.autocrlf false` local: funciona, mas não é versionado e não protege outros clones.

## R4. Preservação do trabalho staged e ordem dos commits (FR-004, FR-005)

- **Decision**:
  1. **Antes de tudo**:
     - `git fetch upstream` e confirmar que o `upstream/main` continua em `22f1e3b`;
     - criar a branch `backup/pre-upstream-sync` em `25b0006`;
     - capturar o índice e o working tree com `git stash create` e guardar o SHA como `backup/pre-upstream-sync-staged`. Isso não altera nada no working tree.
  2. Separar o staged em **dois commits** antes do merge:
     - **C1** `fix(hook): remove investigation logging and fix undeclared vtbl`: só `src/hook.cpp` e `src/hook.h`.
     - **C2** `chore(tooling): add Spec Kit scaffolding and upstream-sync spec`: `.claude/skills/**`, `.specify/**`, `specs/**`.
  3. Fazer um **build de checkpoint depois do C1** (ver R6). Ele isola a validação da correção do `vtbl` antes do merge.
- **Rationale**: commits separados por natureza (produto ou ferramenta) mantêm o histórico legível e deixam o C1 isolado. O `git stash create` gera um commit de backup do índice sem mexer no estado atual.
- **Alternatives considered**:
  - `git stash push`: altera o working tree e o índice e exige restaurá-los depois.
  - Um commit único com tudo: mistura produto e tooling.

## R5. Resolução de `src/hook.*`

- **Decision**: adotar o blob do upstream byte a byte.
- **Rationale**: depois do C1, o fork difere do upstream só em (a) alinhamento de colunas nas atribuições e declarações de `hook.h` e (b) formatação do lambda de `std::jthread` em `hook.cpp`. São 7 linhas e nenhuma muda o comportamento. Adotar o upstream zera o diff desses arquivos (item 3 do inventário).
- **Alternatives considered**: manter a formatação do fork. Foi descartado porque deixaria um diff cosmético permanente contra o upstream.

## R6. Validação de build e o efeito colateral de deploy

- **Observação**:
  - O build atual está em `build/release-msvc`: Ninja, `x64-windows-skse`, `RelWithDebInfo`, overlay em `cmake/`.
  - O build é chamado via `build_temp.bat` (`vcvars64.bat` do VS 18 + `cmake --build`).
  - Com `SKYRIM_MODS_FOLDER=D:\ModdingSkyrim\MO2\mods`, o `CMakeLists.txt` **copia a DLL e o PDB** para `D:\ModdingSkyrim\MO2\mods\Skip Equip Animations\SKSE\Plugins\` a cada build. Isso sobrescreve a versão instalada (DLL de 559.104 bytes, 2026-10-16).
- **Decision**:
  - Antes do primeiro build, fazer backup da DLL e do PDB instalados no scratchpad da sessão, ou numa pasta escolhida pelo usuário.
  - Fazer build em três pontos: (1) depois do C1; (2) depois do merge resolvido; (3) na branch de contribuição.
  - Antes de cada build, confirmar com o usuário que o deploy no MO2 pode acontecer.
- **Rationale**: o FR-008 e o SC-004 exigem compilar. O deploy automático afeta o ambiente de jogo do usuário e precisa ser reversível.
- **Alternatives considered**: desabilitar o deploy (sem `SKYRIM_MODS_FOLDER` na sessão). É possível, mas aí o teste no jogo (SC-007) exigiria uma cópia manual. Fica como opção se o usuário preferir.

## R7. Branch de contribuição ao upstream (FR-012, FR-012a, Q3 = B)

- **Decision**:
  - Criar a branch `contrib/build-improvements` a partir do `upstream/main`, com commits em quatro partes:
    1. `git cherry-pick -x ba679c4`, que **preserva o autor AnArchos**. É esperado um conflito add/add em `cmake/x64-windows-skse.cmake` (o conteúdo é idêntico e só o EOL difere), resolvido com o blob do upstream;
    2. um commit novo `refactor(event): drop redundant virtual on ProcessEvent override`;
    3. um commit novo `chore(cmake): remove unused headerlist/sourcelist`, já que o build usa `GLOB_RECURSE`;
    4. um commit **opcional** com o `.gitattributes` do R3.
  - Validar com build e com um `git diff --stat upstream/main` restrito à allowlist ([contracts/contribution-branch.md](./contracts/contribution-branch.md)).
  - Push para `origin` e abertura do PR **só com confirmação explícita** (FR-014).
- **Rationale**:
  - O cherry-pick mantém a autoria de quem escreveu a melhoria de build, atendendo o FR-012a.
  - Commits pequenos e temáticos facilitam a revisão do mantenedor.
  - Partir do `upstream/main` garante zero conflitos.
- **Alternatives considered**:
  - PR direto do `main` do fork: levaria `TASKS.md`, `.claude/`, `.specify/` e `specs/`.
  - Um commit único com tudo: perderia a autoria e dificultaria a revisão.

## R8. Governança e autoria dos commits

- **Decision**: todos os commits criados nesta feature terminam com a linha `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`, e a descrição do PR termina com a linha de atribuição do Claude Code. O commit cherry-picked do `ba679c4` mantém o autor original.
- **Constituição**: `.specify/memory/constitution.md` ainda é o template, sem princípios ratificados. Não existem gates formais. Os gates usados no plano vêm dos FRs da spec (sem perda de trabalho, sem push sem confirmação, o build tem que passar). Recomenda-se rodar `/speckit-constitution` depois desta feature.

## R9. Upstream avançar durante a execução

- **Decision**: o primeiro passo da execução refaz o `git fetch upstream` e confirma `upstream/main == 22f1e3b`. Se o HEAD for outro, a execução para e o inventário da spec é refeito contra o novo HEAD antes de continuar.

## Observações fora do escopo

- O `vcpkg.json` tem `"name": "convenient-slow-motion"`, herdado de template nos dois lados. Não é tratado aqui.
- A DLL instalada no MO2 (2026-10-16 21:02) tem a mesma data do `d242488`, que não compila no estado commitado. Ela provavelmente foi gerada a partir de um working tree diferente e pode conter o logging de investigação.
