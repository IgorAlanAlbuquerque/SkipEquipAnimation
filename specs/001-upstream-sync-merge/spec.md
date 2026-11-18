# Feature Specification: Sincronização do fork com o upstream (PhiloSocio/SkipEquipAnimation)

**Feature Branch**: `001-upstream-sync-merge`

**Created**: 2026-11-17

**Status**: Implemented (2026-11-17). main sincronizado publicado em origin (6dc7c30) e contrib/build-improvements publicada (4c581af); falta só o usuário abrir o PR no upstream.

**Input**: User description: "resolver conflitos entre a minha branch de fork e a branch original de PhiloSocio/SkipEquipAnimation:main - fazer uma analise de quais mudanças feitas por mim devem ser preservadas e quais devem ser substituidas por feitas por ele"

## Contexto e diagnóstico *(levantado em 2026-11-17)*

- **Fork**: `origin` = IgorAlanAlbuquerque/SkipEquipAnimation, branch `main` (HEAD `25b0006`).
- **Original**: `upstream` = PhiloSocio/SkipEquipAnimation, branch `main` (HEAD `22f1e3b`).
- **Base comum**: `292a0df` (2025-11-14). Desde então o fork tem 16 commits e o upstream tem 1.
- **Achado central**: o único commit do upstream (`22f1e3b`, "Changed build system for 1.7.99 compatibility", autor AnArchos/PhiloSocio) é um **squash do próprio trabalho do fork** até o commit `9866463`, que tem o mesmo título e foi feito 2 minutos antes. Ignorando diferenças de fim de linha (CRLF/LF), o conteúdo do upstream é **idêntico** ao `9866463` do fork, exceto por manter 3 arquivos de build que o fork removeu.
- **Consequência**: os conflitos não vêm de mudanças concorrentes de lógica. Como o squash apagou o vínculo de histórico, o git compara tudo contra a base antiga. O trabalho que o fork fez **depois** do `9866463` colide com o squash. A simulação de merge mostra:
  - **7 arquivos em conflito** num merge direto;
  - **4 arquivos em conflito** quando as diferenças de fim de linha são ignoradas (`CMakePresets.json`, `src/event.h`, `src/hook.cpp`, `src/hook.h`).
- **Trabalho do fork posterior ao squash**:
  - `ba679c4` (AnArchos, 2026-09-21): melhorias de build que **não** foram publicadas no upstream;
  - `d242488` (Igor, 2026-10-16): logging de investigação, `TASKS.md`, `.claude/settings.json` e remoção de `virtual` redundante;
  - `25b0006`: merge dos dois anteriores;
  - **mudanças staged e não commitadas** em `src/hook.cpp`/`src/hook.h`, que removem o logging de investigação e **corrigem um erro de compilação**: o commit `d242488` usa a variável `vtbl`, que não está declarada;
  - **arquivos staged do Spec Kit** (`.specify/`, `.claude/skills/`).
- **Inventário validado em 2026-11-17** contra main=`d0cb5b3` (C2) e upstream/main=`22f1e3b`: os 7 conflitos foram confirmados por `git merge-tree`, e 100% dos 47 caminhos divergentes (ignorando EOL) correspondem a itens do inventário (3, 4, 5, 6, 7, 9, 11 e 12). O C1 (`35744b5`) compila: build Release com exit 0 e 0 warnings em `src/`.

## Clarifications

### Session 2026-11-17

- Q: Estratégia de integração (merge commit, rebase ou branch nova a partir do upstream)? → A: **Merge commit no `main`**, sem reescrever histórico e sem force-push.
- Q: Qual configuração de build manter no `CMakePresets.json`? → A: **A do fork (`ba679c4`)**: triplet `x64-windows-skse` + overlay em `cmake/` + gerador Ninja.
- Q: Destino do resultado: só o fork ou também uma contribuição ao upstream? → A: **Sincronizar o fork e preparar uma branch de PR para o upstream** com só as mudanças de produto. A abertura do PR depende de confirmação explícita.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Análise classificada das mudanças (Priority: P1)

Como mantenedor do fork, quero um inventário de cada mudança divergente entre o meu fork e o upstream. Cada item deve ter uma classificação explícita (preservar a minha versão, adotar a versão do upstream, descartar ou decidir) e uma justificativa, para saber exatamente o que vai sobreviver à sincronização antes de mexer no histórico.

**Why this priority**: é o pedido explícito do usuário e a base de todas as outras decisões. Sem ele, a resolução de conflitos seria arbitrária e poderia perder trabalho (por exemplo, as melhorias de build do `ba679c4`) ou reintroduzir código temporário (o logging de investigação).

**Independent Test**: pode ser validado lendo só o inventário. Toda diferença entre `main` (incluindo o staged) e `upstream/main` deve aparecer em exatamente uma linha, com classificação e justificativa.

**Acceptance Scenarios**:

1. **Given** o fork e o upstream divergentes, **When** o inventário é produzido, **Then** cada arquivo que difere entre os dois lados aparece com origem (commit/autor), classificação e justificativa.
2. **Given** um arquivo cuja diferença é só de fim de linha, **When** ele é classificado, **Then** o inventário o identifica como "ruído de EOL", separado das mudanças de conteúdo.
3. **Given** uma mudança cuja classificação depende de preferência do usuário, **When** o inventário é apresentado, **Then** ela fica marcada como "decisão pendente", com as opções e as consequências de cada uma.

---

### User Story 2 - Fork sincronizado com o upstream e sem conflitos (Priority: P1)

Como mantenedor do fork, quero que o meu `main` passe a conter o histórico do `upstream/main` com os conflitos resolvidos conforme o inventário. Assim, sincronizações futuras não repetem os mesmos conflitos.

**Why this priority**: é o resultado prático pedido ("resolver conflitos"). Também tem P1 porque, sem ele, toda atualização futura vinda do upstream voltaria a gerar os mesmos 4 a 7 conflitos.

**Independent Test**: depois da sincronização, o `upstream/main` é ancestral do `main` do fork, e uma nova simulação de merge contra o `upstream/main` não reporta conflitos.

**Acceptance Scenarios**:

1. **Given** o inventário aprovado, **When** a sincronização é feita, **Then** o `main` do fork contém o `22f1e3b` no histórico e nenhum arquivo tem marcadores de conflito.
2. **Given** o `main` sincronizado, **When** se compara o `main` com o `upstream/main`, **Then** as únicas diferenças restantes são as mudanças classificadas como "preservar (fork)".
3. **Given** que havia trabalho staged e não commitado antes do início, **When** a sincronização termina, **Then** esse trabalho está incorporado ao resultado e nada dele se perdeu.

---

### User Story 3 - Resultado compila e mantém o comportamento do mod (Priority: P2)

Como mantenedor, quero que o resultado da sincronização compile em Release e que o plugin mantenha os comportamentos já entregues (skip equip, InstantEquipAnim, skip de draw/sheath, correção do "travado no último frame"). Assim, a sincronização não introduz regressões.

**Why this priority**: um merge sem conflitos que não compila não tem valor. Hoje o `main` commitado já não compila (`vtbl` não declarado), então este critério também cobre essa correção.

**Independent Test**: o build Release conclui sem erros e gera o `.dll`. Num teste manual no jogo, os caminhos skip, instant e normal de equip se comportam como antes.

**Acceptance Scenarios**:

1. **Given** o `main` sincronizado, **When** o build de Release é executado com o preset configurado, **Then** ele conclui sem erros e gera o artefato do plugin.
2. **Given** o plugin gerado, **When** o jogador equipa uma arma em cada modo configurado, **Then** o comportamento é igual ao da versão 1.0.6+ do fork.

---

### User Story 4 - Mudanças do fork prontas para contribuição ao upstream (Priority: P3)

Como contribuidor, quero que as mudanças de produto que o fork tem e o upstream não (por exemplo, as melhorias de build do `ba679c4` e a limpeza do `event.h`) possam ser propostas ao upstream sem levar junto arquivos que só fazem sentido no fork.

**Why this priority**: o usuário confirmou que quer a contribuição (Q3 = B), mas ela depende das User Stories 1 a 3 estarem concluídas e não é necessária para resolver os conflitos.

**Independent Test**: existe uma branch criada a partir do `upstream/main` que contém só mudanças de produto, sem `TASKS.md`, `.claude/`, `.specify/` e `specs/`. Essa branch compila e não tem conflitos com o `upstream/main`.

**Acceptance Scenarios**:

1. **Given** o fork sincronizado, **When** se prepara a branch de contribuição, **Then** ela parte do `upstream/main` e contém só os itens do inventário marcados como "candidato a PR".
2. **Given** a branch de contribuição pronta, **When** se compara com o `upstream/main`, **Then** o diff não contém nenhum artefato local de ferramenta ou de investigação.
3. **Given** a branch de contribuição pronta, **When** o usuário ainda não confirmou, **Then** nem a branch é publicada nem o PR é aberto.

---

### Edge Cases

- **Diferenças só de fim de linha**: `src/util.h`, `src/event.cpp` e `SkipEquipAnimation_BDI.json` aparecem com milhares de linhas alteradas, mas são idênticos quando o EOL é ignorado. A resolução não deve tratar isso como mudança de conteúdo nem deixar o arquivo com EOL misto.
- **Arquivos removidos de um lado e mantidos no outro**: `cmake/headerlist.cmake` e `cmake/sourcelist.cmake` foram removidos pelo próprio mantenedor do upstream no fork (`9866463`), mas continuam no upstream. Nenhum dos dois lados os usa, porque o build lista as fontes automaticamente.
- **Commit intermediário quebrado**: o `d242488`/`25b0006` não compila. A sincronização não deve ter como resultado final um estado que reproduza esse erro.
- **Trabalho staged misturado**: o staged contém tanto a correção do hook quanto os arquivos do Spec Kit. Eles devem ser separados conforme a sua natureza (produto ou ferramenta local).
- **Histórico redundante**: com o merge commit, o histórico terá o squash `22f1e3b` e os commits originais equivalentes lado a lado. Isso é esperado e aceito, porque evita reescrever o histórico.
- **Branch de contribuição desatualizada**: se o upstream avançar depois que a branch de contribuição for criada, ela deve ser recriada ou atualizada a partir do novo `upstream/main` antes do PR.
- **Novo commit no upstream durante o trabalho**: se o `upstream/main` avançar antes da conclusão, o inventário deve ser reavaliado contra o novo HEAD.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: A análise DEVE listar toda diferença entre o estado atual do fork (`main` + staged) e o `upstream/main`, com origem (commit e autor), classificação e justificativa.
- **FR-002**: A análise DEVE separar diferenças de conteúdo de diferenças só de fim de linha.
- **FR-003**: A classificação DEVE usar exatamente uma destas categorias por item: **Preservar (fork)**, **Adotar (upstream)**, **Descartar** (código temporário ou morto) ou **Decisão pendente**.
- **FR-004**: Antes de qualquer alteração de histórico, o trabalho staged e não commitado DEVE ser salvo de forma recuperável.
- **FR-005**: O processo DEVE criar um ponto de restauração do `main` atual (backup) que permita voltar ao estado anterior por completo.
- **FR-006**: Ao final, o `upstream/main` DEVE fazer parte do histórico do `main` do fork, para que futuras sincronizações não repitam estes conflitos.
- **FR-007**: O resultado NÃO DEVE conter o logging de investigação do `d242488` (contador de equip, `LogGraphState`, hooks `NotifyAnimationGraph` e `hkbStateMachine::Update`), porque o próprio usuário já preparou a remoção dele no staged.
- **FR-008**: O resultado DEVE compilar. Isso inclui a correção do `vtbl` não declarado em `src/hook.h`.
- **FR-009**: Os arquivos que diferem só em fim de linha DEVEM terminar com uma convenção de EOL única e consistente, e essa convenção DEVE ser registrada para não voltar a gerar diffs falsos.
- **FR-010**: A integração DEVE ser feita por **merge commit** do `upstream/main` no `main` do fork. O histórico existente do `main` NÃO DEVE ser reescrito, e nenhum force-push DEVE ser necessário.
- **FR-011**: O `CMakePresets.json` resultante DEVE manter a configuração do fork (`ba679c4`): triplet `x64-windows-skse` como target e host, overlay de triplets apontando para `cmake/` e gerador Ninja nos presets Debug e Release. O arquivo `cmake/x64-windows-skse.cmake` DEVE continuar versionado.
- **FR-012**: Depois da sincronização do fork, DEVE ser preparada uma **branch de contribuição** criada a partir do `upstream/main`, contendo só os itens do inventário marcados como "candidato a PR". Essa branch DEVE compilar e não ter conflitos com o `upstream/main`.
- **FR-012a**: A descrição da contribuição DEVE explicar cada mudança incluída e deixar claro que as mudanças de build (`ba679c4`) foram feitas pelo próprio mantenedor depois do squash.
- **FR-013**: Artefatos que só fazem sentido no fork (`TASKS.md`, `.claude/settings.json`, `.claude/skills/`, `.specify/`, `specs/`) DEVEM continuar no fork e NÃO DEVEM fazer parte de nenhuma contribuição ao upstream.
- **FR-014**: Nenhum push para `origin` ou `upstream`, nenhuma publicação da branch de contribuição e nenhuma abertura de PR DEVEM acontecer sem confirmação explícita do usuário.

### Key Entities

- **Mudança divergente**: uma diferença entre o fork e o upstream. Atributos: arquivo(s), origem (commit e autor), tipo (conteúdo ou EOL), classificação, justificativa.
- **Ponto de restauração**: referência ao estado do `main` e do staged antes da operação, usada para rollback.
- **Inventário (resultado da análise)**:

| # | Arquivo(s) | Origem | Tipo | Classificação | Candidato a PR | Justificativa |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | `src/hook.cpp`, `src/hook.h`: logging de investigação | `d242488` (Igor) | Conteúdo | **Descartar** | Não | Código de diagnóstico temporário. O próprio usuário já preparou a remoção no staged. O upstream não o tem. |
| 2 | `src/hook.h`: `vtbl` → `hkbClipGeneratorVtbl` | staged (Igor) | Conteúdo | **Adotar (upstream)** / equivalente | Não (o upstream já está correto) | O commit atual não compila. A correção staged deixa o código igual ao do upstream. |
| 3 | `src/hook.cpp`, `src/hook.h`: alinhamento de colunas nas atribuições/declarações | `d242488` (Igor) | Formatação | **Adotar (upstream)** | Não | Mudança só cosmética. Adotar o upstream zera a diferença nesses arquivos e simplifica sincronizações futuras. |
| 4 | `src/event.h`: remoção de `virtual` redundante em `ProcessEvent ... override` | `d242488` (Igor) | Conteúdo (cosmético) | **Preservar (fork)** | **Sim** | `override` já implica virtual. Melhora a legibilidade sem mudar o comportamento. |
| 5 | `CMakePresets.json`: triplet `x64-windows-skse` + overlay + Ninja | `ba679c4` (AnArchos) | Conteúdo | **Preservar (fork)** (Q2 = A) | **Sim** | Melhoria mais recente do próprio mantenedor, feita depois do squash e nunca publicada no upstream. É o build atual do usuário. |
| 6 | `CMakeLists.txt`: `/OPT:REF` em Release/RelWithDebInfo | `ba679c4` (AnArchos) | Conteúdo | **Preservar (fork)** | **Sim** | Faz merge automático e reduz o tamanho do `.dll`. Não conflita. |
| 7 | `.gitignore`: remoção da regra `*.cmake` | `ba679c4` (AnArchos) | Conteúdo | **Preservar (fork)** | **Sim** | Necessária para versionar o triplet `cmake/x64-windows-skse.cmake`. Sem ela, o arquivo é ignorado. |
| 8 | `cmake/x64-windows-skse.cmake` | `ba679c4` / upstream | EOL | **Preservar** (conteúdo idêntico) | Não (o upstream já tem o arquivo) | Os dois lados têm o mesmo conteúdo, só com EOL diferente. |
| 9 | `cmake/headerlist.cmake`, `cmake/sourcelist.cmake` | removidos em `9866463` (AnArchos) | Arquivo removido | **Descartar** (manter removidos) | **Sim** (remoção) | Não são usados em nenhum dos lados, porque o build lista as fontes automaticamente. Quem removeu foi o próprio mantenedor. |
| 10 | `src/util.h`, `src/event.cpp`, `SKSE/.../SkipEquipAnimation_BDI.json` | squash `22f1e3b` | Só EOL | **Adotar convenção única** (FR-009) | Opcional (só a regra de EOL, em commit separado) | Conteúdo idêntico. A diferença é CRLF no upstream e LF no fork. |
| 11 | `TASKS.md` | `d242488` (Igor) | Arquivo do fork | **Preservar (fork)** | Não | Documenta a investigação em andamento do bug arma → magia. |
| 12 | `.claude/settings.json`, `.claude/skills/`, `.specify/`, `specs/` | Igor / Spec Kit | Ferramenta local | **Preservar (fork)** | Não | Configuração de ferramenta local. Não pertence ao produto. |
| 13 | Funcionalidades até `e4dcb5d` (spell fix, dual-path, InstantEquipAnim, draw/sheath, frame travado, versão 1.0.6) | fork (Igor) | Conteúdo | **Já no upstream** | Não | Já fazem parte do squash `22f1e3b`. Não há ação a tomar. |
| 14 | `SKSE/Plugins/BehaviorDataInjector/SkipEquipAnimation_BDI.json`: `"name" "InstantEquipAnim"` → `"name": "InstantEquipAnim"` | staged pelo usuário (2026-11-17 22:05). O bug existe desde `f857e9f`, nos dois lados | Conteúdo (bug fix) | **Preservar (fork)** | **Sim** | Faltava o `:`, então o JSON do repositório era inválido (a cópia instalada no MO2 já estava correta). Não apareceu como divergência porque fork e upstream tinham o mesmo erro. Encontrado depois da análise inicial. |

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% das diferenças entre o fork e o upstream aparecem no inventário com classificação e justificativa, sem nenhum item sem categoria.
- **SC-002**: Zero marcadores de conflito restantes no repositório depois da sincronização.
- **SC-003**: Uma nova simulação de merge do `main` sincronizado contra o `upstream/main` reporta zero conflitos.
- **SC-004**: O build de Release do resultado conclui com sucesso na primeira tentativa após a sincronização.
- **SC-005**: A diferença restante entre o `main` e o `upstream/main` contém só os itens classificados como "Preservar (fork)", sem nenhuma diferença só de EOL.
- **SC-006**: O estado anterior (`main` + staged) pode ser restaurado por completo a partir do ponto de restauração em menos de 1 minuto.
- **SC-007**: Nenhum comportamento de equip já entregue (skip, instant, draw/sheath, frame travado) regride no teste manual no jogo.
- **SC-008**: A branch de contribuição tem 0 arquivos fora da lista de candidatos a PR (itens 4, 5, 6, 7, 9 e 14, e opcionalmente 10) e 0 conflitos contra o `upstream/main`.
- **SC-009**: O histórico do `main` anterior à sincronização continua intacto: todos os 16 commits do fork permanecem acessíveis a partir do novo HEAD.

## Assumptions

- O `22f1e3b` é tratado como equivalente ao `9866463` do fork, mais a manutenção dos arquivos cmake e mudanças de EOL. Isso foi verificado por comparação de conteúdo ignorando EOL.
- O logging de investigação do `d242488` é temporário e deve sair. O usuário já preparou a remoção no staged, e o `TASKS.md` registra os achados.
- A convenção de EOL padrão a adotar é a do upstream (CRLF nos arquivos que ele já usa assim), registrada por regra de repositório, para minimizar diffs em sincronizações futuras.
- O ambiente de build atual do usuário (MSVC + vcpkg + presets) é o ambiente de referência para validar a compilação.
- O teste de comportamento no jogo é manual, porque o projeto não tem testes automatizados.
- O trabalho é local até o usuário autorizar o push.
