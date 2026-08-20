# 《Stellar Front》7 天基础游玩逻辑实施计划

> 更新：2026-08-18  
> 范围：把现有第一人称射击工程做成一个可在 **2 人 Listen Server PIE** 中完整游玩的最小闭环。  
> 原则：优先复用当前 C++ 工程；服务器拥有规则、状态与判定；不为未来 20v20 建框架。

## 1. 本周交付目标

本周结束时，两名玩家可以进入同一张地图并完成如下流程：

```text
WaitingToStart
  -> PreDeploy
  -> OrbitalCombat（控制节点）
  -> SearchKey（拾取并护送密钥）
  -> UpLoad（上传）
  -> Evacuation（撤离）
  -> 结算 / 回到等待状态
```

- Red：进攻方；推进控制节点、取得密钥、完成上传。
- Blue：防守方；阻止进攻，并在撤离阶段逃离。
- 目标人数是 2 人，最多先验证 4 人；不做 20v20 平衡、匹配、在线服务、资源经济、武器 Tier、载具、职业选择、随机部署或完整计分系统。
- 每天结束都必须先通过 C++ 编译，再做与当天有关的双窗口 PIE 验证；编译成功不等于 PIE 已通过。

## 2. 现有工程边界

| 责任 | 当前工程位置 | 本周约定 |
|---|---|---|
| 比赛规则与阶段 | `Framework/Match/SGameMode_StellarFront.*` | 只在服务器运行；唯一负责阶段推进和胜负。 |
| 全局同步状态 | `Framework/Match/SGameState.*` | 只保存要让所有客户端读取的状态。 |
| 玩家队伍/死亡/目标状态 | `Framework/Player/SPlayerState.*` | 服务器写入，客户端通过复制读取。 |
| 玩家控制器 | `Framework/Player/SPlayerController.*` | 后期作为持久 HUD 与客户端 RPC 入口。 |
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

- 当前 HEAD 仍为 `b9f97fb`（`finish with AssignTeam logic & Projectile Replicate Movement`）。
- Day 2 改动尚未形成新提交，当前工作区为 dirty 状态。
- `Config/DefaultEngine.ini` 当前将默认 GameMode 指向 `/Game/Blueprints/SGameMode_BP.SGameMode_BP_C`。
- `SGameMode_BP` 使用 `ASGameMode_StellarFront`，并配置当前 `BP_Player`、`SGameState`、`SPlayerState` 和 `SPlayerController_BP` 类链。
- `FirstPersonExampleMap` 当前有 4 个 PlayerStart：2 个 `Red`、2 个 `Blue`。
- `git diff --check` 当前仍报告多处 trailing whitespace；这不影响已完成的 PIE 功能验证，但必须在建立 Day 2 提交基线前清理。
- 工作区还包含 `Content/Environment/M_Cube_Inst.uasset` 和 `问题StellarFront.docx` 等非 Day 2 主链改动；提交 Day 2 时必须单独确认归属，不要无条件混入死亡/重生提交。

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
- 不能假设 `ASGameMode_StellarFront::SetPhase()` 已经限制合法单向推进；这是 Day 3 要补齐的阶段规则。
- 不能假设 `ASGameState::OnRep_Phase()` 已经提供表现；当前函数为空。
- 控制节点只应在确认当前阶段为 `OrbitalCombat` 时累计进度；测试前先确认比赛已离开 `WaitingToStart/PreDeploy`。

### 已知但不阻断 Day 3 的技术债

1. `RespawnPlayer()` 执行完成后没有立即从 `PendingRespawnTimers` 移除对应条目；同一 Controller 后续仍可复用该 Handle，但 Match End 的取消数量可能包含已经执行完毕的 Timer。
2. `MulticastHealthChanged()` 接收 `NewHealth`，当前广播时使用成员变量 `Health`；客户端 RPC 与属性复制顺序不保证一致，后续 HUD 工作前应重新检查。
3. `ApplyHealthChange()` 被客户端直接调用时不会修改 Health，但返回值仍可能表示“本来可以发生变化”；当前正式伤害入口已由 `ApplyDamage()` 的 Authority 检查规避。
4. `HandlePlayerDeath()` 当前只从 Pawn 或 Controller 解析 Killer PlayerState；当前弹丸链传入玩家 Pawn，已通过测试，但未来陷阱/环境 Actor 伤害需要另行定义归属。

### Day 3 开始门槛

Day 2 功能门槛已经满足，不需要继续扩展死亡/重生系统。开始 Day 3 前只需把当前 Day 2 工作区视为待固定基线：确认非 Day 2 资产归属、清理 `git diff --check`、完成一次与当前源码对应的 Build，并保存/编译相关 Blueprint。随后 Day 3 只实现计划中的单一用途 `ASControlNode`，不要把上述非阻断技术债扩展成新的通用系统。

---

# Day 3：轨道控制节点与阶段推进

## 目标

在 `OrbitalCombat` 阶段用一个最小控制节点决定是否推进到 `SearchKey`。

## 新建与修改

- 新建 `World/Objectives/SControlNode.h/.cpp`，类名 `ASControlNode`。
- 修改 `SGameMode_StellarFront.*`、`SGameState.*`。
- 地图放置 1 个 ControlNode；不要先做 2~3 个节点和资源收益。

## 工作项

1. `ASControlNode` 使用 Box/Sphere Collision 记录区域内 Pawn，并复制：控制方、进度（0~1）。
2. 由服务器 0.1~0.2 秒 Timer 计算进度：
   - 只有 Red 存活玩家在区内：Red 进度增长。
   - 只有 Blue 存活玩家在区内：Blue 进度增长或阻止 Red。
   - 双方同时在区内：暂停。
3. 服务器每次计算前过滤无效、死亡、已离开区域的 Pawn。
4. 节点首次被 Red 控制时，GameMode 更新 `SGameState` 的节点计数并进入 `SearchKey`。
5. `SetPhase` 只允许合法的单向推进；Day 3 不做倒计时自动跳阶段。

## 验收

- Phase 不是 `OrbitalCombat` 时，节点不积累进度。
- Red 单独占点可推进；Blue 进入后暂停/反制。
- 两端看到相同节点进度与 GameState 阶段变化。

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

- 当前阶段：`PreDeploy` / `OrbitalCombat` / `SearchKey` / `UpLoad` / `Evacuation`。
- 队伍、Alive 状态、持钥状态。
- 控制点进度、上传进度、撤离倒计时（当该阶段激活时显示）。
- 保留当前弹药 UI。

## 工作项

1. 将 HUD 创建和引用持久化到 `ASPlayerController`；不要继续依赖 `Pawn::BeginPlay`，因为 Pawn 会重生。
2. `UMainWidget` 从 PlayerState/GameState 的复制变量读取显示值；UI 不调用服务器规则函数。
3. 给关键阶段变化添加临时日志或最小文字提示，便于 PIE 排错。
4. 按一次完整流程验证：
   - 两人加入、分队、出生。
   - Red 控制节点。
   - Red 拾取密钥，死亡后掉落并可重拾。
   - Red 到上传区，Blue 能暂停上传。
   - 上传完成进入撤离；Blue 撤离或时间到结束比赛。
5. 逐项记录问题：复现步骤、服务器/客户端、日志、预期与实际；只修复阻断主流程的问题。

## 最终验收清单

| 检查项 | 通过条件 |
|---|---|
| 连接与出生 | 两人稳定分队、出生点正确。 |
| 战斗 | 弹丸移动同步；伤害和死亡只由服务器决定。 |
| 节点 | Red 控制节点后阶段推进一次。 |
| 密钥 | 拾取、掉落、重拾和同步正确。 |
| 上传 | 服务器推进，Blue 可暂停，完成一次。 |
| 撤离与结束 | Blue 撤离或时间到，双方进入同一结算状态。 |
| HUD | 两端看到同一阶段与必要目标状态。 |

## 本周不做的内容

- 20v20 人数、专用服务器、匹配/房间、Steam/EOS。
- 资源产出、轨道控制度、武器解锁、职业选择、建筑、载具、传送门。
- 随机部署、假信号、复杂撤离舰、完整计分、助攻、排行榜。
- 复杂命中反馈、击杀回放、小地图、音频混音和正式 UI 美术。

这些内容只会在 2 人完整流程稳定后再评估。
