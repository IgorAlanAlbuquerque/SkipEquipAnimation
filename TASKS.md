# Fix: Skip Method para trocas Arma ↔ Magia

## Diagnóstico correto do problema

### Fluxo normal (playAnim=true)

```text
Engine → NotifyAnimationGraph("ForceEquipEvent") → [BG transiciona estado interno]
  → [BG roda clip de equip]
  → [BG dispara WeapEquip_Out / eventos ao fim do clip]
    → Engine atualiza estado (attach 3D, AI, etc.)
```

### Fluxo Skip atual (playAnim=false)

```text
Engine equipa item (sem animação)
  → BG nunca é notificado
  → BG fica no estado anterior (ex: arma)
  → [delay] SendEquipEvents → ProcessEvent(WeapEquip_Out)
    → Engine atualiza seu estado ✅
    → BG permanece inconsistente ❌
```

### Por que funciona para arma→arma mas falha em arma→magia

Para arma→arma: o BG já está no estado "arma equipada". A troca é uma auto-transição.
O estado resultante é o mesmo. A inconsistência passa despercebida.

Para arma→magia: o BG precisa de uma transição **entre estados diferentes**
(`iState=0` → `iState=9`). Sem a notificação correta ao BG via `NotifyAnimationGraph`,
ele permanece em `iState=0` (idle de arma) enquanto a engine sabe que há uma magia
equipada. Resultado: personagem fica em idle de combate com arma e não consegue lançar.

### O que InstantAnim faz diferente (e por que funciona)

InstantAnim chama `_OnEquipItemPC(a_this, true)` — **com animação**. O BG inicia
normalmente a transição de estado. O hook em `Update_Hook` comprime o clip num único
frame, mas o BG **processa todas as transições internas** e dispara seus próprios
eventos ao fim. O estado do BG permanece sempre consistente.

### Distinção crítica entre as duas APIs

| API | Direção | Efeito |
| --- | ------- | ------ |
| `ProcessEvent(BSAnimationGraphEvent*, source)` | BG → Engine | Notifica ouvintes da engine que uma animação ocorreu |
| `NotifyAnimationGraph(BSFixedString)` | Engine → BG | Envia evento para o BG processar e transicionar estados |
| `SetGraphVariable*(name, value)` | Engine → BG | Escreve variável que o BG lê em condições de transição |

O código atual usa apenas `ProcessEvent` — que só corrige o lado da engine, nunca o BG.

---

## Resultados da investigação (etapas 1 e 2 concluídas)

### Variáveis relevantes do behavior graph

Apenas estas persistiram nos logs (as demais candidatas não existem no grafo do player):

| Variável | Tipo | Valores observados |
| -------- | ---- | ------------------ |
| `iState` | int | 0 = modo arma/melee, 9 = modo mágico |
| `iRightHandType` | int | 0=vazio, 1=espada 1H, 5=arma 2H, 7=arco, 9=magia |
| `iLeftHandType` | int | 0=vazio, 5=arma 2H, 7=arco, 9=magia |
| `IsEquipping` | bool | true durante animação de equip, false no idle |
| `bEquipOk` | bool | sempre true (não relevante para transições) |

### Eventos ForceEquip (Engine → BG via NotifyAnimationGraph)

Cada ação de equip do player gera **2–3 chamadas** a `OnEquipItemPC`. Apenas a última
contém o evento `NotifyAnimationGraph`. As anteriores só atualizam o estado da engine.

| Evento | Quando é enviado | Efeito imediato no BG |
| ------ | ---------------- | --------------------- |
| `WeapOutRightReplaceForceEquip` | Equipa arma/arco na mão direita | `iState` → 0 (síncrono apenas neste evento), `iRightHandType` → tipo da arma, `IsEquipping` → true |
| `MagicForceEquipRight` | Equipa magia na mão direita (de arma→magia) | `iRightHandType` → 9, `IsEquipping` → true |
| `MagicForceEquip` | Equipa magia em ambas as mãos (de 2H→dual magic) | `iRightHandType` → 9, `iLeftHandType` → 9, `IsEquipping` → true |
| `MagicForceEquipLeft` | Equipa magia na mão esquerda | `iLeftHandType` → 9, `IsEquipping` → true |
| `WeapSoloEquip` | Remove item da mão esquerda (mão direita tem arma) | `iLeftHandType` → 0 |

**Nota**: `iState` muda síncronamente de 9→0 ao enviar `WeapOutRightReplaceForceEquip` (magia→arma).
Para magic equip (`MagicForceEquipRight`/`MagicForceEquip`), `iState` permanece 0 imediatamente
e só chega a 9 de forma assíncrona, após a animação de equip completar.

### Transições completas observadas em log (sessão 2026-06-02)

**Desarmado → Espada 1H** (eq=3)

```text
NotifyAnimationGraph('WeapOutRightReplaceForceEquip')
  iState:         0 → 0   (já era arma)
  iRightHandType: 0 → 1
  iLeftHandType:  0 → 0
  IsEquipping:  false → true
```

**Espada 1H → Magia direita** (eq=6)

```text
NotifyAnimationGraph('MagicForceEquipRight')
  iState:         0 → 0   ← permanece 0 imediatamente!
  iRightHandType: 1 → 9
  iLeftHandType:  0 → 0
  IsEquipping:  false → true
  [iState transiciona para 9 de forma ASSÍNCRONA, após a animação completar]
```

**Magia direita → Desarmado** (eq=9 — desequipar)

```text
NotifyAnimationGraph('WeapOutRightReplaceForceEquip')
  iState:         9 → 9   ← permanece 9 (assíncrono)
  iRightHandType: 9 → 0
  iLeftHandType:  0 → 0
  IsEquipping:  false → true
```

**Desarmado → Arco** (eq=12)

```text
NotifyAnimationGraph('WeapOutRightReplaceForceEquip')
  iState:         0 → 0
  iRightHandType: 0 → 7
  iLeftHandType:  0 → 7
  IsEquipping:  false → true
```

**Arco → Espada 2H** (eq=15)

```text
NotifyAnimationGraph('WeapOutRightReplaceForceEquip')
  iState:         0 → 0
  iRightHandType: 7 → 5
  iLeftHandType:  7 → 5
  IsEquipping:  false → true
```

**Espada 2H → Magia direita + Magia esquerda** (eq=20)

```text
NotifyAnimationGraph('MagicForceEquip')
  iState:         0 → 0   ← permanece 0 imediatamente!
  iRightHandType: 5 → 9
  iLeftHandType:  5 → 9
  IsEquipping:  false → true
  [iState transiciona para 9 de forma ASSÍNCRONA, após a animação completar]
```

### Observações críticas

1. **`iState` é síncrono apenas em arma (`WeapOutRightReplaceForceEquip`)**: sempre muda
   imediatamente de 9→0 quando enviamos esse evento. Para todos os eventos de magia
   (`MagicForceEquipRight`, `MagicForceEquip`), `iState` permanece em 0 logo após o evento
   e só chega a 9 após a animação de equip terminar e o BG completar a transição interna.

2. **`MagicForceEquipRight` vs `MagicForceEquip`**:
   - `MagicForceEquipRight` = equipa magia apenas na mão direita (mantém mão esquerda)
   - `MagicForceEquip` = equipa magia em ambas as mãos (ex: vindo de 2H que ocupa as duas)

3. **Tipos de iRightHandType/iLeftHandType confirmados**:
   - 0 = vazio
   - 1 = espada 1H (provavelmente qualquer arma 1H)
   - 5 = arma 2H (espada, machado, etc.)
   - 7 = arco (e possivelmente besta)
   - 9 = magia

4. **`iState` é SEMPRE assíncrono** (confirmado em ambas as direções):
   - `MagicForceEquipRight` → `iRightHandType=9` síncronamente, `iState` permanece 0 imediatamente
   - `WeapOutRightReplaceForceEquip` → `iRightHandType=1` síncronamente, `iState` permanece 9 imediatamente
   - Ambos mudam `iState` apenas após a **animação de equip completar** via clip triggers.
   - A mudança "síncrona 9→0" vista na sessão anterior pode ter sido coincidência de timing.

5. **`hkbStateMachine::currentStateID` NUNCA muda durante transições arma↔magia**:
   Hook em `VTABLE_hkbStateMachine[0]::Update` (sem filtro, logando TODOS os personagens)
   não produziu nenhuma entrada `[SM]` mesmo após duas sessões de teste. Conclusão definitiva:
   o modo arma/magia **não é rastreado por hkbStateMachine::currentStateID**. `iState` é
   mantido por **clip triggers** — pequenos eventos HKB embutidos no arquivo de animação que
   disparam quando o clip progride até um certo frame. Sem rodar o clip, não há como acionar
   esses eventos.

---

## Etapas

### [x] 1. Investigar variáveis do BG

Concluído. Ver resultados acima.

---

### [x] 2. Investigar eventos Engine → BG durante equip

Concluído. Ver tabela de eventos ForceEquip acima.
Hook em `VTABLE_PlayerCharacter[3]` offset `0x01` funciona corretamente.

---

### [~] 3a. Manipulação direta da hkbStateMachine — DESCARTADA

**Resultado da investigação (2026-06-02):** Hook em `VTABLE_hkbStateMachine[0]::Update`
sem nenhum filtro (loga todos os personagens, todos os SMs) não produziu nenhuma entrada
`[SM]` durante transições arma↔magia. `hkbStateMachine::currentStateID` não muda durante
essas transições. O modo arma/magia é controlado por **clip triggers** no arquivo de
animação, não por posição de state machine. Manipulação direta de `currentStateID` não
resolveria o problema porque não é esse campo que governa o comportamento de ataque.

**Abordagem descartada.**

---

### [ ] 3b. Implementar a correção de estado em duas fases

#### Contexto de uso real — o machado do Kratos

O modo Skip é usado por mods de animação personalizada (ex.: arremesso de machado) onde:

1. O personagem arremessa a arma → axe desequipada + removida do inventário
2. O personagem executa **animação de espera** (braço levantado aguardando o retorno)
3. O machado retorna → equip disparado via `OnEquipItemPC`
4. O personagem executa **animação de catch** (pegar o machado em pleno ar)

O Skip existe para que o step 3 não dispare a animação de equip padrão, que interromperia
a animação de catch. Com Skip, catch completa normalmente.

**O problema**: se havia uma magia equipada antes do arremesso, `iState=9` persiste após
o catch. O personagem não consegue atacar com o machado.

#### Por que as abordagens óbvias falham

| Abordagem | Resultado |
| --------- | --------- |
| `NotifyAnimationGraph` imediato | Inicia animação de equip → interrompe catch |
| `NotifyAnimationGraph` + bloquear clip | Triggers HKB internos do clip não disparam → BG trava no estado intermediário |
| `SetGraphVariableInt("iState", 0)` | BG sobrescreve na próxima iteração de Update |
| InstantAnim (fast-forward em 1 frame) | Mesmo se um frame, interrompe blending do catch |

As transições de estado do BG são dirigidas por **eventos HKB com integer ID** embutidos
nos `hkClipTrigger` do clip. Esses eventos só disparam quando o clip **roda até o fim**.
Não há como dispará-los sem rodar o clip de alguma forma.

#### A janela de oportunidade: depois que o catch termina

Enquanto o catch está em curso → qualquer NotifyAnimationGraph interrupe.
**Depois que o catch termina** → o personagem está em idle, nenhuma animação customizada
em curso. Um NotifyAnimationGraph + fast-forward nesse momento:

- Clip de equip começa e termina no mesmo frame (invisível)
- Todos os HKB triggers disparam → BG atinge o estado correto
- Nenhuma interferência visual porque o catch já concluiu

#### Estratégia: duas fases separadas pelo delay

```text
FASE 1 — Imediato (catch em curso, sem animação)
  _OnEquipItemPC(a_this, false)   → item equipa instantaneamente, BG não notificado
  Capturar: preRType, preLType, postRType, postLType
  Detectar se é transição cross-type (arma↔magia)
  → catch continua sem interrupção

FASE 2 — Após LoadBoundObjectDelay (catch concluído)
  Se cross-type:
    g_forwardNextEquipClip = true
    a_this->NotifyAnimationGraph(forceEquipEvent)
      → BG recebe evento, inicia clip de equip
      → Activate_Hook detecta flag → adiciona clip a g_pendingClips
      → Update_Hook fast-forward → triggers HKB disparam → BG no estado correto
      → BG dispara BSAnimationGraphEvent de conclusão → engine atualiza estado
  Se same-type:
    SendEquipEvents atual (ProcessEvent) → continua funcionando como antes
```

#### Por que `SetGraphVariable` não resolve o ataque (mesmo que resolva o sprint)

`SetGraphVariable` escreve **variáveis de saída** do BG:

- O sistema de locomotion/sprint as **lê** para escolher qual clip tocar → sprint fica correto
- O sistema de ataque checa a **posição interna da state machine** (`currentStateID`
  em `hkbStateMachine`), não as variáveis de saída → ataque continua bloqueado

Quando o equip normal termina (via clip triggers), a state machine move para o nó
"weapon ready". Sem essa transição, a máquina permanece na posição errada e o handler
de input (`attackStart`) não recebe resposta. `SetGraphVariable` não altera posição
de state machine. Essa abordagem foi descartada.

#### Detecção de idle via clip tracking (substitui delay fixo)

O problema com o `LoadBoundObjectDelay` fixo:

- Muito curto → correção dispara durante o catch, interrompendo-o
- Muito longo → delay desnecessário após o catch terminar
- Depende de configuração manual por mod

**Solução**: rastrear a atividade real de clips do player nos hooks existentes.

Durante o catch: um `hkbClipGenerator` de catch está ativo. Quando ele é desativado
(`Deactivate_Hook`), o personagem entra em idle — esse é o momento correto para a
correção. Usar `IsEquipping` não funciona porque o Skip nunca seta `IsEquipping=true`
(usa `playAnim=false`).

**Implementação do tracker:**

```cpp
// Contador atômico de clips não-locomotion ativos do player
static std::atomic<int> g_activePlayerClipCount{0};

// Em Activate_Hook: para clips do player, incrementar
++g_activePlayerClipCount;

// Em Deactivate_Hook: para clips do player, decrementar; 
// se g_pendingBGCorrection está ativa e o contador chegou a 0 → disparar correção
if (--g_activePlayerClipCount == 0 && g_pendingBGCorrection.expectedEvent[0]) {
    // idle detectado, disparar fase 2
}
```

O campo `g_pendingBGCorrection` armazena o evento ForceEquip que precisa ser enviado.
Quando o clip tracker chega a zero (idle), a fase 2 dispara automaticamente.

**Nota:** locomotion clips (idle, caminhar) são constantemente ativos. Para evitar
falsos positivos, filtrar clips cujo nome contém "MT_" ou "idle" — ou contar apenas
clips ativados APÓS o equip Skip (usar timestamp/equipId para correlacionar).

A abordagem mais simples: armazenar a contagem no momento do equip e aguardar ela
retornar ao mesmo nível — qualquer clip adicional (catch, espera) que tenha sido
ativado terá sido desativado.

#### Detecção da transição cross-type

Comparar o tipo das mãos antes e depois (valores do log: 0=vazio, 1=espada 1H, 5=2H, 9=magia):

```cpp
int32_t preR{}, preL{};
a_this->GetGraphVariableInt("iRightHandType", preR);
a_this->GetGraphVariableInt("iLeftHandType",  preL);

_OnEquipItemPC(a_this, false);

int32_t postR{}, postL{};
a_this->GetGraphVariableInt("iRightHandType", postR);
a_this->GetGraphVariableInt("iLeftHandType",  postL);

// cross-type = mudança entre mágico (9) e não-mágico (0,1,5)
bool crossType = ((preR == 9) != (postR == 9)) || ((preL == 9) != (postL == 9));
```

#### Mapeamento de eventos ForceEquip por transição (dados dos logs)

| Pre → Post | Evento |
| ---------- | ------ |
| qualquer → magia na direita | `MagicForceEquip` |
| qualquer → magia na esquerda | `MagicForceEquipLeft` |
| qualquer → arma na direita | `WeapOutRightReplaceForceEquip` |
| qualquer → esquerda vazia (arma solo direita) | `WeapSoloEquip` |

#### Mudanças necessárias no código

1. **`hook.h`** / **`hook.cpp`**: adicionar `static inline std::atomic<bool> g_forwardNextEquipClip{false}`

2. **`Activate_Hook`** ([hook.cpp:191](src/hook.cpp#L191)): além de `InstantEquipAnim`,
   verificar `g_forwardNextEquipClip.exchange(false)` para adicionar ao `g_pendingClips`

3. **`OnEquipItemPC`** case Skip ([hook.cpp:143](src/hook.cpp#L143)):
   - Ler `preR`/`preL` antes de `_OnEquipItemPC`
   - Ler `postR`/`postL` depois
   - Na lambda do delay, se `crossType`: setar flag + chamar `NotifyAnimationGraph`
   - Se same-type: manter `SendEquipEvents` atual

4. **`SendEquipEvents`** ([hook.cpp:92](src/hook.cpp#L92)): manter como está para
   same-type (arma↔arma, magia↔magia da mesma mão). O `ProcessEvent` continua
   necessário para notificar a engine de conclusão nessas transições.

---

### [ ] 5. Remover os hooks de investigação

Após validar que a correção funciona, remover o código de logging temporário:

- `g_equipId` e `LogGraphState` de [hook.cpp](src/hook.cpp)
- `NotifyAnimationGraph_Hook` e `_NotifyAnimationGraph` de [hook.h](src/hook.h) e [hook.cpp](src/hook.cpp)
- Hook de `VTABLE_PlayerCharacter[3]` em `Hook()` ([hook.h:20](src/hook.h#L20))

---

### [ ] 6. Testar todos os cenários de troca

- [ ] Desarmado → Arma 1H direita
- [ ] Desarmado → Arma 2H
- [ ] Arma direita → Arma direita diferente (regressão)
- [ ] Arma 2H → Magia direita
- [ ] Magia direita → Arma 1H direita
- [ ] Magia direita → Magia diferente na direita
- [ ] Magia direita → Magia esquerda (dual magic)
- [ ] Arma direita + Magia esquerda → Arma direita + outra magia esquerda
- [ ] Arma direita + Magia esquerda → Arma direita vazia
- [ ] Em combate e fora de combate
- [ ] Em movimento e parado
- [ ] Confirmar que ataque/magia funciona após cada troca

---

### [ ] 7. Regressão: verificar que InstantAnim não foi afetado

As etapas acima só tocam o caminho `EquipMode::Skip`. Confirmar que
`EquipMode::InstantAnim` continua funcionando corretamente para todos os cenários.
