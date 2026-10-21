# 需求、对象与验收追踪

> `COURSE-20261018-R1` · 修订中。沿用原23FR/18UC；NFR/BR是给已有要求及本轮待确认约束建立引用编号，不表示新增玩法已获批准。

来源与确认状态见[课程基线](README.md)，完整需求与修订后的详细用例见[Word修订稿](Word/AesirWarden需求分析文档.docx)。本页保存映射与验收意图，不复制完整需求书、任务状态或测试操作脚本。具体操作和实际结果归[开发检查清单](../06_Test_Doc/01_VerificationMatrix.md)，正式实验条件归[评估方案](../08_RL_Research/03_EvaluationProtocol.md)。

2026-10-21部署扩展单列`DEP-20261021`，不重编号以下23FR/18UC。正式位置与离线规则见[部署说明](../02_Design_Doc/TDD/06_Deployment.md)，目标来源见[ADR-003](../02_Design_Doc/TDD/Decisions/03_ADR003SplitAIDeployment.md)；原Word/UML未因Markdown修改自动回填。

| 扩展范围 | 既有职责关联 | 验收入口 |
| --- | --- | --- |
| 本地Boss与模型随包/更新 | RL-FR01/02/04/06、R-UC-01/02；共享GAS与冻结模型 | DEP-UE-01/05；AI-DEP-01/02/14/15；Python方案第五/六步 |
| 远程NPC、玩家/存档身份与ASR | NPC-FR01～09、NPC-UC01～07；UE实时执行边界保持 | DEP-UE-02/03/04；AI-DEP-03～13；Python方案第二/三/四步 |
| 缓存同步、重连、记忆重置与运维 | 游戏重试/旧响应隔离、NPC记忆/关系及回执 | DEP-UE-04/06；AI-DEP-09/10/15；Python方案第四/六步 |

以上均未执行。AI-DEP编号来自[部署正文](../02_Design_Doc/TDD/06_Deployment.md)的两仓能力对照；服务侧方案位置也在该页列明，不再引用服务器仓库中没有来源的AT-DEP编号。新增身份、外部ASR协作和同步字段仍须落实到后端需求/契约，旧图形覆盖边界见UML索引。

## 读表规则

- `UC/步骤`指[Word修订稿](Word/AesirWarden需求分析文档.docx)第6章分步表，不再按外部原件或旧提取的步骤数读取。G-UC-05统一为3步；R-UC-02细化为部署1、请求2、完整响应校验3、共享执行4、持续/BT段5、终局失效6。G-UC-06同时对应[本轮详规](01_RetryObjectAnalysis.md#g-uc-06)，其成功出口在恢复可操作状态结束，后续探索和入场分别回到G-UC-02/03。
- A01～A10为[模型索引](../UML/README.md)中的模型ID；表中元素是分析职责，不要求新建同名UE/Python实现类。
- `+`是正例，`−`是边界/失败反例；它们是验收意图，不是执行日志。TCID的步骤、判据与实际结果统一在检查清单维护。
- `待测`表示本轮未执行对应UE、服务、打包或实验；涉及DEC的行还需先闭合规则。图片、源码、编译或历史可玩基础不自动填成通过。
- P0/P1沿用原Word优先级。23项均保留在课程分析覆盖中，未来P2不由本页升为本次必交功能。

## 23条功能需求

### 游戏8条（原Word表06，P0）

| FR及来源要点 | UC/关键步骤 | 模型元素与约束 | 正反例及TCID | 结果/待决 |
| --- | --- | --- | --- | --- |
| GAME-FR01 开始游戏：菜单到开场/探索；SRC-01 §5.2.1、SRC-05流程 | G-UC-01/1～2；开始动作是触发，菜单是前置 | A01/A02；A05 `GameSession/Playthrough`；A08阶段交接；BR-GAME-01 | +结束/跳过开场恢复相同控制；−连点开始、菜单输入穿透：FLOW-01 | 待测 |
| GAME-FR02 探索与入场：只触发一次，入场期间不提前决策/伤害；SRC-01、SRC-05 | G-UC-02/1～2、G-UC-03/1～3及步骤绑定异常 | A05 `BossEncounter`尚未开战；A08探索→入场→战斗；BR-GAME-01/02 | +自然结束与跳过均开战；−重复进触发区、过场中攻击：FLOW-01、BT-01 | 待测；资源异常出口已写入详规，仍需运行验证 |
| GAME-FR03 战斗操作与合法性；SRC-01、SRC-05战斗 | G-UC-04/1、3 | A05 `Player.health/combatState`与`BossPolicy`；动作/冷却合法性由BR-GAME-06、BR-RL-01及A09执行职责说明，A05未逐项展开技能类 | +各动作在合法条件执行；−忙碌/冷却/失效目标拒绝且无残留：COMBAT-01、C-02/03、RL-ACTION-01 | 待测；窗口/公式配置DEC-04 |
| GAME-FR04 格挡、防御压力、重攻击及不可防御攻击；SRC-01 §5.7、SRC-05 | G-UC-04/2及拒绝分支 | A05角色守防状态与能力；BR-GAME-05 | +普通攻击累压、重击可完美防御；−用两类防御应对不可防御攻击均无效：C-04/05/06 | 待测；精确窗口/系数DEC-04 |
| GAME-FR05 指定Boss/玩家死亡及唯一终局；SRC-01、SRC-05 | G-UC-04/3→G-UC-05/1～3 | A05 `Playthrough/CombatResult/BossEncounter`；A08终局；BR-GAME-02/04 | +Boss死亡胜利/玩家死亡失败；−普通敌人死亡不通关、重复终局无效、同帧双方死亡按失败、终局后伤害无效：MATCH-01、MATCH-01a/b | 需求DEC-01已确认；运行待测，KI-02 |
| GAME-FR06 重试至初始可玩位置，清理旧状态；SRC-01、SRC-05 | G-UC-06恢复步骤及失败分支，见深入详规 | A05 `Playthrough—BossEncounter`；[REL-GAME-01](01_RetryObjectAnalysis.md#relationship-rel-game-01)、R-PT-01/02、R-BE-01/02、INV-PT-01/INV-BE-01及BCE；A08；BR-GAME-03/04 | +连续两轮重试、恢复时无新遭遇；−重复点击/迟到响应无效；恢复失败禁战斗且可再试/回菜单，菜单失败可再试/退出：C-08、C-08a/b/c/d、EXIT-03 | 需求DEC-01/02已确认；运行待测 |
| GAME-FR07 返回营火菜单与控制恢复；SRC-01、SRC-05 | G-UC-07/1～2 | A05 `GameSession`与旧游玩失效；A08菜单恢复；BR-GAME-01/04 | +返回后再次开始；−连续点击/恢复中旧指令不得触发：C-08、FLOW-01 | 待测 |
| GAME-FR08 菜单/结算退出、输入隔离、离线不阻塞；SRC-01、SRC-05 | G-UC-08/1～2；关联G-UC-01/05 | A08退出与阶段权限；BR-GAME-01/07、NFR-GAME-01 | +Windows包从菜单/结算退出；−服务挂起仍可退出、点击不触发攻击：EXIT-01/02/03、BUILD-01 | 待测；PIE停止不作Windows进程退出证据 |

### Boss RL 6条（原Word表07，P1）

| FR及来源要点 | UC/关键步骤 | 模型元素与约束 | 正反例及TCID | 结果/待决 |
| --- | --- | --- | --- | --- |
| RL-FR01 可复现训练与候选选择；SRC-01、SRC-06 | R-UC-01/1～3及中断分支 | A03；A07 `TrainingRun/TrainingConfig/ModelArtifact/PolicySelection`；BR-RL-04/05 | +配置/检查点/选模依据齐备；−缺契约或来源不得标正式可部署：RL-MODEL-01、RL-COMPARE-01 | 待核查产物；本轮不训练 |
| RL-FR02 加载明确冻结模型，契约不符拒绝启用；SRC-01、SRC-06 | R-UC-02/1及部署失败分支 | A07 `PolicyDeployment/ModelArtifact`；A09；BR-RL-04 | +模型版本一致且参数冻结；−SHA/Schema不符拒绝启用：RL-MODEL-01、RL-CONTRACT-01 | 待测；不把文件存在当真实模型部署通过 |
| RL-FR03 BT/PPO同一高层动作执行；SRC-01、SRC-06 | R-UC-02/4，关联G-UC-04/1；BT持续见R-UC-02/5 | A05策略/能力；A07决策与执行反馈；A09共享执行职责；BR-RL-01 | +两策略同条件走共享入口；−策略不能绕过UE/GAS改生命，Accepted不算命中：RL-ACTION-01 | 待测 |
| RL-FR04 非法响应与故障处理；SRC-01 §5.3、SRC-06 | R-UC-02/2～3故障及旧局分支、5的BT保持、6及其结束后回调分支 | A07 `DecisionRecord/BossEncounter`与策略段来源记录；A09；BR-RL-02/03 | +合法响应清零、合法无动作不故障；−离线/超时、坏协议、缺失/越界/分数动作连续3次切BT且保持到终局，迟到响应不执行：RL-FAULT-01a～g、RL-EPISODE-01 | 需求已确认DEC-02；运行待测，KI-01仍须修复复测 |
| RL-FR05 独立条件比较冻结策略与基线；SRC-01、SRC-06 | R-UC-03/1～3 | A07 `EvaluationRun.sampleIds/metricsVersion`、`BossEncounter/StrategySegment`；指标和报告细节归评估文档，不宣称图内另有独立结果类；BR-RL-01/05 | +相同条件/种子采集，回退样本独立归组；−版本/环境不同不混成同条件结论：RL-COMPARE-01、RL-METRICS-01 | 待采集；分组需求DEC-02已确认，样本/指标DEC-04 |
| RL-FR06 区分选模/独测与模拟器/UE；SRC-01、SRC-06 | R-UC-01/2～3、R-UC-03/1、3 | A07候选属于候选集、样本用途与运行环境；BR-RL-04/05 | +报告回查原始Episode/策略段/模型/条件；−选模900局不作独测、回退局不混入纯PPO、模拟器不冒充UE结果：RL-COMPARE-01、RL-METRICS-01 | 待独测/UE成组证据；不预设PPO更优 |

### NPC服务9条（原Word表08，P1）

| FR及来源要点 | UC/关键步骤 | 模型元素与约束 | 正反例及TCID | 结果/待决 |
| --- | --- | --- | --- | --- |
| NPC-FR01 人格、真实记忆与上下文约束；SRC-01、SRC-07 | NPC-UC01/1～3 | A04；A06 `CompanionIdentity/PersonaProfile/MemoryContext`及执行上下文值；BR-NPC-01/06 | +规则/LLM路径均尊重身份事实；−未知角色、幻觉经历、后端失败不能借默认角色蒙混：NPC-CHAT-01、NPC-SCOPE-01 | 待服务与UE分别验；实际后端/外发DEC-05 |
| NPC-FR02 文本意图到授权行为建议；SRC-01、SRC-07 | NPC-UC02/1～3 | A06 `Directive/AbilitySpec`、有身份的执行记录；A10；BR-NPC-01/03 | +授权能力合法建议；−无技能日常建议不被强制要技能、未授权/模糊/无效目标拒绝或澄清：NPC-COMMAND-01 | 待测；能力数量DEC-03 |
| NPC-FR03 语音进入同一指令流程；SRC-01、SRC-07 | NPC-UC03/1～3 | A04语音入口；A10复用文本校验/执行；BR-NPC-01/06 | +有效语音转文本后走同一校验；−格式/识别/外部服务失败不执行并保留文本入口：NPC-VOICE-01 | 待测；语音格式与外发DEC-05 |
| NPC-FR04 心跳自主建议与抑制；SRC-01 §5.7、SRC-07 | NPC-UC04/1～3 | A06 `DirectiveExecution`的快照上下文值与`Directive`；A10；BR-NPC-01/05 | +允许场景返回一个建议或无动作；−过场/讲话/施法/弹窗、过期或重复快照不刷行为：NPC-AUTO-01 | 待测；实际执行/回执DEC-03 |
| NPC-FR05 事件驱动关系/记忆，重复幂等；SRC-01、SRC-07 | NPC-UC05/1～3 | A06 `MemoryContext.eventFacts/eventDedupKeys`与`CompanionIdentity.relationshipState`；`DirectiveExecution`的作用域和`directiveId`关联执行去重；BR-NPC-01/02 | +可信事件按规则更新；−重复ID沿用首次结果，未授权/存储失败明确响应：NPC-EVENT-01 | 待测；不把世界事件重复当新贡献 |
| NPC-FR06 指定角色查询与解释；SRC-01、SRC-07 | NPC-UC06/1～2及不存在分支 | A04调试人员通路；A06人格、关系、记忆与指令记录；BR-NPC-01/06 | +按角色查状态和指定决策来源；−未知角色/请求有明确错误、不暴露密钥：NPC-QUERY-01、NPC-SCOPE-01 | 待测；SRC-07已有调试端点，部署可达性/指定历史请求解释仍须核验 |
| NPC-FR07 指定记忆清理、保留人格与关系；SRC-01、SRC-07 | NPC-UC07/1～2及持久清理失败 | A06人格/记忆生命周期；BR-NPC-04 | +目标角色清理，关系和其他角色不变；−持久清理失败返回失败，不声称事务回滚：NPC-RESET-01 | 待测；不新增游戏内重置按钮 |
| NPC-FR08 游戏/角色/事件/能力隔离；SRC-01、SRC-07 | NPC-UC01～07的入口校验与拒绝分支 | A06 `CompanionIdentity/GameCapabilityProfile`与`AbilitySpec`作用域；A04/A10；BR-NPC-01、NFR-NPC-02 | +当前游戏已注册角色；−跨游戏/未知身份、未授权事件/能力均拒绝且不落默认角色：NPC-SCOPE-01 | 待测；真实配置与外发DEC-05 |
| NPC-FR09 建议与实际阶段/终态回执关联；SRC-01、SRC-07 | NPC-UC02/3、NPC-UC04/3、NPC-UC05/3 | A06 `DirectiveExecution/ExecutionReceipt`；A10；BR-NPC-03 | +反馈关联原指令及真实阶段；−重复/乱序/过期/旧局回执不制造已完成：NPC-RECEIPT-01 | 待联调；阶段与终态DEC-03 |

## NFR：质量要求的稳定编号

原Word表09、表11的质量要求按已有职责聚合编号，不把这些目标误记为实测值。精确参数未闭合时，只能验对应结构/故障约束，不能先判性能通过。

| ID、来源与规则位置 | UC/步骤与模型约束 | 正反例与TCID | 结果/未决 |
| --- | --- | --- | --- |
| NFR-GAME-01 非阻塞与可靠交互；SRC-01 §4.2/5.3/5.6；[流程](../02_Design_Doc/GDD/01_GameFlow.md) | G-UC-05～08；A08、A09/A10异步边界；BR-GAME-04/07 | +服务正常/离线同样能结算、重试、菜单、退出；−挂起请求不锁住这些操作：C-08、EXIT-03、BUILD-01 | 待测 |
| NFR-GAME-02 Windows运行性能；SRC-01 §4.2/5.3；[评估方案](../08_RL_Research/03_EvaluationProtocol.md) | G-UC-04、R-UC-03/2～3；A07评估条件/指标 | +冻结机器/图形/地图后测平均、1% low及极端帧时间；−不把PIE或未知硬件的均值当Windows达标：BUILD-01、RL-COMPARE-01 | 30 FPS是项目目标；判据、样本DEC-04 |
| NFR-RL-01 决策与网络延迟；SRC-01表09；[RL契约](../08_RL_Research/01_BossRLContract.md) | R-UC-02/2～3、R-UC-03/2；A09等待与超时、A07指标 | +分别测HTTP往返/策略计算；−超时/丢包进入受控路径且不以超时请求补成功样本：RL-FAULT-01、RL-COMPARE-01 | 0.25/0.20/3是接入配置；统计口径DEC-04，语义DEC-02 |
| NFR-NPC-01 响应目标；SRC-01表09；[服务交界](../02_Design_Doc/TDD/05_NetworkNote.md) | NPC-UC01～05；A10协作/失败，A06标识 | +分路径测战斗P95<3秒、自主P95<1秒、规则路径P95<100毫秒的既有目标；−不混规则与LLM、不同样本/硬件或漏失败后宣布达标：NPC-METRICS-01 | 本轮未测；测量起止/并发/样本DEC-04 |
| NFR-NPC-02 隔离、外发与敏感配置；SRC-01 §5.6，SRC-07；[服务交界](../02_Design_Doc/TDD/05_NetworkNote.md) | NPC-UC01～07；A04外部协作者，A06游戏/角色归属，A10边界；BR-NPC-01/04/06 | +只访问获准角色与允许上下文；−跨域拒绝，解释/日志无密钥，删除范围不冒充外部后端删除：NPC-SCOPE-01、NPC-QUERY-01、NPC-RESET-01、NPC-DATA-01 | 隔离待测；外发/保留DEC-05 |

可维护性、可移植性和可验证性仍保留原Word含义：接口/模型/人格包有版本，地址可配置，规则与实际结果可追踪。其具体约束通过RL-FR02/06、NPC-FR06/08及下列BR映射，不因未单列更多NFR号而删除原质量要求。

## BR：规则、对象约束与边界

编号完整定义在各规则来源，下面仅摘要与追踪。未决部分保留DEC，不由本表批准。

| BRID及规则来源 | UC/步骤；模型职责或不变量 | 正反例/TCID；结果 |
| --- | --- | --- |
| BR-GAME-01 阶段权限与交接；[游戏流程](../02_Design_Doc/GDD/01_GameFlow.md) | G-UC-01～03、05、07/阶段交接；A05/A08 `GameSession` | +跳过/结束状态一致；−菜单/入场/结算的战斗输入不穿透：FLOW-01；待测 |
| BR-GAME-02 唯一终局；同上 | G-UC-04/3、05/1；A05 `Playthrough/CombatResult/BossEncounter` | +指定目标胜负、同帧双方死亡按失败；−普通敌人/重复死亡/终局后事件不得再结算：MATCH-01、MATCH-01a/b；需求DEC-01已确认，运行待测 |
| BR-GAME-03 重试起点与重置；同上、[重试分析](01_RetryObjectAnalysis.md#g-uc-06) | G-UC-06；A05游玩身份/遭遇关系，A08恢复出口 | +恢复可玩即完成、再次入场才建Episode；−恢复失败保持禁战斗，允许再试/回菜单，菜单失败可再试/退出：C-08、C-08a/c/d、EXIT-03；需求DEC-01已确认，运行待测 |
| BR-GAME-04 旧响应与旧状态失效；同上 | G-UC-05～08与R-UC-02/3、6及其异常；A05身份、A08/A09/A10有效性 | +新局独立；−旧局请求/回执、重复委托不污染：C-08b、RL-FAULT-01f、RL-EPISODE-01、NPC-RECEIPT-01；待测 |
| BR-GAME-05 防御分类；[战斗设计](../02_Design_Doc/GDD/03_CombatDesign.md) | G-UC-04/2；A05角色状态/守防压力 | +普通/重击分别防御；−不可防御攻击不能靠两类防御抵消：C-04～06；待测，DEC-04精确窗口 |
| BR-GAME-06 动作合法；同上 | G-UC-04/1；A05 `Player.combatState`与`BossPolicy`，A09共享执行职责；完整条件归规则文字 | +合法启动；−忙碌、死亡、冷却、无效目标拒绝，Accepted≠命中：C-02/03、RL-ACTION-01；待测 |
| BR-GAME-07 合法界面退出；[游戏流程](../02_Design_Doc/GDD/01_GameFlow.md) | G-UC-08/1～2；A08退出 | +菜单/结算退出；−网络挂起及恢复失败出口仍可退出：EXIT-01/02/03；待Windows实测 |
| BR-RL-01 共享执行；[RL契约](../08_RL_Research/01_BossRLContract.md) | R-UC-02/4；A05能力、A07执行反馈、A09 | +BT/PPO同入口；−策略不能直接制造伤害/胜负：RL-ACTION-01；待测 |
| BR-RL-02 请求及遭遇身份；同上 | G-UC-03/3、05/1与R-UC-02/2～3、6及其异常；A07观测/决策归属和策略段，A09 | +关联当前请求/来源段；−旧局、乱序、终局后响应被丢弃：RL-CONTRACT-01、RL-FAULT-01f、RL-EPISODE-01；需求DEC-02已确认，待测 |
| BR-RL-03 故障回退；同上 | R-UC-02/2～3异常、5及其保持BT分支；A09计数/接管，A07来源记录 | +完整合法响应清零、无动作不故障；−通信/协议/非法动作连续3次切BT保持到终局：RL-FAULT-01a～g；需求DEC-02已确认，运行待测、KI-01 |
| BR-RL-04 冻结模型与兼容版本；同上、[训练来源](../08_RL_Research/02_TrainingHistory.md) | R-UC-01/3、02/1；A07模型/部署/选模 | +已选模型兼容并冻结；−缺来源/不兼容不标可部署：RL-MODEL-01、RL-CONTRACT-01；待核查 |
| BR-RL-05 样本隔离与环境分报；[评估方案](../08_RL_Research/03_EvaluationProtocol.md) | R-UC-01/2、03/1～3；A07 `EvaluationRun/PolicySelection`及策略段 | +独测与选模分离，回退独立归组；−900局选模不作独测、回退局不混纯PPO、模拟器不代替UE：RL-COMPARE-01、RL-METRICS-01；待正式采集 |
| BR-NPC-01 角色/能力作用域；[服务交界](../02_Design_Doc/TDD/05_NetworkNote.md) | NPC-UC01～07入口，02/1～3；A06归属/能力引用，A10 | +有技能建议授权、日常可无技能；−跨游戏/默认角色回退/未授权拒绝：NPC-SCOPE-01、NPC-COMMAND-01；待测，DEC-03 |
| BR-NPC-02 事件幂等；同上 | NPC-UC05/1～3；A06事件/关系更新及去重记录 | +首次事件作用一次；−重复ID不重复加关系/发相同反应：NPC-EVENT-01；待测 |
| BR-NPC-03 回执阶段；同上 | NPC-UC02/3、04/3、05/3；A06指令记录/回执，A10 | +原指令关联真实阶段；−Accepted不等于完成，重复/乱序不制造终态：NPC-RECEIPT-01；待测，DEC-03 |
| BR-NPC-04 指定记忆重置；同上 | NPC-UC07/1～2；A06人格、关系与记忆生命周期 | +清理目标记忆保留人格/关系；−持久失败返回失败、其他角色不变：NPC-RESET-01；待测 |
| BR-NPC-05 被动快照与抑制；同上 | NPC-UC04/1～3；A06快照与候选建议 | +心跳返回建议/无动作；−无心跳不主动推送，过场/重复/过期不刷行为：NPC-AUTO-01；待测 |
| BR-NPC-06 外部调用边界；同上 | NPC-UC01/2、03/1～2、06/查询；A04外部协作者、A10边界 | +仅允许内容触发已配置后端；−不可用时受控降级，无密钥外泄，不虚称远端删除：NPC-DATA-01、NPC-CHAT-01、NPC-VOICE-01、NPC-QUERY-01；待规则与运行，DEC-05 |

## 18个UC的来源与修订落点

18个主要UC在[Word修订稿](Word/AesirWarden需求分析文档.docx)第6章均保留原编号，回填步骤绑定异常、成功保证、失败最小保证及验收入口；G-UC-06另做深入对象分析。下表的`TABLE`仅定位历史原件提取，以保留来源链；当前步骤和修订正文以Word修订稿为准。

| 主要UC | 历史原Word定位 | 当前修订稿及模型对应 |
| --- | --- | --- |
| G-UC-01 开始游戏 | §6.2 TABLE12 | A02/A05/A08；重复开始、跳过交接与输入隔离 |
| G-UC-02 探索关卡 | TABLE13 | A02/A05/A08；重复入口不重复建遭遇 |
| G-UC-03 挑战Boss | TABLE14 | 高风险；异常绑定入场/开战步骤、是否建立Episode、失败出口，A08/A09 |
| G-UC-04 进行战斗 | TABLE15 | 高风险；成功/拒绝/继续与终局交接分开，窗口配置DEC-04 |
| G-UC-05 查看结算 | TABLE16 | 修订稿3步；DEC-01已确认同帧双方死亡按失败，唯一终局与结算退出同步A05/A08 |
| G-UC-06 重试 | TABLE17→[本轮详规](01_RetryObjectAnalysis.md#g-uc-06) | 唯一深入样例；候选对象/CRC/不变量/BCE及关系；DEC-01恢复失败合法出口已确认，仍待运行 |
| G-UC-07 返回菜单 | TABLE18 | A08；菜单交互恢复，旧游玩失效 |
| G-UC-08 退出游戏 | TABLE19 | A08；菜单/结算入口与打包进程结束 |
| R-UC-01 训练并选择候选策略 | §6.3 TABLE20 | A03/A07；训练、选模、模型来源和中断 |
| R-UC-02 部署冻结策略并接入战斗 | TABLE21 | 修订稿6步：1部署、2请求、3完整校验、4共享执行、5持续与BT保持、6结束失效；A07/A09；故障计数及段归组DEC-02 |
| R-UC-03 独立对照评估 | TABLE22 | A03/A07；独测隔离、环境分报和原始结果 |
| NPC-UC01 人格化对话 | §6.4 TABLE23 | A04/A06；规则/模型降级、真实记忆及身份 |
| NPC-UC02 文本协作指令 | TABLE24 | A04/A06/A10；能力引用、真实执行与回执DEC-03 |
| NPC-UC03 语音协作入口 | TABLE25 | A04/A10；复用文本约束，不将音频接收当命令有效，DEC-05 |
| NPC-UC04 自主行为心跳 | TABLE26 | A04/A06/A10；被动响应、抑制与无行为结果 |
| NPC-UC05 世界事件 | TABLE27 | A04/A06/A10；幂等、关系/记忆合法更新与回执 |
| NPC-UC06 查询与解释 | TABLE28 | A04/A06；调试人员通路、指定请求与敏感信息边界 |
| NPC-UC07 指定记忆重置 | TABLE29 | A04/A06；部分失败真实报告，保留关系/人格，不自动增加游戏按钮 |

## 结果登记与反向核查

实际执行记录统一填在[检查记录](../06_Test_Doc/01_VerificationMatrix.md#阶段检查与记录)，正式模型接受记录填在[AI建模证据](03_AIModelingEvidence.md)。执行后从TCID回指本页FR/BR/NFR，保留日期、预期与实际差异及日志/录屏；本页不复制每次结果快照。

模型核查时反向抽查：A05游玩/遭遇关系应回到GAME-FR02/06与BR-GAME-02/03/04；A06能力引用应回到NPC-FR02/08与BR-NPC-01；A07样本/来源应回到RL-FR01/05/06与BR-RL-04/05；A08/A09/A10每条关键分支应分别找到UC异常、规则或DEC。找不到来源的关系/分支不能只因工具能画就当既定需求。

未决规则、缺陷及未运行测试分别管理：DEC见[基线](README.md#待决项)，缺陷见[KnownIssues](../06_Test_Doc/02_KnownIssues.md)，未运行结果保持待测。当前追踪建立不等于23条FR已实现或H2已由人工接受。
