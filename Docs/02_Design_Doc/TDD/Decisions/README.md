# 架构决策

这里只记录影响系统结构、接口或难以逆转的决定。每条记录包含状态、背景、决定、后果与依据；日常调参、缺陷和测试结果分别放在设计表、KnownIssues 和验证矩阵。新决定若取代旧决定，应新建记录并标明 `Superseded`，不悄悄改写历史结论。

| ID | 决定 | 状态 |
| --- | --- | --- |
| [ADR-001](ADR-001-shared-boss-executor.md) | BT 与 RL 共用 Boss Action→GAS 执行边界 | Accepted（依据项目既定方向及当前源码） |
| [ADR-002](ADR-002-rl-first-defense.md) | 答辩优先单 Boss RL 对照，同伴扩展延期 | Accepted（依据项目范围决定） |

以上状态表示方向已明确，不表示实现或验收完成。
