# 符文与能力设计

> 状态：成长规则为目标设计；当前仅核对到部分属性和 GAS 基础 · 2026-09-29

## 目标规则

玩家每场装备两枚符文；当前基线是假设每枚符文授予一个主动能力。施放能力走 GAS 激活与冷却，不为玩家增加法力或体力。永久等级为 Strength、Defense、Rune、Vitality、Cooldown，各 1–9 级；`Rune` 提高符文伤害或效能，`Cooldown` 通过有上界的 haste 缩短冷却。等级、拥有/装备的符文和未用技能点属于存档数据，不能直接当作瞬时 AttributeSet。

## 当前可定位的基础

[玩家 AttributeSet](../../../Source/AesirWarden/Public/Abilities/AesirPlayerAttributeSet.h)定义 RunePower、CooldownHaste、PhysicalPower、DamageReduction 与 GuardPressure 等运行时字段；[当前 Ability 资产目录](../../../Content/Aesir/Abilities)可见 Boss 与 Guard 能力。尚未从当前源码确认两符文装备、符文获取、经验/技能点和完整符文施放链路，因此它们是计划，不是已完成系统。

## 将来落地的边界

`符文定义 → 装备槽授权能力 → 输入/战术选择 → GAS 合法性检查 → 动画/效果/伤害 → 冷却 → UI 反馈`。更换符文只替换与该符文有关的能力；玩家固有盾防仍由基础 Ability Set 持有。详细实现入口见[SpellPipeline](../TDD/SpellPipeline.md)。
