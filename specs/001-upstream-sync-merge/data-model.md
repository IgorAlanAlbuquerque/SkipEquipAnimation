# Data Model: Sincronização do fork com o upstream

**Feature**: [spec.md](./spec.md) | **Research**: [research.md](./research.md)

Esta feature não tem dados persistidos de aplicação. As "entidades" são objetos do repositório git manipulados durante a sincronização.

## Entidades

### MudançaDivergente

Uma diferença entre o estado do fork (`main` + staged) e o `upstream/main`.

| Campo | Descrição | Validação |
| --- | --- | --- |
| `id` | Número do item no inventário da spec (1 a 13) | Único |
| `arquivos` | Caminho(s) afetado(s) | Existe em pelo menos um dos lados ou foi removido de um deles |
| `origem` | Commit(s) e autor que introduziram a mudança | SHA válido no histórico local |
| `tipo` | `conteúdo` \| `formatação` \| `só-EOL` \| `arquivo-removido` \| `arquivo-do-fork` \| `ferramenta-local` | Um valor |
| `classificação` | `Preservar (fork)` \| `Adotar (upstream)` \| `Descartar` \| `Já no upstream` | Um valor, sem `Decisão pendente` restante (spec FR-003) |
| `candidatoPR` | `sim` \| `não` \| `opcional` | Obrigatório. `sim` só para mudanças de produto (FR-013) |
| `fonteDoBlob` | `upstream/main` \| `HEAD` \| `merge` \| `edição` \| `—` | Definida na tabela do [research.md R2](./research.md#r2-método-de-resolução-do-merge) |

### PontoDeRestauração

| Campo | Valor planejado |
| --- | --- |
| `branchMain` | `backup/pre-upstream-sync` → `25b0006` |
| `refStaged` | `backup/pre-upstream-sync-staged` → SHA de `git stash create` (índice + working tree) |
| `backupDeploy` | Cópia de `SkipEquipAnimation.dll` + `.pdb` de `MO2\mods\Skip Equip Animations\SKSE\Plugins\` |

**Regras**: é criado antes de qualquer commit, merge ou build. Só é removido por pedido explícito do usuário, depois que a feature for validada.

### ConvençãoEOL

| Campo | Valor |
| --- | --- |
| `padrão` | CRLF para os caminhos compartilhados com o upstream |
| `exceções` | `.gitignore` (LF nos dois lados) |
| `registro` | `.gitattributes` com `-text` nos caminhos compartilhados ([research.md R3](./research.md#r3-convenção-de-fim-de-linha-e-como-registrá-la-fr-009)) |

### BranchDeContribuição

| Campo | Valor |
| --- | --- |
| `nome` | `contrib/build-improvements` |
| `base` | `upstream/main` (`22f1e3b`) |
| `commits` | cherry-pick `-x ba679c4` (autor AnArchos); `refactor(event)`; `chore(cmake)`; `.gitattributes` (opcional) |
| `allowlist` | ver [contracts/contribution-branch.md](./contracts/contribution-branch.md) |
| `publicada` | `false` até confirmação explícita (FR-014) |

## Transições de estado da sincronização

```text
[Inicial]                main=25b0006, staged misto, upstream não ancestral
   │  fetch + confirmar upstream/main == 22f1e3b   (se não for → PARAR e refazer o inventário)
   ▼
[Backup criado]          backup/pre-upstream-sync, backup/pre-upstream-sync-staged, backup da DLL instalada
   │  C1 (hook) + C2 (tooling)
   ▼
[Staged commitado]       working tree limpo
   │  build de checkpoint (C1 tem que compilar)          (falha → corrigir antes de seguir)
   ▼
[Merge em andamento]     git merge --no-ff --no-commit upstream/main → 7 conflitos
   │  resolução por arquivo (research R2)
   ▼
[Resolvido]              0 marcadores, EOL conforme a convenção
   │  commit do merge + build                             (falha → abortar via backup ou corrigir)
   ▼
[Sincronizado]           upstream/main ancestral de main; diff = contrato fork-main-after-sync
   │  commit do .gitattributes
   ▼
[Convenção registrada]   git status limpo; ls-files --eol ok
   │  criar contrib/build-improvements
   ▼
[Contribuição pronta]    allowlist ok, build ok, publicada=false
   │  confirmação explícita do usuário
   ▼
[Publicado]              push origin (main e contrib) + PR aberto
```

**Rollback em qualquer estado** antes de `[Publicado]`:

```text
git merge --abort (se o merge estiver em andamento)
git switch main
git reset --hard backup/pre-upstream-sync
git stash apply --index backup/pre-upstream-sync-staged
```

Depois disso, restaurar a DLL e o PDB a partir do `backupDeploy`.
