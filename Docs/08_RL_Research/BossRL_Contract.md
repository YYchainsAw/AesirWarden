# Boss RL 契约

> 状态：UE/Python 源码静态一致性核对 · Schema v4 · 2026-09-29；完整 UE Episode 待测。

## 观察

[UE 定义](../../Source/AesirWarden/Public/AI/Boss/AesirBossObservationTypes.h)与 `AesirBossObservationTypes.cpp::ToFeatureVector` 为 v4、26 个 `[0,1]` 特征；Python `rl/boss/contract.py` 定义同序。索引分组如下：

| 索引 | 内容 |
| --- | --- |
| 0–4 | 双方生命比例、距离、朝向、视线 |
| 5–12 | Boss/目标战斗状态、Boss 韧性、玩家守防压力与闪避 |
| 13–16 | 近期玩家攻击/守防/闪避频率与距离趋势 |
| 17–25 | 与下表九动作顺序对应的可用性标志 |

同伴位置和遭遇阶段**不在 v4**。变更字段、顺序、归一化或动作意义须升 Schema 并重新训练，不得暗改现有模型输入。

## 动作与执行

| ID | UE `EAesirBossAction` | 边界 |
| --- | --- | --- |
| 0 | LightAttack | 共享 Action → GAS |
| 1 | HeavyAttack | 同上 |
| 2 | Defend | 同上 |
| 3 | Dodge | 同上 |
| 4 | Pursue | GAS 移动能力，内部可用 Navigation |
| 5 | Disengage | 同上 |
| 6 | UseAbility | GAS |
| 7 | GapCloserSkill | GAS |
| 8 | UnblockableAreaSkill | GAS |

[枚举](../../Source/AesirWarden/Public/AI/Boss/AesirBossDecisionTypes.h)与 Python `BossAction` 目前静态匹配。执行器会复核目标/状态/ASC/能力配置并返回 typed Outcome；`Accepted` 只表示能力启动。策略不直接操控 Montage、动画变量、逐帧移动或伤害。

## 奖励与服务

当前 UE [RewardSettings](../../Source/AesirWarden/Public/AI/Boss/AesirBossRewardComponent.h)有伤害、受伤、胜负、成功防御、拒绝、重复与决策成本项；Python 模拟器奖励修订为 `boss-reward-003`。两端权重**不同**，不能把模拟器奖励数值写成 UE 实战奖励。正式实验分别保存每个奖励项、权重、版本和修改原因。

[Policy Client](../../Source/AesirWarden/Public/AI/Boss/AesirBossPolicyClientComponent.h)默认每 0.25 秒请求一次，超时 0.20 秒，连续失败阈值 3，访问 `127.0.0.1:8012`。错误响应应记录原因并切本地 BT；当前失败计数逻辑存在[待修复问题](../06_Test_Doc/KnownIssues.md)，回退尚未验收。
