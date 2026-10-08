# UML 图与图源

> 状态：新工程重新建模 · 2026-09-29

| 图源 | PNG 预览 | 口径 | 来源 |
| --- | --- | --- | --- |
| [04_SystemArchitecture.puml](04_SystemArchitecture.puml) | [04_SystemArchitecture.png](../Diagram/04_SystemArchitecture.png) | 当前源码/资产的组件边界；目标端到端流程另标待验收 | UE 模块、Boss 组件、独立 Python 服务 |
| [06_BossDecisionSequence.puml](06_BossDecisionSequence.puml) | [06_BossDecisionSequence.png](../Diagram/06_BossDecisionSequence.png) | 高层决策到共享 GAS 执行的接口契约 | Boss Controller/Action/Policy/Telemetry 源码 |
| [05_GasArchitecture.puml](05_GasArchitecture.puml) | [05_GasArchitecture.png](../Diagram/05_GasArchitecture.png) | 当前 GAS 类型与关键数据流 | ASC、AttributeSet、Boss GA/GE |
| [01_ProjectUseCases.puml](01_ProjectUseCases.puml) | [01_ProjectUseCases.png](../Diagram/01_ProjectUseCases.png) | 整组 UE + Boss RL + NPC 需求用例总览（To-Be） | [开发路线](../../05_Development_Guide/06_DevelopmentRoadmap.md)、AI Spec v1.1 |
| [02_GameUseCases.puml](02_GameUseCases.puml) | [02_GameUseCases.png](../Diagram/02_GameUseCases.png) | 当前游戏目标用例与 M5 NPC 交互（To-Be） | [游戏流程](../../02_Design_Doc/GDD/01_GameFlow.md)、[战斗设计](../../02_Design_Doc/GDD/03_CombatDesign.md) |
| [03_GameConceptModel.puml](03_GameConceptModel.puml) | [03_GameConceptModel.png](../Diagram/03_GameConceptModel.png) | 游戏核心概念类、多重性与目标边界（To-Be） | [游戏流程](../../02_Design_Doc/GDD/01_GameFlow.md)、[核心循环](../../02_Design_Doc/GDD/02_CoreLoop.md) |
| [07_BossRLUseCases.puml](07_BossRLUseCases.puml) | [07_BossRLUseCases.png](../Diagram/07_BossRLUseCases.png) | Boss RL 独立用例；R-UC-01～03、训练恢复与故障回退（To-Be） | [Boss RL 契约](../../08_RL_Research/01_BossRLContract.md)、[评估方案](../../08_RL_Research/03_EvaluationProtocol.md)、Python 训练与推理源码 |
| [08_BossRLConceptModel.puml](08_BossRLConceptModel.puml) | [08_BossRLConceptModel.png](../Diagram/08_BossRLConceptModel.png) | Boss RL 核心概念、记录归属与多重性（To-Be） | [Boss RL 契约](../../08_RL_Research/01_BossRLContract.md)、[证据登记](../../08_RL_Research/04_EvidenceRegister.md)、两端观测与评估记录 |

2026-10-08 新增需求模型与旧技术图分开维护：新图不提升运行状态，待验收项仍以验证矩阵为准。SRS 目录已由用户删除，本目录的图与图源保留。对应 PNG 从权威脚本生成，仅用于阅读。

图源保存在本目录，PNG 预览保存在相邻的 [Diagram](../Diagram/) 目录，沿用当前项目的目录安排。每份 `NN_Name.puml` 对应同名的 `NN_Name.png`，共 8 张图；修改脚本后同步生成预览。

NPC 的 13 份图源在 AI 仓库 `docs/design/uml/src/`，预览按用户最新调整位于 `docs/design/uml/png/`，沿用编号与短横线命名。UML-11～13 分别为 NPC 用例图、分析类图和对象图；直接在 AI 仓库维护，UE 不再保留素材副本或生成课程 ZIP。

这些图为新工程的可编辑源；同名 PNG 已从图源生成，供 GitHub 阅读。没有本轮 PIE/打包证据的路径在图内标为待验收；图中“源码存在”不等于运行通过。此批未复制旧项目 17 对图：旧 As-Is 图需针对新资产重新核验，旧 To-Be 图的设计内容已吸收到 GDD/TDD/RL 文档；同伴命令的三对图留在后续范围。

修改图时同步[绘图规范](../../03_Code_Standard/04_UmlGuideline.md)和相关技术文档。若要长期维护中英双语，需逐图核对语义后再增加英文版，不仅替换标题。
