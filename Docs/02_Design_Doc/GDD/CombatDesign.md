# 战斗设计

> 状态：规则基线 + 静态源码核对 · 2026-09-29；数值、手感与打包体验仍需实测。

## 玩家操作与可读性

玩家拥有生命值，无玩家法力或体力资源。基础操作为移动、镜头、锁定、轻击、重击、闪避与固有盾防。当前工程存在 Enhanced Input 资产、[战斗组件](../../../Source/AesirWarden/Public/Combat/AesirCombatComponent.h)、[目标锁定组件](../../../Source/AesirWarden/Public/Combat/AesirTargetingComponent.h)和玩家 Guard Gameplay Ability 资产。具体输入键位以 [InputMapping](../../04_Engine_Config/InputMapping.md) 在编辑器核对后的清单为准。

## 防御规则

| 敌方攻击类型 | 普通防御 | 完美防御 | 闪避 |
| --- | --- | --- | --- |
| 普通攻击 | 减伤并累积 GuardPressure | 允许反击窗口 | 可规避 |
| 重攻击 | 打破普通防御 | 可成功防御 | 可规避 |
| 不可防御攻击 | 无效 | 无效 | 必须依赖闪避或脱离范围 |

GuardPressure 从零累积；达到 MaxGuardPressure 后进入限时破防/硬直。现有 [玩家 AttributeSet](../../../Source/AesirWarden/Public/Abilities/AesirPlayerAttributeSet.h)含 GuardPressure、MaxGuardPressure 等字段，[Gameplay Tags](../../../Config/DefaultGameplayTags.ini)含 `state.guard_broken`、`state.perfect_guard` 和相应事件。上表是统一设计契约；每种敌方攻击在 Blueprint/GAS 中的实际接线须用战斗用例验证。

## 伤害与状态责任

GAS 管理能力生命周期、Gameplay Effect、冷却与运行时属性；[伤害计算类](../../../Source/AesirWarden/Public/Abilities/Executions/AesirDamageExecutionCalculation.h)和旧战斗组件同时存在。最终伤害公式、护甲边界、完美防御窗口帧数、无敌帧与 Montage Notify 时点未冻结，不在此写入虚构常数。测试时逐项记录公式版本、攻击类型、输入时间窗、受击/硬直结果。

Boss 策略只选高层动作，不直接播放动画、移动每帧或施加伤害。动作合法性和能力激活见[GAS 架构](../TDD/GAS_Architecture.md)。
