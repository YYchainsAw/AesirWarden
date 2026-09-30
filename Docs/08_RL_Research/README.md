# Boss RL 研究入口

> 状态：研究进行中 · 2026-09-29；当前没有新工程的正式 BT–PPO 对照结论。

研究问题：在第三人称 Boss 战中，RL 高层战术策略相对 UE 行为树基线，是否改善战况响应性和行为多样性，并维持可读性、公平性与运行性能？

| 文件 | 用途 |
| --- | --- |
| [BossRL_Contract](BossRL_Contract.md) | 当前 UE/Python Observation、Action、执行、奖励和回退边界 |
| [TrainingHistory](TrainingHistory.md) | 已有模拟器训练/选模产物与证据限制 |
| [EvaluationProtocol](EvaluationProtocol.md) | 独立测试与 UE 对照在运行前需要冻结的方案 |
| [EvidenceRegister](EvidenceRegister.md) | 证据文件、哈希、来源、验证状态 |

正式 `Results.md` 仅在独立测试和 UE 成组 Episode 有原始数据后建立；不得提前填写预测成绩。Python `RuleBossPolicy` 是模拟器规则基线，UE `BT_AesirBoss` 才是游戏内基线。冻结策略只表示会根据 Observation 响应战况，不表示部署时在线更新权重。
