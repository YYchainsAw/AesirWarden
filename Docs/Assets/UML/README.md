# UML 图源

> 状态：新工程重新建模 · 2026-09-29

| 图 | 口径 | 来源 |
| --- | --- | --- |
| [SystemArchitecture.puml](SystemArchitecture.puml) | 当前源码/资产的组件边界；目标端到端流程另标待验收 | UE 模块、Boss 组件、独立 Python 服务 |
| [BossDecisionSequence.puml](BossDecisionSequence.puml) | 高层决策到共享 GAS 执行的接口契约 | Boss Controller/Action/Policy/Telemetry 源码 |
| [GasArchitecture.puml](GasArchitecture.puml) | 当前 GAS 类型与关键数据流 | ASC、AttributeSet、Boss GA/GE |

这些图为新工程的可编辑源；同名 PNG 已从图源生成，供 GitHub 阅读。没有本轮 PIE/打包证据的路径在图内标为待验收；图中“源码存在”不等于运行通过。此批未复制旧项目 17 对图：旧 As-Is 图需针对新资产重新核验，旧 To-Be 图的设计内容已吸收到 GDD/TDD/RL 文档；同伴命令的三对图留在后续范围。

修改图时同步[绘图规范](../../03_Code_Standard/UmlGuideline.md)和相关技术文档。若要长期维护中英双语，需逐图核对语义后再增加英文版，不仅替换标题。
