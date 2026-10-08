# 统一 GitHub Project

> 2026-10-07 · 当前 T1–T30，共 30 条：UE 17、Python 13。原 17 条任务及其 Project 状态/顺序保留，本批新增 NPC 13 条。

[AesirWarden Project](https://github.com/users/YYchainsAw/projects/2) 当前为公开；两仓 Milestone 独立维护，通过同一阶段、跨仓依赖和父子任务协作。

| 阶段 | 当前任务范围 | UE | Python | 交付目标 |
| --- | --- | --- | --- | --- |
| M1 | T1–T8 | 8 | 0 | 完整游玩流程与 BT Boss 基线 |
| M2 | T9–T17 | 5 | 4 | Boss 系统完善与 RL 实战闭环 |
| M5 | T18–T26 | 4 | 5 | NPC 服务与真实 UE 聊天、战斗联调 |
| N2 | T27–T29 | 0 | 3 | 人格包与 L0/L3 最小跨游戏接入 |
| N3 | T30 | 0 | 1 | 外部人格包校验与审核门禁 |

用户负责 UE 游戏、玩家、Boss、同伴执行与接入，以及 Python Boss RL；队友负责 Python NPC 服务。责任写入正文，负责人沿用用户在 GitHub 的最新设置；本批发布未自动填写账号。M3/M4/M6 保留目标，尚未拆分 Issue。

字段：Title、Assignees、Status、Labels、Milestone、Repository、交付阶段。Status 为 Todo / In Progress / Blocked / Done；阶段为 M1–M6、N2、N3、后续。原生依赖约束最终验收，准备工作可以并行；原生 Blocked 提示与 Project Status 分别维护。

本次回读状态数量：Todo = 16；In Progress = 1；Blocked = 13；Done = 0。新任务全部 Todo，原 17 项状态保持。

- [总览](https://github.com/users/YYchainsAw/projects/2/views/1)：ROADMAP_LAYOUT；筛选 `全部`。
- [任务看板](https://github.com/users/YYchainsAw/projects/2/views/2)：BOARD_LAYOUT；筛选 `全部`。
- [UE 开发](https://github.com/users/YYchainsAw/projects/2/views/3)：TABLE_LAYOUT；筛选 `repo:YYchainsAw/AesirWarden`。
- [Python AI](https://github.com/users/YYchainsAw/projects/2/views/4)：TABLE_LAYOUT；筛选 `repo:YYchainsAw/aesir-ai-service`。
- [NPC 同伴](https://github.com/users/YYchainsAw/projects/2/views/5)：TABLE_LAYOUT；筛选 `label:area:companion`。

NPC 同伴视图显示本批 13 项；按当前 Milestone 和交付阶段查看 M5、N2 或 N3。沿用现有视图布局、筛选、公开性及字段；未增设自动加入或里程碑/阶段映射工作流。

T22 汇总 Python 服务验收；T26 汇总 UE 联调；T29 汇总跨游戏服务。NPC 的 priority:later 表示不作为 Boss M1/M2 前置，可由队友并行推进。T26 使用 T8 的 BT Windows 基线，不等待 RL、N2 或 N3。

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

[当前里程碑](10_GitHubMilestones.md) · [开发路线](06_DevelopmentRoadmap.md) · [NPC 当前任务](08_CompanionIssues.md) · [全部同步记录](12_CurrentIssues.json)。
