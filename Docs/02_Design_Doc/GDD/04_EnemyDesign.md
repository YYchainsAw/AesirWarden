# 敌人设计

> 状态：Boss 静态实现核对 + 战术目标 · 2026-09-29

## 答辩 Boss

同一 Boss 遭遇支持两种高层决策来源：本地 `BT_AesirBoss` 基线与冻结 PPO 策略。两者提出 [9 类 Boss Action](../../08_RL_Research/01_BossRLContract.md)，由 [共享执行组件](../../../Source/AesirWarden/Public/AI/Boss/AesirBossActionComponent.h)检查 Avatar、权威、目标、死亡/眩晕/忙碌和 GAS 能力；反馈 Accepted 或带原因的拒绝。攻击动画、寻路、命中与伤害属于可靠执行层。

课程模型把业务上的一场 Boss 遭遇与策略/遥测运行段分开：策略切换不等于玩家重新入场，也不新建一次业务遭遇。连续故障达到阈值后由 BT 接管至本遭遇结束；故障、清零及运行段的唯一规则来源是 [BR-RL-02/03](../../08_RL_Research/01_BossRLContract.md)，本页不另存默认值或计数条件。该需求依据 2026-10-18 的 `DEC-02`，实际接管仍须验收。

当前 [BT 资产](../../../Content/Aesir/AI/Boss/BT_AesirBoss.uasset)、[Blackboard](../../../Content/Aesir/AI/Boss/BB_AesirBoss.uasset)、Boss Controller、Policy Client 与遥测源码可定位。当前资产与旧项目的文件哈希不同，旧文档列出的具体 BT 分支和 Blackboard 键需在新编辑器重新核验。不能只凭资产存在断言完整战斗或回退通过。

## 设计与评估要求

Boss 应对玩家攻击、守防、闪避、距离和自身状态作可读响应。有效性以伤害/胜负、非法动作率、重复率、动作分布、延迟、反应窗口和玩家可读性共同评估；策略强度不能只用胜率定义。独立模拟器规则基线与 UE 行为树基线必须分开报告。

攻击的普通/重击/不可防御分类以 [BR-GAME-05](03_CombatDesign.md#br-game-05)为准；入场、当前目标 Boss 判胜和终局停战以 [BR-GAME-01/02](01_GameFlow.md)为准。各类图的策略配置、当前策略与实际决策记录要能支持同局回退，不能把任何一方的高层建议当作命中事实。

普通敌人和两敌 MARL 协作属于后续研究；答辩先完成单 Boss 对照。[实验入口](../../08_RL_Research/README.md)。
