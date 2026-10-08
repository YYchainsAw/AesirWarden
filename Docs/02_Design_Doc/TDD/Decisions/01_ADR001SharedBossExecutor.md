# ADR-001：Boss 决策共用 GAS 执行边界

- 状态：Accepted
- 记录日期：2026-09-29（依据旧项目决定和当前源码追补）

## 背景

要公平比较 Behavior Tree 与 PPO，策略必须只控制同一层级的高层动作，不能让其中一方绕过能力合法性、动画或伤害规则。

## 决定

BT 与 RL 都选择 `EAesirBossAction`；[UAesirBossActionComponent](../../../../Source/AesirWarden/Public/AI/Boss/AesirBossActionComponent.h)统一校验目标、权威、状态和配置，并通过 ASC/GAS 激活能力，返回 typed Outcome。低层动画、寻路、命中与伤害仍归执行层。

## 后果

优点是两策略执行预算和合法性可对齐，遥测可比较；代价是新增动作必须同时维护枚举、Ability 映射、Observation 可用性、Python 契约与测试。当前代码实现不等于完整 UE 实验已经通过。
