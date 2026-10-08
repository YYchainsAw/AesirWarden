# NPC 与跨仓联调 Issue 清单（已发布）

> 2026-10-07 · 用户已批准，13 条 T18–T30 已通过 gh 创建并回读核验；原 T1–T17 的内容、状态与 Project 设置保留，负责人按 GitHub 最新设置同步。

本批已创建 13 项：Python 9 项由队友推进，UE 4 项由用户接入。继续使用简短 `T编号 · 动作与目标` 标题、目标仓库、既有基础、范围、验收证据、标签及原生依赖/父子关系。

| 阶段 | 任务范围 | Python | UE | 对应交付 |
| --- | --- | --- | --- | --- |
| M5 | **T18–T26** | 5 | 4 | NPC 服务与当前游戏聊天/战斗联调 |
| N2 | **T27–T29** | 3 | 0 | 自有人格包与 L0/L3 最小跨游戏接入 |
| N3 | **T30** | 1 | 0 | 外部包实际校验与审核门禁 |

编号接续当前统一 Project，不改 T1–T17。Python SDD 中历史 T001–T088 保留为源文档编号，本批 T18–T30 是 Project 追踪编号。M3/M4/M6 暂未分配新任务号。上述任务范围与实际链接已写入 4 个里程碑的描述和统一 Project；20 条新原生依赖、10 组新父子关系已核验。

## 已核对的当前基础

- Python 公开 main `1678dd6`（2026-09-30）仍为最新；队友 develop-dyh `81ec1ca` 在 main 后面 3 个提交，没有未合并新增提交。本地 Python `7caf4d9` 较旧，本轮使用与公开 commit 核对过的独立快照。
- 聊天/语音、记忆/关系、自主行为、事件、回执、端口配置、S0/S1 和 S2 loader/validator 已有实现。本次逐文件核对 39 份源码/文档与公开 Git blob 一致。
- 指标脚本未遍历 game_id 子目录、未知能力等级可能通过及 checksum 占位仍可由当前逻辑定位；前两项的前轮合成函数样例单独作为历史检查，本轮未重跑完整测试。
- UE 的 SendChatMessage 走玩家指令解析；follow/wait 仅在探索消费。服务能力示例声明五项，UE 战术组件默认只列 basic_attack/explosion；声明和资产存在需通过真实执行核验。
- UE 本地流程实现与公开 main 有差异，流程接入以用户实际 develop/资产版本为准。重试保留长期记忆/关系的规则与本局动作重置分别定义。

本轮未编译/运行 UE、启动真实 LLM/ASR、重新训练或跑完整 pytest；所有完成条件保持待执行。

## 当前任务总表

| 任务 | 阶段 | 目标仓库 / 责任 | Issue | 验收前置 | 汇总任务 |
| --- | --- | --- | --- | --- | --- |
| T18 | M5 | aesir-ai-service / 队友（Python NPC） | [T18 · 对齐 NPC 协议与联调配置](https://github.com/YYchainsAw/aesir-ai-service/issues/31) | 无 | [T22](https://github.com/YYchainsAw/aesir-ai-service/issues/35) |
| T19 | M5 | aesir-ai-service / 队友（Python NPC） | [T19 · 修复 NPC 指标漏采](https://github.com/YYchainsAw/aesir-ai-service/issues/32) | 无 | [T22](https://github.com/YYchainsAw/aesir-ai-service/issues/35) |
| T20 | M5 | aesir-ai-service / 队友（Python NPC） | [T20 · 验证 NPC 状态与时间边界](https://github.com/YYchainsAw/aesir-ai-service/issues/33) | 无 | [T22](https://github.com/YYchainsAw/aesir-ai-service/issues/35) |
| T21 | M5 | aesir-ai-service / 队友（Python NPC） | [T21 · 提供 NPC 联调与故障样例](https://github.com/YYchainsAw/aesir-ai-service/issues/34) | [T18](https://github.com/YYchainsAw/aesir-ai-service/issues/31) | [T22](https://github.com/YYchainsAw/aesir-ai-service/issues/35) |
| T22 | M5 | aesir-ai-service / 队友（Python NPC） | [T22 · 验收 NPC 服务与人格表现](https://github.com/YYchainsAw/aesir-ai-service/issues/35) | [T18](https://github.com/YYchainsAw/aesir-ai-service/issues/31)、[T19](https://github.com/YYchainsAw/aesir-ai-service/issues/32)、[T20](https://github.com/YYchainsAw/aesir-ai-service/issues/33)、[T21](https://github.com/YYchainsAw/aesir-ai-service/issues/34) | [T26](https://github.com/YYchainsAw/AesirWarden/issues/22) |
| T23 | M5 | AesirWarden / YYchainsAw（UE 接入） | [T23 · 接通队友聊天与探索指令](https://github.com/YYchainsAw/AesirWarden/issues/19) | [T18](https://github.com/YYchainsAw/aesir-ai-service/issues/31)、[T21](https://github.com/YYchainsAw/aesir-ai-service/issues/34) | [T26](https://github.com/YYchainsAw/AesirWarden/issues/22) |
| T24 | M5 | AesirWarden / YYchainsAw（UE 接入） | [T24 · 接通队友战斗与执行回执](https://github.com/YYchainsAw/AesirWarden/issues/20) | [T4](https://github.com/YYchainsAw/AesirWarden/issues/6)、[T18](https://github.com/YYchainsAw/aesir-ai-service/issues/31)、[T21](https://github.com/YYchainsAw/aesir-ai-service/issues/34) | [T26](https://github.com/YYchainsAw/AesirWarden/issues/22) |
| T25 | M5 | AesirWarden / YYchainsAw（UE 接入） | [T25 · 接通队友事件与流程重置](https://github.com/YYchainsAw/AesirWarden/issues/21) | [T7](https://github.com/YYchainsAw/AesirWarden/issues/7)、[T23](https://github.com/YYchainsAw/AesirWarden/issues/19)、[T24](https://github.com/YYchainsAw/AesirWarden/issues/20) | [T26](https://github.com/YYchainsAw/AesirWarden/issues/22) |
| T26 | M5 | AesirWarden / YYchainsAw（UE 接入） | [T26 · 验收队友聊天与战斗联调](https://github.com/YYchainsAw/AesirWarden/issues/22) | [T8](https://github.com/YYchainsAw/AesirWarden/issues/8)、[T22](https://github.com/YYchainsAw/aesir-ai-service/issues/35)、[T25](https://github.com/YYchainsAw/AesirWarden/issues/21) | — |
| T27 | N2 | aesir-ai-service / 队友（Python NPC） | [T27 · 完成人格包迁移与校验](https://github.com/YYchainsAw/aesir-ai-service/issues/36) | 无 | [T29](https://github.com/YYchainsAw/aesir-ai-service/issues/38) |
| T28 | N2 | aesir-ai-service / 队友（Python NPC） | [T28 · 发布游戏档案与协议 Schema](https://github.com/YYchainsAw/aesir-ai-service/issues/37) | [T27](https://github.com/YYchainsAw/aesir-ai-service/issues/36) | [T29](https://github.com/YYchainsAw/aesir-ai-service/issues/38) |
| T29 | N2 | aesir-ai-service / 队友（Python NPC） | [T29 · 接通 L0/L3 跨游戏服务](https://github.com/YYchainsAw/aesir-ai-service/issues/38) | [T27](https://github.com/YYchainsAw/aesir-ai-service/issues/36)、[T28](https://github.com/YYchainsAw/aesir-ai-service/issues/37) | — |
| T30 | N3 | aesir-ai-service / 队友（Python NPC） | [T30 · 完成外部人格包校验](https://github.com/YYchainsAw/aesir-ai-service/issues/39) | [T29](https://github.com/YYchainsAw/aesir-ai-service/issues/38) | — |

全部新任务为 Open，Project 初始状态为 Todo，按 M5/N2/N3 设置交付阶段。priority:later 沿用批准的 NPC 独立工作线规则；本批未自动填写 Assignee，用户在 GitHub 同时设置的负责人已保留并记录。完成条件均待实际执行，创建任务不表示功能已验收。

[统一 Project](09_GitHubProject.md) · [当前里程碑](10_GitHubMilestones.md) · [本批创建与核验记录](13_CurrentCompanionIssues.json) · [全部当前任务记录](12_CurrentIssues.json)。

## T18 · 对齐 NPC 协议与联调配置

[实际 Issue](https://github.com/YYchainsAw/aesir-ai-service/issues/31) · 标签：`documentation`、`area:contract`、`priority:later`、`area:companion`、`area:services`

<!-- aesir-npc-review:2026-10-07:T18 -->

**任务编号：** `T18`

**目标仓库：** `YYchainsAw/aesir-ai-service`

**里程碑：** [M5 · NPC 服务基线与 UE 聊天、战斗联调支持](https://github.com/YYchainsAw/aesir-ai-service/milestone/3)

**责任：** 队友（Python NPC）；YYchainsAw 提供 UE 能力与实际执行记录。

**汇总任务：** [T22 · 验收 NPC 服务与人格表现](https://github.com/YYchainsAw/aesir-ai-service/issues/35)。

### 已有基础与剩余工作

NPC 已有 v0.1/v0.2 兼容端点、v0.3 agent/world/console 和批量回执；端口外置已完成。独立 v0.3 契约及文档状态仍需同步。Python 默认 8000，UE 两个 Subsystem 默认 8011；服务能力示例有五项，UE 组件当前只声明 basic_attack/explosion，均需实际核验。

### 任务范围

- 固定当前两仓版本、单 NPC Alice、端口、角色/游戏 ID、聊天与指令分流，以及版本兼容边界；从现有端口配置继续联调。

- 完成独立 v0.3 契约、能力/表现 ID 对照表和验收矩阵；逐项区分已实现、启用、未制作及拒绝行为。同步 SDD、todo、开发日志中的旧状态。

### 完成条件

- [ ] 聊天、玩家指令、心跳、世界事件、单条/批量回执的字段、ID、时间、终态、错误及版本有一致样例；基础语音与文本进入约定链路。

- [ ] 明确一项真实可用的战术能力作为演示目标，以现有 basic_attack/explosion 为核查起点；治疗/护盾等未制作能力按实际状态禁用或拒绝，能力声明与 UE 实现一致。

- [ ] 两端指向同一 NPC 服务实例，记录实际端口、启动命令与 health 结果；NPC 和 Boss 8012 分开。响应时间/SC-011 的采样点、样本与通过口径先定义，历史测试数量按 commit 标注。

### 验收证据

独立契约、两仓能力对照表、配置/启动记录、接口样例与验收矩阵。

### 前置依赖

无最终验收前置，可直接准备和推进。

依赖约束最终验收，可以提前准备；遇到实际阻断时记录原因和对应任务。

### 范围边界

收口现有服务和当前 UE 能力；不重新开发已完成的端口、批量回执或多角色路由。不增加对话 RL、流式 ASR/VAD 或未确认的新技能。

### 来源与基线

- [aesir-ai-service · docs/planning/todo.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/todo.md)

- [aesir-ai-service · docs/planning/aesir-agent-sdd-v1.1.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/aesir-agent-sdd-v1.1.md)

- [aesir-ai-service · docs/protocols/ue-protocol-contract-v0.1.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/protocols/ue-protocol-contract-v0.1.md)

- [aesir-ai-service · app/config.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/config.py)

- [aesir-ai-service · data/games/aesir/capability.yaml](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/data/games/aesir/capability.yaml)

- [AesirWarden · Source/AesirWarden/Public/Services/Companion/CommandServiceSubsystem.h](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Public/Services/Companion/CommandServiceSubsystem.h)

- [AesirWarden · Source/AesirWarden/Private/AI/Companion/TacticalOrderComponent.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Companion/TacticalOrderComponent.cpp)

服务已有实现、静态核对、历史测试与本任务实际验收分别记录；执行时复核最新版本。本任务已按用户审查批准发布。


## T19 · 修复 NPC 指标漏采

[实际 Issue](https://github.com/YYchainsAw/aesir-ai-service/issues/32) · 标签：`bug`、`priority:later`、`area:companion`、`area:observability`

<!-- aesir-npc-review:2026-10-07:T19 -->

**任务编号：** `T19`

**目标仓库：** `YYchainsAw/aesir-ai-service`

**里程碑：** [M5 · NPC 服务基线与 UE 聊天、战斗联调支持](https://github.com/YYchainsAw/aesir-ai-service/milestone/3)

**责任：** 队友（Python NPC）；YYchainsAw 提供 UE 能力与实际执行记录。

**汇总任务：** [T22 · 验收 NPC 服务与人格表现](https://github.com/YYchainsAw/aesir-ai-service/issues/35)。

### 已有基础与剩余工作

S1 的回执和对话信号已按 game_id 分目录写入，metrics_report 仍只 glob 根目录 *.jsonl。前轮直接函数样例记录新布局各写入一条后统计均为 0；当前静态核对仍是同一逻辑。

### 任务范围

- 从实际存储路径调整采集，明确按游戏/角色筛选、跨游戏汇总与旧布局兼容口径。

- 补足旧布局、新布局、混合布局、空目录和损坏行的必要回归；给样例与报告对应索引。

### 完成条件

- [ ] 新布局的已知回执与对话记录都被采到；混合目录同一记录不重复统计，不跨游戏混入指定筛选结果。

- [ ] 回执分布、降级、重复文本和风格违规等数值可从原始 JSONL 重算；空样本的比例明确为无数据，损坏行有可查处理结果。

- [ ] 报告区分 rejected/failed 代理指标、风格检查次数和真实越界下发，指标名称不会把不同事件混为同一验收项。

### 验收证据

修复前后最小样例、回归结果、原始 JSONL 与机器可读报告。

### 前置依赖

无最终验收前置，可直接准备和推进。

依赖约束最终验收，可以提前准备；遇到实际阻断时记录原因和对应任务。

### 范围边界

沿用 JSONL 和现有报告脚本；不新增数据库或监控平台。前轮样例仅证明采集函数缺口，不当作真实 UE 运行结果。

### 来源与基线

- [aesir-ai-service · scripts/metrics_report.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/scripts/metrics_report.py)

- [aesir-ai-service · app/services/tactical/receipt_store.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/services/tactical/receipt_store.py)

- [aesir-ai-service · app/services/companion/dialogue_signals.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/services/companion/dialogue_signals.py)

- [aesir-ai-service · docs/planning/todo.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/todo.md)

服务已有实现、静态核对、历史测试与本任务实际验收分别记录；执行时复核最新版本。本任务已按用户审查批准发布。


## T20 · 验证 NPC 状态与时间边界

[实际 Issue](https://github.com/YYchainsAw/aesir-ai-service/issues/33) · 标签：`type:test`、`priority:later`、`area:companion`、`area:memory`、`area:agency`

<!-- aesir-npc-review:2026-10-07:T20 -->

**任务编号：** `T20`

**目标仓库：** `YYchainsAw/aesir-ai-service`

**里程碑：** [M5 · NPC 服务基线与 UE 聊天、战斗联调支持](https://github.com/YYchainsAw/aesir-ai-service/milestone/3)

**责任：** 队友（Python NPC）；YYchainsAw 提供 UE 能力与实际执行记录。

**汇总任务：** [T22 · 验收 NPC 服务与人格表现](https://github.com/YYchainsAw/aesir-ai-service/issues/35)。

### 已有基础与剩余工作

记忆/关系存储、五场景、自主行为仲裁、幂等和节流已有实现。todo 的 CODE-06/08 是待核验风险：resolve_scene 当前直接使用 ctx.scene，关系冷却使用最多 20 条近期事件，节流配置在单例创建时读取。

### 任务范围

- 按当前规则验证场景切换、时间前进/回退、高频事件、重复事件、重启、显式重置及游戏/角色隔离。

- 用超过近期事件上限的样例核对冷却；定义配置修改的生效方式，复现缺口后修复并回写规则，未复现项记录证据。

### 完成条件

- [ ] 重复 world event 不重复计分/重复反应；高频事件挤出历史、时间跳变与跨角色/游戏样例都有预期结果和实际记录。

- [ ] 场景与禁打断/节流走向一致；策略修改后按已声明的重启或重载方式生效，不把未实现的热更新作为已支持能力。

- [ ] 记忆/关系的重启保留与显式清空符合约定；发现的问题已有修复回归或与验收范围一致的解决记录，CODE/EXT 对应条目同步。

### 验收证据

可复现输入、时间/事件序列、必要回归、状态前后对照与规则修订记录。

### 前置依赖

无最终验收前置，可直接准备和推进。

依赖约束最终验收，可以提前准备；遇到实际阻断时记录原因和对应任务。

### 范围边界

先验证当前边界；不预判场景分叉已发生，不引入完整存档系统或配置热更新平台。

### 来源与基线

- [aesir-ai-service · docs/planning/todo.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/todo.md)

- [aesir-ai-service · app/services/relationship/rules.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/services/relationship/rules.py)

- [aesir-ai-service · app/services/agency/throttle.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/services/agency/throttle.py)

- [aesir-ai-service · tests/services/test_relationship_state.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/tests/services/test_relationship_state.py)

- [aesir-ai-service · tests/services/test_memory_persistence.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/tests/services/test_memory_persistence.py)

服务已有实现、静态核对、历史测试与本任务实际验收分别记录；执行时复核最新版本。本任务已按用户审查批准发布。


## T21 · 提供 NPC 联调与故障样例

[实际 Issue](https://github.com/YYchainsAw/aesir-ai-service/issues/34) · 标签：`type:test`、`area:contract`、`priority:later`、`area:companion`、`area:observability`

<!-- aesir-npc-review:2026-10-07:T21 -->

**任务编号：** `T21`

**目标仓库：** `YYchainsAw/aesir-ai-service`

**里程碑：** [M5 · NPC 服务基线与 UE 聊天、战斗联调支持](https://github.com/YYchainsAw/aesir-ai-service/milestone/3)

**责任：** 队友（Python NPC）；YYchainsAw 提供 UE 能力与实际执行记录。

**汇总任务：** [T22 · 验收 NPC 服务与人格表现](https://github.com/YYchainsAw/aesir-ai-service/issues/35)。

### 已有基础与剩余工作

Python HTTP、指令信封、世界事件、单条/批量回执和相关 API 测试已有基础；UE 仍需要与当前协议和实际能力清单匹配的可重放样例。

### 任务范围

- 交付聊天、follow/wait、已约定战术能力、条件等待/取消、心跳/事件和执行回执的请求响应样例。

- 为断线、超时、错误 JSON/版本/ID、重复/乱序、目标消失和能力不可用提供受控故障回放方法与预期。

### 完成条件

- [ ] 样例带 game_id/companion_id、request/directive/order/event ID 及预期终态，能够沿约定链路追踪；单条与批量回执各有可运行输入。

- [ ] 正常和故障样例能用明确命令复现，Mock、真实服务、真实 LLM/ASR 分别标识；测试环境不覆盖真实用户运行数据。

- [ ] UE 接入者可按说明验证拒绝/降级及过期回包；对未启用能力有明确结果，协议与能力版本写入样例索引。

### 验收证据

样例包、回放命令、预期/实际对照、日志与版本清单。

### 前置依赖

- [T18 · 对齐 NPC 协议与联调配置](https://github.com/YYchainsAw/aesir-ai-service/issues/31)

依赖约束最终验收，可以提前准备；遇到实际阻断时记录原因和对应任务。

### 范围边界

服务样例负责接入准备；游戏中是否执行成功由 UE 任务提供实际证据。

### 来源与基线

- [aesir-ai-service · app/api/v1/agent.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/api/v1/agent.py)

- [aesir-ai-service · app/api/v1/world.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/api/v1/world.py)

- [aesir-ai-service · app/api/v1/tactical.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/api/v1/tactical.py)

- [aesir-ai-service · app/schemas/tactical_execution.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/schemas/tactical_execution.py)

- [aesir-ai-service · tests/api/test_agent_step.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/tests/api/test_agent_step.py)

服务已有实现、静态核对、历史测试与本任务实际验收分别记录；执行时复核最新版本。本任务已按用户审查批准发布。


## T22 · 验收 NPC 服务与人格表现

[实际 Issue](https://github.com/YYchainsAw/aesir-ai-service/issues/35) · 标签：`type:test`、`priority:later`、`area:companion`、`area:persona`、`area:memory`、`area:observability`

<!-- aesir-npc-review:2026-10-07:T22 -->

**任务编号：** `T22`

**目标仓库：** `YYchainsAw/aesir-ai-service`

**里程碑：** [M5 · NPC 服务基线与 UE 聊天、战斗联调支持](https://github.com/YYchainsAw/aesir-ai-service/milestone/3)

**责任：** 队友（Python NPC）；YYchainsAw 提供 UE 能力与实际执行记录。

**汇总任务：** [T26 · 验收队友聊天与战斗联调](https://github.com/YYchainsAw/AesirWarden/issues/22)。

### 已有基础与剩余工作

US1–US8 的实现和多轮实测复盘已有记录。SDD、CHANGELOG 和开发日志的测试数量对应不同时间/版本，不能合并为本轮新验证；修复后需形成当前服务交付证据。

### 任务范围

- 按 T18 的矩阵验收人格风格、记忆回读、关系、事件、自主行为、隔离、降级与可观测结果。

- 固定实际 LLM/ASR 后端、模型/参数、依赖、代码和人格版本；运行代表性多轮对话与基础语音，归档误识别、重复、拒绝和失败样例。

### 完成条件

- [ ] 当前 commit 的回归命令、通过/失败/跳过数量、资源条件和原始结果可查；历史测试数字注明来源，剩余未测边界明确。

- [ ] 真实 LLM 多轮对话、记忆保留/重置、关系区分和基础 ASR 链路有记录；通过标准使用 T18 定义的口径，环境缺失明确作为待验证/阻断，Mock 证据单列。

- [ ] 指标由修复后的采集脚本对原始记录生成；配置调优与 style_guard 样例更新有前后证据。服务交付支持 UE 联调，服务测试不被记成 UE 执行通过。

### 验收证据

服务验收报告、原始会话/信号/回执、测试结果、LLM/ASR 条件、失败样例与复现索引。

### 前置依赖

- [T18 · 对齐 NPC 协议与联调配置](https://github.com/YYchainsAw/aesir-ai-service/issues/31)

- [T19 · 修复 NPC 指标漏采](https://github.com/YYchainsAw/aesir-ai-service/issues/32)

- [T20 · 验证 NPC 状态与时间边界](https://github.com/YYchainsAw/aesir-ai-service/issues/33)

- [T21 · 提供 NPC 联调与故障样例](https://github.com/YYchainsAw/aesir-ai-service/issues/34)

依赖约束最终验收，可以提前准备；遇到实际阻断时记录原因和对应任务。

### 范围边界

完成 Python M5 仓库的服务交付；M5 总体完成还需 T26 的真实 UE 联调验收。

### 来源与基线

- [aesir-ai-service · docs/planning/aesir-agent-sdd-v1.1.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/aesir-agent-sdd-v1.1.md)

- [aesir-ai-service · docs/logs/2026-09-30.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/logs/2026-09-30.md)

- [aesir-ai-service · docs/guides/getting-started.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/guides/getting-started.md)

- [aesir-ai-service · scripts/metrics_report.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/scripts/metrics_report.py)

服务已有实现、静态核对、历史测试与本任务实际验收分别记录；执行时复核最新版本。本任务已按用户审查批准发布。


## T23 · 接通队友聊天与探索指令

[实际 Issue](https://github.com/YYchainsAw/AesirWarden/issues/19) · 标签：`enhancement`、`priority:later`、`area:companion`、`area:services`、`area:ui`

<!-- aesir-npc-review:2026-10-07:T23 -->

**任务编号：** `T23`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** [M5 · AI 队友聊天与战斗联调](https://github.com/YYchainsAw/AesirWarden/milestone/4)

**责任：** YYchainsAw（UE 接入）；队友提供 Python 协议、样例与服务记录。

**汇总任务：** [T26 · 验收队友聊天与战斗联调](https://github.com/YYchainsAw/AesirWarden/issues/22)。

### 已有基础与剩余工作

已有聊天 UI/HTTP、3 秒心跳和 follow/wait 控制器路径。SendChatMessage 当前把全部输入发给 agent/step 的指令解析路径；普通人格对话需与玩家命令分流。follow/wait 只在探索且未开战时处理。

### 任务范围

- 接通聊天与命令入口的分流：人格对话使用现有聊天服务，玩家命令/心跳使用约定 agent 链路；补齐当前 UI、角色 ID、会话与地址配置。

- 在实际场景验证聊天、UI 焦点、follow/wait 和现有基础语音入口；保留单 NPC Alice，记录异常和当前已启用能力。

### 完成条件

- [ ] 普通聊天进入记忆/关系对话链，follow/wait 进入命令链；连续输入、ID 匹配、错误提示和 UI 开关正确，不把普通聊天全部当不可识别指令。

- [ ] 探索下的跟随/等待实际生效，开始战斗时按明确规则交接；当前控制器、BT、角色和语音路径有实际接线/运行记录。

- [ ] 聊天 UI 焦点正确，关闭后恢复移动/攻击输入；基础语音按已有 WAV/ASR 接口走正确链路，断线/超时显示可理解的结果。

### 验收证据

PIE/实际 UE 录屏、UI/BT 配置、请求响应与角色/会话版本记录。

### 前置依赖

- [T18 · 对齐 NPC 协议与联调配置](https://github.com/YYchainsAw/aesir-ai-service/issues/31)

- [T21 · 提供 NPC 联调与故障样例](https://github.com/YYchainsAw/aesir-ai-service/issues/34)

依赖约束最终验收，可以提前准备；遇到实际阻断时记录原因和对应任务。

### 范围边界

复用已有 UI、聊天、命令和语音接口；流式 SSE 展示、ASR/VAD 升级及正式演出另行安排。

### 来源与基线

- [AesirWarden · Source/AesirWarden/Private/Services/Companion/CompanionChatSubsystem.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/Services/Companion/CompanionChatSubsystem.cpp)

- [AesirWarden · Source/AesirWarden/Private/AI/Companion/AesirCompanionAIController.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Companion/AesirCompanionAIController.cpp)

- [AesirWarden · Source/AesirWarden/Public/Services/Companion/CommandServiceSubsystem.h](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Public/Services/Companion/CommandServiceSubsystem.h)

- [aesir-ai-service · docs/protocols/ue-protocol-contract-v0.1.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/protocols/ue-protocol-contract-v0.1.md)

服务已有实现、静态核对、历史测试与本任务实际验收分别记录；执行时复核最新版本。本任务已按用户审查批准发布。


## T24 · 接通队友战斗与执行回执

[实际 Issue](https://github.com/YYchainsAw/AesirWarden/issues/20) · 标签：`enhancement`、`priority:later`、`area:combat`、`area:companion`、`area:services`

<!-- aesir-npc-review:2026-10-07:T24 -->

**任务编号：** `T24`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** [M5 · AI 队友聊天与战斗联调](https://github.com/YYchainsAw/AesirWarden/milestone/4)

**责任：** YYchainsAw（UE 接入）；队友提供 Python 协议、样例与服务记录。

**汇总任务：** [T26 · 验收队友聊天与战斗联调](https://github.com/YYchainsAw/AesirWarden/issues/22)。

### 已有基础与剩余工作

TacticalOrderComponent 已提供接收、触发校验、完成/取消和遭遇状态；声明的能力 ID 不证明 BP/技能已经执行。当前控制器并不消费一般战斗指令，两个同伴 Subsystem 未发现执行回执上传路径。

### 任务范围

- 按 T18 清单接通有效目标选择、基础攻击、约定战术能力的 BT/技能执行，以及玩家命令/心跳指令进入同一校验与执行路径。

- 接通请求/指令/订单到真实动作终态的单条/批量回执；记录受击、冷却、死亡、取消、替换、过期和目标丢失。

### 完成条件

- [ ] 同伴在实际 Boss 场景能选取有效目标、产生真实基础攻击伤害，并执行 T18 固定的战术能力；实际动画/伤害/效果与能力 ID 对应。

- [ ] 触发时重新校验目标、资源、距离、冷却与遭遇状态；未制作能力明确拒绝，战斗指令不会静默沿探索 follow/wait 路径丢弃。

- [ ] 接收与完成分开记录，成功/失败/拒绝/取消回执符合协议；请求、决策、技能效果和落盘回执能由 ID 串联，重复提交不重复执行或结算。

### 验收证据

真实技能/BT 接线、战斗录屏、请求响应、伤害/效果记录和服务落盘回执。

### 前置依赖

- [T4 · 接通 Boss 入口与 BT 开战](https://github.com/YYchainsAw/AesirWarden/issues/6)

- [T18 · 对齐 NPC 协议与联调配置](https://github.com/YYchainsAw/aesir-ai-service/issues/31)

- [T21 · 提供 NPC 联调与故障样例](https://github.com/YYchainsAw/aesir-ai-service/issues/34)

依赖约束最终验收，可以提前准备；遇到实际阻断时记录原因和对应任务。

### 范围边界

补齐本轮明确启用的能力与执行链；不要求一次制作服务示例中的全部治疗/护盾技能，也不预设必须迁移某个执行系统。

### 来源与基线

- [AesirWarden · Source/AesirWarden/Private/AI/Companion/TacticalOrderComponent.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Companion/TacticalOrderComponent.cpp)

- [AesirWarden · Source/AesirWarden/Private/AI/Companion/AesirCompanionAIController.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Companion/AesirCompanionAIController.cpp)

- [AesirWarden · Source/AesirWarden/Private/Services/Companion/CommandServiceSubsystem.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/Services/Companion/CommandServiceSubsystem.cpp)

- [aesir-ai-service · app/api/v1/tactical.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/api/v1/tactical.py)

- [aesir-ai-service · app/schemas/tactical_execution.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/schemas/tactical_execution.py)

服务已有实现、静态核对、历史测试与本任务实际验收分别记录；执行时复核最新版本。本任务已按用户审查批准发布。


## T25 · 接通队友事件与流程重置

[实际 Issue](https://github.com/YYchainsAw/AesirWarden/issues/21) · 标签：`enhancement`、`priority:later`、`area:companion`、`area:services`、`area:level-flow`

<!-- aesir-npc-review:2026-10-07:T25 -->

**任务编号：** `T25`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** [M5 · AI 队友聊天与战斗联调](https://github.com/YYchainsAw/AesirWarden/milestone/4)

**责任：** YYchainsAw（UE 接入）；队友提供 Python 协议、样例与服务记录。

**汇总任务：** [T26 · 验收队友聊天与战斗联调](https://github.com/YYchainsAw/AesirWarden/issues/22)。

### 已有基础与剩余工作

GameInstance 同伴子系统已有持续心跳和请求序号，控制器存在绑定/解绑；NPC 的世界事件、遭遇结束、重试与返回还需与 M1 流程共同验收。长期记忆/关系和本局动作属于不同生命周期。

### 任务范围

- 接通约定的世界/战斗事件、快照和已启用自主行为，保存事件 ID、角色/游戏 ID 与可追踪反应。

- 对齐探索、开战、胜败、死亡、重试和返回菜单的心跳、队列、异步请求、执行回执、UI 和角色状态；明确长期状态保留规则。

### 完成条件

- [ ] 事件仅对约定边沿上报，重复事件不重复反应/计分；自主行为按启用清单执行并反馈终态，未实现行为明确拒绝。

- [ ] 胜败与返回后停用旧遭遇动作；连续两次重试恢复角色/输入/BT 状态，旧响应、目标、委托和命令不会作用到新局，取消有对应回执。

- [ ] 战斗重试按约定保留长期人格记忆/关系，仅重置本局状态；显式清空沿用已有服务接口，不在普通重试时误删长期数据。

### 验收证据

事件/回执轨迹、连续重试和返回录屏、异步旧回包样例、状态保留/重置对照。

### 前置依赖

- [T7 · 接通重试与返回菜单](https://github.com/YYchainsAw/AesirWarden/issues/7)

- [T23 · 接通队友聊天与探索指令](https://github.com/YYchainsAw/AesirWarden/issues/19)

- [T24 · 接通队友战斗与执行回执](https://github.com/YYchainsAw/AesirWarden/issues/20)

依赖约束最终验收，可以提前准备；遇到实际阻断时记录原因和对应任务。

### 范围边界

本任务接入已有 M1 流程；不重复实现 M1 结算/重试，不增加持久存档系统。当前本地流程实现与公开主分支的版本差异单独记录。

### 来源与基线

- [AesirWarden · Source/AesirWarden/Private/Services/Companion/CompanionChatSubsystem.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/Services/Companion/CompanionChatSubsystem.cpp)

- [AesirWarden · Source/AesirWarden/Private/AI/Companion/AesirCompanionAIController.cpp](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Source/AesirWarden/Private/AI/Companion/AesirCompanionAIController.cpp)

- [aesir-ai-service · app/api/v1/world.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/api/v1/world.py)

- [aesir-ai-service · app/api/v1/console.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/api/v1/console.py)

服务已有实现、静态核对、历史测试与本任务实际验收分别记录；执行时复核最新版本。本任务已按用户审查批准发布。


## T26 · 验收队友聊天与战斗联调

[实际 Issue](https://github.com/YYchainsAw/AesirWarden/issues/22) · 标签：`priority:later`、`area:combat`、`area:companion`、`area:services`、`area:ui`、`type:test`

<!-- aesir-npc-review:2026-10-07:T26 -->

**任务编号：** `T26`

**目标仓库：** `YYchainsAw/AesirWarden`

**里程碑：** [M5 · AI 队友聊天与战斗联调](https://github.com/YYchainsAw/AesirWarden/milestone/4)

**责任：** YYchainsAw（UE 接入）；队友提供 Python 协议、样例与服务记录。

### 已有基础与剩余工作

M5 的整体目标需要当前 UE 中的实际聊天与战斗、正确回执及全流程状态；服务 Mock、命令 JSON 或资产清单只能作为准备证据。

### 任务范围

- 在 T8 的完整 BT 流程基础上，用 T22 的真实服务版本验收聊天/基础语音、探索、约定战斗能力、事件和回执。

- 覆盖断线/超时、无效能力、取消、胜败、连续重试与返回；记录游戏执行和服务状态的同一次证据，并汇总跨仓阶段验收。

### 完成条件

- [ ] 当前 UE 中实际完成聊天、follow/wait、基础攻击与约定战术能力；关联同一次请求、决策、动作效果及完成/失败回执。

- [ ] 实际胜败、至少连续两次重试和返回后角色、输入、UI、请求、回执正确；故障有可见提示和约定降级，不以已接受订单代替执行完成。

- [ ] 两仓 commit、资产快照、服务/模型配置、协议/人格版本、录屏与日志齐备；Python 仓库先验收自身服务，UE 汇总 M5 总体运行证据。

### 验收证据

真实 UE 完整演示、故障回归表、双仓版本与完整证据索引。

### 前置依赖

- [T8 · 验收 BT Windows 包](https://github.com/YYchainsAw/AesirWarden/issues/8)

- [T22 · 验收 NPC 服务与人格表现](https://github.com/YYchainsAw/aesir-ai-service/issues/35)

- [T25 · 接通队友事件与流程重置](https://github.com/YYchainsAw/AesirWarden/issues/21)

依赖约束最终验收，可以提前准备；遇到实际阻断时记录原因和对应任务。

### 范围边界

本阶段先按 BT 基线验收 NPC；不阻塞 M1/M2，也不等待 RL Boss 与 NPC 同场综合演示。综合 Windows 交付归 M6，M3 的单玩家对照条件另行冻结。

### 来源与基线

- [AesirWarden · Docs/06_Test_Doc/VerificationMatrix.md](https://github.com/YYchainsAw/AesirWarden/blob/facba39a3daef0e7904902c92497f84dccaa1654/Docs/06_Test_Doc/VerificationMatrix.md)

- [aesir-ai-service · docs/planning/aesir-agent-sdd-v1.1.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/aesir-agent-sdd-v1.1.md)

- [aesir-ai-service · docs/protocols/combat-tactical-protocol-v0.2.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/protocols/combat-tactical-protocol-v0.2.md)

服务已有实现、静态核对、历史测试与本任务实际验收分别记录；执行时复核最新版本。本任务已按用户审查批准发布。


## T27 · 完成人格包迁移与校验

[实际 Issue](https://github.com/YYchainsAw/aesir-ai-service/issues/36) · 标签：`enhancement`、`area:contract`、`priority:later`、`area:companion`、`area:persona`

<!-- aesir-npc-review:2026-10-07:T27 -->

**任务编号：** `T27`

**目标仓库：** `YYchainsAw/aesir-ai-service`

**里程碑：** [N2 · S2–S4 人格包与跨游戏最小接入](https://github.com/YYchainsAw/aesir-ai-service/milestone/4)

**责任：** 队友（Python NPC）；YYchainsAw 提供 UE 能力与实际执行记录。

**汇总任务：** [T29 · 接通 L0/L3 跨游戏服务](https://github.com/YYchainsAw/aesir-ai-service/issues/38)。

### 已有基础与剩余工作

S2 loader/validator 与测试已经存在，注册表兼容旧单 YAML。公开树未包含 Alice/Bruno 的 data/personas 包；未知能力等级被映射为 -1，校验器仅比较等级大小，checksum 仍是 S5 占位。

### 任务范围

- 沿用已有 loader/validator 迁移 Alice 与 Bruno，明确目录包/旧 YAML 共存期的去重、唯一性、优先级和回退行为。

- 明确拒绝未知 capability 等级、归属/ID/场景/事件/技能冲突、缺文件和非法 YAML；将当前冲突清单与最小包样例补齐。

### 完成条件

- [ ] Alice/Bruno 的包可从仓库或明确版本化位置恢复并加载；人物标识与旧路由兼容，迁移前后人格/记忆引用关系有回归。

- [ ] 未知等级、错误 game_id、重复角色、越界行为/表现/能力 ID、缺文件和损坏包被明确拒绝，冲突不会静默选第一个来源。

- [ ] 合法包、兼容旧配置与非法包有必要回归，S2 的完成状态与日志同步；Bruno 的人格/路由隔离在服务侧验证。

### 验收证据

版本化人格包、迁移说明、冲突报告、合法/非法及兼容回归结果。

### 前置依赖

无最终验收前置，可直接准备和推进。

依赖约束最终验收，可以提前准备；遇到实际阻断时记录原因和对应任务。

### 范围边界

从已实现 S2 继续；外部包 checksum/审核在 T30，UE 双 NPC 同屏不作为本项验收。

### 来源与基线

- [aesir-ai-service · app/services/companion/persona_pack_loader.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/services/companion/persona_pack_loader.py)

- [aesir-ai-service · app/services/companion/persona_pack_validator.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/services/companion/persona_pack_validator.py)

- [aesir-ai-service · app/services/companion/profile_repository.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/services/companion/profile_repository.py)

- [aesir-ai-service · tests/services/test_persona_pack_validator.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/tests/services/test_persona_pack_validator.py)

- [aesir-ai-service · tests/services/test_persona_pack_loader.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/tests/services/test_persona_pack_loader.py)

- [aesir-ai-service · docs/planning/game-design-doc-v0.2.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/game-design-doc-v0.2.md)

服务已有实现、静态核对、历史测试与本任务实际验收分别记录；执行时复核最新版本。本任务已按用户审查批准发布。


## T28 · 发布游戏档案与协议 Schema

[实际 Issue](https://github.com/YYchainsAw/aesir-ai-service/issues/37) · 标签：`enhancement`、`area:contract`、`priority:later`、`area:companion`、`area:persona`

<!-- aesir-npc-review:2026-10-07:T28 -->

**任务编号：** `T28`

**目标仓库：** `YYchainsAw/aesir-ai-service`

**里程碑：** [N2 · S2–S4 人格包与跨游戏最小接入](https://github.com/YYchainsAw/aesir-ai-service/milestone/4)

**责任：** 队友（Python NPC）；YYchainsAw 提供 UE 能力与实际执行记录。

**汇总任务：** [T29 · 接通 L0/L3 跨游戏服务](https://github.com/YYchainsAw/aesir-ai-service/issues/38)。

### 已有基础与剩余工作

已有 Aesir capability 示例，但 profile_repository 固定使用 Aesir 档案路径并扫描各游戏包；canonical schema、完整游戏档案与一进程一游戏的选择/限制仍需完成。

### 任务范围

- 发布版本化 canonical schema、capability/directives 档案及必要样例；将当前固定枚举与每游戏子集校验的约定落实。

- 实现配置化选择当前游戏、加载对应档案/人格、限制注册范围，配套独立端口的单游戏实例启动说明。

### 完成条件

- [ ] 游戏档案、协议/schema 与人格/指令引用能对应到固定版本，未声明的场景/事件/动作/表现 ID 明确拒绝，破坏性变化升级版本。

- [ ] 当前进程仅加载配置游戏，档案来源不再固定为 Aesir；跨游戏角色错误不会回退到默认角色或混用另一游戏能力。

- [ ] Aesir 和自有第二游戏档案有可复现配置与兼容样例，旧接口兼容或迁移路径明确；声明的技能不被自动记作已经实现。

### 验收证据

canonical schema、游戏档案、版本化样例、启动/迁移说明与校验回归。

### 前置依赖

- [T27 · 完成人格包迁移与校验](https://github.com/YYchainsAw/aesir-ai-service/issues/36)

依赖约束最终验收，可以提前准备；遇到实际阻断时记录原因和对应任务。

### 范围边界

沿用一进程一游戏、固定枚举配子集校验；不引入同进程多游戏热插拔架构。

### 来源与基线

- [aesir-ai-service · data/games/aesir/capability.yaml](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/data/games/aesir/capability.yaml)

- [aesir-ai-service · app/services/companion/profile_repository.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/services/companion/profile_repository.py)

- [aesir-ai-service · app/config.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/config.py)

- [aesir-ai-service · docs/planning/game-design-doc-v0.2.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/game-design-doc-v0.2.md)

服务已有实现、静态核对、历史测试与本任务实际验收分别记录；执行时复核最新版本。本任务已按用户审查批准发布。


## T29 · 接通 L0/L3 跨游戏服务

[实际 Issue](https://github.com/YYchainsAw/aesir-ai-service/issues/38) · 标签：`enhancement`、`priority:later`、`area:companion`、`area:persona`、`area:agency`、`area:services`

<!-- aesir-npc-review:2026-10-07:T29 -->

**任务编号：** `T29`

**目标仓库：** `YYchainsAw/aesir-ai-service`

**里程碑：** [N2 · S2–S4 人格包与跨游戏最小接入](https://github.com/YYchainsAw/aesir-ai-service/milestone/4)

**责任：** 队友（Python NPC）；YYchainsAw 提供 UE 能力与实际执行记录。

### 已有基础与剩余工作

S4 目标已经在策划中确认：L0 只对话/记忆/关系，L3 启用约定完整链路；当前有能力等级数据与校验子集，尚需真实路由门控和自有跨游戏最小接入验证。

### 任务范围

- 根据 T28 档案落实 L0/L3 门控与显式降级，把可启用功能、缺能力时的响应和原因写入约定。

- 用 Aesir 与自有 demo-vn/narrator 等第二游戏样例分别启动，证明配置接入、包加载、数据与端口隔离。

### 完成条件

- [ ] L0 的聊天/记忆/关系在缺完整世界快照时可用，不产生战斗/自主行为/事件链路越界；L3 按当前档案子集启用，未知等级拒绝。

- [ ] 自有第二游戏可按版本化配置接入并产生真实服务记录；状态、人格、知识、信号和回执按游戏/角色隔离，一游戏一实例可复现。

- [ ] 缺能力、坏档案和错误游戏/角色 ID 的拒绝/降级有原因及回归；Aesir 的已有聊天/战术服务保留可用基线。

### 验收证据

两个自有游戏的配置/启动命令、正常与降级日志、隔离样例与回归报告。

### 前置依赖

- [T27 · 完成人格包迁移与校验](https://github.com/YYchainsAw/aesir-ai-service/issues/36)

- [T28 · 发布游戏档案与协议 Schema](https://github.com/YYchainsAw/aesir-ai-service/issues/37)

依赖约束最终验收，可以提前准备；遇到实际阻断时记录原因和对应任务。

### 范围边界

仅自有包和服务侧最小接入；外部包接入等 T30，完整第二款游戏或 UE 双 NPC 演出另行安排。

### 来源与基线

- [aesir-ai-service · docs/planning/game-design-doc-v0.2.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/game-design-doc-v0.2.md)

- [aesir-ai-service · data/games/aesir/capability.yaml](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/data/games/aesir/capability.yaml)

- [aesir-ai-service · app/api/v1/agent.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/api/v1/agent.py)

- [aesir-ai-service · app/services/companion/profile_repository.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/services/companion/profile_repository.py)

服务已有实现、静态核对、历史测试与本任务实际验收分别记录；执行时复核最新版本。本任务已按用户审查批准发布。


## T30 · 完成外部人格包校验

[实际 Issue](https://github.com/YYchainsAw/aesir-ai-service/issues/39) · 标签：`enhancement`、`area:contract`、`priority:later`、`area:companion`、`area:persona`

<!-- aesir-npc-review:2026-10-07:T30 -->

**任务编号：** `T30`

**目标仓库：** `YYchainsAw/aesir-ai-service`

**里程碑：** [N3 · S5 外部人格包守卫与接入门禁](https://github.com/YYchainsAw/aesir-ai-service/milestone/5)

**责任：** 队友（Python NPC）；YYchainsAw 提供 UE 能力与实际执行记录。

### 已有基础与剩余工作

S5 已在策划中安排为外部包接入前条件。当前 validator 对 checksum 仅检查字符串类型，没有真实内容哈希验证；S2 的归属校验是可继续复用的基础。

### 任务范围

- 在自有包基线上补外部包的 schema、归属、版本/路径/配额检查、异常/注入样例 lint 与完整冲突报告。

- 定义校验文件范围和哈希生成/比对规则，并在加载路径执行；保存包版本、哈希、报告与实际审核结果。

### 完成条件

- [ ] 外部包 checksum 对实际内容计算并比对，篡改/损坏/缺校验信息明确拒绝；校验结果可由命令重算，而非只检查字符串存在。

- [ ] 非法结构、越界引用、未知能力、归属/版本冲突和约定异常样例产生可查 lint 报告，失败包不会进入正常加载。

- [ ] 外部包加载具有对应版本和哈希的实际审核记录；正常自有包、未通过/篡改外部包和降级路径回归，保留 T29 的可复现基线。

### 验收证据

lint/校验命令、哈希与冲突报告、审核记录、正常/异常包回归。

### 前置依赖

- [T29 · 接通 L0/L3 跨游戏服务](https://github.com/YYchainsAw/aesir-ai-service/issues/38)

依赖约束最终验收，可以提前准备；遇到实际阻断时记录原因和对应任务。

### 范围边界

本项按既有 S5 目标设计，外部包接入在通过后进行；不新增公网平台或人格自演化。

### 来源与基线

- [aesir-ai-service · docs/planning/game-design-doc-v0.2.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/game-design-doc-v0.2.md)

- [aesir-ai-service · app/services/companion/persona_pack_validator.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/services/companion/persona_pack_validator.py)

- [aesir-ai-service · app/services/companion/persona_pack_loader.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/services/companion/persona_pack_loader.py)

服务已有实现、静态核对、历史测试与本任务实际验收分别记录；执行时复核最新版本。本任务已按用户审查批准发布。
