# 目标与范围

> 可玩目标基线：2026-10-07；AI部署扩展：2026-10-21用户采纳。阶段顺序见开发路线，任务状态见GitHub，功能验证结果见开发检查记录。

课程分析采用 [COURSE-20261018-R1](../09_Course_Analysis/README.md)，状态为“修订中，原有需求沿用；新增规则待确认”。它选择游戏、Boss RL 与 NPC 三个子域分析，保留原有 23 条功能需求和 18 个主要用例；P0/P1/P2 表示优先级，具体本次提交范围、来源版本和确认记录由课程入口维护。

## 已确认目标

- 可完整游玩的 Windows 垂直切片，体验规则见[第一关流程](../02_Design_Doc/GDD/01_GameFlow.md)。
- 共享执行规则下的 BT/PPO Boss、故障回退，以及可回查的训练与真实 UE 对照结果。
- 一致的玩家数值、可用技能与经过实测调整的战斗手感。
- AI 队友聊天、指令、实际战斗与执行回执，并整合为综合演示。
- Boss本地推理及模型交付；远程NPC服务的玩家/存档隔离、缓存同步、外部ASR和模型版本发布，按[ADR-003](../02_Design_Doc/TDD/Decisions/03_ADR003SplitAIDeployment.md)及[部署说明](../02_Design_Doc/TDD/06_Deployment.md)开发验收。多个独立单机客户端共享服务，不引入多人联机战斗或在线学习。

当前先完成 BT 全流程，后续阶段、负责人和依赖见[开发路线](../05_Development_Guide/06_DevelopmentRoadmap.md)。

## 尚未确定的细节

具体技能清单、数值、训练预算与时间仍待细化。符文槽数、属性等级、资源消耗、掉落和持久存档等已有草案集中在[技能与成长设计](../02_Design_Doc/GDD/05_MagicSystem.md)，实施前重新确认。普通敌人、多敌 MARL、大量装备和关卡内容尚未列为当前阶段的完成条件；Python NPC 跨游戏扩展按该仓库路线维护。

## 研究边界

BT 与 PPO 必须使用同一高层 Action/GAS 执行规则；模拟器 `RuleBossPolicy` 成绩与 UE BT 成绩分别报告。冻结策略按离线训练后推理描述，研究结论同时考虑有效性、可读性、公平性、多样性和运行代价。

## 变更规则

总体范围变化同步本页和开发路线；影响结构或接口的重要取舍记入 [Decisions](../02_Design_Doc/TDD/Decisions/README.md)。源码/资产存在、本人确认可玩与运行验收分别记录，实际结果见[开发检查清单](../06_Test_Doc/01_VerificationMatrix.md)。

课程 Word 是选定版本的自包含提交快照；规则仍在各自的玩法/接口来源维护，课程[追踪表](../09_Course_Analysis/02_Traceability.md)关联规则、用例、对象和验收，不另建一份实时开发计划。G-UC-06 的局部对象分析见[重试用例与对象职责](../09_Course_Analysis/01_RetryObjectAnalysis.md)。
