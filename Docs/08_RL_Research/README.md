# Boss RL 研究

研究问题：相比 UE 行为树基线，RL 高层策略能否改善 Boss 对战况的响应和行为多样性，同时维持可读性、公平性及运行性能？

| 文档 | 什么时候看 |
| --- | --- |
| [UE–Python 契约](01_BossRLContract.md) | 修改观测、动作、奖励或推理服务接口时 |
| [模型来源与训练记录](02_TrainingHistory.md) | 查找接入模型、选模数据、训练产物和复现材料时 |
| [BT–PPO 评估方案](03_EvaluationProtocol.md) | 准备独立测试、游戏内对照和正式报告时 |
| [开发检查清单](../06_Test_Doc/01_VerificationMatrix.md) | 日常联调、动作合法性和故障回退检查时 |

当前RL用例按[训练与选模](../UML/png/03a_BossRLTrainingUseCases.drawio.png)、[部署与回退](../UML/png/03b_BossRLDeploymentUseCases.drawio.png)、[独立评估](../UML/png/03c_BossRLEvaluationUseCases.drawio.png)分三页；分析类按[训练、选模与部署](../UML/png/07a_BossRLTrainingClasses.drawio.png)和[遭遇与决策记录](../UML/png/07b_BossRLEncounterClasses.drawio.png)分两页，另有[回退分析时序](../UML/png/09_BossFallbackSequence.drawio.png)。这些图属于课程需求目标模型；10主题共14页的可编辑源、分页对应和审查状态见[UML索引](../UML/README.md)。本次版式返修尚不代表用户已接受，图源/导出检查也不能代替真实模型接入或正式实验。

Python `RuleBossPolicy` 是模拟器规则基线，UE `BT_AesirBoss` 是游戏内基线。冻结 PPO 会根据观测选择动作，部署时保持模型权重不变。选模数据的含义和已有产物见模型记录，正式结论由独立测试和 UE 对照数据支持。
