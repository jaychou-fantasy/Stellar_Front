# 《Stellar Front》Demo 实现计划

---
## 当前进度

| 步骤 | 状态 | 完成时间 |
|------|------|---------|
| 1 — SPlayerState Replicated 变量 | ✅ 完成 | — |
| 2 — ASGameState 全局状态 | ✅ 基本完成 | — |
| 3 — 修复 GameMode 裸指针 | ⬜ 待做 | — |
| 4 — 队伍分配与比赛启动 | ⬜ 待做 | — |
| 5 — 阶段转换系统 | ⬜ 待做 | — |
| 6 — 资源系统 | ⬜ 待做 | — |
| 7 — 武器层级解锁 | ⬜ 待做 | — |
| 8 — 空间站控制节点占领 | ⬜ 待做 | — |
| 9 — 网络密钥搜索与持有 | ⬜ 待做 | — |
| 10 — 主节点上传攻防 | ⬜ 待做 | — |
| 11 — 撤离系统 | ⬜ 待做 | — |
| 12 — 计分与胜负 | ⬜ 待做 | — |
| 13 — Demo 地图搭建 | ⬜ 待做 | — |
| 14 — HUD/UI | ⬜ 待做 | — |
| 15 — 端到端测试 | ⬜ 待做 | — |

> **SGameState 待修复提醒**: `GetLifetimeReplicatedProps` 尚未实现，当前所有 Replicated 变量实际上不会复制。需要在 `.h` 加 `virtual void GetLifetimeReplicatedProps(...) const override;`，在 `.cpp` 加 `DOREPLIFETIME` 注册所有变量。

---
## 总览

本文档基于《Stellar Front》游戏策划案（`Stellar_Front_Game_Design.md`）和 Demo 架构设计方案（`DemoStrategy.docx`），将 Demo 开发过程拆分为 **5 个阶段、15 个步骤**。

**Demo 目标**: 实现 20v20 的垂直切片——"空间站 → 登陆区 → 能源基地"，包含四阶段完整核心循环（轨道争夺 → 搜索密钥 → 主节点上传 → 撤离）。

---

## 第一阶段：网络复制基础设施

### 步骤 1：完善 SPlayerState 的 Replicated 变量

**目标**: 确保每个玩家的个人状态在所有客户端上正确同步。

**涉及文件**: `SPlayerState.h`, `SPlayerState.cpp`

**具体工作**:
1. **添加新变量**（标记 `Replicated`）:
   - `bIsAlive` (bool) — 是否存活（被击杀后为 false，复活后为 true）

2. **关于 Score（得分）**: 无需自定义变量。APlayerState 基类已内置 `float Score` 成员（Replicated）和 `GetScore()`/`SetScore()`/`AddScore()` 公有函数，直接使用基类即可。

3. **关于 Assists（助攻）**: **保留注释状态，不取消注释。** 助攻的判定边界（什么算"有效助攻"）尚未确定——是造成伤害 ≥N%？是击杀前 X 秒内命中过目标？还是其他标准？在助攻判定规则明确之前，Assists 的设计保留但代码全部保持注释化。

4. **关于撤离**: 撤离只有成功/不成功两种状态，用 `bHasEvacuated` (bool) 表示即可。**不需要** `SuccessfulEvacs` 次数统计。一局比赛中每个玩家最多撤离一次，布尔值完全够用。

2. **修复现有变量**:
   - `bHasEvaucaute` 有拼写错误（应为 `bHasEvacuated`），如果外部代码未引用则重命名
   - 确认 `ETeam` 枚举定义位置 —— 如果保留在 SPlayerState.h 则不需移动，如果在 SGameState.h 也定义了一份则删除重复

3. **在 `GetLifetimeReplicatedProps` 中注册所有新变量的复制**:
   ```cpp
   // DOREPLIFETIME(ASPlayerState, Assists);  // 保留注释，助攻判定规则待定
   DOREPLIFETIME(ASPlayerState, bIsAlive);
   // Score 使用基类 APlayerState 内置的 Score，无需额外注册
   // 撤离只有 bool，无需 SuccessfulEvacs 次数
   ```

4. **辅助函数**（仅服务器调用，带 `HasAuthority()` 检查）:
   - `SetIsAlive(bool bAlive)` — 标记玩家存活/死亡
   - `MarkEvacuated()` — 标记撤离成功（内部只设置 `bHasEvacuated = true`，不需要计数器）
   - `SetCarryingKey(bool bCarry)` — 设置密钥持有状态
   - `AddScore()` 由基类 APlayerState 提供，无需自定义
   - `AddAssist()` — 保留注释，助攻判定规则待定

**验收标准**: 编译通过，在 PIE 多窗口模式下，Server 调用 AddKill/AddDeath 后，Client 窗口可以看到对应的 PlayerState 变量更新。

---

### 步骤 2：构建 ASGameState 全局状态

**目标**: 让 GameState 承载 Demo 所需的所有全局战场状态，客户端通过自动复制感知战局变化。

**涉及文件**: `SGameState.h`, `SGameState.cpp`

**具体工作**:

1. **清理旧代码**:
   - 删除 `EMatchState` 枚举（用 UE 内置的 `FName MatchState` 代替）
   - 保留 `EGamePhase` 枚举（OrbitalCombat, SearchKey, UpLoad, Evacuation）

2. **新增 `FTeamResources` 结构体**:
   ```cpp
   USTRUCT(BlueprintType)
   struct FTeamResources
   {
       GENERATED_BODY()
       UPROPERTY(BlueprintReadOnly)
       float Energy = 100.0f;
       UPROPERTY(BlueprintReadOnly)
       float WarResources = 100.0f;
       UPROPERTY(BlueprintReadOnly)
       float OrbitalControl = 50.0f;
   };
   ```

3. **新增 ASGameState 成员变量**（全部 `Replicated`）:
   - **阶段相关**: `PhaseTimeRemaining` (float)
   - **阵营资源**: `RedResources`, `BlueResources` (FTeamResources，带 `ReplicatedUsing = OnRep_TeamResources`)
   - **武器层级**: `RedWeaponTier`, `BlueWeaponTier` (int32，默认 1)
   - **空间站控制**: `RedControlNodes`, `BlueControlNodes` (int32)
   - **密钥状态**: `bKeyFound` (bool，带 `ReplicatedUsing = OnRep_KeyStatus`), `KeyHolder` (APlayerState*), `UploadZoneLocation` (FVector), `UploadProgress` (float, 0.0~1.0)
   - **撤离状态**: `EvacShipLocation` (FVector), `EvacTimeRemaining` (float)
   - **双方比分**: `RedScore`, `BlueScore` (int32)

4. **实现 RepNotify 回调函数**:
   - `OnRep_Phase()` — 阶段切换时更新客户端 UI 和游戏规则
   - `OnRep_TeamResources()` — 资源变化时更新客户端资源条
   - `OnRep_KeyStatus()` — 密钥状态变化时播报提示

5. **便捷访问函数**:
   - `GetPhase()` → 返回 `CurrentPhase`
   - `GetTeamResources(ETeam Team)` → 返回对应队伍的 FTeamResources 引用

6. **实现 `GetLifetimeReplicatedProps`**，注册所有变量的复制规则

**验收标准**: 编译通过。在 GameMode 中修改 GameState 变量后，所有客户端自动同步。

---

### 步骤 3：修复 ASGameMode_StellarFront 的裸指针问题

**目标**: 清理当前代码中的严重错误，为后续逻辑打下正确基础。

**涉及文件**: `SGameMode_StellarFront.h`, `SGameMode_StellarFront.cpp`

**具体工作**:
1. **删除裸枚举指针**（第 40-41 行）:
   ```cpp
   // 删除这两行
   EMatchState* MatchState;   // 裸指针，毫无意义
   EGamePhase* GamePhase;     // 裸指针，毫无意义
   ```

2. **删除冗余的前向声明**（`enum class EMatchState : uint8;` 和 `enum class EGamePhase : uint8;` 已在 SGameState.h 中定义且有 `#include`）

3. **删除 `bReady` (bool) 成员** — 如果后续不需要则删；如果需要，改为读取 GameState 的状态来判断

4. **添加辅助函数**用于获取 GameState 的便捷转换:
   ```cpp
   ASGameState* GetStellarGameState() const;
   ```

**验收标准**: 编译通过，无警告。GameMode 不再包含裸指针和未定义枚举的前向声明。

---

## 第二阶段：核心游戏循环

### 步骤 4：队伍分配与比赛启动

**目标**: 实现两队平衡分配、角色选择（Assault/Engineer）、比赛启动条件判断。

**涉及文件**: `SGameMode_StellarFront.h`, `SGameMode_StellarFront.cpp`

**具体工作**:

1. **完善 `AssignTeam(APlayerState* PlayerState)`**:
   - 当前实现有一个 bug：`MyPS->GetTeam()` 被赋值给 `MyTeam`（值拷贝），然后 `MyTeam` 被修改但从未写回 `MyPS`。需要用正确的方式将队伍写入 PlayerState：
     ```cpp
     ETteam AssignedTeam = (RedCnt <= BlueCnt) ? ETeam::Red : ETeam::Blue;
     // 需要直接设置 PlayerState 的 Team 成员
     ```
   - 注意：`Team` 在 SPlayerState 中是 protected 成员，需要添加一个 `SetTeam()` 函数或将其改为 public 并在 GameMode 中通过 friend/访问函数设置

2. **添加角色分配逻辑**:
   - 在 SPlayerState 中添加 `SetRole(EPlayerRole NewRole)` 函数
   - 在 PostLogin 或战前部署阶段允许玩家选择角色
   - Demo 阶段可简化为：默认 Assault，玩家可通过 UI 切换为 Engineer

3. **完善 `ReadyToStartMatch_Implementation()`**:
   - 检查条件：双方队伍至少各有 1 名玩家（Demo 阶段放宽）
   - 可加入倒计时：人数满足后等待 30 秒自动开始

4. **完善 `HandleMatchHasStarted()`**:
   - 将 MatchState 设置为 `InProgress`（UE 内置机制）
   - 调用 `SetPhase(EGamePhase::OrbitalCombat)` 进入第一阶段
   - 初始化 GameState 各变量的默认值

**验收标准**: 玩家登录后被正确分配到 Red/Blue 队伍，人数尽量均衡。比赛满足条件后自动开始并进入 OrbitalCombat 阶段。

---

### 步骤 5：阶段转换系统

**目标**: 实现四阶段（OrbitalCombat → SearchKey → UpLoad → Evacuation）的完整自动转换逻辑。

**涉及文件**: `SGameMode_StellarFront.h`, `SGameMode_StellarFront.cpp`

**具体工作**:

1. **实现 `SetPhase(EGamePhase NewPhase)`**:
   - 写入 `ASGameState::CurrentPhase`（触发自动复制）
   - 设置 `PhaseTimeRemaining`
   - 根据新阶段启用/禁用对应的游戏规则：
     - `OrbitalCombat`: 允许太空战机生成，激活空间站控制点
     - `SearchKey`: 揭示密钥藏匿区域提示，激活假信号源，KeyHolder 周期性暴露位置
     - `UpLoad`: 揭示上传区域位置，重置 UploadProgress
     - `Evacuation`: 禁用防守方复活和雷达，生成撤离舰位置

2. **实现阶段评估函数**（通过 FTimerHandle 定时调用）:
   - `EvaluateOrbitalPhase()`: 检查是否一方控制空间站节点达标 → 转入 SearchKey
   - `EvaluateSearchPhase()`: 检查进攻方是否将密钥送达上传区 → 转入 UpLoad
   - `EvaluateUploadPhase()`: 检查 UploadProgress ≥ 1.0 → 转入 Evacuation
   - `EvaluateEvacuation()`: 检查撤离时间耗尽或所有防守方已撤离 → 比赛结束

3. **阶段转换条件汇总表**:

   | 当前阶段 | 触发条件 | 下一阶段 |
   |---------|---------|---------|
   | 等待中 | MatchState → InProgress | OrbitalCombat |
   | OrbitalCombat | 一方控制节点 ≥ 阈值 或 Timer 耗尽 | SearchKey |
   | SearchKey | KeyHolder 抵达 UploadZoneLocation | UpLoad |
   | UpLoad | UploadProgress ≥ 1.0 (持续占领30秒) | Evacuation |
   | Evacuation | Timer耗尽 或 防守方全员撤离/死亡 | MatchEnd |

4. **设置定时检查**:
   - 在 GameMode 构造函数中初始化 `FTimerHandle PhaseCheckTimer`
   - 每 1 秒调用一次当前阶段的 Evaluate 函数

**验收标准**: 可以手动触发阶段推进条件（如控制台命令），观察 GameState 的 CurrentPhase 在各客户端正确更新。

---

### 步骤 6：资源系统（Demo 简化版）

**目标**: 实现能源值、战争资源的基本模拟，以及对防御设施和复活的影响。

**涉及文件**: `SGameMode_StellarFront.h`, `SGameMode_StellarFront.cpp`

**具体工作**:

1. **简化版资源模型**（设计文档第七章的 Demo 简化版）:
   - 不用完整的 "遍历所有战略区计算占有率" 逻辑
   - 用一个简单的公式代替：
     - 每控制一个空间站节点 → `Energy += 5/min`, `WarResources += 3/min`
     - 能源基地被控制 → `Energy += 10/min`
     - 轨道控制度 = 已方空间站节点数 / 总节点数

2. **实现 `UpdateTeamResources()`**（每 5 秒一次 Timer）:
   - 根据当前控制状态计算增量
   - 更新 GameState 的 `RedResources` / `BlueResources`
   - 检查资源是否低于阈值（低于 30 触发"雪崩效应"警告）
   - 雪崩效应（Demo 版暂不实现具体效果，仅打印日志和触发 UI 警告）

3. **资源使用的 Demo 简化**:
   - Demo 阶段暂不实现完整的资源消耗（轨道炮、传送门等消耗在第二版再做）
   - 仅做资源数值的累积和同步展示

4. **添加蓝图可调用接口**:
   - `GetTeamEnergy(ETeam Team)`, `GetTeamWarResources(ETeam Team)` 等

**验收标准**: 在 GameState 可以看到 RedResources/BlueResources 每 5 秒自动更新并同步到客户端。UI 资源条可以正确显示。

---

### 步骤 7：武器层级解锁系统（Demo 简化版）

**目标**: Demo 阶段先全部开放武器，但搭建好层级系统的数据框架，方便第二版实现递进解锁。

**涉及文件**: `SGameMode_StellarFront.h`, `SGameMode_StellarFront.cpp`（可选：新文件 `SWeaponTierData.h`）

**具体工作**:

1. **定义武器层级数据结构**:
   ```cpp
   USTRUCT(BlueprintType)
   struct FWeaponTierConfig
   {
       GENERATED_BODY()
       UPROPERTY(EditDefaultsOnly)
       int32 Tier;
       UPROPERTY(EditDefaultsOnly)
       FString UnlockConditionDescription;
       UPROPERTY(EditDefaultsOnly)
       TArray<TSubclassOf<ASProjectileBase>> AvailableWeapons; // 或武器ID列表
   };
   ```

2. **在 GameMode 中添加**:
   - `TArray<FWeaponTierConfig> WeaponTierConfigs` (EditDefaultsOnly, 蓝图配置)
   - `CheckWeaponTierUnlock(ETeam Team)` — 检查是否满足解锁条件
   - `UnlockWeaponTier(ETeam Team, int32 NewTier)` — 解锁新层级
   - `GetTeamWeaponTier(ETeam Team)` — 查询当前层级

3. **解锁条件配置**（可在蓝图编辑器中调整）:

   | 层级 | 条件 |
   |------|------|
   | Tier 1 | 默认可用 |
   | Tier 2 | 占领 1 个空间站控制节点 |
   | Tier 3 | 太空电梯占领 ≥ 50% 或 占领 2 个空间站节点 |
   | Tier 4 | 占领首个星球地表战略区 |
   | Tier 5 | 太空电梯完全控制 |

4. **Demo 阶段处理方式**:
   - 在 `HandleMatchHasStarted()` 中直接将 `RedWeaponTier` 和 `BlueWeaponTier` 设为 5（全部解锁）
   - `CheckWeaponTierUnlock()` 函数保留但暂时不调用
   - 这样 Demo 可以验证完整武器池，但架构支持后续加入递进逻辑

**验收标准**: GameState 的 `RedWeaponTier`/`BlueWeaponTier` 正确初始化和同步。架构预留了解锁回调接口。

---

## 第三阶段：核心玩法系统

### 步骤 8：空间站控制节点占领系统

**目标**: 实现 OrbitalCombat 阶段的核心玩法——占领空间站控制节点。

**涉及文件**: 新建 `SControlNode.h/.cpp`（或复用/扩展 SActionComponent 机制）

**具体工作**:

1. **创建 `ASControlNode` Actor 类**（放置在空间中转站地图中）:
   - 使用 Box/Sphere Collision 作为占领区域检测
   - 状态枚举：`Neutral`, `RedContested`, `BlueContested`, `RedControlled`, `BlueControlled`
   - `ControlProgress` (float, -1.0~1.0，负数为红方进度，正数为蓝方进度)
   - `CaptureRate` (float)：单人所提供的占领速度（默认 0.1/秒）

2. **占领逻辑**:
   - 区域内只有红方玩家 → ControlProgress 向 -1.0 移动
   - 区域内只有蓝方玩家 → ControlProgress 向 +1.0 移动
   - 区域内双方都有 → ControlProgress 不变（争夺中）
   - ControlProgress 达到 ±1.0 后节点被该方控制

3. **与 GameState 的联动**:
   - 节点被控制后更新 GameState 的 `RedControlNodes` / `BlueControlNodes`
   - 节点丢失后同步更新

4. **声音与视觉反馈**:
   - 占领进度变化 → 播放警报音效
   - 节点控制权变更 → 全局播报 + 颜色变化

**验收标准**: 玩家进入控制节点区域后可以看到进度条变化，节点被占领后 GameState 的 ControlNodes 计数正确更新。

---

### 步骤 9：网络密钥搜索与持有系统

**目标**: 实现 SearchKey 阶段的核心——密钥拾取、持有者标记、送达上传点。

**涉及文件**: `SGameMode_StellarFront.h/.cpp`, `ASPlayerState`（步骤1已有 `bIsCarryingKey`）, 新建 `SNetworkKey.h/.cpp`

**具体工作**:

1. **创建 `ASNetworkKey` Actor 类**:
   - 碰撞体用于检测拾取交互
   - `bIsPickedUp` (Replicated)：是否已被拾取
   - `PickUp(APlayerState* Player)`：拾取逻辑
   - `Drop(FVector Location)`：持有者死亡时掉落
   - 密钥位置（由 GameMode 在 SearchKey 阶段开始时随机选择或从预配置位置中选择）

2. **GameMode 密钥管理**:
   - `OnKeyPickedUp(APlayerState* Player)`:
     - 验证：玩家在密钥附近、密钥未被取走
     - 更新 `GameState->bKeyFound = true`
     - 更新 `GameState->KeyHolder = Player`
     - 更新 `ASPlayerState->bIsCarryingKey = true`
     - 启动 KeyHolder 位置周期性暴露 Timer（每 15 秒广播一次）
   - `OnKeyDropped()`:
     - KeyHolder 死亡时自动掉落
     - 更新 GameState 相关状态
     - 队友可重新拾取，敌方也可争夺
   - `CheckKeyDelivery()`:
     - KeyHolder 进入上传区域的碰撞检测
     - 满足条件 → 转入 UpLoad 阶段

3. **KeyHolder 标记机制**（Demo 版简化）:
   - 持有者头顶显示特殊图标（所有玩家可见）
   - 每 15 秒全局广播一次持有者位置（小地图脉冲）

**验收标准**: 玩家可以拾取密钥，GameState 和 PlayerState 的对应变量正确同步。所有客户端看到持有者标记。

---

### 步骤 10：主节点上传攻防系统

**目标**: 实现 UpLoad 阶段——进攻方需要在上传区域内持续防守 30 秒完成病毒上传。

**涉及文件**: `SGameMode_StellarFront.h/.cpp`, 新建 `SUploadZone.h/.cpp`

**具体工作**:

1. **创建 `ASUploadZone` Actor 类**:
   - 上传区域碰撞体
   - `UploadProgress` (float, 0.0~1.0)：当前上传进度
   - `RequiredTime` (float, 默认 30 秒)：所需上传时间

2. **上传进度计算**（在 GameMode 的 Tick 或 Timer 中）:
   - 区域内进攻方人数 > 0 → 进度增加（速率 = 基础速率 × 进攻方人数）
   - 区域内防守方人数 > 0 → 进度减少（防守方干扰）
   - 区域内双方人数相等 → 进度不变
   - 进度达到 1.0 → 调用 `OnUploadComplete()`
   - 进度降回 0 → 继续等待重新积累

3. **上传完成处理**:
   - `OnUploadComplete()`：
     - 防守方进入"系统崩溃状态"
     - 禁用防守方复活机制（在 GameMode 的复活逻辑中检查）
     - 禁用防守方小地图雷达
     - 转入 Evacuation 阶段

4. **上传区域视觉与音效**:
   - 上传中播放紧张背景音乐
   - 进度条 HUD 显示
   - 接近完成时警报音效渐强

**验收标准**: 进攻方在上传区域停留完成 30 秒上传后，阶段自动推进到 Evacuation。

---

### 步骤 11：撤离系统

**目标**: 实现 Evacuation 阶段——防守方幸存玩家逃亡，进攻方追击拦截。

**涉及文件**: `SGameMode_StellarFront.h/.cpp`, 新建 `SEvacShip.h/.cpp`

**具体工作**:

1. **撤离舰生成**:
   - `GenerateEvacShipLocation()`：在防守方星球地表随机生成撤离舰位置（远离进攻方主力位置）
   - 生成撤离舰 Actor（包含登舰区域碰撞体）
   - 全地图标记撤离舰位置

2. **撤离流程**:
   - 玩家进入撤离舰登舰区域 → 按交互键开始登舰
   - 登舰需要 3 秒引导时间（期间不可移动，可被击杀打断）
   - 登舰成功后：
     - `ASPlayerState->bHasEvacuated = true`（修复拼写：原 `bHasEvaucaute`）
     - 该玩家从战场移除（或进入旁观模式）
   - 撤离只有成功/不成功两种结果，不需要次数统计。一局比赛中每个玩家最多撤离一次

3. **撤离舰保护**:
   - 撤离舰有生命值（如 5000 HP）
   - 进攻方可以攻击摧毁撤离舰 → 需要等待新的撤离舰生成（30 秒后）
   - 所有防守方玩家撤离完毕或撤离舰被摧毁 → 比赛结束

4. **撤离阶段特殊规则**:
   - 防守方雷达关闭（`bDefenderRadarDisabled = true`）
   - 防守方无法复活
   - 撤离倒计时在 HUD 上显示

**验收标准**: Evacuation 阶段开始后撤离舰在随机位置生成，防守方玩家可以登舰撤离。撤离倒计时归零时比赛结束。

---

## 第四阶段：计分与胜负

### 步骤 12：计分系统与胜负判定

**目标**: 实现完整的计分逻辑和比赛胜负判定。

**涉及文件**: `SGameMode_StellarFront.h/.cpp`

**具体工作**:

1. **计分规则**（在玩家完成各种行为时调用）:
   - 击杀敌人 (Kill)：+100 分
   - 助攻 (Assist)：**待定**（助攻判定边界尚未确定——什么算"有效助攻"？暂不实现，代码保持注释化）
   - 占领空间站控制节点：+300 分（全队共享）
   - 拾取网络密钥：+200 分（个人）
   - 持有密钥送达上传点：+500 分（个人）
   - 完成上传：+1000 分（进攻方全队）
   - 成功撤离 (bHasEvacuated == true)：+300 分（个人，防守方，只判定成功与否，不按次数加分）
   - 击杀撤离途中的防守方：+200 分（个人，进攻方）
   - 摧毁撤离舰：+1500 分（进攻方全队）

2. **胜负判定** (`DetermineWinner()`):
   - **进攻方胜利条件**: 病毒上传成功 AND 至少拦截了一定比例的防守方撤离（如 ≥ 50%）
   - **防守方胜利条件**: 成功防守上传区域直到时间耗尽 OR 成功撤离 ≥ 70% 的幸存玩家
   - **平局**: 双方分数差值 < 阈值（罕见情况）
   - **Demo 简化**: 进攻方完成上传 = 进攻方胜利，否则防守方胜利

3. **比赛结束流程** (`HandleMatchHasEnded()`):
   - 显示比分面板
   - 展示 MVP（最高分玩家）
   - 30 秒后返回大厅

**验收标准**: 比赛结束后正确显示双方分数和胜负结果。

---

## 第五阶段：Demo 地图与集成

### 步骤 13：Demo 地图搭建

**目标**: 创建三个连接区域的基础关卡：空间站 → 登陆区 → 能源基地。

**涉及文件**: 新建 `.umap` 关卡文件

**具体工作**:

1. **空间站区域**（轨道）:
   - 2~3 层走廊和房间结构
   - 2~3 个控制节点放置点
   - 1 个太空电梯入口标记
   - 供近身战斗的狭窄走廊和转角

2. **登陆区**（过渡）:
   - 连接太空电梯出口和能源基地的中间地带
   - 提供部分掩体和开阔地带
   - 放置 1 个上传区域候选点

3. **能源基地**（地面）:
   - 主建筑群（室内+室外混合）
   - 密钥藏匿点（Demo 阶段固定 3~5 个位置随机选一）
   - 上传区域（固定位置）
   - 撤离舰可能的生成点（2~3 个随机选一）

4. **放置蓝图**:
   - PlayerStart（红/蓝各 20 个）
   - 控制节点 Actor
   - 上传区域 Actor
   - 网络密钥 Actor
   - 撤离舰生成点标记

**验收标准**: 三个区域可通过太空电梯和走廊连通。所有关键 Actor 正确放置。

### 步骤 14：HUD/UI 基础集成

**目标**: 搭建 Demo 所需的最基本 UI——阶段提示、资源条、密钥状态、上传进度、撤离倒计时。

**涉及文件**: 新建 UMG Widget Blueprint（或在 C++ 中创建 HUD 类）

**具体工作**:

1. **创建基础 HUD Widget**（UMG 蓝图）:
   - **左上角**: 当前阶段名称 + 阶段剩余时间
   - **顶部中央**: 红/蓝双方比分 + 资源条
   - **右下角**: 武器层级显示
   - **中央（条件显示）**: 
     - 密钥状态提示（"密钥已被发现！"）
     - 上传进度条（只在 UpLoad 阶段显示）
     - 撤离倒计时（只在 Evacuation 阶段显示）
   - **小地图**: 控制节点状态、KeyHolder 位置标记、撤离舰位置

2. **数据绑定**:
   - 绑定 `ASGameState` 的 `OnRep_Phase()` → 更新阶段文字
   - 绑定 `ASGameState` 的 `OnRep_TeamResources()` → 更新资源条
   - 绑定 `ASPlayerState` 的 `bIsCarryingKey` → 显示/隐藏密钥图标

3. **全局事件播报**:
   - "空间站节点已被红队占领！"
   - "网络密钥已被 [PlayerName] 拾取！"
   - "上传开始！防守方全体注意！"
   - "撤离舰已到达，请立即前往撤离！"

**验收标准**: 进入游戏后 HUD 正确显示阶段、资源、密钥状态等信息。阶段切换时 UI 同步更新。

### 步骤 15：端到端集成测试与调优

**目标**: 确保完整的 Demo 循环可以运行，修复集成问题，调优参数。

**涉及文件**: 所有上述文件

**具体工作**:

1. **完整流程测试**:
   - 在 PIE 多窗口模式下运行完整 Demo 流程
   - 验证：OrbitalCombat → SearchKey → UpLoad → Evacuation 全流程正常推进
   - 验证：网络复制在所有阶段正常工作

2. **边界情况处理**:
   - 一方玩家全部离开时的处理
   - KeyHolder 掉线时的密钥处理（自动掉落或随机转移）
   - 撤离舰被摧毁后生成新撤离舰
   - 上传进度在无人时的衰减

3. **参数调优**:
   - 空间站控制节点的占领速度
   - 上传所需时间（30 秒是否合理）
   - 撤离倒计时长度
   - 密钥持有者标记广播间隔

4. **性能检查**:
   - 网络带宽（Replicated 变量不要过多过频）
   - 40 人场景下的帧率
   - Timer 周期是否合理

**验收标准**: 完整 Demo 循环可从头到尾运行，无明显 Bug 或网络同步问题。

---

## 附录 A：Demo 推荐参数配置

| 参数 | 推荐值 |
|------|--------|
| 玩家规模 | 20 v 20 |
| 战前部署时长 | 180 秒 |
| 轨道争夺阶段最大时间 | 300 秒 |
| 密钥搜索阶段 | 不限时 |
| 上传所需时间 | 30 秒 |
| 撤离阶段最大时间 | 300 秒 |
| 资源更新间隔 | 5 秒 |
| 密钥持有者标记广播间隔 | 15 秒 |

## 附录 B：文件变更范围汇总

| 文件 | 操作 | 涉及步骤 |
|------|------|---------|
| `SPlayerState.h` | 修改 | 步骤 1, 9 |
| `SPlayerState.cpp` | 修改 | 步骤 1 |
| `SGameState.h` | 大幅重写 | 步骤 2 |
| `SGameState.cpp` | 大幅重写 | 步骤 2 |
| `SGameMode_StellarFront.h` | 大幅重写 | 步骤 3, 4, 5, 6, 7, 9, 10, 11, 12 |
| `SGameMode_StellarFront.cpp` | 大幅重写 | 步骤 3, 4, 5, 6, 7, 9, 10, 11, 12 |
| `SControlNode.h/.cpp` | 新建 | 步骤 8 |
| `SNetworkKey.h/.cpp` | 新建 | 步骤 9 |
| `SUploadZone.h/.cpp` | 新建 | 步骤 10 |
| `SEvacShip.h/.cpp` | 新建 | 步骤 11 |
| `Stellar_Front.Build.cs` | 检查 | 步骤 1（确认 NetCore 模块） |
| Demo 关卡 (.umap) | 新建 | 步骤 13 |
| HUD Widget Blueprint | 新建 | 步骤 14 |

## 附录 C：数据流速查

```
GameMode（仅服务器，规则逻辑）
    │
    │  写入 GameState / PlayerState
    ▼
GameState（全量复制到所有客户端）
    │
    │  OnRep_XXX() 回调触发
    ▼
客户端（HUD 更新、游戏逻辑响应）
    │
    │  输入事件 → Server RPC
    ▼
GameMode（处理 RPC，修改 GameState）
    （循环）
```
