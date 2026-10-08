# M1 / M2 已批准 Issue 清单

> 当前任务编号：**M1 = T1–T8；M2 = T9–T17**。两仓连续编号；GitHub 原生 Issue 编号保留。

> 2026-10-07 · 用户批准后已通过 `gh` 发布。17 项任务：更新已有 10 项、新建 7 项；M1 为 8 项，M2 为 9 项。

每项从已经完成的基础继续，关闭条件按实际剩余工作拆分。M1 交付完整 BT 流程，M2 完善 Boss 并完成 RL 实战；后期 16 条 Issue 曾导出备份后删除，阶段目标仍保留；本地备份已由用户于 2026-10-08 移除。

| 任务 | 目标仓库 | Issue | 验收前置 |
| --- | --- | --- | --- |
| T1 | AesirWarden | [AesirWarden#1 T1 · 整理 BT 工程与资产恢复说明](https://github.com/YYchainsAw/AesirWarden/issues/1) | 无 |
| T2 | AesirWarden | [AesirWarden#14 T2 · 接通菜单、开场与探索流程](https://github.com/YYchainsAw/AesirWarden/issues/14) | 无 |
| T3 | AesirWarden | [AesirWarden#3 T3 · 验证玩家基础操作](https://github.com/YYchainsAw/AesirWarden/issues/3) | 无 |
| T4 | AesirWarden | [AesirWarden#6 T4 · 接通 Boss 入口与 BT 开战](https://github.com/YYchainsAw/AesirWarden/issues/6) | [T2](https://github.com/YYchainsAw/AesirWarden/issues/14) |
| T5 | AesirWarden | [AesirWarden#4 T5 · 验证 BT Boss 战斗与结算](https://github.com/YYchainsAw/AesirWarden/issues/4) | [T3](https://github.com/YYchainsAw/AesirWarden/issues/3)、[T4](https://github.com/YYchainsAw/AesirWarden/issues/6) |
| T6 | AesirWarden | [AesirWarden#15 T6 · 接通胜败判定与结果页](https://github.com/YYchainsAw/AesirWarden/issues/15) | [T4](https://github.com/YYchainsAw/AesirWarden/issues/6) |
| T7 | AesirWarden | [AesirWarden#7 T7 · 接通重试与返回菜单](https://github.com/YYchainsAw/AesirWarden/issues/7) | [T4](https://github.com/YYchainsAw/AesirWarden/issues/6)、[T6](https://github.com/YYchainsAw/AesirWarden/issues/15) |
| T8 | AesirWarden | [AesirWarden#8 T8 · 验收 BT Windows 包](https://github.com/YYchainsAw/AesirWarden/issues/8) | [T1](https://github.com/YYchainsAw/AesirWarden/issues/1)、[T3](https://github.com/YYchainsAw/AesirWarden/issues/3)、[T5](https://github.com/YYchainsAw/AesirWarden/issues/4)、[T6](https://github.com/YYchainsAw/AesirWarden/issues/15)、[T7](https://github.com/YYchainsAw/AesirWarden/issues/7) |
| T9 | AesirWarden | [AesirWarden#16 T9 · 完善 Boss 动作与战斗规则](https://github.com/YYchainsAw/AesirWarden/issues/16) | 无 |
| T10 | aesir-ai-service | [aesir-ai-service#29 T10 · 同步 UE 与 RL 训练规则](https://github.com/YYchainsAw/aesir-ai-service/issues/29) | [T9](https://github.com/YYchainsAw/AesirWarden/issues/16) |
| T11 | aesir-ai-service | [aesir-ai-service#30 T11 · 验证模型并整理训练记录](https://github.com/YYchainsAw/aesir-ai-service/issues/30) | [T10](https://github.com/YYchainsAw/aesir-ai-service/issues/29) |
| T12 | aesir-ai-service | [aesir-ai-service#17 T12 · 校验模型并启动推理服务](https://github.com/YYchainsAw/aesir-ai-service/issues/17) | [T11](https://github.com/YYchainsAw/aesir-ai-service/issues/30) |
| T13 | aesir-ai-service | [aesir-ai-service#18 T13 · 验证 HTTP 接口与故障处理](https://github.com/YYchainsAw/aesir-ai-service/issues/18) | [T10](https://github.com/YYchainsAw/aesir-ai-service/issues/29) |
| T14 | AesirWarden | [AesirWarden#2 T14 · 修复连续失败计数与 BT 回退](https://github.com/YYchainsAw/AesirWarden/issues/2) | [T13](https://github.com/YYchainsAw/aesir-ai-service/issues/18) |
| T15 | AesirWarden | [AesirWarden#17 T15 · 接通 RL 开战、结束与重试](https://github.com/YYchainsAw/AesirWarden/issues/17) | [T4](https://github.com/YYchainsAw/AesirWarden/issues/6)、[T7](https://github.com/YYchainsAw/AesirWarden/issues/7)、[T12](https://github.com/YYchainsAw/aesir-ai-service/issues/17)、[T14](https://github.com/YYchainsAw/AesirWarden/issues/2) |
| T16 | AesirWarden | [AesirWarden#5 T16 · 验证 UE 与 RL 模型完整战斗](https://github.com/YYchainsAw/AesirWarden/issues/5) | [T5](https://github.com/YYchainsAw/AesirWarden/issues/4)、[T9](https://github.com/YYchainsAw/AesirWarden/issues/16)、[T12](https://github.com/YYchainsAw/aesir-ai-service/issues/17)、[T13](https://github.com/YYchainsAw/aesir-ai-service/issues/18)、[T14](https://github.com/YYchainsAw/AesirWarden/issues/2)、[T15](https://github.com/YYchainsAw/AesirWarden/issues/17) |
| T17 | AesirWarden | [AesirWarden#18 T17 · 验收 RL 与回退 Windows 包](https://github.com/YYchainsAw/AesirWarden/issues/18) | [T8](https://github.com/YYchainsAw/AesirWarden/issues/8)、[T16](https://github.com/YYchainsAw/AesirWarden/issues/5) |

## 本轮依赖调整

原生依赖共 28 条，已按批准清单同步并核对：解除 UE #1 对 Python #17 的阻塞，以及 UE #7 对 UE #5 的阻塞。Python #17/#18 保留为 UE #5 子任务。验收前置允许提前准备，已有任务状态与负责人保留，新任务为 Todo。

可先启动 T1、T2、T3 和 T9；完整游戏交付按 M1 检查点推进，Boss/RL 的规则盘点可提前进行。

[当前里程碑](10_GitHubMilestones.md) · [统一 Project](09_GitHubProject.md) · [创建与同步记录](12_CurrentIssues.json)。

## T1 · 整理 BT 工程与资产恢复说明

[AesirWarden#1](https://github.com/YYchainsAw/AesirWarden/issues/1)；标签：`documentation`、`priority:p0`、`area:assets`、`area:build`

**任务编号：** `T1`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** M1 · 完整游玩流程与 BT Boss 基线

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

现有源码、菜单/结果资产和本地地图可以继续使用；默认地图及第三方依赖未完整进入公开仓库。模型交付已经有 Python 任务，当前先收口 BT 工程恢复。

### 任务范围

- 明确当前地图、完整资产依赖和本地 develop 基线。

- 记录工程/资产备份、依赖恢复、构建与运行步骤，并标明公开克隆的可运行边界。

### 完成条件

- [ ] 一份对应当前 BT 版本的完整工程/资产快照可定位，恢复步骤可执行，第三方素材来源可查。

- [ ] 先记录 UE commit、资产快照 ID/哈希、引擎与构建配置；正式包哈希和运行记录由 T8 补入同一索引，不作为本项完成前置。

- [ ] 恢复和 BT 启动不要求 Python RL 模型或 NPC 服务先完成。

### 验收证据

恢复说明、资产/构建索引、验证机器与实际恢复/构建记录；包索引可在 T8 集成时补齐。

### 前置依赖

无最终验收前置任务，可直接准备或实施。

### 范围边界

本项处理 BT 工程交付；模型、训练和推理依赖索引交给 T12。

### 来源与基线

- [AesirWarden · Docs/05_Development_Guide/SetupEnv.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/05_Development_Guide/SetupEnv.md)

- [AesirWarden · Docs/06_Test_Doc/KnownIssues.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/06_Test_Doc/KnownIssues.md)

- [AesirWarden · Config/DefaultEngine.ini](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Config/DefaultEngine.ini)

- [AesirWarden · .gitignore](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/.gitignore)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T2 · 接通菜单、开场与探索流程

[AesirWarden#14](https://github.com/YYchainsAw/AesirWarden/issues/14)；标签：`enhancement`、`priority:p0`、`area:level-flow`、`area:ui`、`needs:runtime-verification`

**任务编号：** `T2`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** M1 · 完整游玩流程与 BT Boss 基线

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

6f24ff3/ea4c805 已实现菜单到探索的 C++，LevelFlowGuide 记录编译通过；BP、场景引用、菜单绑定及 PIE 全流程尚未验收。

### 任务范围

- 沿用现有 FlowDirector、WBP_MainMenu 与当前地图完成父类、引用、按钮和 HUD 配置。

- 核对菜单输入隔离、自然结束/跳过的相机与角色交接；开场可先省略或用占位 Sequence。

### 完成条件

- [ ] 冷启动/PIE 进入正确菜单，玩家移动/攻击不穿透菜单；点击开始后进入探索并恢复输入、相机与 HUD。

- [ ] 重复点击只启动一次，缺少配置有明确报错；自然结束与跳过得到一致可玩状态。

- [ ] 连续两次启动不残留菜单、输入锁、定时器或过场委托。

### 验收证据

实际场景/BP 配置记录，开始、重复点击、跳过和连续启动的录屏及日志。

### 前置依赖

无最终验收前置任务，可直接准备或实施。

### 范围边界

继续现有 C++ 接线；正式营火构图、起身演出与美术打磨归 M6。

### 来源与基线

- UE 本地 `ea4c805`：`Docs/05_Development_Guide/05_LevelFlowGuide.md`（实现/文档尚未同步到公开 main）。

- UE 本地 `ea4c805`：`Source/AesirWarden/Public/Framework/Flow/AesirLevelFlowDirector.h`（实现/文档尚未同步到公开 main）。

- UE 本地 `ea4c805`：`Source/AesirWarden/Private/Framework/Flow/AesirLevelFlowDirector.cpp`（实现/文档尚未同步到公开 main）。

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T3 · 验证玩家基础操作

[AesirWarden#3](https://github.com/YYchainsAw/AesirWarden/issues/3)；标签：`type:test`、`priority:p0`、`area:player`、`area:combat`、`area:gas`、`needs:runtime-verification`

**任务编号：** `T3`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** M1 · 完整游玩流程与 BT Boss 基线

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

用户确认玩家移动、攻击、翻滚已可用；当前代码还包含锁定、轻重击、防御与反击入口。现有完成能力需要形成当前关卡的回归记录。

### 任务范围

- 按当前已启用操作核对输入、伤害、防御、受击和死亡；记录尚未启用的机制。

- 固定 M1 使用的能力、GE、Montage 与数值版本，区分源码入口与资产实际接线。

### 完成条件

- [ ] 移动、攻击、翻滚及已启用锁定/防御在目标场地有效，伤害与死亡结果正确。

- [ ] 普通/重/不可防御攻击的已启用规则有实际记录，未启用项和缺陷明确登记。

- [ ] 结果与 COMBAT-01 对应到当前代码/资产版本，达到 BT 基线可玩要求。

### 验收证据

COMBAT-01 回归表、视频/日志、能力与数值版本、未启用项/缺陷链接。

### 前置依赖

无最终验收前置任务，可直接准备或实施。

### 范围边界

记录已有功能和当前回归；新增技能、系统性数值/手感完善归 M4。

### 来源与基线

- [AesirWarden · Source/AesirWarden/Private/Characters/Player/AesirPlayerCharacter.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/Characters/Player/AesirPlayerCharacter.cpp)

- [AesirWarden · Source/AesirWarden/Private/Combat/AesirCombatComponent.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/Combat/AesirCombatComponent.cpp)

- [AesirWarden · Docs/06_Test_Doc/TestCase_Combat.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/06_Test_Doc/TestCase_Combat.md)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T4 · 接通 Boss 入口与 BT 开战

[AesirWarden#6](https://github.com/YYchainsAw/AesirWarden/issues/6)；标签：`enhancement`、`priority:p0`、`area:level-flow`、`area:boss-ai`、`needs:runtime-verification`

**任务编号：** `T4`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** M1 · 完整游玩流程与 BT Boss 基线

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

流程控制器的 BossIntro/BossCombat 仍是预留状态；BossController 当前在 OnPossess 启动 BT/策略和 Episode，需要与入口及过场交接协调。

### 任务范围

- 在现有探索流程加入一次性 Boss 入口和最小入场交接。

- 明确控制器接管、BT 启停、Episode 开始/停止与玩家输入/HUD 的责任。

### 完成条件

- [ ] 菜单、探索和 Boss 入场期间不提前决策或伤害；入场结束/跳过后只启动一次 BT 与 Episode。

- [ ] 过场时间不计战斗，重复触发和重复回调不生成重复 Episode。

- [ ] 固定 Boss 放置/登记方式有记录；战斗终局可以停止 BT 并交给结果流程。

### 验收证据

入口/BP 配置、阶段与 Episode 日志、重复触发及自然结束/跳过记录。

### 前置依赖

- [T2 · T2 · 接通菜单、开场与探索流程](https://github.com/YYchainsAw/AesirWarden/issues/14)

依赖约束最终验收；代码盘点、样例和资料可以提前准备。

### 范围边界

先用 BT 收口；PPO 请求与回退的生命周期由 T15 扩展。

### 来源与基线

- UE 本地 `ea4c805`：`Source/AesirWarden/Public/Framework/Flow/AesirLevelFlowTypes.h`（实现/文档尚未同步到公开 main）。

- [AesirWarden · Source/AesirWarden/Private/AI/Boss/AesirBossAIController.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Boss/AesirBossAIController.cpp)

- [AesirWarden · Docs/02_Design_Doc/GDD/GameFlow.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/02_Design_Doc/GDD/GameFlow.md)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T5 · 验证 BT Boss 战斗与结算

[AesirWarden#4](https://github.com/YYchainsAw/AesirWarden/issues/4)；标签：`type:test`、`priority:p0`、`area:boss-ai`、`area:gas`、`needs:runtime-verification`

**任务编号：** `T5`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** M1 · 完整游玩流程与 BT Boss 基线

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

用户确认 BT Boss 可以战斗，BT/BB、共享执行器与 Telemetry 已存在；当前第一关的完整运行证据仍需记录。

### 任务范围

- 在 M1 实际入口和战斗边界下运行 BT，覆盖玩家胜利与失败。

- 核对共享 Action/GAS、typed Outcome、伤害、终局和 Episode 文件。

### 完成条件

- [ ] BT 经现有执行器完成战斗，动作、结果、伤害与最终胜负可回查。

- [ ] Accepted 与命中/完成分开说明，拒绝原因和未配置动作可识别。

- [ ] 开战/终局只发生一次，Episode 与所用代码/资产版本关联。

### 验收证据

BT-01 完整 Episode、视频/日志、动作与终局原始记录及版本索引。

### 前置依赖

- [T3 · T3 · 验证玩家基础操作](https://github.com/YYchainsAw/AesirWarden/issues/3)

- [T4 · T4 · 接通 Boss 入口与 BT 开战](https://github.com/YYchainsAw/AesirWarden/issues/6)

依赖约束最终验收；代码盘点、样例和资料可以提前准备。

### 范围边界

验收现有 BT 可玩基线；九动作系统性收口在 T9。

### 来源与基线

- [AesirWarden · Source/AesirWarden/Private/AI/Boss/AesirBossActionComponent.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Boss/AesirBossActionComponent.cpp)

- [AesirWarden · Source/AesirWarden/Private/AI/Boss/AesirBossTelemetryComponent.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Boss/AesirBossTelemetryComponent.cpp)

- [AesirWarden · Docs/06_Test_Doc/VerificationMatrix.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/06_Test_Doc/VerificationMatrix.md)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T6 · 接通胜败判定与结果页

[AesirWarden#15](https://github.com/YYchainsAw/AesirWarden/issues/15)；标签：`enhancement`、`priority:p0`、`area:level-flow`、`area:ui`、`needs:runtime-verification`

**任务编号：** `T6`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** M1 · 完整游玩流程与 BT Boss 基线

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

GameMode 有一次性结算保护，但当前按 BeginPlay 枚举的全部敌人计数判定通关；WBP_CombatResult 已存在，需要与单 Boss 第一关目标接通。

### 任务范围

- 明确本阶段 Boss 标识与登记方式，绑定目标 Boss/玩家终局。

- 复用结果 Widget 接通胜利/失败展示、输入焦点与后续按钮事件。

### 完成条件

- [ ] 指定 Boss 死亡触发通关，玩家死亡触发失败；普通敌人或重复死亡回调不会误触通关/重复结算。

- [ ] 终局停止战斗并正确结束 Episode，结果页只显示一次且可操作。

- [ ] 固定放置 Boss 的适用边界写明；若实际采用动态生成，登记/清理在本任务实现并验证。

### 验收证据

Boss 登记方案、胜败与重复回调记录、结果页截图/视频和终局日志。

### 前置依赖

- [T4 · T4 · 接通 Boss 入口与 BT 开战](https://github.com/YYchainsAw/AesirWarden/issues/6)

依赖约束最终验收；代码盘点、样例和资料可以提前准备。

### 范围边界

只为当前单 Boss 通关处理；不附加普通敌人生成系统。重试与返回由 T7 完成。

### 来源与基线

- [AesirWarden · Source/AesirWarden/Private/Framework/AesirCombatPrototypeGameMode.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/Framework/AesirCombatPrototypeGameMode.cpp)

- [AesirWarden · Content/Aesir/UI/Results/WBP_CombatResult.uasset](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Content/Aesir/UI/Results/WBP_CombatResult.uasset)

- [AesirWarden · Docs/06_Test_Doc/KnownIssues.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/06_Test_Doc/KnownIssues.md)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T7 · 接通重试与返回菜单

[AesirWarden#7](https://github.com/YYchainsAw/AesirWarden/issues/7)；标签：`enhancement`、`priority:p0`、`area:level-flow`、`area:ui`、`needs:runtime-verification`

**任务编号：** `T7`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** M1 · 完整游玩流程与 BT Boss 基线

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

原 #7 合并了结算、结果和重试，且依赖 PPO 验收。新版把结果判定拆为 T6，当前用 BT 先完成完整游玩。

### 任务范围

- 绑定现有结果页的重试/返回事件，复用统一流程初始化。

- 重置角色/敌人、能力/冷却、输入、相机、UI、入口、委托与 BT 遥测。

### 完成条件

- [ ] 重试回到第一关起点，默认跳过菜单和开场，重新走到 Boss；不直接在 Boss 入口复活。

- [ ] 至少连续失败并重试两次，旧角色状态、能力、UI、定时器、委托和触发事件不残留，每次开战为新 Episode。

- [ ] 返回菜单恢复正确相机/输入与可操作菜单，再次开始得到全新流程。

### 验收证据

MATCH-01 记录、连续重试/返回视频、旧状态清理与新 Episode ID 日志。

### 前置依赖

- [T4 · T4 · 接通 Boss 入口与 BT 开战](https://github.com/YYchainsAw/AesirWarden/issues/6)

- [T6 · T6 · 接通胜败判定与结果页](https://github.com/YYchainsAw/AesirWarden/issues/15)

依赖约束最终验收；代码盘点、样例和资料可以提前准备。

### 范围边界

本阶段 BT 重试不等待 UE #5/PPO 验收；RL 请求/回退重置在 T15。

### 来源与基线

- [AesirWarden · Docs/02_Design_Doc/TDD/SaveSystem.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/02_Design_Doc/TDD/SaveSystem.md)

- [AesirWarden · Docs/02_Design_Doc/GDD/GameFlow.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/02_Design_Doc/GDD/GameFlow.md)

- UE 本地 `ea4c805`：`Source/AesirWarden/Private/Framework/Flow/AesirLevelFlowDirector.cpp`（实现/文档尚未同步到公开 main）。

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T8 · 验收 BT Windows 包

[AesirWarden#8](https://github.com/YYchainsAw/AesirWarden/issues/8)；标签：`type:test`、`priority:p0`、`area:build`、`area:level-flow`、`needs:runtime-verification`

**任务编号：** `T8`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** M1 · 完整游玩流程与 BT Boss 基线

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

原 #8 把 BT、PPO 与故障回退放在同一基线验收。用户当前先交付 BT 完整流程；资产恢复、流程与基础玩法已有独立任务。

### 任务范围

- 固定当前构建、地图和完整资产，打包 Windows 基线。

- 从冷启动覆盖开始、基础玩法、BT 胜败、连续重试和返回，归档缺陷与性能基线。

### 完成条件

- [ ] 目标 Windows 机器冷启动完成开始→探索→Boss→胜败→重试/返回，连续两次重试有效。

- [ ] M1 已启用玩家操作与 BT Episode 在包内通过，菜单/结果焦点及状态清理正确。

- [ ] 包哈希、UE/资产版本、运行环境、日志和恢复/操作说明可查；性能先记录实际基线。

### 验收证据

BUILD-01/MATCH-01 验收表、包和哈希、运行录像/日志、机器配置与帧表现。

### 前置依赖

- [T1 · T1 · 整理 BT 工程与资产恢复说明](https://github.com/YYchainsAw/AesirWarden/issues/1)

- [T3 · T3 · 验证玩家基础操作](https://github.com/YYchainsAw/AesirWarden/issues/3)

- [T5 · T5 · 验证 BT Boss 战斗与结算](https://github.com/YYchainsAw/AesirWarden/issues/4)

- [T6 · T6 · 接通胜败判定与结果页](https://github.com/YYchainsAw/AesirWarden/issues/15)

- [T7 · T7 · 接通重试与返回菜单](https://github.com/YYchainsAw/AesirWarden/issues/7)

依赖约束最终验收；代码盘点、样例和资料可以提前准备。

### 范围边界

当前使用占位表现；PPO 与故障回退打包测试另在 T17，正式发布表现归 M6。

### 来源与基线

- [AesirWarden · Docs/07_Release/ReleaseChecklist.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/07_Release/ReleaseChecklist.md)

- [AesirWarden · Docs/06_Test_Doc/VerificationMatrix.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/06_Test_Doc/VerificationMatrix.md)

- [AesirWarden · Docs/05_Development_Guide/SetupEnv.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/05_Development_Guide/SetupEnv.md)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T9 · 完善 Boss 动作与战斗规则

[AesirWarden#16](https://github.com/YYchainsAw/AesirWarden/issues/16)；标签：`enhancement`、`priority:p0`、`area:boss-ai`、`area:boss-rl`、`area:gas`、`needs:runtime-verification`

**任务编号：** `T9`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** M2 · Boss 系统完善与 RL 实战闭环

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

共享 BossActionComponent、九动作枚举、GA 资产、冷却与观测掩码已有实现；具体资产映射和运行缺口尚需逐项收口。

### 任务范围

- 建立动作→GA/输入参数→目标/距离→状态/冷却→Outcome 的实际清单。

- 继续现有执行器与能力，补约定缺口；记录支持与禁用动作，定义 UE 规则版本。

### 完成条件

- [ ] 九动作的实际配置、合法/拒绝条件与测试结果逐项可查；本阶段约定可用动作通过。

- [ ] 未实现/未配置动作在可用掩码和执行器中一致禁用/拒绝，不报告为成功。

- [ ] 忙碌、受击、死亡、无效目标和冷却等边界有运行记录，BT/PPO 使用同一接口。

- [ ] 需要新增招式/阶段时先登记具体范围，后续 Python 同步可引用明确规则。

### 验收证据

Boss 动作规则表、GA/BP 配置记录、运行与拒绝样例、规则/资产版本。

### 前置依赖

无最终验收前置任务，可直接准备或实施。

### 范围边界

完善现有单 Boss；不预设额外 Boss 数量、强制二阶段或全新技能数量。

### 来源与基线

- [AesirWarden · Source/AesirWarden/Public/AI/Boss/AesirBossDecisionTypes.h](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Public/AI/Boss/AesirBossDecisionTypes.h)

- [AesirWarden · Source/AesirWarden/Private/AI/Boss/AesirBossActionComponent.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Boss/AesirBossActionComponent.cpp)

- [AesirWarden · Source/AesirWarden/Private/AI/Boss/AesirBossObservationComponent.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Boss/AesirBossObservationComponent.cpp)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T10 · 同步 UE 与 RL 训练规则

[aesir-ai-service#29](https://github.com/YYchainsAw/aesir-ai-service/issues/29)；标签：`enhancement`、`priority:p0`、`area:boss-rl`、`area:training`、`area:contract`

**任务编号：** `T10`

**目标仓库：** `YYchainsAw/aesir-ai-service`

**里程碑：** M2 · Boss RL 训练、模型与推理联调基线

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

rl/boss 已有 sim005、reward003、v4/26/9 与动作掩码，训练脚本支持检查点和可复现种子；新版需要显式核对 UE 实际规则。

### 任务范围

- 以 T9 清单核对动作语义、时步/秒、距离与归一化、伤害、防御、冷却和受击/锁定规则。

- 为必要差异修正模拟器/奖励/掩码，说明抽象限制并登记修订与兼容性。

### 完成条件

- [ ] UE→模拟器规则映射和保留抽象有依据，新增/修正机制有边界回归。

- [ ] 动作可用掩码和锁定状态匹配约定规则，观察维度/顺序/动作 ID 无静默变化。

- [ ] simulation/reward/schema 修订、训练命令与种子配置同步；破坏契约时明确需要重训。

### 验收证据

跨仓规则对照表、差异/限制、必要回归结果、修订 manifest 与训练配置。

### 前置依赖

- [T9 · T9 · 完善 Boss 动作与战斗规则](https://github.com/YYchainsAw/AesirWarden/issues/16)

依赖约束最终验收；代码盘点、样例和资料可以提前准备。

### 范围边界

复用现有训练框架；在 UE 当前规则已明确后收口，不要求逐帧复制游戏物理。

### 来源与基线

- [aesir-ai-service · rl/boss/contract.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/rl/boss/contract.py)

- [aesir-ai-service · rl/boss/sim.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/rl/boss/sim.py)

- [aesir-ai-service · rl/boss/rewards.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/rl/boss/rewards.py)

- [aesir-ai-service · rl/boss/env.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/rl/boss/env.py)

- [aesir-ai-service · scripts/rl/boss/train.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/scripts/rl/boss/train.py)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T11 · 验证模型并整理训练记录

[aesir-ai-service#30](https://github.com/YYchainsAw/aesir-ai-service/issues/30)；标签：`type:test`、`priority:p0`、`area:boss-rl`、`area:training`、`area:artifacts`

**任务编号：** `T11`

**目标仓库：** `YYchainsAw/aesir-ai-service`

**里程碑：** M2 · Boss RL 训练、模型与推理联调基线

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

本机已定位 sim005 三种子模型，每组 manifest 记录 1,015,808 timesteps；seed 1 部署候选 SHA 与 selection 一致，900 局属于选模数据。

### 任务范围

- 根据 T10 的版本和规则判断现有候选是否适用，保留模型 shape/动作空间、版本和运行核验。

- 整理训练配置、种子、曲线/检查点与选择依据；需要新训练时另记录实际预算、参数和新产物。

### 完成条件

- [ ] 交付一个有明确适用规则的冻结候选，SHA、schema、simulation/reward、观测/动作空间与训练来源可查。

- [ ] 兼容性结论有依据；不兼容时保留拒绝记录并交付适用的新产物，不能仅因文件可加载就通过。

- [ ] 训练/选模 manifest、复现命令和保管索引齐备，历史 900 局继续标为选模集。

### 验收证据

候选兼容性记录、模型哈希、训练/选择 manifest、配置/曲线/检查点索引；新训练仅在需要时记录。

### 前置依赖

- [T10 · T10 · 同步 UE 与 RL 训练规则](https://github.com/YYchainsAw/aesir-ai-service/issues/29)

依赖约束最终验收；代码盘点、样例和资料可以提前准备。

### 范围边界

不把正式 BT–PPO 成组研究放在此任务；正式独立测试与报告在 M3。

### 来源与基线

- Python 本机训练产物：`models/rl/boss/deployment/boss_policy_schema_v4_sim005.selection.json`（不在公开 Git 仓库，需由模型交付任务提供可访问索引）。

- Python 本机训练产物：`models/rl/boss/deployment/boss_policy_schema_v4_sim005.training.manifest.json`（不在公开 Git 仓库，需由模型交付任务提供可访问索引）。

- [aesir-ai-service · scripts/rl/boss/train.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/scripts/rl/boss/train.py)

- [aesir-ai-service · rl/boss/inference.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/rl/boss/inference.py)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T12 · 校验模型并启动推理服务

[aesir-ai-service#17](https://github.com/YYchainsAw/aesir-ai-service/issues/17)；标签：`enhancement`、`priority:p0`、`area:boss-rl`、`area:inference`、`area:artifacts`

**任务编号：** `T12`

**目标仓库：** `YYchainsAw/aesir-ai-service`

**里程碑：** M2 · Boss RL 训练、模型与推理联调基线

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

独立 FastAPI/8012 服务和冻结 MaskablePPO Runtime 已实现；当前 load 主要按文件存在和 MaskablePPO.load 加载，manifest/SHA/shape 的完整交付校验待收口。

### 任务范围

- 复用现有 Runtime/serve，落实模型获取、manifest/SHA/维度/动作空间兼容性校验。

- 按说明启动真实候选服务，输出健康、模型身份、正常决策和锁定状态证据。

### 完成条件

- [ ] 缺失、哈希错误、manifest 或空间不匹配明确拒绝，失败原因可查。

- [ ] 在记录的依赖/环境下真实冻结模型启动成功，model_id 与交付模型一致。

- [ ] 正常请求返回合法动作；全锁定 observation 返回非 actionable，不伪造动作；启动说明和版本索引可用于 UE。

### 验收证据

模型交付说明、校验通过/拒绝记录、真实 /health 和 decide 样例、延迟与依赖版本。

### 前置依赖

- [T11 · T11 · 验证模型并整理训练记录](https://github.com/YYchainsAw/aesir-ai-service/issues/30)

依赖约束最终验收；代码盘点、样例和资料可以提前准备。

### 范围边界

Boss 服务独立于 NPC app/；服务运行不能代替 UE Episode。

### 来源与基线

- [aesir-ai-service · rl/boss/inference.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/rl/boss/inference.py)

- [aesir-ai-service · rl/boss/inference_app.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/rl/boss/inference_app.py)

- [aesir-ai-service · scripts/rl/boss/serve.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/scripts/rl/boss/serve.py)

- [aesir-ai-service · rl/README.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/rl/README.md)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T13 · 验证 HTTP 接口与故障处理

[aesir-ai-service#18](https://github.com/YYchainsAw/aesir-ai-service/issues/18)；标签：`bug`、`type:test`、`priority:p0`、`area:boss-rl`、`area:contract`、`area:inference`

**任务编号：** `T13`

**目标仓库：** `YYchainsAw/aesir-ai-service`

**里程碑：** M2 · Boss RL 训练、模型与推理联调基线

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

HTTP 契约和推理测试已有基础；共享 observation 校验当前仅用范围比较，NaN 的有限值约束需补齐。UE 对异常响应的样例需要可复现交付。

### 任务范围

- 核对并补齐共享观测/HTTP 的有限值、维度、schema、标识与动作字段约束。

- 提供可重复运行的开发故障响应，支持 UE 断连/超时/坏 JSON/协议错/缺失或越界动作验收。

### 完成条件

- [ ] 正常请求与 request_id/sequence/model_id 回显有记录；NaN/Inf、错误维度/schema 等边界有明确验证，注明直接函数与 HTTP 的测试层级。

- [ ] 故障样例可按文档重放，异常用途明确，不影响正常模型服务路径。

- [ ] 服务锁定响应与异常响应分开，UE #2 可验证三次连续缺失/越界 Action 后回退。

### 验收证据

契约边界结果、golden/异常样例、故障工具及重放说明。

### 前置依赖

- [T10 · T10 · 同步 UE 与 RL 训练规则](https://github.com/YYchainsAw/aesir-ai-service/issues/29)

依赖约束最终验收；代码盘点、样例和资料可以提前准备。

### 范围边界

Docs/08_RL_Research/01_BossRLContract.md 属 UE 来源；真实模型启动在 T12，UE 真实注入在 T14。

### 来源与基线

- [aesir-ai-service · rl/boss/contract.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/rl/boss/contract.py)

- [aesir-ai-service · rl/boss/inference_app.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/rl/boss/inference_app.py)

- [aesir-ai-service · tests/rl/boss/test_inference.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/tests/rl/boss/test_inference.py)

- [AesirWarden · Docs/08_RL_Research/BossRL_Contract.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/08_RL_Research/BossRL_Contract.md)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T14 · 修复连续失败计数与 BT 回退

[AesirWarden#2](https://github.com/YYchainsAw/AesirWarden/issues/2)；标签：`bug`、`priority:p0`、`area:boss-rl`、`area:services`、`needs:runtime-verification`

**任务编号：** `T14`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** M2 · Boss 系统完善与 RL 实战闭环

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

PolicyClient 当前在解析 action_id 前 ResetFailures；连续 missing_action/invalid_action 可能每次重新计为一次，源码和 KI-01 仍记录此问题。

### 任务范围

- 调整成功/失败计数的归属，沿用当前三次失败阈值与 BT 接管入口。

- 使用 T13 样例覆盖错误响应连续失败、正常响应恢复和非 actionable 锁定。

### 完成条件

- [ ] 连续三次缺失或越界动作达到阈值并启动 BT，实际失败计数/原因可查。

- [ ] 正常合法响应和正常锁定响应按约定清理计数；错误 JSON/schema/断连/超时同样可记录和接管。

- [ ] 回退后 Boss 能继续战斗并结算；更新 KI-01/RL-FAULT-01 当前版本证据。

### 验收证据

修复差异、连续注入日志、BT 接管/终局记录与当前构建版本。

### 前置依赖

- [T13 · T13 · 验证 HTTP 接口与故障处理](https://github.com/YYchainsAw/aesir-ai-service/issues/18)

依赖约束最终验收；代码盘点、样例和资料可以提前准备。

### 范围边界

保留现有故障设计；不阻塞 M1 的纯 BT 交付。

### 来源与基线

- [AesirWarden · Source/AesirWarden/Private/AI/Boss/AesirBossPolicyClientComponent.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Boss/AesirBossPolicyClientComponent.cpp)

- [AesirWarden · Source/AesirWarden/Private/AI/Boss/AesirBossAIController.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Boss/AesirBossAIController.cpp)

- [AesirWarden · Docs/06_Test_Doc/KnownIssues.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/06_Test_Doc/KnownIssues.md)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T15 · 接通 RL 开战、结束与重试

[AesirWarden#17](https://github.com/YYchainsAw/AesirWarden/issues/17)；标签：`enhancement`、`priority:p0`、`area:boss-rl`、`area:level-flow`、`needs:runtime-verification`

**任务编号：** `T15`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** M2 · Boss 系统完善与 RL 实战闭环

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

BossController OnPossess 自动启动策略与 Episode；PolicyClient Stop 可取消当前请求；回退当前结束旧 Episode 并新开 Fallback Episode，需要与 M1 流程和研究索引协调。

### 任务范围

- 沿用 M1 战斗边界接通 RL 请求、目标、Episode 和终局，不让过场提前决策。

- 检查迟到响应、取消请求、定时器、重试和返回；记录回退前后 Episode 的关联规则。

### 完成条件

- [ ] RL 只在正式开战后请求，终局/离开战斗停止请求，过场时间不计战斗。

- [ ] 旧请求取消或迟到响应不会作用于下一局，连续重试生成新身份并重置目标/计数/遥测。

- [ ] 回退前后 Episode/策略来源有可回查关联，不把分段 Episode 误记为不同完整遭遇。

- [ ] PPO 与 BT 回退都能完成胜败、重试和返回。

### 验收证据

请求/遭遇/Episode 关联表、取消/迟到响应与回退记录、连续重试/返回日志。

### 前置依赖

- [T4 · T4 · 接通 Boss 入口与 BT 开战](https://github.com/YYchainsAw/AesirWarden/issues/6)

- [T7 · T7 · 接通重试与返回菜单](https://github.com/YYchainsAw/AesirWarden/issues/7)

- [T12 · T12 · 校验模型并启动推理服务](https://github.com/YYchainsAw/aesir-ai-service/issues/17)

- [T14 · T14 · 修复连续失败计数与 BT 回退](https://github.com/YYchainsAw/AesirWarden/issues/2)

依赖约束最终验收；代码盘点、样例和资料可以提前准备。

### 范围边界

扩展当前流程与组件生命周期；回退遥测分段策略由本任务明确，不强制重写整个遥测系统。

### 来源与基线

- [AesirWarden · Source/AesirWarden/Private/AI/Boss/AesirBossAIController.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Boss/AesirBossAIController.cpp)

- [AesirWarden · Source/AesirWarden/Private/AI/Boss/AesirBossPolicyClientComponent.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Boss/AesirBossPolicyClientComponent.cpp)

- [AesirWarden · Source/AesirWarden/Private/AI/Boss/AesirBossTelemetryComponent.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Boss/AesirBossTelemetryComponent.cpp)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T16 · 验证 UE 与 RL 模型完整战斗

[AesirWarden#5](https://github.com/YYchainsAw/AesirWarden/issues/5)；标签：`type:test`、`priority:p0`、`area:boss-rl`、`area:gas`、`area:services`、`needs:runtime-verification`

**任务编号：** `T16`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** M2 · Boss 系统完善与 RL 实战闭环

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

UE/Python v4、26 维、九动作静态一致；实际当前构建的 PPO 完整 Episode 和请求/响应证据尚待验收。

### 任务范围

- 使用 M2 服务、动作规则和生命周期，保存实际请求/响应/Outcome。

- 在与 BT 基线一致的执行器和场地下完成冻结 PPO 战斗。

### 完成条件

- [ ] 特征顺序、schema、request_id/sequence、模型身份和动作 ID 与真实请求匹配。

- [ ] 各动作实际可用/拒绝路径与掩码一致，Accepted、命中和完成分别可识别。

- [ ] PPO 完成实际 UE Episode，伤害、策略、终局及版本可追踪，服务锁定与故障接管记录清晰。

### 验收证据

RL-CONTRACT-01 原始请求/响应、动作核验表、完整 PPO Episode、视频与版本索引。

### 前置依赖

- [T5 · T5 · 验证 BT Boss 战斗与结算](https://github.com/YYchainsAw/AesirWarden/issues/4)

- [T9 · T9 · 完善 Boss 动作与战斗规则](https://github.com/YYchainsAw/AesirWarden/issues/16)

- [T12 · T12 · 校验模型并启动推理服务](https://github.com/YYchainsAw/aesir-ai-service/issues/17)

- [T13 · T13 · 验证 HTTP 接口与故障处理](https://github.com/YYchainsAw/aesir-ai-service/issues/18)

- [T14 · T14 · 修复连续失败计数与 BT 回退](https://github.com/YYchainsAw/AesirWarden/issues/2)

- [T15 · T15 · 接通 RL 开战、结束与重试](https://github.com/YYchainsAw/AesirWarden/issues/17)

依赖约束最终验收；代码盘点、样例和资料可以提前准备。

### 范围边界

集成验收单个完整闭环，正式成组 BT–PPO 比较归 M3。既有跨仓子任务关系可保留。

### 来源与基线

- [AesirWarden · Source/AesirWarden/Public/AI/Boss/AesirBossObservationTypes.h](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Public/AI/Boss/AesirBossObservationTypes.h)

- [AesirWarden · Source/AesirWarden/Private/AI/Boss/AesirBossObservationTypes.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Boss/AesirBossObservationTypes.cpp)

- [AesirWarden · Docs/08_RL_Research/BossRL_Contract.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/08_RL_Research/BossRL_Contract.md)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。

## T17 · 验收 RL 与回退 Windows 包

[AesirWarden#18](https://github.com/YYchainsAw/AesirWarden/issues/18)；标签：`type:test`、`priority:p0`、`area:boss-rl`、`area:build`、`area:services`、`needs:runtime-verification`

**任务编号：** `T17`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** M2 · Boss 系统完善与 RL 实战闭环

**责任：** YYchainsAw；跨仓服务准备与 UE 集成分别提供实际证据。

### 已有功能与剩余工作

M1 交付 BT Windows 基线；M2 的真实服务和 UE 联调完成后需要在打包环境验证相同流程与故障行为。

### 任务范围

- 固定 M1 基线之上的 RL 构建、地图、模型、服务和依赖。

- 在目标 Windows 环境覆盖 PPO、故障回退、胜败、重试与返回。

### 完成条件

- [ ] 真实模型服务可按说明启动，PPO 和 BT 回退都能完成完整第一关。

- [ ] 断连/超时/异常动作可重放，接管后战斗不挂起，至少连续两次重试没有旧状态残留。

- [ ] 记录包/模型/资产/代码版本、运行环境、日志、延迟和帧表现，未通过项登记可追踪缺陷。

### 验收证据

包/模型哈希、RL-FAULT-01 与 BUILD-01 回归记录、连续重试视频、性能和故障日志。

### 前置依赖

- [T8 · T8 · 验收 BT Windows 包](https://github.com/YYchainsAw/AesirWarden/issues/8)

- [T16 · T16 · 验证 UE 与 RL 模型完整战斗](https://github.com/YYchainsAw/AesirWarden/issues/5)

依赖约束最终验收；代码盘点、样例和资料可以提前准备。

### 范围边界

M2 阶段打包回归；M3 冻结正式性能判据与实验，M6 负责最终表现和整合。

### 来源与基线

- [AesirWarden · Docs/07_Release/ReleaseChecklist.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/07_Release/ReleaseChecklist.md)

- [AesirWarden · Docs/06_Test_Doc/VerificationMatrix.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/06_Test_Doc/VerificationMatrix.md)

- [AesirWarden · Docs/08_RL_Research/EvidenceRegister.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/08_RL_Research/EvidenceRegister.md)

实现存在、用户确认的可玩状态、静态核对和实际验收分别记录；执行时复核当前版本。
