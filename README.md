# AesirWarden

第三人称战斗原型，研究强化学习 Boss 的高层战术决策在**相同 GAS 执行规则**下，是否比 Behavior Tree 基线更能响应战况、呈现多样行为。

**当前状态：开发中。**共享 Boss Action/GAS 边界及相关源码、资产已存在；UE 内 BT–PPO 成组对照、完整重试流程和 Windows 打包验收尚未完成。

**公开仓库范围：**收录 C++、配置、文档，以及项目制作的 Blueprint、GAS、BT、输入、UI、AnimBP、Blend Space 和 Montage。第三方原始素材、包含外部动作数据的 Animation Sequence、模板地图及其外部 Actor 数据不在公开仓库中；克隆后不能直接运行完整 Boss 战。完整工程保留在本地或受限访问的存储中。

继续开发从[开发笔记](Docs/README.md)和[当前计划](Docs/05_Development_Guide/06_DevelopmentRoadmap.md)开始：先接通 BT 的完整游玩流程，再完善 RL Boss、玩家与 AI 队友。研究方法见 [Boss RL](Docs/08_RL_Research/README.md)，素材归属见[来源清单](Docs/01_Project_Overview/05_CreditsAndProvenance.md)。
