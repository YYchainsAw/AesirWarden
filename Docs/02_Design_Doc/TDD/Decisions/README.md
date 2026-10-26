# 架构决策

这里只记录影响系统结构、接口的重要取舍。日常调参、缺陷和测试结果分别放在设计表、KnownIssues 和验证矩阵。决定调整时保留原文、标明取代范围并链接后续依据。

| ID | 决定 | 状态 |
| --- | --- | --- |
| [ADR-001](01_ADR001SharedBossExecutor.md) | BT 与 RL 共用 Boss Action→GAS 执行边界 | Accepted（依据项目既定方向及当前源码） |
| [ADR-002](02_ADR002RLFirstDefense.md) | 单 Boss RL 研究主线及后续开发顺序调整 | Partially Superseded（推进顺序以 2026-10-07 路线为准） |
| [ADR-003](03_ADR003SplitAIDeployment.md) | Boss本地推理、远程NPC、玩家隔离与缓存同步 | Accepted（2026-10-21用户采纳；实现与跨端验收待完成） |

以上状态表示方向已明确，不表示实现或验收完成。
