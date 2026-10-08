# AesirWarden 文档

> 状态：建设中 · 目录与命名核对：2026-10-08 · 当前 UE 工程：`AesirWarden`（UE 5.8）。业务与验收状态以各页原基线为准。

本目录是项目设计、实现、验证和发布资料的入口。项目和 UE 工程统一使用 `AesirWarden`。本文档只描述当前可核对的事实和明确标注的目标；源码存在、资产存在、PIE 冒烟、Windows 打包及正式实验是不同的证据层级。

## 从这里开始

| 读者 | 入口 |
| --- | --- |
| 评审与试玩者 | [项目简介](01_Project_Overview/01_ProjectBrief.md) → [目标与范围](01_Project_Overview/02_GoalsAndScope.md) → [RL 研究入口](08_RL_Research/README.md) |
| 需求模型 | [UML 图与图源](Assets/UML/README.md)，包含整组用例、游戏用例、游戏概念类图及 Boss RL 独立用例与核心概念类图；SRS 目录已由用户删除 |
| 游戏设计 | [游戏流程](02_Design_Doc/GDD/01_GameFlow.md)、[核心循环](02_Design_Doc/GDD/02_CoreLoop.md)、[战斗设计](02_Design_Doc/GDD/03_CombatDesign.md)、[敌人设计](02_Design_Doc/GDD/04_EnemyDesign.md) |
| 技术开发 | [总体架构](02_Design_Doc/TDD/01_Architecture.md)、[GAS 架构](02_Design_Doc/TDD/02_GasArchitecture.md)、[环境搭建](05_Development_Guide/01_SetupEnv.md) |
| 当前开发与任务 | [开发路线](05_Development_Guide/06_DevelopmentRoadmap.md)、[M1/M2 当前 Issue](05_Development_Guide/07_M1M2Issues.md)、[NPC 当前 Issue](05_Development_Guide/08_CompanionIssues.md)、[统一 Project](05_Development_Guide/09_GitHubProject.md)、[里程碑](05_Development_Guide/10_GitHubMilestones.md)、[标签约定](05_Development_Guide/11_GitHubLabels.md)、[开发工作流](05_Development_Guide/04_Workflow.md) |
| 审查依据与历史 | [Python 仓库审查](05_Development_Guide/16_PythonRepoReview.md)、[NPC 工作审查](05_Development_Guide/17_CompanionWorkReview.md)；旧批次资料、草稿与备份已移除 |
| 验证与发布 | [验证矩阵](06_Test_Doc/01_VerificationMatrix.md)、[已知问题](06_Test_Doc/02_KnownIssues.md)、[发布检查](07_Release/01_ReleaseChecklist.md) |
| 文档维护 | [命名规范](03_Code_Standard/06_DocumentNaming.md) |

## 文档规则

文件命名见[文档命名规范](03_Code_Standard/06_DocumentNaming.md)；证据层级按下列规则记录。

1. **单一事实来源**：设计规则归 GDD/TDD；重大架构决定归 `02_Design_Doc/TDD/Decisions/`；测试结论归验证矩阵；原始遥测与训练产物只被引用，不在报告中手工改写。
2. **状态明确**：`已由源码/资产核对`、`运行已验证`、`目标设计`、`待核验`不得混用。此批文档是静态迁移审查，不表示新工程已完成运行测试。
3. **证据可追溯**：结论应链接到当前源码、资产、配置、实验 manifest 或测试记录。迁移前项目的资料只作为线索，不能自动成为新工程验收结果。
4. **随变更更新**：改变契约、Boss 动作、奖励、资源规则或发布流程时，同步更新对应文档和验证项。

旧项目 `AesirCombatPrototype` 的历史资料保留在原仓库。本文档树不复制旧审查报告；需要重现旧判断时按来源回查，并以本工程重新验证。
