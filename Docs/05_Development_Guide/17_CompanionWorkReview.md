# NPC 同伴工作进度与跨仓计划

> 历史审查记录：现存任务资料按 T1–T30 维护，包含 M1/M2 的 17 项和 NPC 的 13 项 T18–T30。旧批次清单、发布中间记录、删除索引和本地备份已按用户确认移除；正文中的旧文件名仅作历史记录。当前安排见 [M1/M2 清单](07_M1M2Issues.md)、[NPC 当前清单](08_CompanionIssues.md)、[统一 Project](09_GitHubProject.md)和[里程碑](10_GitHubMilestones.md)。

> 2026-10-07：按项目文档与源码审查，补充两项临时合成样例的直接函数边界检查。本轮没有运行完整 pytest、外部 LLM/ASR、真实 UE 或 Windows 包，历史记录与本轮验证分别说明。

## 分工与审查版本

用户本次确认：Boss RL、训练及研究由本人负责，Python 仓库的 AI 同伴/NPC 等其他模块由队友负责。策划书将 Python NPC 服务负责人记录为 dyh，UE 接入负责人为 yjx；具体贡献署名仍以本人和队友的实际确认及提交为准。

| 资料 | 本次核对基线 |
| --- | --- |
| 本地 Python 仓库 | `C:/Users/YYchainsaw/PycharmProjects/aesir-ai-service`；main `7caf4d9`，2026-09-24；工作区干净 |
| Python 公开 main | `1678dd6a467bc2fd9c077124ede527a82958e332`，2026-09-30 合并 PR #15；审查使用独立临时快照 |
| UE 公开 main | `facba39a3daef0e7904902c92497f84dccaa1654` |
| 当前 UE 本地 develop | `ea4c8052f5df319c2d3b7b8c8dfcf2f684efb408`；流程控制器的本地实现与公开 main 分开说明 |

本地 Python 版本缺少后来公开 main 的 S0/S1、端口及 S2 增量。本轮未拉取、切分支或修改该工作区；下列当前状态优先依据公开 main。来源：[策划 v0.2](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/game-design-doc-v0.2.md)、[SDD v1.1](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/aesir-agent-sdd-v1.1.md)。

## 队友的工作已到哪里

| 模块 | 当前实现与文档证据 | 尚需交付或核验 |
| --- | --- | --- |
| 人格对话与表达 | YAML 人设、短期会话、文本/SSE、情绪惯性、关系阶段边界；多轮实测复盘及幻觉/复读修正已记录 | 当前版本的代表性对话、失败样本与风格回归，和 UE 表现映射分开验收 |
| 四层记忆与关系 | 短期/摘要/档案/印象接线、事实接地、持久化与损坏降级；事件及对话信号推动关系，阶段与防刷规则存在 | 重启/重置、时间跳变、高频事件、游戏/角色隔离与实际验收数据 |
| 自主行为与世界事件 | 五场景、11 类行为白名单、六级仲裁、禁打断、节流、幂等反应与只读工具实现存在 | 产出指令与真实 UE 执行分别验收；未接通行为不能算游戏内完成 |
| 可靠性与可观测 | style_guard、LLM 熔断/回退、调试台 Web UI、中文关系阶段、单条/批量回执、信号与指标脚本存在 | 当前 SC/AT 验收矩阵、响应时间口径、指标脚本漏采修正与原始证据 |
| S0/S1 跨游戏前置与隔离 | 9/29–30 日志记录角色词外置、按角色路由、记忆/关系/知识库/信号/回执的 game_id 分层；服务端口可配置 | 数据路径/配置与分析工具同步；实际跨实例隔离不能仅由目录存在推断 |
| S2 人格包 | main 已有 loader/validator、目录注册表兼容及测试，Aesir capability 示例存在 | 公开 tree 未包含 Alice/Bruno 的 data/personas 包；迁移、兼容和归属校验需收口，未知等级检查有可复现缺口 |
| UE 同伴接入 | 当前 AesirWarden 已有 HTTP/订单组件，AI Controller 已处理探索域玩家 follow/wait | 当前 BT/资产、战术能力、事件/自主行为、执行终态与回执闭环、实际演示仍需核验 |

US1–US8/T001–T088 在文档中标为实现；可以据此说明队友已交付较完整的服务端功能。完整课程指标、真实 UE 执行与外部人格包接入仍需各自证据。

### 本轮直接检查

1. 在临时目录中按 S1 新布局分别写入一条合成回执与一条合成对话信号，直接调用 `metrics_report` 采集函数，结果为 **0 条回执、0 轮对话**。原因是采集仍使用旧根目录的 `glob("*.jsonl")`，未遍历 game_id 子目录。
2. 给 `PersonaPackValidator` 传入合法 Aesir L3 capability，以及 `requires_capability=UNKNOWN_LEVEL` 的最小合成 manifest，函数未拒绝。未知等级被映射为 -1，应在 S2 收口时明确拒绝。此项仅为直接函数检查，没有宣称 HTTP 端点已复现。

源码依据：[metrics_report.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/scripts/metrics_report.py)、[receipt_store.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/services/tactical/receipt_store.py)、[persona_pack_validator.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/app/services/companion/persona_pack_validator.py)。

### 文档同步缺口

- CHANGELOG 锚点为 9/28 的 678 通过 + 2 跳过，9/30 日志记录 714 通过，策划 v0.2 写 726 通过 + 2 跳过。它们是不同历史记录，本轮没有重新测得一个统一数量。
- 9/30 日志仍把 S2 列为下一步，但同一公开 main 已有 S2 loader/validator；应更新实现状态并从迁移/校验/验收收口推进。
- EXT-04 同时包含独立 v0.3 契约与批量回执，todo 已勾选，但公开 protocols 目录没有独立 v0.3 文件，SDD 待确认项也未同步。批量回执已实现，文档治理尚需核对。
- CODE-06 的场景分叉仅是风险登记：当前 `resolve_scene` 直接返回 `ctx.scene`，本轮未复现分叉。CODE-08 的策略生效时机与 20 条近期事件冷却边界应先以样例核验，再决定修复或约束。
- Python NPC 默认端口为 8000；当前 UE 两个同伴 Subsystem 默认均为 8011。旧文档关于 Chat=8001 的记录不直接适用于当前工程，应以两端实际配置联调。Boss 8012 继续独立。

## 文档中记录的下一步

既有计划的首要集成项是 EXT-01：UE 订单实际执行闭环，yjx 主责，dyh 提供协议和回执支持。队友可独立继续人格包路线：已实现的 S0/S1 作为基线引用，S2 收口并同时迁移 Bruno，随后 S3 游戏档案/canonical schema → S4 L0/L3 最小接入 → S5 外部包守卫。

策划书 9/30 的裁决已接受一进程一游戏、固定枚举配子集校验、S5 后置及 Bruno 随 S2 迁移。阶段 6 属后续演进；原文人天估算是历史规划值，不作为本次承诺日期或工期。S5 完成前仅使用自有包，外部包接入需通过实际审核门禁。对话 RL 导演层仍属 OUT-05 排除项。

来源：[9/29 记录](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/logs/2026-09-29.md)、[9/30 记录](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/logs/2026-09-30.md)、[todo](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/todo.md)。

## 里程碑与 Issue 管理

NPC 使用独立 N 系列：N1 服务与 Aesir 联调、N2 S2–S4 人格包最小复用、N3 S5 外部包守卫。各仓库里程碑独立维护，N1 在 UE 与 Python 各有一份。统一 Project 的交付阶段为后续，表示该线独立于当前 Boss P0，可与 Boss 开发并行推进。

旧批次发布记录已按用户确认删除；现行 NPC Issue 与跨仓依赖见 [NPC 当前清单](08_CompanionIssues.md)，机器可读记录见 [当前 NPC 任务记录](13_CurrentCompanionIssues.json)。Boss 当前任务见 [M1/M2 清单](07_M1M2Issues.md)。

EXT-07 流式 ASR/VAD 和空输入话术属于后续设计方向，尚未写入本轮必交验收；现有基础语音的可用条件纳入 N1 验收。EXT-08 时间/记忆边界、EXT-09 指标、FIX-04/07/08 口径及参数/风格回归已纳入本批。FIX-02 敌人血条和 FIX-06 输入重叠先在真实 UE 演示及已有基础战斗验收核验，确认本工程缺陷后按影响拆修复任务。
