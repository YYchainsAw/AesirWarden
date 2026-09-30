# 项目简介

> 状态：静态核对 + 目标说明 · 2026-09-29

## 一句话概括

**AesirWarden** 是第三人称动作战斗原型，答辩核心研究问题是：**RL 驱动的 Boss 高层战术决策，能否比传统 Behavior Tree 在相同执行规则下表现出更强的战况响应性与行为多样性？**

## 可玩目标与范围

目标为 Windows 上约 10–15 分钟的稳定垂直切片：进入 Boss 战、完成胜负、查看结果、重试或返回。主菜单、引导、检查点可在核心实验完成后补齐；普通敌人遭遇与语音战术同伴不阻塞答辩。当前工程已有玩家战斗、Boss BT/GAS、Observation、Policy Client、Telemetry 的代码或资产；完整 PPO 对照、打包流程与体验仍需验证。逐项状态见[范围表](02_GoalsAndScope.md)。

## 技术与个人工作

- Unreal Engine 5.8、C++、Blueprint、Gameplay Ability System、Behavior Tree、Enhanced Input、Motion Warping；工程模块为 `AesirWarden`，依据 [uproject](../../AesirWarden.uproject) 与 [Build.cs](../../Source/AesirWarden/AesirWarden.Build.cs)。
- Boss 推理接口使用独立本机 Python 服务（默认 `127.0.0.1:8012`）；同伴命令服务使用另一路本机服务（默认 `8011`），本轮不作为 Boss 必需链路。
- 个人贡献应以可核验的 C++/Blueprint、AI 架构、动画整合、镜头交互、实验配置和结果为准；外部资产来源单独列明。

题材细节、发布版本号及完整开发周期尚未定稿，不在此页臆造。面向 GDAT 的完整扩展目标见[范围表](02_GoalsAndScope.md)。
