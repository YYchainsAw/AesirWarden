# Boss RL 契约

> 2026-10-18 文档修订。Schema v4 为既有契约；DEC-02的故障与分段规则已于2026-10-18由用户确认，来源见[课程基线](../09_Course_Analysis/README.md)。源码静态对应、目标要求和运行结果分别记录，本页不代表运行验收通过。

## 规则与需求对应

| ID | 可判定规则 | 对应需求 / 确认边界 |
| --- | --- | --- |
| BR-RL-01 | BT 与冻结 PPO 只提出高层动作，经同一 Action/GAS 执行器复核目标、状态、能力和冷却；`Accepted` 仅表示启动，伤害/命中/终局由真实事件决定 | RL-FR03；R-UC-02，既有约束 |
| BR-RL-02 | 仅当前有效游玩/遭遇、当前策略请求的响应可进入执行。核对协议、请求标识、序号、Schema、模型与动作；终局、返回和恢复开始后停止请求并使旧响应失效 | RL-FR02/04；R-UC-02，既有身份与终局隔离目标；身份实现方式需联调核对 |
| BR-RL-03 | 周期0.25秒、请求超时0.20秒；连续3次通信、协议或非法动作错误后切本地BT并保持到本场终局。仅完整合法响应清零；合法暂时无动作不算故障，满足完整协议时清零 | RL-FR04；R-UC-02；DEC-02已确认，运行仍待验 |
| BR-RL-04 | 部署加载明确模型并冻结参数；启用前检查观测/动作契约与模型交付材料。部署身份能追溯文件、版本和哈希；不匹配时拒绝作为正式可部署版本 | RL-FR01/02/06；R-UC-01/02 |
| BR-RL-05 | 训练/调参/选模数据与独立测试隔离；模拟器规则策略、UE BT、冻结PPO与回退分别标识，报告只覆盖实际环境和样本 | RL-FR05/06；R-UC-01/03；DEC-02已确认一遭遇多策略段、回退单独归组 |
| NFR-RL-01 | 分别采集HTTP往返、服务推理与UE执行请求耗时，按策略、故障与运行阶段统计；0.20秒超时配置不是已达到延迟目标的证据 | RL-FR04/05；R-UC-02/03 |

## 观测与动作

[UE 定义](../../Source/AesirWarden/Public/AI/Boss/AesirBossObservationTypes.h)及 `ToFeatureVector` 与 Python `rl/boss/contract.py` 的既有基线是 v4、26 个 `[0,1]` 特征。

| 索引 | 内容 |
| --- | --- |
| 0–4 | 双方生命比例、距离、朝向、视线 |
| 5–12 | Boss/目标战斗状态、Boss韧性、玩家守防压力与闪避 |
| 13–16 | 近期玩家攻击/守防/闪避频率与距离趋势 |
| 17–25 | 按下表九动作顺序排列的可用性标志 |

同伴位置和遭遇阶段不在v4。改变字段、顺序、归一化或动作含义时升Schema并评估重训，不能暗改现有模型输入。

| ID | UE `EAesirBossAction` | 执行边界 |
| --- | --- | --- |
| 0 | LightAttack | 共享Action → GAS |
| 1 | HeavyAttack | 同上 |
| 2 | Defend | 同上 |
| 3 | Dodge | 同上 |
| 4 | Pursue | GAS移动能力，内部可用Navigation |
| 5 | Disengage | 同上 |
| 6 | UseAbility | GAS |
| 7 | GapCloserSkill | GAS |
| 8 | UnblockableAreaSkill | GAS |

[动作枚举](../../Source/AesirWarden/Public/AI/Boss/AesirBossDecisionTypes.h)与Python `BossAction` 静态对应。策略不直接修改生命、伤害、Montage、动画变量或逐帧移动；合法动作仍可能被执行端按当下状态拒绝。

## 游玩、遭遇与策略记录段

| 概念 | 本页含义 | 当前实现与限制 |
| --- | --- | --- |
| Playthrough 本次游玩 | 从关卡起点开始到终局；重试建立新身份 | 游戏体验规则见[游戏流程](../02_Design_Doc/GDD/01_GameFlow.md) |
| BossEncounter 遭遇 | 当前游玩中与指定Boss发生的一场物理战斗 | 不因PPO故障切BT就复活Boss或重开战斗；一次遭遇可有多个策略记录段 |
| 策略记录段 | 遭遇内某一策略来源连续生效的遥测段 | 当前源码称 `Episode`；回退会将RL段记为Aborted，再创建Fallback段 |
| 请求 / 决策 | 对特定观测提出一次高层动作建议 | 属于有效遭遇及策略段，不能用迟到回包启动新局 |

DEC-02已确认一次遭遇可包含多个策略记录段，回退单独归组。`ActivateBehaviorTreeFallback` 的现有分段与此目标相容；仍需把同一遭遇与前后段关联到真实运行记录。类图不可把“一场遭遇”与“一个遥测Episode段”无条件等同。

## HTTP请求与有效性

服务默认 `http://127.0.0.1:8012/v1/boss/policy/decide`，默认周期/超时/阈值见[PolicyClient.h](../../Source/AesirWarden/Public/AI/Boss/AesirBossPolicyClientComponent.h)。

| 方向 | 当前协议1.0字段 | 接收条件 |
| --- | --- | --- |
| UE → 服务 | `request_id, schema_version, sequence, observation` | Schema4、26维、值域合法；同一客户端只保留一个在途请求 |
| 服务 → UE | `request_id, schema_version, sequence, model_id, actionable, action_id, reason, inference_milliseconds` | 请求/序号匹配，协议/Schema匹配，模型身份对应选定部署；reason可定位，推理耗时有限且非负；可行动时Action为整数0–8，无动作时不带可执行动作 |
| 合法无动作 | `actionable=false`，如 `state_locked` | 不启动动作；不能仅因当前无可用动作而当作传输故障 |
| 旧响应 | 已取消请求、旧游玩/遭遇、旧策略来源或已过终局 | 丢弃，不执行、不新建策略段、不恢复结束状态 |

当前请求没有显式 `encounter_id/episode_id`。已有 `ActiveRequest` 身份、`bRunning`、请求UUID和观测序号保护，见[PolicyClient.cpp](../../Source/AesirWarden/Private/AI/Boss/AesirBossPolicyClientComponent.cpp)。实现可通过显式身份或等效generation绑定满足BR-RL-02，不能从“不含Episode字段”直接推断完全没有迟到保护。

“完整合法响应”指上述当前请求与全部适用字段均通过校验，再允许清零；无动作响应免于动作执行条件，不免于身份和协议检查。当前UE只检查模型ID非空，未完整核验reason/推理耗时及预期部署身份；这些是目标与现实现的差距，不因文档列明而算已完成。

停止/取消必须在终局、返回、恢复开始和策略切换时与流程协同。当前 `EndBossEpisode` 只结束遥测；后续决策可能自动建立新段，不能据此认定终局停推理已完成。需核对Blueprint接线并执行RL-FAULT-01f和C-08b。

## R-UC-02 部署与故障细化

前置：模型及契约材料可定位，服务地址已配置，共享执行端存在，遭遇处于允许战斗的阶段。触发：启用冻结PPO并提交当前观测。

1. 加载明确模型并核验交付信息；缺少材料或不匹配时报告原因，不进入正式实验。
2. 当前有效遭遇发起请求；服务校验并返回动作或合法无动作。
3. UE先核验当前请求身份、版本与响应完整性，再处理失败状态和动作。
4. 合法可行动建议进入共享执行器；记录实际Outcome及后续奖励事实。
5. 通信、协议或非法动作故障累计达到3次后，本地BT接管并保持到本场终局；前后策略段关联同一遭遇，回退单独归组。
6. 终局/返回/恢复时取消请求并失效旧身份；迟到响应不得启动动作或新段。

| 分支 | 必须保持的保证 | 细化状态 |
| --- | --- | --- |
| 断连、HTTP失败、超时、JSON/协议/Schema错误、缺失/非法动作 | 不执行该响应；原因和计数可追溯 | 每次计一次；仅完整合法响应清零，KI-01待修复 |
| 合法无动作 | 不执行、不绕过GAS；与协议故障区分 | 完整合法无动作响应清零；不是通信/协议故障 |
| 策略切换、终局或重试后的旧响应 | 丢弃，不影响新游玩或已结束状态 | 目标明确，游戏内接线和运行待验 |
| BT接管后服务恢复 | 不隐式重开遭遇 | 本场保持BT至终局；不自动切回PPO |

成功保证：冻结策略的合法建议被执行或有明确拒绝原因；失败最小保证：错误不产生非法动作，游戏可继续处理终局、重试、返回和退出。当前源码与运行差距登记在[已知问题](../06_Test_Doc/02_KnownIssues.md)。

## 奖励与交付资料

UE [RewardSettings](../../Source/AesirWarden/Public/AI/Boss/AesirBossRewardComponent.h)包含伤害、受伤、胜负、成功防御、拒绝、重复与决策成本项；Python奖励修订为 `boss-reward-003`。两端权重不同，分别保存奖励项、权重和版本。

模型路径、哈希、训练与选模来源见[模型记录](02_TrainingHistory.md)。正式实验和回退归组见[评估方案](03_EvaluationProtocol.md)，检查步骤见[开发检查清单](../06_Test_Doc/01_VerificationMatrix.md)。
