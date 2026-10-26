# 项目简介

> 目标基线：2026-10-07；实现状态依据 2026-09-29 至 10-07 的记录，运行验收见验证矩阵。

## 一句话概括

**AesirWarden** 是第三人称动作战斗原型，答辩核心研究问题是：**RL 驱动的 Boss 高层战术决策，能否比传统 Behavior Tree 在相同执行规则下表现出更强的战况响应性与行为多样性？**

## 可玩目标与范围

当前先完成 **BT Boss 的完整游玩流程**，随后以 RL Boss 为重点推进玩家数值、技能和手感，以及 AI 队友的聊天与战斗接入，最后整合 Windows 演示。具体顺序以 [开发路线](../05_Development_Guide/06_DevelopmentRoadmap.md)为准，交付边界见[目标与范围](02_GoalsAndScope.md)。

可玩目标为约 10–15 分钟的垂直切片，玩家路径、菜单、镜头、结算与重试规则集中在[第一关流程](../02_Design_Doc/GDD/01_GameFlow.md)。2026-10-07 记录中，玩家基础移动、攻击、翻滚及 BT Boss 战斗已由本人确认可用；完整流程、PPO 联调和打包验收仍有待办，具体状态见路线与[验证矩阵](../06_Test_Doc/01_VerificationMatrix.md)。

## 技术与个人工作

- Unreal Engine 5.8、C++、Blueprint、Gameplay Ability System、Behavior Tree、Enhanced Input、Motion Warping；工程模块为 `AesirWarden`，依据 [uproject](../../AesirWarden.uproject) 与 [Build.cs](../../Source/AesirWarden/AesirWarden.Build.cs)。
- 当前Boss与NPC通过独立本机Python服务联调；2026-10-21采纳Boss本地推理、远程NPC服务、玩家/存档隔离及缓存同步的正式目标，连接现状见[工程速查](../05_Development_Guide/01_ProjectQuickReference.md)，交付和断网规则见[部署说明](../02_Design_Doc/TDD/06_Deployment.md)。
- 个人贡献应以可核验的 C++/Blueprint、AI 架构、动画整合、镜头交互、实验配置和结果为准；外部资产来源单独列明。

UE 部分由本人独立开发；Python Boss RL 也由本人负责，Python NPC 服务由队友负责。接口协作仍需保留，文档主要供自己开发回查、服务联调及答辩/作品集展示使用。

## 展示重点

- 玩家体验：清楚的攻击预兆、守防与闪避选择，以及稳定的胜负反馈，见[战斗设计](../02_Design_Doc/GDD/03_CombatDesign.md)。
- 答辩研究：相同执行规则下的 BT–PPO 对照、实验可复现性和适用边界，见[RL 研究](../08_RL_Research/README.md)。
- 作品集：系统设计、Unreal 工程、AI、动画整合和个人贡献，见[架构](../02_Design_Doc/TDD/01_Architecture.md)及[贡献与来源](05_CreditsAndProvenance.md)。

题材细节、发布版本号与开发周期尚未定稿；玩家访谈和可用性研究尚无已记录结果。
