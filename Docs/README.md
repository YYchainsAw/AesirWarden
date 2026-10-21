# 开发笔记

这套文档供自己继续开发 UE、接入 Python 服务，以及准备演示和研究资料。先按下面的问题找入口。

| 我现在想做什么 | 从这里看 |
| --- | --- |
| 看当前目标和下一步 | [当前计划](05_Development_Guide/06_DevelopmentRoadmap.md) · [UE 任务看板](https://github.com/users/YYchainsAw/projects/2/views/3) |
| 查游戏流程和玩法规则 | [第一关流程](02_Design_Doc/GDD/01_GameFlow.md) · [战斗](02_Design_Doc/GDD/03_CombatDesign.md) · [Boss](02_Design_Doc/GDD/04_EnemyDesign.md) · [技能与成长草案](02_Design_Doc/GDD/05_MagicSystem.md) |
| 给蓝图和场景接线 | [流程控制器接入](05_Development_Guide/05_LevelFlowGuide.md) · [技术架构](02_Design_Doc/TDD/01_Architecture.md) · [GAS](02_Design_Doc/TDD/02_GasArchitecture.md) |
| 启动、编译、改配置或恢复工程 | [工程速查](05_Development_Guide/01_ProjectQuickReference.md) |
| 逐项对照15项AI能力的位置、已有计划、同步与断网边界 | [AI部署与离线行为](02_Design_Doc/TDD/06_Deployment.md) · [ADR-003](02_Design_Doc/TDD/Decisions/03_ADR003SplitAIDeployment.md) |
| 查 Bug 或检查改动有没有影响旧功能 | [已知问题](06_Test_Doc/02_KnownIssues.md) · [日常检查表](06_Test_Doc/01_VerificationMatrix.md) · [打包检查](07_Release/01_ReleaseChecklist.md) |
| 接入 RL Boss 或做对照实验 | [Boss RL 入口](08_RL_Research/README.md) |
| 查看课程 Word、说明对象职责或检查需求与 UML | [Word需求文档修订稿](09_Course_Analysis/Word/AesirWarden需求分析文档.docx) · [课程分析与基线](09_Course_Analysis/README.md) · [重试对象分析](09_Course_Analysis/01_RetryObjectAnalysis.md) · [需求追踪](09_Course_Analysis/02_Traceability.md) |

## 配套资料

- [项目简介](01_Project_Overview/01_ProjectBrief.md)和[范围](01_Project_Overview/02_GoalsAndScope.md)：介绍项目及已经确定的目标。
- [个人开发约定](03_Code_Standard/01_ProjectConventions.md)：命名、蓝图组织和日志等速查。
- [UML 源与预览](UML/README.md)：10个课程主题、10份draw.io源、14页Desktop导出图和技术参考。
- [素材与贡献来源](01_Project_Overview/05_CreditsAndProvenance.md)：素材归属和展示署名。
- [本轮建模证据](09_Course_Analysis/03_AIModelingEvidence.md)：真实输入、AI预审与修订，人工审查和运行验收分别登记。

Word位于Docs内的修订副本已按COURSE-20261018-R1回填，外部原件保留；[建模与修订记录中的排版核对](09_Course_Analysis/03_AIModelingEvidence.md)已完成。2026-10-21部署扩展单列于Markdown/追踪，尚未回填Word或重导UML。成员最终语义接受及UE/服务/实验运行验收仍需分别记录。

## 怎么维护

- 任务的进行中、阻塞和完成状态只在 GitHub 看板更新；本地计划只维护目标、顺序和开发入口。
- 一条规则只在一处完整说明。流程规则归游戏流程，接口归契约，Bug 状态归已知问题，其余页面用链接引用。
- 设计说明写清“要做什么、为什么、怎么接、还有什么没确定”。具体方案待确认时保留问题，不自动扩成任务。
- 日常检查记日期、结果和异常；正式打包与研究实验再记录完整的代码、资产、模型和测试条件。
