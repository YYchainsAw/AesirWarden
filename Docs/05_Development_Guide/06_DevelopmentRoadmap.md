# 按用户确认目标整理的开发路线与里程碑

> 当前任务编号：**M1 = T1–T8；M2 = T9–T17；M5 = T18–T26；N2 = T27–T29；N3 = T30**。两仓连续编号，GitHub 原生 Issue 编号保留。

> 2026-10-07 · 用户已批准 M1–M6 及 M1/M2、NPC 拆分。当前 30 条 Issue、正文、标签、依赖及 Project 已同步；历史删除的 16 条 Issue 保留索引，本地备份已由用户移除，NPC 按新稿创建 T18–T30。

## 目标与顺序

用户确认：玩家已经可以基础移动、攻击和翻滚，BT Boss 已经可以战斗。当前第一目标是完成一遍完整的游玩流程。后续必须推进玩家数值、技能和手感，制作能够对接 AI 服务聊天与战斗的队友，并重点完善 RL Boss；用户同时负责 UE Boss 系统与 Python 仓库的 Boss RL。

本稿采用以下顺序：**先交付 BT 完整流程；随后以 RL Boss 为主要工作线，玩家和队友按需要并行推进；最后整合演示。**后续技能清单、数值、训练预算与时间尚未确定，因此后续里程碑先明确交付边界和验收方法，不预填工期或功能数量。

用户负责 UE 侧流程、玩家、Boss、队友执行及服务接入，以及 Python `rl/boss/` 与 `scripts/rl/boss/`。队友负责 Python NPC 服务，包括聊天、人格、记忆、关系、行为与协议支持。负责人沿用 GitHub 最新设置，实际账号见当前任务记录；这里说明责任范围。

## 本次核对的开发进度

### Git 与版本边界

| 来源 | 当前版本与记录 | 对计划的意义 |
| --- | --- | --- |
| UE 本地 `develop` | `ea4c805`，2026-09-30；`6f24ff3` 新增流程控制器，`ea4c805` 补充初始化检查、流程处理和接入说明 | 菜单到探索已有 C++，应从现有接线继续推进 |
| UE 历史边界 | 玩家、Boss、同伴的主要代码随 `faec22c` 首次公开导入；本地历史只有五个提交 | 提交日期不能代表各个机制最初完成的日期，需结合代码、资产和实际运行记录判断 |
| UE 公开 `main` | `facba39`，2026-10-06，最新为 Issue 模板提交 | 本地流程实现尚未进入公开主分支；里程碑证据要记录实际使用的本地代码版本 |
| Python 用户本地 `main` | `7caf4d9`，2026-09-24，工作区干净 | 可核对实际训练文件与本地 RL 代码；不自动拉取或切换用户工作区 |
| Python 公开 `main` | `1678dd6`，2026-09-30；与此前保存的只读快照一致 | NPC 最新规划和实现应以这个版本补充核对，避免只读旧本地文档 |
| 用户 RL 提交 | `5d0730d` 结构初始化；`9b9d024` v4 契约；`ae99eca` 动作掩码；`ea58b9c` 可复现重置种子；`055f2de` 冻结推理服务 | 训练、契约、服务已有基础，后续重点是规则同步与实际游戏交付 |

### 已有实现与剩余工作

| 模块 | 已核对的基础 | 下一步需要解决的边界 |
| --- | --- | --- |
| 玩家战斗 | 用户确认基础移动、攻击、翻滚已可用；源码还包含轻重击、连段输入缓冲、目标锁定、防御与反击窗口等入口 | 在当前关卡保留可玩基线；后续单独收口技能、数值、打击反馈与输入体验 |
| 完整流程 | `AesirLevelFlowDirector` 提供菜单、开场、探索及相机/输入交接；文档记录 09-30 编译通过；菜单和结果 UI 资产存在 | `BossIntro/BossCombat/EndingCinematic/Results` 仍是预留状态；完整接入和运行验收待做 |
| 胜负与重试 | GameMode 有一次性结算保护，绑定玩家与敌人死亡；结果 Widget 存在 | 当前按 BeginPlay 时敌人计数判定胜利；需明确只由目标 Boss 触发通关，并接通重试、返回和旧状态清理 |
| 玩家数值 | Health、GuardPressure、PhysicalPower、DamageReduction、RunePower、CooldownHaste 已有 AttributeSet；伤害执行类已有力量与减伤计算 | 字段和公式实现不等于完整数值系统验收；需核对实际 GE/能力走向、初值、边界、冷却和 UI |
| UE Boss/RL | BT、共享 Action/GAS 执行器、Observation、Telemetry、奖励组件、策略客户端和回退入口存在；v4/26 特征/9 动作 | 核对每个动作实际配置与执行结果，修复连续非法 Action 计数，明确策略/Episode 与关卡流程启停边界 |
| Python Boss RL | 模拟器、奖励、MaskablePPO 训练、检查点、评估、独立 8012 推理服务存在 | 完善与 UE 当前规则的一致性、模型交付校验、冻结 PPO 独立评估入口及逐 Episode 原始导出 |
| AI 队友 UE | CompanionCharacter、BT 资产、聊天/命令 HTTP 子系统、战术命令校验和状态管理存在；控制器已处理探索 follow/wait | follow/wait 只在限定条件下处理；战斗时会忽略这条探索指令路径。命令被接受与实际技能完成需用游戏执行和回执证明 |
| Python NPC | 最新公开版本已有聊天、语音入口、记忆、关系、行为、事件、降级与回执基础；队友规划继续收口人格包和游戏档案 | 复用已有服务，重点推进当前 UE 联调；跨游戏 N2/N3 保留独立扩展路线 |

本机存在 sim005 的 seed 0/1/2 模型和 manifest，每组记录 `1,015,808` timesteps。部署候选来自 seed 1；本次重新计算 ZIP SHA-256 为 `dd79ceca0e0f079634364754d1959212b0cdb357439ab9ef8e36d3ba4ed64add`，与 selection 文件一致。selection 中的 900 局用于选模，后续独立评估不得复用。

本次没有重新编译、运行 PIE、打包、训练或启动真实服务联调。资产清单不能证明 Blueprint 节点和配置正确；用户确认的基础可玩状态与本次静态核对分别记录。

依据：[流程接入](05_LevelFlowGuide.md)、[游戏流程](../02_Design_Doc/GDD/01_GameFlow.md)、[战斗设计](../02_Design_Doc/GDD/03_CombatDesign.md)、[GAS 架构](../02_Design_Doc/TDD/02_GasArchitecture.md)、[验证矩阵](../06_Test_Doc/01_VerificationMatrix.md)、[已知问题](../06_Test_Doc/02_KnownIssues.md)、[训练记录](../08_RL_Research/02_TrainingHistory.md)、[评估方案](../08_RL_Research/03_EvaluationProtocol.md)、[NPC 审查](17_CompanionWorkReview.md)。

## 六个主要交付目标

统一使用 M1–M6 定位交付目标，实际推进按依赖安排；GitHub Milestone 数字编号与标题中的阶段编号是不同字段。M4/M5 可与 M2 协调并行。

| 编号与已确认标题 | 用通俗语言描述完成后的样子 | 工作安排 | 主要仓库与责任 |
| --- | --- | --- | --- |
| **M1 · 完整游玩流程与 BT Boss 基线** | 打开游戏，开始游玩，走到 Boss，打赢或失败，能够重试和返回菜单 | 当前先完成 | UE；用户 |
| **M2 · Boss 系统完善与 RL 实战闭环** | Boss 能通过完整动作系统战斗；Python 冻结策略在真实 UE 中驱动同一 Boss，故障时继续由 BT 战斗 | M1 后的重点；工具准备可提前 | UE + Python RL；用户 |
| **M3 · RL 对照评估与复现证据** | 能说明训练如何产生策略、策略在游戏里怎样表现，以及与 BT 的差异和限制 | M2 稳定后正式采集 | UE + Python RL；用户 |
| **M4 · 玩家数值、技能与手感** | 玩家属性、伤害与冷却有一致规则，约定技能可用，攻击和翻滚衔接经过实测调整 | M1 后推进；与 M2 协调战斗版本 | UE；用户 |
| **M5 · AI 队友聊天与战斗联调** | 队友在游戏里能聊天、响应约定指令、实际参与战斗，并反馈成功或失败 | 服务侧可以现在准备；UE 接入与 M1 协调 | UE 用户 + Python NPC 队友 |
| **M6 · 综合演示与交付** | 在一个明确版本中演示完整流程、玩家能力、RL Boss 和 AI 队友，附启动说明与成果证据 | 各项选定范围完成后整合 | UE 用户；Python 两条线配合 |

队友既有 **N2 人格包与跨游戏接入 / N3 外部包校验**继续保留，属于 NPC 服务扩展。它们可以按队友计划推进，不作为 M1、M2、M3 的关闭条件。

## M1：先完成一遍完整游玩

**目标：**利用已可战斗的 BT Boss，把各个已有部件连成完整第一关。

**检查点：**开始与输入交接 → 到达 Boss 与开战 → 胜败结算 → 重试/返回 → Windows 运行。

- [ ] 当前地图中的流程控制器、菜单和玩家相机完成配置；开始后正确交还移动和战斗输入，HUD 正确显示。
- [ ] 玩家可以从第一关起点走到 Boss 入口；触发开战只发生一次，开战前 Boss 不攻击玩家。开场和入场允许先采用简短或占位表现。
- [ ] BT Boss 能完成一场战斗；仅目标 Boss 死亡触发通关，玩家死亡触发失败，终局停止战斗并只显示一次正确结果。
- [ ] 重试从第一关起点重新游玩，默认跳过菜单和开场；至少连续重试两次，恢复生命、冷却、敌人、输入、UI、委托和触发状态。返回菜单后可以再次开始。
- [ ] Windows 基线包冷启动完成上述流程，保存构建版本、完整资产恢复/备份说明、录屏和日志。

**关闭依据：**一份可运行 BT 基线包，加上开始、胜利、失败、重试和返回的实际记录。冻结 PPO 联调在 M2 验收。正式动画、美术和新增技能可在后续完善。

**本轮最先执行：**核对当前地图接线；接入 Boss 入口和策略启停；完善 Boss 胜利判定；接通结果与重试；最后打包复测。现有代码与资产尽量复用。

## M2：完善 Boss，并让 RL 真正在游戏里战斗

**目标：**用户的 UE Boss 工作与 Python RL 工作形成一个可追踪的实际战斗闭环。

**检查点：**Boss 动作与规则清单 → Python 环境/奖励/模型一致性 → 真实模型服务 → UE Episode 与故障回退 → Windows 回归。

- [ ] 盘点现有九动作、GA 配置、目标/距离限制、忙碌/受击/死亡状态、冷却和动作 Outcome；列出已完成、缺失与本阶段新增项，核验合法和拒绝执行路径。新增招式或阶段在盘点后确定范围。
- [ ] 核对 UE 与模拟器的动作语义、时间单位、伤害、防御、冷却和 Observation。规则改动记录 simulation/reward/schema 版本；原模型是否可复用有实际依据，需要重训时归档新训练配置和产物。
- [ ] 交付可定位的模型、训练/选模 manifest、SHA、依赖和启动命令；服务正常响应与锁定状态有真实模型记录。
- [ ] BT 与 PPO 使用同一 Boss 执行器；保存同一次 UE 请求/响应、动作执行、伤害和终局，确认冻结 PPO 完成完整 Episode。
- [ ] 修复 KI-01；断连、超时、坏 JSON、Schema 错误、缺失/越界动作都有故障记录，连续失败后的 BT 接管与终局正确。
- [ ] 在 M1 Windows 基线之上完成 PPO 和故障回退验证，重试后没有上一局请求、决策或遥测残留。

**关闭依据：**明确版本的 Boss 动作系统、匹配的 Python 训练/推理交付、真实 PPO Episode、回退证据和打包回归。

RL 主线暂沿用离线训练与冻结推理。在线自适应、额外 Boss 数量和额外算法比较需要单独立项。

## M3：用实验说明 RL 的效果

**目标：**交付可以复核的研究结果，回答 RL 策略怎样响应战况、行为是否不同、运行成本是多少。

**检查点：**冻结实验 → 独立模拟器评估 → UE 同条件对照 → 分析与归档。

- [ ] 正式采集前固定代码、资产、模型、机器、数值、技能、初态、测试条件、种子、局数、指标和排除规则。
- [ ] 冻结 PPO 可独立评估，无需重新训练；导出逐 Episode 数据、失败/超时/排除记录和版本 manifest。训练集、选模集和独立测试集分开。
- [ ] 使用同一 Boss、执行器和遭遇规则完成 UE BT–PPO 对照；模拟器规则策略结果单独说明。
- [ ] 从原始数据生成胜负、时长、伤害、动作多样性/重复、拒绝、回退、延迟、帧表现和失败案例，保留命令、报告及限制。

**关闭依据：**预定实验已执行，原始数据与报告可以复现。允许报告改善有限或未发现改善。

默认正式对照沿用单玩家对单 Boss，不把新同伴作为实验变量。M4、M5、M6 的改动若改变伤害、技能时序、输入、Boss 行为或性能，使用冻结研究版本，或以新版本重新验证受影响条件。

## M4：完善玩家数值、技能和手感

**目标：**把已可操作的玩家做成规则一致、能力完整且经过体验调整的战斗角色。

**检查点：**现有机制盘点 → 数值与技能范围冻结 → 实现/接线 → 手感调整 → 回归。

- [ ] 明确本阶段的属性、初值、伤害/减伤、冷却规则及来源；核对 AttributeSet、GE、旧战斗组件、UI 和重试重置是否一致。
- [ ] 列出本阶段技能清单及输入、目标、效果、冷却、取消/受击、动画和反馈要求，再逐项实现；不使用“补齐所有技能”作为无限范围的关闭条件。
- [ ] 调整已存在的连段缓冲、攻击/翻滚衔接、无敌/命中窗口、锁定和相机，保存调整参数与具体体验问题的前后记录。
- [ ] 在 BT/PPO 场景回归伤害、防御、死亡、胜负和重试，说明对已冻结 RL 模型/实验的影响。

**关闭依据：**约定技能清单完成、数值规则可查、实际战斗回归通过，手感问题有具体处理记录。角色成长、装备、持久存档和新增资源机制待需要明确后另行设计。

## M5：让 AI 队友真正参与游戏

**目标：**用户实现并接通 UE 同伴，队友提供可靠的 Python NPC 服务与联调支持。

**检查点：**协议与真实能力对齐 → 聊天/探索 → 基础战斗/指令 → 执行回执 → 全流程回归。

- [ ] 先启用一个 NPC，明确人格/角色 ID、协议、端口和真实 UE 能力清单；未制作的技能明确拒绝。基础战斗能力与协作技能的清单在接入任务中确定。
- [ ] 游戏内文本聊天、对话 UI 焦点、探索跟随/等待有效；基础语音沿用现有入口并注明实际可用的服务条件。
- [ ] 同伴在真实场景能选取有效目标并使用约定能力；玩家指令、条件触发、取消/替换、冷却、死亡和遇敌结束有明确处理。
- [ ] 请求/指令 ID、UE 校验、执行、完成/失败/取消及单条/批量回执能够串联；不能把“已接受”记成“已经完成”。
- [ ] 修正服务指标漏采并补边界验收；断线/超时/能力不可用有可见结果，胜败、重试和返回后不保留旧命令与 UI/角色状态。

**关闭依据：**当前 UE 版本中的真实聊天与战斗演示，加上服务侧证据和可追踪的执行回执。Python Mock、命令 JSON 和目录声明分别作为准备证据。

## M6：整合成可展示的项目版本

这是建议的最终整合目标，具体演示内容在前述范围明确后冻结。

- [ ] 选定 M1、M2、M4、M5 的交付版本，完成正式菜单、必要过场、引导、收尾、UI 和音画；记录自然结束/跳过的控制权一致性。
- [ ] 同一 Windows 包可以演示完整第一关、约定玩家技能、RL Boss 和 NPC；服务启动、离线行为、胜败和重试通过回归。
- [ ] 发布包、两仓代码、完整资产快照、模型、依赖、素材来源、启动说明、演示视频和 M3 研究资料有对应索引。

M1、M2、M3 可以先分别完成并展示；玩家和同伴的后续工作按本阶段约定范围整合。

## 已同步的 GitHub 里程碑

旧链接保留：UE 原 M2 研究阶段成为 M3，原 M3 演示阶段成为 M6，原 NPC N1 成为 M5；新增 UE M2 和 M4。Python 原 Boss M1/M2 分别成为 M2/M3，NPC N1 成为 M5，N2/N3 保留。

| 仓库 | 阶段 | 当前标题与实际链接 |
| --- | --- | --- |
| AesirWarden | M1 | [M1 · 完整游玩流程与 BT Boss 基线](https://github.com/YYchainsAw/AesirWarden/milestone/1) |
| AesirWarden | M2 | [M2 · Boss 系统完善与 RL 实战闭环](https://github.com/YYchainsAw/AesirWarden/milestone/5) |
| AesirWarden | M3 | [M3 · RL 对照评估与复现证据](https://github.com/YYchainsAw/AesirWarden/milestone/2) |
| AesirWarden | M4 | [M4 · 玩家数值、技能与手感](https://github.com/YYchainsAw/AesirWarden/milestone/6) |
| AesirWarden | M5 | [M5 · AI 队友聊天与战斗联调](https://github.com/YYchainsAw/AesirWarden/milestone/4) |
| AesirWarden | M6 | [M6 · 综合演示与交付](https://github.com/YYchainsAw/AesirWarden/milestone/3) |
| aesir-ai-service | M2 | [M2 · Boss RL 训练、模型与推理联调基线](https://github.com/YYchainsAw/aesir-ai-service/milestone/1) |
| aesir-ai-service | M3 | [M3 · 独立模拟器评估与 RL 复现证据](https://github.com/YYchainsAw/aesir-ai-service/milestone/2) |
| aesir-ai-service | M5 | [M5 · NPC 服务基线与 UE 聊天、战斗联调支持](https://github.com/YYchainsAw/aesir-ai-service/milestone/3) |
| aesir-ai-service | N2 | [N2 · S2–S4 人格包与跨游戏最小接入](https://github.com/YYchainsAw/aesir-ai-service/milestone/4) |
| aesir-ai-service | N3 | [N3 · S5 外部人格包守卫与接入门禁](https://github.com/YYchainsAw/aesir-ai-service/milestone/5) |

实际创建与回读记录见 [里程碑同步记录](14_MilestoneSyncRecord.json)。保留任务沿用原生编号和链接，现有跨仓父子关系保留；后期删除记录另存备份。

### 当前 30 条 Issue 的阶段归属

| 任务 | 阶段 | 目标仓库 | Issue | 验收前置 | 汇总任务 |
| --- | --- | --- | --- | --- | --- |
| T1 | M1 | AesirWarden | [T1 · 整理 BT 工程与资产恢复说明](https://github.com/YYchainsAw/AesirWarden/issues/1) | 无 | — |
| T2 | M1 | AesirWarden | [T2 · 接通菜单、开场与探索流程](https://github.com/YYchainsAw/AesirWarden/issues/14) | 无 | — |
| T3 | M1 | AesirWarden | [T3 · 验证玩家基础操作](https://github.com/YYchainsAw/AesirWarden/issues/3) | 无 | — |
| T4 | M1 | AesirWarden | [T4 · 接通 Boss 入口与 BT 开战](https://github.com/YYchainsAw/AesirWarden/issues/6) | [T2](https://github.com/YYchainsAw/AesirWarden/issues/14) | — |
| T5 | M1 | AesirWarden | [T5 · 验证 BT Boss 战斗与结算](https://github.com/YYchainsAw/AesirWarden/issues/4) | [T3](https://github.com/YYchainsAw/AesirWarden/issues/3)、[T4](https://github.com/YYchainsAw/AesirWarden/issues/6) | — |
| T6 | M1 | AesirWarden | [T6 · 接通胜败判定与结果页](https://github.com/YYchainsAw/AesirWarden/issues/15) | [T4](https://github.com/YYchainsAw/AesirWarden/issues/6) | — |
| T7 | M1 | AesirWarden | [T7 · 接通重试与返回菜单](https://github.com/YYchainsAw/AesirWarden/issues/7) | [T4](https://github.com/YYchainsAw/AesirWarden/issues/6)、[T6](https://github.com/YYchainsAw/AesirWarden/issues/15) | — |
| T8 | M1 | AesirWarden | [T8 · 验收 BT Windows 包](https://github.com/YYchainsAw/AesirWarden/issues/8) | [T1](https://github.com/YYchainsAw/AesirWarden/issues/1)、[T3](https://github.com/YYchainsAw/AesirWarden/issues/3)、[T5](https://github.com/YYchainsAw/AesirWarden/issues/4)、[T6](https://github.com/YYchainsAw/AesirWarden/issues/15)、[T7](https://github.com/YYchainsAw/AesirWarden/issues/7) | — |
| T9 | M2 | AesirWarden | [T9 · 完善 Boss 动作与战斗规则](https://github.com/YYchainsAw/AesirWarden/issues/16) | 无 | — |
| T10 | M2 | aesir-ai-service | [T10 · 同步 UE 与 RL 训练规则](https://github.com/YYchainsAw/aesir-ai-service/issues/29) | [T9](https://github.com/YYchainsAw/AesirWarden/issues/16) | — |
| T11 | M2 | aesir-ai-service | [T11 · 验证模型并整理训练记录](https://github.com/YYchainsAw/aesir-ai-service/issues/30) | [T10](https://github.com/YYchainsAw/aesir-ai-service/issues/29) | — |
| T12 | M2 | aesir-ai-service | [T12 · 校验模型并启动推理服务](https://github.com/YYchainsAw/aesir-ai-service/issues/17) | [T11](https://github.com/YYchainsAw/aesir-ai-service/issues/30) | [T16](https://github.com/YYchainsAw/AesirWarden/issues/5) |
| T13 | M2 | aesir-ai-service | [T13 · 验证 HTTP 接口与故障处理](https://github.com/YYchainsAw/aesir-ai-service/issues/18) | [T10](https://github.com/YYchainsAw/aesir-ai-service/issues/29) | [T16](https://github.com/YYchainsAw/AesirWarden/issues/5) |
| T14 | M2 | AesirWarden | [T14 · 修复连续失败计数与 BT 回退](https://github.com/YYchainsAw/AesirWarden/issues/2) | [T13](https://github.com/YYchainsAw/aesir-ai-service/issues/18) | — |
| T15 | M2 | AesirWarden | [T15 · 接通 RL 开战、结束与重试](https://github.com/YYchainsAw/AesirWarden/issues/17) | [T4](https://github.com/YYchainsAw/AesirWarden/issues/6)、[T7](https://github.com/YYchainsAw/AesirWarden/issues/7)、[T12](https://github.com/YYchainsAw/aesir-ai-service/issues/17)、[T14](https://github.com/YYchainsAw/AesirWarden/issues/2) | — |
| T16 | M2 | AesirWarden | [T16 · 验证 UE 与 RL 模型完整战斗](https://github.com/YYchainsAw/AesirWarden/issues/5) | [T5](https://github.com/YYchainsAw/AesirWarden/issues/4)、[T9](https://github.com/YYchainsAw/AesirWarden/issues/16)、[T12](https://github.com/YYchainsAw/aesir-ai-service/issues/17)、[T13](https://github.com/YYchainsAw/aesir-ai-service/issues/18)、[T14](https://github.com/YYchainsAw/AesirWarden/issues/2)、[T15](https://github.com/YYchainsAw/AesirWarden/issues/17) | — |
| T17 | M2 | AesirWarden | [T17 · 验收 RL 与回退 Windows 包](https://github.com/YYchainsAw/AesirWarden/issues/18) | [T8](https://github.com/YYchainsAw/AesirWarden/issues/8)、[T16](https://github.com/YYchainsAw/AesirWarden/issues/5) | — |
| T18 | M5 | aesir-ai-service | [T18 · 对齐 NPC 协议与联调配置](https://github.com/YYchainsAw/aesir-ai-service/issues/31) | 无 | [T22](https://github.com/YYchainsAw/aesir-ai-service/issues/35) |
| T19 | M5 | aesir-ai-service | [T19 · 修复 NPC 指标漏采](https://github.com/YYchainsAw/aesir-ai-service/issues/32) | 无 | [T22](https://github.com/YYchainsAw/aesir-ai-service/issues/35) |
| T20 | M5 | aesir-ai-service | [T20 · 验证 NPC 状态与时间边界](https://github.com/YYchainsAw/aesir-ai-service/issues/33) | 无 | [T22](https://github.com/YYchainsAw/aesir-ai-service/issues/35) |
| T21 | M5 | aesir-ai-service | [T21 · 提供 NPC 联调与故障样例](https://github.com/YYchainsAw/aesir-ai-service/issues/34) | [T18](https://github.com/YYchainsAw/aesir-ai-service/issues/31) | [T22](https://github.com/YYchainsAw/aesir-ai-service/issues/35) |
| T22 | M5 | aesir-ai-service | [T22 · 验收 NPC 服务与人格表现](https://github.com/YYchainsAw/aesir-ai-service/issues/35) | [T18](https://github.com/YYchainsAw/aesir-ai-service/issues/31)、[T19](https://github.com/YYchainsAw/aesir-ai-service/issues/32)、[T20](https://github.com/YYchainsAw/aesir-ai-service/issues/33)、[T21](https://github.com/YYchainsAw/aesir-ai-service/issues/34) | [T26](https://github.com/YYchainsAw/AesirWarden/issues/22) |
| T23 | M5 | AesirWarden | [T23 · 接通队友聊天与探索指令](https://github.com/YYchainsAw/AesirWarden/issues/19) | [T18](https://github.com/YYchainsAw/aesir-ai-service/issues/31)、[T21](https://github.com/YYchainsAw/aesir-ai-service/issues/34) | [T26](https://github.com/YYchainsAw/AesirWarden/issues/22) |
| T24 | M5 | AesirWarden | [T24 · 接通队友战斗与执行回执](https://github.com/YYchainsAw/AesirWarden/issues/20) | [T4](https://github.com/YYchainsAw/AesirWarden/issues/6)、[T18](https://github.com/YYchainsAw/aesir-ai-service/issues/31)、[T21](https://github.com/YYchainsAw/aesir-ai-service/issues/34) | [T26](https://github.com/YYchainsAw/AesirWarden/issues/22) |
| T25 | M5 | AesirWarden | [T25 · 接通队友事件与流程重置](https://github.com/YYchainsAw/AesirWarden/issues/21) | [T7](https://github.com/YYchainsAw/AesirWarden/issues/7)、[T23](https://github.com/YYchainsAw/AesirWarden/issues/19)、[T24](https://github.com/YYchainsAw/AesirWarden/issues/20) | [T26](https://github.com/YYchainsAw/AesirWarden/issues/22) |
| T26 | M5 | AesirWarden | [T26 · 验收队友聊天与战斗联调](https://github.com/YYchainsAw/AesirWarden/issues/22) | [T8](https://github.com/YYchainsAw/AesirWarden/issues/8)、[T22](https://github.com/YYchainsAw/aesir-ai-service/issues/35)、[T25](https://github.com/YYchainsAw/AesirWarden/issues/21) | — |
| T27 | N2 | aesir-ai-service | [T27 · 完成人格包迁移与校验](https://github.com/YYchainsAw/aesir-ai-service/issues/36) | 无 | [T29](https://github.com/YYchainsAw/aesir-ai-service/issues/38) |
| T28 | N2 | aesir-ai-service | [T28 · 发布游戏档案与协议 Schema](https://github.com/YYchainsAw/aesir-ai-service/issues/37) | [T27](https://github.com/YYchainsAw/aesir-ai-service/issues/36) | [T29](https://github.com/YYchainsAw/aesir-ai-service/issues/38) |
| T29 | N2 | aesir-ai-service | [T29 · 接通 L0/L3 跨游戏服务](https://github.com/YYchainsAw/aesir-ai-service/issues/38) | [T27](https://github.com/YYchainsAw/aesir-ai-service/issues/36)、[T28](https://github.com/YYchainsAw/aesir-ai-service/issues/37) | — |
| T30 | N3 | aesir-ai-service | [T30 · 完成外部人格包校验](https://github.com/YYchainsAw/aesir-ai-service/issues/39) | [T29](https://github.com/YYchainsAw/aesir-ai-service/issues/38) | — |

M1 为 8 项，M2 为 9 项，M5 为 9 项，N2 为 3 项，N3 为 1 项。M1/M2 原批更新 10 项、新建 7 项；本批 NPC 新建 13 项，按已有实现补剩余工作。M3/M4/M6 保留里程碑目标，尚未拆分任务。详见 [M1/M2 当前清单](07_M1M2Issues.md)、[NPC 当前清单](08_CompanionIssues.md)。旧批次资料与本地备份已按用户确认删除。

### 已实施的范围与依赖

1. UE #1 的 BT 工程恢复解除对 Python #17 模型交付的阻塞；正式包哈希由 T8 回填，不阻塞恢复准备。
2. UE #7 的 BT 重试解除对 UE #5 PPO 验收的阻塞；Boss 入口和结果页为新的验收前置。
3. UE #8 验收 M1 BT Windows 基线；T17 另验收 PPO 与故障回退的 Windows 流程。
4. M2 分别收口 Boss 九动作、环境/奖励版本、候选模型、真实推理、契约边界、连续失败回退、Episode 生命周期和完整战斗。
5. 原 M1/M2 的 28 条依赖保留；NPC 新增 20 条，总计 48 条且无循环。父子关系共 12 组，原 Python #17/#18 仍为 UE #5 子任务，新增 T22/T26/T29 汇总关系。旧后期依赖完整保存于备份。
6. Project 当前显示 30 项任务，原 17 项状态和顺序、阶段选项及五个视图保留。NPC 按 M5/N2/N3 分阶段，M3/M4/M6 目标暂未拆分。

## 排期与关闭规则

- 当前先细排 M1；M2 是随后主线，M3 的方案与工具可以提前准备。M4/M5 的正式规模先依据技能和联调清单估算。
- 设置截止日期前核对剩余工作、每周投入时间和真实演示/答辩日期；历史文档里的服务人天估算不能直接替代整体项目工期。
- 每个检查点记录实现负责人、联调支持和实际验收者，以及精确代码/资产/模型版本。
- Issue 关闭数量仅表示任务进度。里程碑关闭必须具有本稿规定的运行交付和证据；修改战斗条件后说明回归范围。
- 本稿与 GitHub 里程碑及 Project 阶段已按用户确认的目标同步。批准的 Issue 正文、依赖与新增任务已发布。旧批次创建记录为历史；本地备份已由用户移除，删除任务只保留历史索引和文件名说明。
