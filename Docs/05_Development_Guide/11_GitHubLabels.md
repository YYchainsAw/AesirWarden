# GitHub 标签约定

适用仓库：[`YYchainsAw/AesirWarden`](https://github.com/YYchainsAw/AesirWarden)。设计日期：2026-10-04。

标签依据当前源码、已跟踪资产及[目标与范围](../01_Project_Overview/02_GoalsAndScope.md)、[架构](../02_Design_Doc/TDD/01_Architecture.md)、[开发工作流](04_Workflow.md)、[已知问题](../06_Test_Doc/02_KnownIssues.md)、[验证矩阵](../06_Test_Doc/01_VerificationMatrix.md)和[研究计划](../08_RL_Research/README.md)设计。

原创建记录包含 19 个项目标签，并保留已有 10 个默认标签。项目标签的名称、颜色和描述完整保存在下方[标签完整字段](#标签完整字段)；单独的 JSON 快照已按用户确认删除。

## 优先级：通常选一个

| 标签 | 使用范围 | 颜色 |
| --- | --- | --- |
| `priority:p0` | 当前验收必需的单 Boss 闭环、BT/PPO 对照、结果与重试、Windows 稳定演示 | 红色 `b60205` |
| `priority:p1` | P0 稳定后的营火菜单、运镜、引导、入场/收尾动画与表现打磨 | 黄色 `fbca04` |
| `priority:later` | 普通敌人、MARL、同伴真实 GAS 指令闭环、符文装备、持久存档等后续范围 | 浅绿 `c2e0c6` |

优先级跟随当前范围和验收目标判断。例如菜单通常为 P1，导致 P0 完整演示无法启动的菜单缺陷仍可为 P0。

## 模块：按实际影响选择一个或多个

模块标签统一采用蓝色 `1d76db`，用于按代码或资产责任范围筛选。

| 标签 | 模块边界与主要依据 |
| --- | --- |
| `area:combat` | 战斗、锁定、生命、防御与受击；`Source/AesirWarden/Public/Combat` |
| `area:gas` | 能力、属性、效果、伤害结算与标签；`Public/Abilities`、`Public/Tags`、`Content/Aesir/Abilities` |
| `area:player` | 玩家角色、移动、输入与相机；`Public/Characters/Player`、`Content/Aesir/Input` |
| `area:boss-ai` | UE BT 基线、共享 Action 执行器、动作合法性与回退；`Public/AI/Boss`、`Content/Aesir/AI/Boss` |
| `area:boss-rl` | Boss Observation/Action/Reward、策略接入、Telemetry 与评估；`Public/AI/Boss`、`Docs/08_RL_Research` |
| `area:companion` | 同伴角色、BT、订单及指令集成；`Public/AI/Companion`、`Public/Characters/Companion` |
| `area:services` | UE 侧 HTTP/JSON、跨仓库协议、超时及响应校验；`Public/Services/Companion`、`AesirBossPolicyClientComponent` |
| `area:animation` | AnimBP、Blend Space、Montage、Root Motion 与通知；`Content/Aesir/Animations` |
| `area:ui` | HUD、血条、菜单、结果与聊天界面；`Content/Aesir/UI` |
| `area:level-flow` | 第一关阶段、镜头/输入交接、遭遇、结算与重试；`Public/Framework/Flow`、GameMode |
| `area:build` | 模块依赖、工程配置、编译、Cook 与 Windows 打包；`Build.cs`、`Target.cs`、`Config`、发布清单 |
| `area:assets` | 来源许可、公开范围、依赖恢复及完整资产快照；来源清单、KI-05/KI-06、AUTH-01 |

`area:combat` 描述玩法行为，`area:gas` 描述能力与结算机制，可在同一问题中共同使用。`area:boss-ai` 描述 BT 与共享执行器，`area:boss-rl` 描述 RL 契约、策略与研究。

`AesirAIService/README.md` 指向独立的 `aesir-ai-service` 仓库；`area:services` 表示本仓库的 UE 集成影响。标签的存在不代表对应功能已完成，也不改变同伴闭环等功能的后续范围。

## 任务类型

已有 `bug`、`enhancement`、`documentation`、`question` 继续用于缺陷、功能、文档和问题讨论。新增以下两个类型，通常按任务的主要目的选择：

| 标签 | 任务目的 | 颜色 |
| --- | --- | --- |
| `type:research` | 契约设计、训练选模、评估方案、统计与原始数据 | 紫色 `5319e7` |
| `type:test` | PIE/打包验收、故障注入、端到端及回归测试 | 绿色 `0e8a16` |

其余现有标签 `accessibility`、`duplicate`、`help wanted`、`good first issue`、`invalid`、`wontfix` 按原用途保留。仅当任务确实适合新贡献者且环境/步骤清晰时使用 `good first issue`。

## 尚需补齐的验证

这两个浅红色 `f9d0c4` 标签用于筛选仍需执行的验证工作：

| 标签 | 加入条件 | 移除条件 |
| --- | --- | --- |
| `needs:runtime-verification` | 仍需在当前工程执行 PIE 或 Windows 包验收 | Issue 中对应验收项已实际运行并附可追溯结果 |
| `needs:experiment-evidence` | 仍需冻结配置并采集 UE BT/PPO 成组实验原始证据 | Issue 要求的配置、Episode、遥测及分析证据齐备 |

源码或资产存在不能作为移除依据。具体证据、版本、运行步骤和结果写在 Issue 验收清单，并同步验证矩阵/证据登记；移除标签也不代表未列入该任务的其他验证已通过。

## 使用示例

以下仅演示标签组合，不创建 Issue，也不声称已经运行复现。

| 任务 | 建议组合 |
| --- | --- |
| KI-01：Policy Client 非法动作失败计数与 BT 接管 | `bug`、`priority:p0`、`area:boss-rl`、`area:boss-ai`、`area:services`、`needs:runtime-verification` |
| KI-03：同条件 UE BT/PPO 成组对照 | `type:research`、`priority:p0`、`area:boss-rl`、`needs:experiment-evidence` |
| KI-04：结果界面及连续重试验收 | `type:test`、`priority:p0`、`area:level-flow`、`area:ui`、`needs:runtime-verification` |
| 营火菜单与开场运镜完善 | `enhancement`、`priority:p1`、`area:level-flow`、`area:ui` |
| KI-05/KI-06：公开范围、依赖恢复与资产版本说明 | `documentation`、`priority:p0`、`area:assets` |

任务进度（待办、进行中、完成、阻塞）适合未来 Projects 的状态字段；版本、交付阶段和截止日期适合里程碑。优先级及模块等标签不承担这些信息。

## 标签完整字段

以下 19 条名称、颜色和描述来自整理前的本地 JSON 快照，已完整保留；删除单独的 JSON 不影响这些字段。此表是资料记录，不表示本次重新读取了 GitHub 当前状态。

| 名称 | 颜色 | 描述 |
| --- | --- | --- |
| `priority:p0` | `b60205` | 当前验收必需：单 Boss 闭环、BT/PPO 对照、结果重试与 Windows 稳定演示 |
| `priority:p1` | `fbca04` | P0 稳定后的演示完善：营火菜单、运镜、引导、入场收尾动画与表现打磨 |
| `priority:later` | `c2e0c6` | 当前验收之外的扩展：普通敌人、MARL、同伴指令闭环、符文装备与持久存档 |
| `area:combat` | `1d76db` | 战斗规则、连招、命中、防御反击、锁定、生命与受击状态 |
| `area:gas` | `1d76db` | GAS 能力、属性、GameplayEffect、伤害结算与 GameplayTags |
| `area:player` | `1d76db` | 玩家角色、移动、Enhanced Input、相机与操作体验 |
| `area:boss-ai` | `1d76db` | Boss 行为树基线、共享动作执行器、合法性判断与策略回退 |
| `area:boss-rl` | `1d76db` | Boss 强化学习观测、动作与奖励契约、PPO 接入、遥测及 BT/PPO 评估 |
| `area:companion` | `1d76db` | 同伴角色、行为树、战术订单与文本/语音指令集成；真实 GAS 闭环为后续范围 |
| `area:services` | `1d76db` | 本仓库 UE 侧外部 AI 服务的 HTTP/JSON 集成、协议校验、超时与错误处理 |
| `area:animation` | `1d76db` | AnimBP、Blend Space、Montage、Root Motion 与动画通知 |
| `area:ui` | `1d76db` | HUD、血条、锁定指示、聊天窗口、菜单与战斗结果界面 |
| `area:level-flow` | `1d76db` | 关卡阶段、菜单与运镜衔接、Boss 遭遇、胜负结算及失败重试流程 |
| `area:build` | `1d76db` | UE 工程配置、模块依赖、编译、Cook、打包与 Windows 演示交付 |
| `area:assets` | `1d76db` | 资源来源与许可、公开仓库资产范围、依赖恢复及资产快照管理 |
| `type:research` | `5319e7` | 研究任务：RL 契约、训练选模、评估方案、统计分析与原始实验数据 |
| `type:test` | `0e8a16` | 测试任务：战斗/流程验收、故障注入、端到端回归与 Windows 实测 |
| `needs:runtime-verification` | `f9d0c4` | 仍需在当前工程执行 PIE 或 Windows 包验收，并记录步骤、日志和运行结果 |
| `needs:experiment-evidence` | `f9d0c4` | 仍需冻结实验配置并采集可追溯的 UE BT/PPO Episode、原始遥测与统计证据 |
