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
