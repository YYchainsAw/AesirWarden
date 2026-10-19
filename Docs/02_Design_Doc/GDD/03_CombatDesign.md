# 战斗设计

> 状态：规则基线 + 静态源码核对 · 2026-09-29；数值、手感与打包体验仍需实测。

## 玩家操作与可读性

玩家生命值、移动、镜头、锁定、轻击、重击、闪避与盾防是当前战斗设计的基础。现有 Enhanced Input 资产、[战斗组件](../../../Source/AesirWarden/Public/Combat/AesirCombatComponent.h)、[目标锁定组件](../../../Source/AesirWarden/Public/Combat/AesirTargetingComponent.h)和玩家 Guard Gameplay Ability 资产可定位；输入键位及编辑器核对项见[工程速查](../../05_Development_Guide/01_ProjectQuickReference.md)。课程核心沿用 Word 的生命、防御压力及冷却规则，不引入尚未定义的法力/体力消耗；后续 M4 的资源与技能方案在[技能与成长设计](05_MagicSystem.md)确认，变更后另记基线。

## 防御规则

<a id="br-game-05"></a>
**BR-GAME-05：攻击分类与守防。** 下表及 GuardPressure 规则沿用 GAME-FR04、G-UC-04。普通防御、完美防御和闪避的效果分别验证，不能把一种成功当作其他条件也通过。

| 敌方攻击类型 | 普通防御 | 完美防御 | 闪避 |
| --- | --- | --- | --- |
| 普通攻击 | 减伤并累积 GuardPressure | 允许反击窗口 | 可规避 |
| 重攻击 | 打破普通防御 | 可成功防御 | 可规避 |
| 不可防御攻击 | 无效 | 无效 | 必须依赖闪避或脱离范围 |

GuardPressure 从零累积；达到 MaxGuardPressure 后进入限时破防/硬直。现有 [玩家 AttributeSet](../../../Source/AesirWarden/Public/Abilities/AesirPlayerAttributeSet.h)含 GuardPressure、MaxGuardPressure 等字段，[Gameplay Tags](../../../Config/DefaultGameplayTags.ini)含 `state.guard_broken`、`state.perfect_guard` 和相应事件。上表是统一设计契约；每种敌方攻击在 Blueprint/GAS 中的实际接线须用战斗用例验证。

## 伤害与状态责任

GAS 管理能力生命周期、Gameplay Effect、冷却与运行时属性；[伤害计算类](../../../Source/AesirWarden/Public/Abilities/Executions/AesirDamageExecutionCalculation.h)和旧战斗组件同时存在。最终伤害公式、护甲边界、完美防御窗口帧数、无敌帧与 Montage Notify 时点未冻结，不在此写入虚构常数。测试时逐项记录公式版本、攻击类型、输入时间窗、受击/硬直结果。

Boss 策略只选高层动作，不直接播放动画、移动每帧或施加伤害。动作合法性和能力激活见[GAS 架构](../TDD/02_GasArchitecture.md)。

<a id="br-game-06"></a>
**BR-GAME-06：动作合法性与执行事实。** UE/GAS 依据当前阶段、角色生命/受控/忙碌、目标有效性、标签及冷却决定动作是否可执行；各技能只检查其实际所需条件。无效锁定目标及时清除。动作被拒绝不产生该动作的伤害或效果，仍给出可理解的失败反馈；能力 `Accepted` 只代表启动，命中、伤害与完成由实际事件分别确认。依据为 GAME-FR03、G-UC-04；BT 与 PPO 的共享执行边界另见 [BR-RL-01](../../08_RL_Research/01_BossRLContract.md)。

<a id="g-uc-04"></a>
## G-UC-04 进行战斗的局部规格

参与者为玩家；触发为 Boss 入场交接完成、进入战斗操作。前置是当前游玩与指定 Boss 遭遇有效，尚未结算。

| 步骤 | 参与者动作与系统响应 |
| --- | --- |
| 1 | 玩家移动、转镜头、锁定、攻击、闪避或举盾；系统按 BR-GAME-06 校验并执行合法操作，显示生命与战斗反馈。 |
| 2 | 遭遇普通、重击或不可防御攻击时，按 BR-GAME-05 及本次测试所固定的资产/窗口配置判定防御、破防、闪避和伤害。 |
| 3 | 尚未死亡时重复步骤1–2；玩家或指定 Boss 死亡时封闭后续战斗提交，将终局事实交给 [G-UC-05](01_GameFlow.md#g-uc-05)。 |

| 扩展 | 条件、响应与落点 |
| --- | --- |
| 1a | 冷却、死亡、受控、忙碌或技能要求的目标无效：拒绝对应动作并说明原因；未终局时继续步骤1，已终局则交 G-UC-05，不播放非法伤害/效果。 |
| 1b | 锁定目标失效：清除该目标；需要有效目标的能力拒绝，仍允许其他合法操作，继续步骤1。 |
| 2a | 超出命中/防御/无敌窗口：按记录的配置边界判定，不因“能力已启动”直接算命中或成功防御；继续步骤3。 |
| 2b | 旧局伤害或策略/同伴建议迟到：按 [BR-GAME-04](01_GameFlow.md#br-game-04)拒绝其影响；当前战斗继续步骤1，终局则保持结束。 |
| 3a | 双方同帧死亡：按 [BR-GAME-02](01_GameFlow.md#br-game-02)交付唯一失败事实，不能分别生成胜负结果。 |

成功结束表示战斗事实已交给唯一终局处理，后续战斗提交封闭；未死亡时仍在步骤1–3循环，并非用例已成功结束。失败最小保证是非法动作、过期消息和服务错误不能绕过 UE 执行规则或恢复已结束战斗。

防御窗口、伤害/减伤公式、无敌帧和冷却的具体值仍需按课程待决表确定，正式边界测试要带配置版本、资产路径、单位、起止时刻及边界是否包含。`C-02`～`C-06`、`MATCH-01a/b` 的步骤与实际记录由[开发检查清单](../../06_Test_Doc/01_VerificationMatrix.md)维护；没有固定参数的用例不能凭文字自填通过。
