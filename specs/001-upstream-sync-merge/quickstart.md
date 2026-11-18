# Quickstart: validação da sincronização

Roteiro para provar que a feature está concluída. Os comandos são para **PowerShell**, a partir da raiz do repositório. As invariantes estão em [contracts/fork-main-after-sync.md](./contracts/fork-main-after-sync.md) e [contracts/contribution-branch.md](./contracts/contribution-branch.md). O procedimento de execução fica em `tasks.md`.

## Pré-requisitos

- Remotes `origin` (fork) e `upstream` (PhiloSocio) configurados.
- VS 18 (`vcvars64.bat`), vcpkg em `$env:VCPKG_ROOT` e `build/release-msvc` já configurado (Ninja, `x64-windows-skse`).
- Ciência de que o build **copia a DLL** para `$env:SKYRIM_MODS_FOLDER\Skip Equip Animations\SKSE\Plugins\` (research R6).

## V0: Pré-condições (antes de executar)

```powershell
git fetch upstream
git rev-parse --short upstream/main          # esperado: 22f1e3b (senão: PARAR e refazer o inventário)
git rev-parse --short main                   # esperado: 25b0006
```

## V1: Ponto de restauração existe (FR-004, FR-005)

```powershell
git rev-parse --short backup/pre-upstream-sync          # 25b0006
git cat-file -t backup/pre-upstream-sync-staged          # commit
Test-Path "<pasta-de-backup>\SkipEquipAnimation.dll"     # True
```

## V2: Ancestralidade e conflitos (C-MAIN-1, C-MAIN-2; SC-002, SC-003, SC-009)

```powershell
git merge-base --is-ancestor upstream/main main; $LASTEXITCODE              # 0
git merge-base --is-ancestor backup/pre-upstream-sync main; $LASTEXITCODE   # 0
git grep -nE '^(<<<<<<<|>>>>>>>)( |$)'; $LASTEXITCODE                       # 1 (nada encontrado)
git merge-tree --write-tree main upstream/main | Out-Null; $LASTEXITCODE    # 0
```

## V3: Diff contra o upstream bate com o contrato (C-MAIN-3, C-MAIN-4; SC-005)

```powershell
git diff --name-status upstream/main main
git diff --name-only upstream/main main | Measure-Object | % Count
git diff --ignore-cr-at-eol --name-only upstream/main main | Measure-Object | % Count   # igual ao anterior
git diff upstream/main main -- src/event.h CMakeLists.txt CMakePresets.json .gitignore
```

**Esperado**: só os caminhos da tabela C-MAIN-3, e as duas contagens iguais (nenhum diff só de EOL).

## V4: Fim de linha e working tree limpo (C-MAIN-4; FR-009)

```powershell
git ls-files --eol src cmake SKSE CMakeLists.txt CMakePresets.json README.md vcpkg.json vcpkg-configuration.json .gitignore
git status --porcelain                                    # vazio
```

**Esperado**: `i/crlf` + `attr/-text` em todos os caminhos compartilhados, e `i/lf` no `.gitignore`.

## V5: Código temporário ausente (C-MAIN-5; FR-007, FR-008)

```powershell
git grep -nE 'LogGraphState|g_equipId|NotifyAnimationGraph_Hook|SM_Update_Hook' -- src; $LASTEXITCODE   # 1
git grep -n 'vtbl\.write_vfunc' -- src/hook.h; $LASTEXITCODE                                           # 1
```

## V6: Build (C-MAIN-6; SC-004)

```powershell
cmd /c build_temp.bat; $LASTEXITCODE                                            # 0
Get-Item build\release-msvc\SkipEquipAnimation.dll | Select Name, Length, LastWriteTime
```

## V7: Comportamento no jogo (SC-007, manual)

Lançar o jogo pelo MO2 com o mod "Skip Equip Animations" ativo e conferir cada modo:

| Cenário | Esperado |
| --- | --- |
| Equipar arma no modo skip | Arma aparece sem animação de equip |
| Equipar no modo InstantEquipAnim | Animação acelerada/instantânea, sem travar no último frame |
| Sacar e guardar a arma (draw/sheath) com InstantEquipAnim | Sem animação longa |
| Equipar magia | Funciona (correção de spell) |
| Equipar com o modo desativado | Animação vanilla normal |

## V8: Branch de contribuição (C-PR-1 a C-PR-5; SC-008)

```powershell
git merge-base contrib/build-improvements upstream/main     # == git rev-parse upstream/main
git diff --name-status upstream/main contrib/build-improvements
git log --format='%h %an | %s' upstream/main..contrib/build-improvements
git switch contrib/build-improvements; cmd /c build_temp.bat; $LASTEXITCODE; git switch main
```

**Esperado**: o diff é só a allowlist do C-PR-2, o primeiro commit tem autor AnArchos e o build retorna exit 0.

## V9: Rollback funciona (SC-006): só em ensaio, sem publicar

Num clone descartável, ou **antes** de qualquer push:

```powershell
git switch main
git reset --hard backup/pre-upstream-sync
git stash apply --index backup/pre-upstream-sync-staged
git status --short           # mostra de novo o staged original (hook.* + Spec Kit)
```

Depois do ensaio, voltar ao estado sincronizado (`git reset --hard <sha-do-merge>`).
