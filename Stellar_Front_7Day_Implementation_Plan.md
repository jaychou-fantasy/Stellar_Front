# 《Stellar Front》7 天基础游玩逻辑实施计划

> 更新：2026-08-21
> 范围：把现有第一人称射击工程做成一个可在 **2 人 Listen Server PIE** 中完整游玩的最小闭环。  
> 原则：优先复用当前 C++ 工程；服务器拥有规则、状态与判定；不为未来 20v20 建框架。

## 1. 本周交付目标

本周结束时，两名玩家可以进入同一张地图并完成如下流程：

```text
WaitingToStart（只等待 Red / Blue 达到最低开局人数）
  -> InProgress
      -> WarmingUp（靶场热身，可移动、射击、死亡和重生）
      -> PreDeploy
      -> OrbitalCombat（控制节点）
      -> SearchKey（拾取并护送密钥）
      -> UpLoad（上传）
      -> Evacuation（撤离）
  -> WaitingPostMatch（结算 / 离开当前对局）
```

- Red：进攻方；推进控制节点、取得密钥、完成上传。
- Blue：防守方；阻止进攻，并在撤离阶段逃离。
- 目标人数是 2 人，最多先验证 4 人；本周不接入 Session、Steam/EOS 或在线匹配服务。
- 不做 20v20 平衡、在线服务、资源经济、武器 Tier、载具、职业选择、随机部署或完整计分系统。
- 每天结束都必须先通过 C++ 编译，再做与当天有关的双窗口 PIE 验证；编译成功不等于 PIE 已通过。

## 2. 现有工程边界

| 责任 | 当前工程位置 | 本周约定 |
|---|---|---|
| 比赛规则与阶段 | `Framework/Match/SGameMode_StellarFront.*` | 只在服务器运行；唯一负责 Warmup、阶段推进和胜负。 |
| 全局同步状态 | `Framework/Match/SGameState.*` | 保存所有客户端都需要读取的 Phase 和目标状态。 |
| 玩家队伍/死亡/目标状态 | `Framework/Player/SPlayerState.*` | 服务器写入 Team、Alive 和统计，客户端通过复制读取。 |
| 玩家控制器 | `Framework/Player/SPlayerController.*` | 作为持久 HUD 的所有者。 |
| 开火、弹丸、伤害 | `Gameplay/Actions/`、`Combat/`、`Gameplay/Attributes/` | 客户端请求，服务器生成弹丸与结算伤害。 |
| 交互 | `Gameplay/Interaction/SInteractionComponent.*`、`SGameplayInterface.h` | 复用现有 `ServerInteract`；目标 Actor 自己在服务器重新验证距离、阶段、队伍和存活状态。 |
| HUD | `UI/HUD/MainWidget.*` | HUD 最终由 PlayerController 创建，不能依赖会死亡和重生的 Pawn。 |

## 3. 全周强制规则

1. `GameMode` 只在服务器做规则判断；客户端不自行改变 Phase、Team、Health、UploadProgress 或胜负。
2. 每个 RPC/交互都在服务器重新检查：Actor 有效、阶段正确、玩家存活、队伍正确、距离足够近、目标未被其他人占用。
3. Timer 回调必须确认仍处于它启动时对应的阶段；例如上传完成 Timer 不能在已经进入 Evacuation 后再次推进阶段。
4. 不创建 `ObjectiveBase`、通用状态机或资源子系统。只创建四个单一用途 Actor：`ASControlNode`、`ASNetworkKey`、`ASUploadZone`、`ASEvacZone`。
5. 目标 Actor 的重叠数组不能盲信：死亡、登出、Pawn 销毁后必须过滤无效/死亡引用。
6. 本周不做友伤规则。若要禁止友伤，单独作为一条需求，不在本周临时加入。
7. `WaitingToStart` 只等待双方最低人数；`WarmingUp` 到 `Evacuation` 全部位于 UE `MatchState::InProgress`。`CurrentPhase` 在比赛开始前使用 `None`，不能提前显示为 `WarmingUp`。

---

# Day 1：联机底座、分队、出生与弹丸

## 目标

两名玩家进入地图后被稳定分到 Red/Blue，各自从正确出生点生成；服务器生成并复制弹丸，服务器唯一结算伤害。

## 已完成内容

- `SGameMode_BP` 已确认继承 `ASGameMode_StellarFront`，蓝图中没有覆盖规则逻辑。
- `SGameMode_BP` 配置了 `SGameState`、`SPlayerState`、`SPlayerController_BP` 与 `BP_Player`。
- `ETeam::None` 与 `Team = ETeam::None` 已存在，避免新玩家被误算作 Red。
- `ASGameMode_StellarFront` 已启用 `bDelayedStart`，并用实时统计替代累积的 `RedCnt` / `BlueCnt`。
- `ReadyToStartMatch_Implementation()` 已要求双方各至少 1 人。
- 地图已放置并标记 `Red`、`Blue` 两个 `PlayerStart`。
- `ASProjectileBase` 已启用 `bReplicates` 与 `SetReplicateMovement(true)`。
- 弹丸命中与 `ApplyDamage` 已限制为服务器权威。

## 必须先修复的阻断问题

当前 `ChoosePlayerStart_Implementation()` 的 `TeamStarts.IsEmpty()` 判断和 `return Super::ChoosePlayerStart_Implementation(Player)` 被放进了 `for (TActorIterator...)` 循环中。它会在检查第一个 `PlayerStart` 后直接回退父类，因此并不会按 Team Tag 选点。

循环结束后才判断数组是否为空，推荐结构：

```cpp
for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
{
    if (It->PlayerStartTag == RequiredTag)
    {
        TeamStarts.Add(*It);
    }
}

if (TeamStarts.IsEmpty())
{
    UE_LOG(LogGameMode, Warning, TEXT("No PlayerStart for team tag: %s"), *RequiredTag.ToString());
    return Super::ChoosePlayerStart_Implementation(Player);
}

return TeamStarts[FMath::RandRange(0, TeamStarts.Num() - 1)];
```

同时把 `AssignTeam` 的参数收紧为 `ASPlayerState*`，移除函数内多余的 `Cast`，避免未来配置不一致时空指针解引用。

## 验收

- C++ 编译成功。
- 1 人 PIE：不进入正式比赛。
- 2 人 Listen Server PIE：第一人 Red、第二人 Blue，且各自出现在对应 Tag 的出生点。
- 两端开火都能看到服务器复制的弹丸；一次命中只在服务器输出一次 Health 变化日志。
- 当前命中闪光/音效是否同步给所有客户端不作为 Day 1 完成条件，留到 HUD/反馈整理时处理。

---

## Day 1 → Day 2 交接（开始 Day 2 前必读）

### 当前工作区状态

Day 1 的相关代码与地图已提交为当前基线（提交 `b9f97fb`，`finish with AssignTeam logic & Projectile Replicate Movement`）。开始 Day 2 前不要重置或覆盖这些文件；当前工作区只保留本计划的交接更新。

| 项目 | 当前状态 | 交接说明 |
|---|---|---|
| `Content/Maps/FirstPersonExampleMap.umap` | Day 1 基线 | 已放置/标记 Red 与 Blue PlayerStart。 |
| `Framework/Match/SGameMode_StellarFront.*` | Day 1 基线 | 已接入延迟开局、实时分队、Team PlayerStart 选择。`AssignTeam` 现已使用 `ASPlayerState*`。 |
| `Combat/Projectiles/SProjectileBase.cpp` | Day 1 基线 | 已启用 Actor 与移动复制；命中销毁只在服务器执行。 |
| `Gameplay/FunctionLibrary/SGameplayFunctionLibrary.cpp` | Day 1 基线 | 已阻止客户端执行伤害与物理冲量。 |
| `Framework/Match/SGameState.h` | Day 1 基线 | 只有格式/空白改动；不属于 Day 1 逻辑，先不要与 Day 2 混在一起处理。 |
| `Stellar_Front_7Day_Implementation_Plan.md` | 当前工作区修改 | 本计划与本交接内容。 |

### Day 1 尚未关闭的阻断项

`ChoosePlayerStart_Implementation()` 仍把“没有找到 TeamStart 的回退判断”放在遍历 `PlayerStart` 的循环里。只要第一个枚举到的 PlayerStart 不匹配当前队伍，就会过早回退 UE 默认选点。

在开始 Day 2 之前，把它固定为以下结构：先遍历完所有 PlayerStart，再判断 `TeamStarts` 是否为空。

```cpp
for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
{
    if (It->PlayerStartTag == RequiredTag)
    {
        TeamStarts.Add(*It);
    }
}

if (TeamStarts.IsEmpty())
{
    UE_LOG(LogGameMode, Warning, TEXT("No PlayerStart for team tag: %s"), *RequiredTag.ToString());
    return Super::ChoosePlayerStart_Implementation(Player);
}

return TeamStarts[FMath::RandRange(0, TeamStarts.Num() - 1)];
```

这不是格式问题：未修复时，Red/Blue 出生正确只能是 UE 默认选点碰巧符合，不能作为 Day 1 或 Day 2 的测试基础。

### Day 2 开始前的 10 分钟预检

1. 修复上面的 PlayerStart 循环位置，并编译。
2. 以 Listen Server、2 Players 运行 PIE：确认第一人 Red、第二人 Blue，且分别从对应 Tag 出生。
3. 两端各开一枪：确认弹丸可见；在服务端日志确认一次命中只记录一次 Health 变化。
4. 以上任一项失败时，先回到 Day 1 修复；不要在错误出生或错误伤害链路上继续加入死亡/重生代码。

### Day 2 可直接复用的现有状态

- `ASPlayerState` 已有复制的 `bIsAlive`、`Kills`、`Deaths`，以及服务器限定的 `SetIsAlive`、`AddKills`、`AddDeaths`。
- `USAttributeComponent` 已在服务器写入 Health，并已有 HealthChanged multicast；当前生命归零处只有占位注释。
- `SGameMode_BP` 是空的 `ASGameMode_StellarFront` 子类，因此死亡、重生和统计应全部写在 C++ GameMode，不写蓝图。
- `SAction_Fire` 已限制服务器生成弹丸；Day 2 不要重新设计开火 RPC。

### Day 2 的第一个实现入口

从 `Gameplay/Attributes/SAttributeComponent.cpp` 的生命归零分支开始：它目前查找的是旧 `ASGameMode`，而本周实际运行的是 `ASGameMode_StellarFront`。Day 2 应改为只在服务器把“伤害来源、死亡 Pawn/Controller”交给 `ASGameMode_StellarFront` 的明确死亡处理函数；由 GameMode 负责 PlayerState 统计、延迟与 `RestartPlayer`。

不要在 AttributeComponent 中直接调用 `RestartPlayer`，也不要在客户端的 Health 回调中增加死亡逻辑。

---

# Day 2：死亡、击杀统计与重生

## 目标

让 Health 归零变成一个可恢复的服务器流程：死亡、统计、延迟重生，而不是只停在 Health 为 0。

## 工作项

1. 在 `USAttributeComponent::ApplyHealthchange()` 的服务器分支中，当生命值第一次降到 0 时通知 `ASGameMode_StellarFront`。
   - 传入伤害来源与受害 Pawn/Controller。
   - 必须防止同一死亡被多次通知。
2. 在 `ASGameMode_StellarFront` 增加一个明确的服务器死亡处理函数。
   - 受害者：`SetIsAlive(false)`、`AddDeaths()`。
   - 击杀者存在且不是自己时：`AddKills()`。
   - 销毁或失能当前 Pawn，使用一个短的服务器 Timer 调用 `RestartPlayer`。
3. 重生前把 PlayerState 的存活状态恢复为 true；重生仍走 Day 1 的 `ChoosePlayerStart`。
4. 只做固定短重生时间（例如 3 秒）；不做观战系统、复活资源、死亡回放或击杀助攻。

## 需要关注

- 不能在客户端的 Health 回调中调用 `RestartPlayer`。
- `InstigatorActor` 不一定是玩家 Pawn；统计前需安全地解析其 PlayerState。
- 登出、地图切换、阶段结束时要让待触发的重生 Timer 不再重生旧 Controller。

## 验收

- Red 击杀 Blue 后，服务器上的 Red Kills +1、Blue Deaths +1。
- 蓝方 Pawn 消失/失能后在延迟结束时出生于 Blue Start。
- 双端 PlayerState 的 Alive/Kills/Deaths 一致。

## Day 2 → Day 3 交接（开始 Day 3 前必读）

### 结论

Day 2 的死亡、击杀统计、Pawn 销毁、延迟重生、重生取消和 2～4 人本队出生点链路已经完成，并经过用户的 Listen Server PIE 验证，可以作为 Day 3 控制节点工作的运行基础。

本交接所依据的证据分为两类：

- 源码/配置确认：服务器伤害入口、首次死亡判断、PlayerState 复制、GameMode 重生 Timer、Logout/MatchEnd 清理、本队出生点占用检查均已存在。
- 用户 PIE 确认：玩家击杀后 Kills/Deaths/Alive 正确，死亡 Pawn 消失并延迟重生，重生取消有效，3～4 人可以从不重叠的本队出生点活动和射击。

本次交接审计没有重新编译，也没有修改 C++、配置或资产。因此不能把静态检查当成一次新的 Build/PIE 证据；运行结论来自本轮用户实际测试。

### 当前基线与工作区

- 当前 HEAD 为 `589d144`（`Respawn Player logic done & Optimize the PlayerStart contribution`），Day 2 死亡/重生与出生点优化已经形成提交。
- 当前工作区存在用户正在编写但尚未提交的 Warmup 代码：`SGameMode_StellarFront.h/.cpp` 已加入 `WarmupTimerHandle`、`WarmupDuration`、`StartWarmup()`，并把 `HandleMatchHasStarted()` 改为调用 `StartWarmup()`；`EndWarmup()` 仍为空。本计划文稿也是当前任务允许修改的文件。不要覆盖或回滚这些源码改动。
- `Config/DefaultEngine.ini` 当前将默认 GameMode 指向 `/Game/Blueprints/SGameMode_BP.SGameMode_BP_C`。
- `SGameMode_BP` 使用 `ASGameMode_StellarFront`，并配置当前 `BP_Player`、`SGameState`、`SPlayerState` 和 `SPlayerController_BP` 类链。
- `FirstPersonExampleMap` 当前有 4 个 PlayerStart：2 个 `Red`、2 个 `Blue`。
- `589d144` 的提交差异中仍包含多处 trailing whitespace；它不影响已完成的 PIE 功能验证，也不应在 Day 3 功能任务中顺手做全文件格式化。Day 3 新增差异自身必须通过 `git diff --check`。
- `589d144` 已经包含 `Content/Environment/M_Cube_Inst.uasset` 和 `问题StellarFront.docx` 等非死亡/重生主链文件。不要为整理历史而回滚或重写该提交；Day 3 只约束自己的新增差异。

### 已完成的权威调用链

```text
服务器弹丸命中
    -> USGameplayFunctionLibrary::ApplyDamage（确认 TargetActor Authority）
    -> USAttributeComponent::ApplyHealthChange
    -> Health 首次从 > 0 降到 0（bJustDied）
    -> ASGameMode_StellarFront::HandlePlayerDeath
    -> Victim PlayerState：Alive=false、Deaths+1
    -> 非自杀的有效 Killer PlayerState：Kills+1
    -> Detach/Destroy 旧 Pawn（ASCharacter::Destroyed 同时销毁武器）
    -> 服务器 Respawn Timer
    -> RespawnPlayer：Alive=true、RestartPlayer
    -> ChoosePlayerStart：优先选择本队未占用出生点
    -> Controller 控制新的 Pawn
```

职责边界已经固定：

- `USAttributeComponent` 只负责生命值与“首次死亡”通知，不直接重生。
- `ASGameMode_StellarFront` 负责死亡规则、统计、Timer、取消和 `RestartPlayer`。
- `ASPlayerState` 保存并复制跨 Pawn 生命周期的 Team、Alive、Kills、Deaths。
- 死亡 Pawn 会被销毁；Controller 与 PlayerState 保留；重生后得到的是新的 Pawn 实例。

### Day 2 验收结果

| 验收项 | 状态 | 依据 |
|---|---|---|
| Red 击杀 Blue 后 Red Kills +1、Blue Deaths +1 | 通过 | 用户日志与源码 |
| 生命值到 0 只通知一次死亡 | 通过 | `OldHealth > 0`、`ActualDelta < 0`、`Health <= 0` |
| 死亡时 Alive=false，重生前恢复为 true | 通过 | PlayerState 服务器写入与用户 PIE |
| 旧 Pawn 和装备武器被销毁 | 通过 | GameMode 与 `ASCharacter::Destroyed()` |
| 固定短延迟后由服务器 `RestartPlayer` | 通过 | Timer 源码与用户 PIE |
| Logout 取消该 Controller 的待执行重生 | 通过 | `CancelRespawn()` 与用户 PIE |
| Match End 清理全部待执行重生 Timer | 源码通过 | `HandleMatchHasEnded()`；未提供独立的新 PIE 日志 |
| Team/Alive/Kills/Deaths 在 PlayerState 复制 | 通过 | `DOREPLIFETIME` 与用户双端验证 |
| 3～4 人使用本队未占用出生点 | 通过 | 2 Red + 2 Blue PlayerStart、占用检查与用户 PIE |

### Day 3 可以依赖的契约

1. `ASPlayerState::GetTeam()` 是控制节点判断 Red/Blue 的权威队伍来源。
2. `ASPlayerState::IsAlive()` 是控制节点判断玩家能否参与占点的权威存活来源。
3. Team 和 Alive 只由服务器写入，并通过 PlayerState 复制给客户端。
4. 玩家死亡后旧 Pawn 会被销毁；重生后的 Pawn 与死亡前不是同一个对象。
5. 死亡到重生之间，Controller 和 PlayerState 仍存在，但 Controller 暂时没有 Pawn。
6. Day 3 的区域重叠集合不能长期盲信缓存引用；每次计算前必须过滤无效/已销毁 Pawn、无 `ASPlayerState` 的 Pawn、`IsAlive()==false` 的玩家，以及实际已离开区域的 Pawn。
7. Day 3 不要重新设计开火、伤害或重生 RPC，也不要新增通用 Objective 框架。

### Day 3 不能假设的内容

- 不能假设死亡前保存的 Pawn 引用在重生后仍有效。
- 不能假设 `PendingRespawnTimers` 在一次成功重生后已经为空。
- 不能假设 Alive/Kills/Deaths 已有客户端 `OnRep` 表现函数；目前只有属性复制。
- 不能假设 `ASGameMode_StellarFront::SetPhase()` 已经限制合法阶段边；正常流程的单向边都是 Day 3 要补齐的规则。
- 不能假设 `ASGameState::OnRep_Phase()` 已经提供表现；当前函数为空。
- 不能假设当前 Warmup 已完成：`StartWarmup()` 已存在并创建 Timer，但 `EndWarmup()` 为空，Warmup 目前无法进入 `PreDeploy`。
- 不能假设 `CurrentPhase` 在 `WaitingToStart` 有正确的非玩法值；当前枚举首项是 `WarmingUp`，需要增加 `None` 并明确初始化。
- 控制节点只应在确认当前阶段为 `OrbitalCombat` 时累计进度；测试前先确认比赛已经依次经过 `WarmingUp`、`PreDeploy` 并进入 `OrbitalCombat`。

### 已知但不阻断 Day 3 的技术债

1. `RespawnPlayer()` 执行完成后没有立即从 `PendingRespawnTimers` 移除对应条目；同一 Controller 后续仍可复用该 Handle，但 Match End 的取消数量可能包含已经执行完毕的 Timer。
2. `MulticastHealthChanged()` 接收 `NewHealth`，当前广播时使用成员变量 `Health`；客户端 RPC 与属性复制顺序不保证一致，后续 HUD 工作前应重新检查。
3. `ApplyHealthChange()` 被客户端直接调用时不会修改 Health，但返回值仍可能表示“本来可以发生变化”；当前正式伤害入口已由 `ApplyDamage()` 的 Authority 检查规避。
4. `HandlePlayerDeath()` 当前只从 Pawn 或 Controller 解析 Killer PlayerState；当前弹丸链传入玩家 Pawn，已通过测试，但未来陷阱/环境 Actor 伤害需要另行定义归属。

### Day 3 开始门槛

Day 2 功能门槛已经满足，不需要继续扩展死亡/重生系统。Day 3 按顺序先闭合 Warmup，再实现单一用途 `ASControlNode`。每个任务单独编译和验收，前一项未通过时不要开始后一项。

---

# Day 3：Warmup、控制节点与阶段推进

## 目标

先把 UE `MatchState` 与项目 `EGamePhase` 的入口闭合，再在 `OrbitalCombat` 阶段用一个最小控制节点推进到 `SearchKey`。Day 3 只分为 Warmup、ControlNode 和整体验收三个任务，不能一次混写。

```text
UE WaitingToStart
    CurrentPhase = None
    -> Red / Blue 达到最低人数
    -> StartMatch

UE InProgress
    -> WarmingUp
    -> PreDeploy
    -> OrbitalCombat
    -> SearchKey
```

正常玩法阶段只允许单向推进，不因此创建通用状态机。

---

## Day 3 - 任务 1：闭合 InProgress 内的 Warmup

### 推荐代码形式

`EGamePhase` 增加非玩法哨兵：

```cpp
enum class EGamePhase : uint8
{
    None,
    WarmingUp,
    PreDeploy,
    OrbitalCombat,
    SearchKey,
    UpLoad,
    Evacuation
};
```

`ASGameState::CurrentPhase` 明确初始化为 `None`。`ReadyToStartMatch_Implementation()` 继续只检查 `WaitingToStart` 与 Red/Blue 最低人数，不等待 Warmup。

`ASGameMode_StellarFront` 保留用户当前已经写入的入口，并补完整职责：

```cpp
FTimerHandle WarmupTimerHandle;

UPROPERTY(EditDefaultsOnly, Category = "GameMode")
float WarmupDuration = 10.0f;

void StartWarmup();
void EndWarmup();
bool SetPhase(EGamePhase NewPhase);
```

- `HandleMatchHasStarted()` 在 `Super` 之后调用 `StartWarmup()`。
- `StartWarmup()` 必须确认 UE MatchState 是 `InProgress`，清理/复用旧 Warmup Handle，设置 `WarmingUp` 后创建 Timer。
- `EndWarmup()` 只调用现有 `StartDeployment()`；阶段合法性集中由 `SetPhase()` 判断。
- `EndDeployment()` 只调用 `StartOrbitCombat()`；阶段合法性集中由 `SetPhase()` 判断。
- `HandleMatchHasEnded()` 清除 Warmup、Deploy、Respawn 以及后续 Objective Timer。
- `SetPhase()` 使用显式 `switch` 判断合法边，不用枚举值 `+1`，也不允许任意倒退。

### 涉及文件

- `Framework/Match/SGameMode_StellarFront.h/.cpp`
- `Framework/Match/SGameState.h/.cpp`

### 风险

- 当前 `EndWarmup()` 为空，不补齐时比赛会永久停在 Warmup。
- 当前 `CurrentPhase` 默认落在枚举首项 `WarmingUp`；不增加 `None` 时，`WaitingToStart` 会被错误显示为已经热身。
- `WarmupTimerHandle`、`StartWarmup()`、`EndWarmup()` 应放在 protected 规则区，不作为公共 Blueprint 操作接口。

### 验收标准

- 1 人 PIE 保持 `WaitingToStart + None`，不会进入 Warmup。
- Red/Blue 各 1 人后 UE MatchState 进入 `InProgress`，Phase 进入 `WarmingUp`。
- Warmup 中可以移动、射击、死亡和按 Day 2 规则重生。
- Warmup 正常结束只进入一次 `PreDeploy`，随后才进入 `OrbitalCombat`。

---

## Day 3 - 任务 2：实现单一用途 `ASControlNode`

### 推荐代码形式

- 新建 `World/Objectives/SControlNode.h/.cpp`，类名 `ASControlNode`。
- 使用一个 Box/Sphere Collision 记录区域内 Pawn；`bReplicates = true`，不开 Actor Tick。
- 复制 `ETeam ControllingTeam` 与 `float CaptureProgress`（0~1）。
- 服务器使用 0.1~0.2 秒 Timer 计算，不在客户端推进。
- 每轮过滤无效/已销毁 Pawn、无 `ASPlayerState`、`IsAlive()==false`、Team=None 和实际已离开区域的 Pawn。
- 只有 Phase 为 `OrbitalCombat` 才计算：Red-only 增长；Blue-only 阻止增长；双方同时存在暂停；无人时保持进度。
- Red 首次达到 1.0 后停止节点 Timer，调用 GameMode 的单一用途完成入口；GameMode 幂等地把 Red 节点数设为 1，并推进到 `SearchKey`。
- 不修改 Warmup、开火或重生 RPC；不创建 `ObjectiveBase`。

### 涉及文件

- 新建 `World/Objectives/SControlNode.h/.cpp`
- 修改 `Framework/Match/SGameMode_StellarFront.h/.cpp`
- 修改 `Framework/Match/SGameState.h/.cpp`
- 新建一个只配置 Mesh/材质/碰撞范围的 `BP_ControlNode`
- `FirstPersonExampleMap` 只放置 1 个节点

### 风险

- 死亡后 Pawn 会销毁并由新 Pawn 重生，不能缓存死亡前引用并长期信任。
- 所有非 `OrbitalCombat` Phase 都必须让节点停止累计。
- Phase 完成回调必须幂等，避免重复增加节点计数或重复进入 SearchKey。

### 验收标准

- `WarmingUp`、`PreDeploy` 和 `SearchKey` 中节点均不增长。
- Red 单独占点增长；Blue 进入后暂停/反制；Red 在区域内死亡后旧 Pawn 不再贡献。
- 两端看到相同控制方、进度、Red 节点数和 `SearchKey` 阶段。

---

## Day 3 - 任务 3：整体验收与提交边界

### 推荐执行形式

本任务不增加新规则，只做 Build、Blueprint 编译、2～4 人 Listen Server PIE 和差异整理。

### 当前进度（2026-09-08）

- Day 3 已提交为 `bcaa041`（`done day3`），本次交接审计开始时工作区干净。
- UE 5.7 `Stellar_FrontEditor Mac Development` C++ Build 已通过。
- `SGameMode_BP`、`MainHUD` 与 `BP_ControlNode` 的定向 Blueprint 编译均为 0 error / 0 warning。
- `BP_ControlNode` 已创建，地图中存在一个 `BP_ControlNode_C_1` 实例。
- PIE 日志在 `01:33:34` 与 `01:36:10` 两次记录 `Phase changed: 3 -> 4` 和 `Red control node completed`，证明 `OrbitalCombat -> SearchKey` 核心完成链可重复运行。
- 用户已完成控制节点检测与双端 UI 检查；日志本身只直接证明阶段推进和完成回调，Red/Blue 争夺、死亡过滤及两端进度一致仍属于用户可视验证证据。

### 涉及文件

- 前两项明确列出的 C++、Blueprint 与地图文件
- `Stellar_Front_7Day_Implementation_Plan.md` 记录实际证据和未完成阻断

### 风险

- 当前用户的 Warmup 源码改动未提交；不要被计划文稿修改覆盖。
- 不要把 `589d144` 已存在的全文件空白债务、无关资产或未来 OnlineSubsystem 接入混入 Day 3。
- Build 只证明 UHT/编译/链接，不能替代双端 PIE。

### 最终验收标准

1. 1 人停在 `WaitingToStart + None`。
2. 2 人进入 `InProgress + WarmingUp`，可在靶场战斗和重生。
3. 无掉线时按 `WarmingUp -> PreDeploy -> OrbitalCombat` 推进。
4. 正常进入 OrbitalCombat 后，Red 完成唯一 ControlNode 并只推进一次到 SearchKey。
5. C++ Build、相关 Blueprint 编译、`git diff --check` 和 2～4 人 Listen Server PIE 全部通过后，才建立 Day 3 提交与 Day 4 交接。

---

## Day 3 → Day 4 交接（开始 Day 4 前必读）

### 基线与证据边界

Day 3 已提交为 `bcaa041`（`done day3`），并推送到当前 `origin/main`。该提交闭合了 Warmup、PreDeploy、OrbitalCombat、ControlNode 和 SearchKey 的入口，也加入了 Team 的事件式 UI 同步。开始 Day 4 时应以该提交为基线，不要重做阶段状态机、占点逻辑或 HUD 所有权。

本交接使用四层证据：

1. **源码证据**：GameMode/GameState 阶段边、PlayerState Team 复制、ControlNode 服务器计算和交互 RPC 调用链。
2. **Build 证据**：UE 5.7 `Stellar_FrontEditor Mac Development` 最新构建结果为 `Succeeded`。
3. **Blueprint 证据**：`SGameMode_BP`、`MainHUD`、`BP_ControlNode` 定向编译为 0 error / 0 warning。
4. **PIE 证据**：双人 Listen Server 日志两次出现 `OrbitalCombat -> SearchKey` 与 `Red control node completed`；Red/Blue 区域检测和 UI 一致性由用户可视测试确认。

Build 和 Blueprint 编译不能替代 PIE；日志里的阶段完成也不能单独证明所有争夺分支和 UI 表现。

### Day 3 已完成的运行契约

```text
WaitingToStart + None
    -> Red / Blue 达到最低人数
    -> InProgress + WarmingUp
    -> PreDeploy
    -> OrbitalCombat
    -> ASControlNode 只在该阶段启动服务器 Timer
    -> Red 单独占点推进 CaptureProgress
    -> 完成时调用 GameMode::CompleteRedControlNode
    -> SetPhase(SearchKey)
```

职责边界如下：

- `ASGameMode_StellarFront::SetPhase()` 是唯一合法阶段边入口。
- `ASGameState::CurrentPhase` 复制给客户端，并通过 `OnPhaseChanged` 通知阶段变化。
- `ASControlNode` 不使用 Actor Tick；只在 `OrbitalCombat` 收到阶段事件后启动服务器 Timer，离开该阶段立即停止。
- ControlNode 每轮重新读取重叠 Pawn，并过滤无 PlayerState、死亡玩家和 Team=None；不长期缓存死亡前 Pawn。
- Red 单独存在时增长；Blue 单独存在或双方争夺时不增长；进度不倒退。
- `ControllingTeam`、`CaptureProgress` 和 `RedControlNodes` 已复制；ControlNode 完成回调以阶段边保证只推进一次。
- `ASPlayerState::Team` 使用 `ReplicatedUsing=OnRep_Team`；服务器 `SetTeam()` 和客户端 `OnRep_Team()` 统一广播 `OnTeamChanged`。
- MainHUD 必须在绑定 `OnTeamChanged` 后立即读取一次 `GetTeam()`，避免 Team 已先完成赋值/复制时漏掉初始事件。

### Day 4 可以直接依赖的内容

1. `EGamePhase::SearchKey` 已能由控制节点完成后进入，且已在双人 PIE 中出现。
2. `ASPlayerState::GetTeam()` 是服务器判断 Red/Blue 的队伍来源；`IsAlive()` 是玩家能否拾取目标的存活来源。
3. Team、Alive 和 `bIsCarryingKey` 位于 PlayerState，能跨 Pawn 死亡/重生保留并复制。
4. `USInteractionComponent` 已由本地 Pawn 寻找焦点，并通过 `ServerInteract()` 把目标交给服务器执行 `ISGameplayInterface::Interact()`。
5. `ASGameState` 已有复制字段 `bKeyFound` 与 `KeyHolder`，但还缺少 Day 4 所需的服务器写入口和有效的 `OnRep_KeyStatus` 表现。
6. `ASGameMode_StellarFront::HandlePlayerDeath()` 在销毁 Victim Pawn 前仍能取得 Victim PlayerState 与位置，适合调用密钥掉落。
7. `ASGameMode_StellarFront::Logout()` 已是登出清理入口，Day 4 可在 `Super::Logout()` 前保存持有者状态和最后有效位置。

### Day 4 不能假设的内容

- 不能假设 `ServerInteract()` 已验证阶段、队伍、存活、距离或目标状态；它目前只检查目标非空。`ASNetworkKey::Interact_Implementation()` 必须在服务器重新完成全部校验。
- 不能假设 `bKeyFound`、`KeyHolder` 或 `bIsCarryingKey` 已形成一致的拾取/掉落事务；目前只有字段与基础复制。
- 不能假设死亡前 Pawn 引用能在重生后继续使用；密钥掉落必须在旧 Pawn 销毁前保存位置并清理持有关系。
- 不能从客户端查询 GameMode；密钥 UI 读取 GameState、PlayerState 或复制的 Key Actor。
- 不能新增专用拾取 RPC；继续复用现有 `ServerInteract()` 和 `ISGameplayInterface`。
- 不创建通用 Objective、Inventory 或 Session 框架；Day 4 只新增单一用途 `ASNetworkKey`。

### 开始 Day 4 前的已知遗留项

1. **ControlNode 检测球干扰武器射线**：`CaptureArea` 当前 Object Type 是 `ECC_WorldDynamic`，而 `ASGunBase::WeaponFire()` 的 Object Query 包含 `ECC_WorldDynamic`。PIE 日志多次出现 `[FireTrace] HitActor=BP_ControlNode_C_1 HitComponent=CaptureArea`。应先为占点检测体使用不会进入武器查询的独立 Object Channel，或用等效方式隔离该检测体；不要让不可见占点球改变弹丸瞄准方向。
2. **提交空白债务**：`git show --check bcaa041` 报告 `SControlNode.h/.cpp` 中存在若干 trailing whitespace。它不影响运行，但 Day 4 提交不要继续扩散，整理时只清理相关行。
3. **旧日志中的已修复 UI 错误**：`01:16:36` 曾出现 MainHUD `Assign On Team Changed` 的 `Accessed None`；后续 PIE 已继续运行且未再出现新的同类错误。Day 4 测试仍需观察最新会话，不能把旧错误删除当成新验证。
4. **单人门槛未在最终 ControlNode 会话重新取证**：阶段核心双人流程已通过；若 Day 4 改动触及开局或阶段入口，需重新验证 1 人仍停在 `WaitingToStart + None`。

### Day 4 推荐任务顺序

1. 先隔离 `CaptureArea` 与武器射线，并完成一次最小双人回归。
2. 新建 `World/Objectives/SNetworkKey.h/.cpp`，实现预放置、服务器权威、单一用途的密钥 Actor。
3. 在 `Interact_Implementation()` 校验 Authority、`SearchKey`、Red、Alive、距离和未被持有。
4. 拾取成功时原子更新 Key Actor、`PlayerState::bIsCarryingKey`、`GameState::bKeyFound` 与 `KeyHolder`。
5. 在死亡和 Logout 路径接入同一个单一用途 Drop 入口；在旧 Pawn 销毁或 `Super::Logout()` 前保存掉落位置。
6. 创建只负责 Mesh/碰撞表现的 `BP_NetworkKey`，在地图预放置一个实例。
7. 先做 C++ Build 和目标 Blueprint 编译，再做 2 人 Listen Server PIE：Blue/错误阶段/远距离均拒绝，Red 正确拾取，死亡和登出都能掉落并再次拾取。

### Day 4 提交边界

Day 4 只应包含：

- `World/Objectives/SNetworkKey.h/.cpp`
- `Framework/Match/SGameMode_StellarFront.h/.cpp`
- `Framework/Match/SGameState.h/.cpp`
- 必要时 `Framework/Player/SPlayerState.h/.cpp`
- `BP_NetworkKey` 与 `FirstPersonExampleMap`
- 本计划中的 Day 4 实际证据更新

不要把上传区、撤离、Inventory、在线服务或通用 Objective 框架混入同一提交。

---

# Day 4：网络密钥拾取、持有与掉落

## 目标

在 `SearchKey` 阶段让 Red 拾取密钥；持有者死亡或登出时密钥回到世界。

## 新建与修改

- 新建 `World/Objectives/SNetworkKey.h/.cpp`，类名 `ASNetworkKey`。
- 修改 `SGameMode_StellarFront.*`、`SGameState.*`、必要时 `SPlayerState.*`。
- 复用 `USInteractionComponent` 与 `ISGameplayInterface::Interact`，不新增拾取输入和 RPC。

## 工作项

1. 密钥是预放置 Actor；只在 `SearchKey` 阶段激活，不做随机出生点。
2. `Interact` 在服务器验证：当前阶段、交互者是 Red、玩家存活、距离足够近、密钥未被持有。
3. 拾取成功后：
   - Key Actor 关联/附着到持有 Pawn 或隐藏其世界表现。
   - `PlayerState::SetCarryingKey(true)`。
   - `GameState` 更新 `bKeyFound` 与 `KeyHolder`。
4. Day 2 的死亡处理与 Logout 调用 Key 的 Drop：清空持有者、在死亡点/最后有效位置重新放回世界。
5. 不做全图脉冲、头顶图标、假信号源或随机密钥点。

## 验收

- Blue 无法拾取。
- Red 只能在 `SearchKey` 阶段、近距离拾取。
- 两端的 KeyHolder 与 CarryingKey 状态同步。
- 持有者死亡/登出后，密钥可再次被拾取。

---

## Day 4 → Day 5 交接（开始 Day 5 前必读）

### 当前基线与证据边界

1. 当前 `HEAD` 仍是 Day 3 提交 `bcaa041`（`done day3`），`origin/main` 也指向该提交；Day 4 尚未提交，全部位于当前工作区。
2. 当前源码静态检查确认 Day 4 已形成以下服务器权威链：
   - `ASNetworkKey` 只在服务器处理拾取和掉落，并复制世界显隐状态。
   - 拾取同时写入 `ASPlayerState::bIsCarryingKey`、`ASGameState::KeyHolder` 与 `bKeyFound`。
   - 手动按键、死亡和 Logout 都复用 `ASNetworkKey::DropKey()`；Drop 不依赖 GameMode 修改 Key 状态。
   - 通用交互组件使用原有 `TraceDistance + TraceRadius` 做服务器距离复核，没有给 Key 新增独立交互距离。
3. 已有 UnrealBuildTool 记录显示工程构建成功；该记录最终增量编译了 GameMode 并完成链接，其他 Day 4 C++ 文件当时为最新状态。它只能作为已有 C++ Build 证据，不替代 Blueprint 编译或 PIE。
4. `BP_Key`、`IA_DropKey`、`IMC_DefaultPlayer`、`BP_Player` 和地图均已有对应工作区改动。Key 蓝图实际命名为 `BP_Key`，与早期计划中的 `BP_NetworkKey` 名称不同；Day 5 不需要为了名称统一而重命名资产。
5. 用户于 2026-09-15 确认当前 Day 4 运行测试通过。本交接没有单独保存新的 PIE 日志，因此只记录“用户确认当前测试通过”，不把未留日志的各分支写成独立自动化证据。

### Day 4 当前运行契约

```text
Primary Interact
    -> USInteractionComponent 服务器复核接口与距离
    -> ASNetworkKey::Interact_Implementation
    -> 校验 Authority / Active / SearchKey / Red / Alive
    -> PlayerState.CarryingKey = true
    -> GameState.KeyHolder = PlayerState，bKeyFound = true
    -> Key 隐藏并关闭碰撞

数字键 5 / 持有者死亡 / Logout
    -> 服务器找到当前 Key
    -> ASNetworkKey::DropKey(原持有者, 掉落位置)
    -> 校验记录持有者与请求持有者一致
    -> PlayerState.CarryingKey = false
    -> GameState.KeyHolder = nullptr，bKeyFound = false
    -> Key 移到掉落位置并重新显示
```

- 手动掉落使用 `IA_DropKey`，由 `BP_Player` 提供给 `ASCharacter::Input_DropKey`，在 `Started` 触发服务器 RPC。
- Key 状态以 GameState/PlayerState 为权威数据；GameMode 只在死亡、Logout 生命周期入口要求 Key 执行同一个 Drop，不负责直接改写 KeyHolder。
- `ForceNetUpdate()` 只用于促使本次 Key Actor 状态尽快进入网络更新，不代替属性复制声明和 `OnRep`。
- `ASGameState::OnRep_KeyFound()` 当前为空。字段仍会复制，但现阶段没有 Key UI 事件；Day 5 如果需要显示状态，应围绕实际 UI 需求补最小事件，不新增通用 Objective 框架。

### 开始 Day 5 前仍需处理的入口问题

Day 4 拾取成功后目前仍停留在 `SearchKey`。虽然 `ASGameMode_StellarFront::SetPhase()` 已允许 `SearchKey -> UpLoad`，但 `EndSearchKey()` 为空，Key 拾取链也没有调用任何阶段完成入口。因此 UploadZone 若严格限定在 `UpLoad`，当前流程不会自行激活它。

此外，Day 5 明确要求持有者在上传阶段死亡或掉落后停止上传。当前 `ASNetworkKey::DropKey()` 和拾取校验只接受 `SearchKey`；如果拾取后立即进入 `UpLoad`，这些校验会拒绝上传阶段的掉落或重新拾取。Day 5 的第一个任务必须一起闭合以下最小规则：

1. GameMode 增加或完成一个单一用途的 SearchKey 完成入口：仅当阶段仍为 `SearchKey` 且 GameState 已有有效 KeyHolder 时，调用现有 `SetPhase(EGamePhase::UpLoad)`。
2. Key 首次在 `SearchKey` 拾取成功后请求该阶段入口；Key 的拾取/掉落状态事务仍由 Key、GameState 和 PlayerState 完成，Drop 本身不调用 GameMode。
3. `ASNetworkKey` 在 `UpLoad` 阶段也允许当前持有者掉落，并允许 Red 重新拾取掉落的 Key；重新拾取时不得再次推进阶段。
4. 不修改 `SetPhase()` 的集中合法边规则，不新增通用 Objective 基类、管理器或事件总线。

### 当前工作区边界

Day 4 相关的未提交内容至少包括：

- `Config/DefaultEngine.ini`
- `Source/Stellar_Front/World/Objectives/SNetworkKey.h/.cpp`
- `Source/Stellar_Front/Character/SCharacter.h/.cpp`
- `Source/Stellar_Front/Gameplay/Interaction/SInteractionComponent.h/.cpp`
- `Source/Stellar_Front/Framework/Match/SGameMode_StellarFront.h/.cpp`
- `Source/Stellar_Front/Framework/Match/SGameState.h/.cpp`
- `Source/Stellar_Front/Framework/Player/SPlayerState.h`
- `Content/Blueprints/BP_Key.uasset`
- `Content/Input/IA_DropKey.uasset`
- `Content/Input/IMC_DefaultPlayer.uasset`
- `Content/Blueprints/BP_Player.uasset`
- `Content/Maps/FirstPersonExampleMap.umap`

工作区同时还有 `SControlNode.cpp`、两个材质资产、`SGameMode_BP.uasset`、本计划和 `问题StellarFront.docx` 等改动。新对话必须先重新查看 `git status` 与差异，不能默认它们都属于 Day 4，也不能覆盖或顺手整理这些用户改动。

当前 `git diff --check` 仍报告若干 C++ 行尾空白。它不否定已经通过的运行测试，但在提交 Day 4 前应只清理本次涉及行的空白并重新检查，不做无关格式化。

### Day 5 推荐任务顺序（一次只做一个）

1. **先闭合 `SearchKey -> UpLoad` 与上传阶段 Key 生命周期。** 按上一节四条最小规则修改，并单独验收阶段推进、上传阶段手动/死亡掉落和重新拾取。
2. **再补 GameState 上传数据。** 增加服务器写入口和复制的 `UploadProgress`；如果本阶段确有 UI，再补对应 `OnRep`/事件，否则只保留可验证的数据状态。
3. **再实现 `ASUploadZone`。** 使用一个区域组件、服务器 Overlap 集合和 Timer；只读取 GameState 的 Phase、KeyHolder、CarryingKey 与队伍状态，不使用客户端 Tick。
4. **再接完成入口。** 上传到 1.0 时由 GameMode 幂等确认 `UpLoad` 并通过现有 `SetPhase()` 进入 `Evacuation`。
5. **最后做蓝图、地图与分层验收。** 放置一个 UploadZone，分别记录 C++ Build、相关 Blueprint Compile 和双人 Listen Server PIE 证据。

Day 5 任务一完成前，不应先创建 UploadZone；否则区域会因为阶段仍停在 `SearchKey` 而看似失效，并掩盖真正的阶段入口缺口。

---

# Day 5：上传区与进度

## 目标

Red 把密钥带到上传区，并在被 Blue 争夺时正确计算服务器权威的上传进度。

## 新建与修改

- 新建 `World/Objectives/SUploadZone.h/.cpp`，类名 `ASUploadZone`。
- 修改 `SGameMode_StellarFront.*`、`SGameState.*`。
- 地图放置 1 个 UploadZone。

## 工作项

1. UploadZone 只在 `UpLoad` 阶段激活。密钥持有者进入区域后由服务器启动上传。
2. 上传进度只保存在 GameState 的 `UploadProgress`，并由服务器 Timer 更新；不要在客户端 Tick 中推进。
3. 最小规则：
   - Red 持有密钥且区域内没有 Blue：进度增长。
   - Blue 在区域内：进度暂停。
   - 持有者离开、死亡、掉落密钥：停止上传，不清零已积累进度。
4. 上传达到 1.0 时，GameMode 确认 Phase 仍是 `UpLoad`，再推进到 `Evacuation`。
5. 不做多人加速、进度倒退、音效层、复杂 UI 动画。

## 验收

- 没有密钥、错误阶段或 Blue 交互时，上传不能开始。
- Blue 进入区域后，两个客户端看到进度停止。
- 上传完成只触发一次 `Evacuation`。

---

# Day 6：撤离与最小胜负

## 目标

让 Blue 在 `Evacuation` 阶段通过撤离区离场；时间结束或撤离成功后结算比赛。

## 新建与修改

- 新建 `World/Objectives/SEvacZone.h/.cpp`，类名 `ASEvacZone`。
- 修改 `SGameMode_StellarFront.*`、`SGameState.*`、`SPlayerState.*`。
- 地图放置 1 个 EvacZone。

## 工作项

1. EvacZone 仅在 `Evacuation` 阶段启用；只允许存活 Blue 玩家交互撤离。
2. 使用现有交互 RPC，并在服务器检查阶段、队伍、存活和距离。
3. 撤离成功后：
   - `PlayerState::MarkEvacuated()`。
   - 从战场移除该 Pawn 或切换为旁观，不再重生。
4. GameMode 维护一个服务器结束 Timer：
   - 时间耗尽：Red 胜。
   - 当前存活的 Blue 都撤离：Blue 胜。
5. 胜负先保存在 GameState 的一个最小枚举/结果字段，随后调用 `EndMatch`。
6. 不做撤离舰生命值、引导条、登舰动画、雷达禁用或多艘撤离舰。

## 验收

- Red 无法撤离，非 Evacuation 阶段无法撤离。
- Blue 撤离后不再重生。
- 两端在同一条件下结束比赛，并得到相同胜负结果。

---

# Day 7：最小 HUD 与端到端验收

## 目标

让玩家能看懂当前流程，并完成 2 人 Listen Server 的一轮完整通关验证。

## HUD 范围

只增加必要信息，不重做 UI 美术：

- 当前阶段：`WarmingUp` / `PreDeploy` / `OrbitalCombat` / `SearchKey` / `UpLoad` / `Evacuation`。
- 队伍、Alive 状态、持钥状态。
- 控制点进度、上传进度、撤离倒计时（当该阶段激活时显示）。
- 保留当前弹药 UI。

## 工作项

1. 将 HUD 创建和引用持久化到 `ASPlayerController`；不要继续依赖 `Pawn::BeginPlay`，因为 Pawn 会重生。
2. `UMainWidget` 从 PlayerState/GameState 的复制变量读取显示值；UI 不调用服务器规则函数。
3. 给关键阶段变化添加临时日志或最小文字提示，便于 PIE 排错。
4. 按一次完整流程验证：
   - 两人加入、分队、出生。
   - 进入 Warmup 并正常结束到 PreDeploy。
   - Red 控制节点。
   - Red 拾取密钥，死亡后掉落并可重拾。
   - Red 到上传区，Blue 能暂停上传。
   - 上传完成进入撤离；Blue 撤离或时间到结束比赛。
5. 逐项记录问题：复现步骤、服务器/客户端、日志、预期与实际；只修复阻断主流程的问题。

## 最终验收清单

| 检查项 | 通过条件 |
|---|---|
| 连接与出生 | 两人稳定分队、出生点正确。 |
| Warmup | UE 已是 InProgress；玩家可在靶场战斗，正常结束后进入 PreDeploy。 |
| 战斗 | 弹丸移动同步；伤害和死亡只由服务器决定。 |
| 节点 | Red 控制节点后阶段推进一次。 |
| 密钥 | 拾取、掉落、重拾和同步正确。 |
| 上传 | 服务器推进，Blue 可暂停，完成一次。 |
| 撤离与结束 | Blue 撤离或时间到，双方进入同一结算状态。 |
| HUD | 两端看到同一阶段与必要目标状态。 |

## 本周不做的内容

- 20v20 人数、专用服务器、真实匹配/房间、Session 生命周期、Steam/EOS。
- 资源产出、轨道控制度、武器解锁、职业选择、建筑、载具、传送门。
- 随机部署、假信号、复杂撤离舰、完整计分、助攻、排行榜。
- 复杂命中反馈、击杀回放、小地图、音频混音和正式 UI 美术。

这些内容只会在 2 人完整流程稳定后再评估。
