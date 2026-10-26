# UML源与预览

2026-10-21按用户要求同步[部署扩展](../02_Design_Doc/TDD/06_Deployment.md)及[NPC交互演进](../02_Design_Doc/TDD/07_NPCInteractionEvolution.md)：A01/A04/A06/A10已通过MCP修改原文件，新增A11部署及A12a/b语音生命周期，均由draw.io Desktop从源逐页导出。新增身份、持久幂等、同步与语音表达需求目标，代码/协议/运行状态仍按两仓正文及检查记录，不把图形完成视为实现验收。

原课程基线为[COURSE-20261018-R1](../09_Course_Analysis/README.md)，本次增加DEP/EVO-20261021目标视图。当前12份本地`.drawio`源共17页：原10份14页保留主题/页名，新增A11一页及A12两页。PNG嵌入可编辑XML，主题数和页数按表达需要选定，不是老师要求固定数量。

先看A08a主流程及A08b恢复子状态机，再看A05及[重试对象分析](../09_Course_Analysis/01_RetryObjectAnalysis.md)理解职责；课程整合内容见[Word需求文档修订副本](../09_Course_Analysis/Word/AesirWarden需求分析文档.docx)。模型表达需求与分析，不表示同名类已实现。用户已退回上一版版式，当前按直线及斜直线为主重排；本次返修的检查、人工语义审查和最终接受状态见[过程证据](../09_Course_Analysis/03_AIModelingEvidence.md)。

## 当前分析模型

图标识使用A01～A12及分页后缀，Word中的阅读图面可由同一源图分段排列。回填[Docs内Word副本](../09_Course_Analysis/Word/AesirWarden需求分析文档.docx)的实际版本、分页与检查记录见[建模记录](../09_Course_Analysis/03_AIModelingEvidence.md)，外部原件保留。UC、BR、对象约束和测试映射见[逐项追踪](../09_Course_Analysis/02_Traceability.md)。

| 模型 / 阅读图号 | 目的与覆盖 | 可编辑源及页 | Desktop预览 |
| --- | --- | --- | --- |
| A01 / 图1 整组用例 | UE、RL与NPC整组边界；23FR与摘要到18主要UC的映射 | [源](src/01_ProjectUseCases.drawio) | [图](png/01_ProjectUseCases.drawio.png) |
| A02 / 图2 游戏用例 | 聚焦G-UC-01～08；退出/重试条件由规则定义。NPC需求仍由A01/A04展示 | [源](src/02_GameUseCases.drawio)，分析模型页 | [图](png/02_GameUseCases.drawio.png) |
| A03a / 图3a RL训练与选模用例 | 训练及候选模型选择，保留include/extend方向与条件 | [源](src/03_BossRLUseCases.drawio)，第1页“训练与选模” | [图](png/03a_BossRLTrainingUseCases.drawio.png) |
| A03b / 图3b RL部署与回退用例 | 冻结模型接入UE、在线决策与故障回退 | 同一[A03源](src/03_BossRLUseCases.drawio)，第2页“部署与回退” | [图](png/03b_BossRLDeploymentUseCases.drawio.png) |
| A03c / 图3c RL独立评估用例 | 独立样本与UE对照，和A03a/b共同覆盖R-UC-01～03 | 同一[A03源](src/03_BossRLUseCases.drawio)，第3页“独立评估” | [图](png/03c_BossRLEvaluationUseCases.drawio.png) |
| A04 / 图4 NPC用例 | NPC-UC01～07；服务局部由UE代玩家发起交互，另含调试人员与可选外部后端。玩家位于A01总览 | [源](src/04_NPCUseCases.drawio)，分析模型页 | [图](png/04_NPCUseCases.drawio.png) |
| A05 / 图5 游戏分析类 | 游玩、遭遇、结果、策略段；单Boss范围下Playthrough—BossEncounter为1对0..1 | [源](src/05_GameAnalysisClasses.drawio) | [图](png/05_GameAnalysisClasses.drawio.png) |
| A06 / 图6 NPC分析类 | 指令内容与执行记录分开；技能条件引用、角色作用域、真实回执和记忆边界 | [源](src/06_NPCAnalysisClasses.drawio) | [图](png/06_NPCAnalysisClasses.drawio.png) |
| A07a / 图7a RL训练与部署分析类 | 训练、产物、选模、冻结部署与评估 | [源](src/07_BossRLAnalysisClasses.drawio)，第1页“训练选模与部署” | [图](png/07a_BossRLTrainingClasses.drawio.png) |
| A07b / 图7b RL遭遇分析类 | 遭遇、策略段及同源决策记录 | 同一[A07源](src/07_BossRLAnalysisClasses.drawio)，第2页“遭遇与决策记录” | [图](png/07b_BossRLEncounterClasses.drawio.png) |
| A08a / 图8a 游戏主流程状态机 | 菜单/开场/探索/入场/战斗/结算；权限与唯一终局 | [源](src/08_GameFlowState.drawio)，第1页“主流程” | [图](png/08a_GameFlowState.drawio.png) |
| A08b / 图8b 恢复子状态机 | 重试与菜单恢复、恢复失败禁战斗及再次尝试/退出 | 同一[A08源](src/08_GameFlowState.drawio)，第2页“恢复子状态机” | [图](png/08b_RecoveryState.drawio.png) |
| A09 / 图9 Boss回退时序 | 合法动作/无动作、错误与超时、连续3次回退、终局与迟到响应 | [源](src/09_BossFallbackSequence.drawio) | [图](png/09_BossFallbackSequence.drawio.png) |
| A10 / 图10 NPC执行时序 | UE校验、拒绝、执行与实际阶段回执、重复/过期/旧局隔离 | [源](src/10_NPCExecutionSequence.drawio) | [图](png/10_NPCExecutionSequence.drawio.png) |
| A11 / 图11 AI部署 | 玩家本机、远程NPC及外部后端；UE世界权威、服务持久状态、同一认证入口与后置TTS | [源](src/11_AIDeployment.drawio)，部署与权威边界页 | [图](png/11_AIDeployment.drawio.png) |
| A12a / 图12a NPC输入与生成 | NPC-UC01/03细化：身份/排队、最终转写、上下文、delta/meta与完整提交边界 | [源](src/12_NPCVoiceLifecycle.drawio)，第1页“输入与生成” | [图](png/12a_NPCInputGeneration.drawio.png) |
| A12b / 图12b NPC播放与打断 | 已采纳后置回复：确认文本、并发合成/有序播放、取消及迟到片段丢弃 | 同一[A12源](src/12_NPCVoiceLifecycle.drawio)，第2页“播放与打断” | [图](png/12b_NPCPlaybackInterruption.drawio.png) |

共同状态：原基线已做同源导出；本次变动的6份源7页再次通过MCP保存，并由Desktop 32.4.1导出和目视检查。Python侧另有4份源5页的对应更新。当前索引共12源17页，源/预览同源检查与Word分页检查分别记录；这些检查不代表用户或同伴已接受全部模型。

用例、类及状态图以直线和斜直线为主，时序消息保持水平。A03/A07/A08通过分页减少交叉，不改变需求覆盖和关系语义。多页文件需在Desktop中逐页导出，不能用旧单页PNG代替；每页预览的文件名见上表。

## 语义与来源边界

- A05只对本课程单Boss关卡声明多重性，不把所有游戏推广为只有一个遭遇；普通关联不承诺结束对象时删除历史记录。
- A06保留DEC-03：具体技能数量未冻结。普通指令可不引用技能，施法必须引用获准技能；0..*配条件约束，不表示所有指令均可零技能。
- A07一条评估运行对应一个冻结策略条件；选模依据仅来自Selection样本，独立样本不反用于本次选模。BT段可无模型部署，回退战局独立归组。
- A08重试成功直接回探索起点，跳过菜单/开场；Boss入场完成才建新遭遇。恢复菜单失败后的“重试”指再次恢复菜单。
- A09阈值3、合法响应清零和BT保持到终局由本轮用户确认；周期/超时沿用契约配置，不是实测延迟。
- A10阶段回执是目标模型，当前协议枚举与目标仍有差异，见[服务交界](../02_Design_Doc/TDD/05_NetworkNote.md)。事实事件幂等键与执行去重键分别处理；有限缓存不承诺跨重启永久恰好一次。

NPC模型依据课程报告和已登记服务版本建立课程视图，不无说明覆盖Python仓库的权威模型。输入版本与未决项见[需求来源](../09_Course_Analysis/README.md#需求来源)。

## EVO-20261021模型同步计划

2026-10-21用户采纳的20项与既有NPC-UC01～07步骤/异常流的对应已写入演进页，随后明确要求修改Word、经MCP更新图源并Desktop导出。以下图形已按目标职责同步；表述的机制仍待代码/协议实施和运行验收。D/V/G/M/C为学习项，不新增20个业务用例，原23FR/18UC及NPC9FR/7UC计数保留。

| 模型 / 实际页 | 同步级别与最小修改 | 对应学习项 / 用例 |
| --- | --- | --- |
| [A06](src/06_NPCAnalysisClasses.drawio)，分析模型 | 已同步目标：认证玩家/存档分支的可变状态归属、共享只读人格与私有记忆/关系、来源/确认/重置世代、交互请求/逻辑命令/回执关联。分析对象不强制映射同名实现类 | D1/D3、G4、M2～M4、C1/C2；NPC-UC01/02/05/06/07 |
| [A10](src/10_NPCExecutionSequence.drawio)，分析时序 | 已同步目标：认证、原结果回放、持久确认与乱序合并；条件登记→本地监听→触发复核GAS/目标/遭遇/TTL；接受≠执行≠命中≠完成 | D2、G1～G4、C1～C3；NPC-UC02/04/05，关联旧局隔离 |
| [A04](src/04_NPCUseCases.drawio)，分析模型 | 已小幅同步：远程/认证前置、最终转写、后置TTS/VAD/打断，仍保留7UC | V1～V4、C1；NPC-UC01/03 |
| [A01](src/01_ProjectUseCases.drawio)，分析模型 | 已小幅同步正式远程NPC/本地Boss边界及EVO范围，仍保留18主要UC | G1、C1；整组范围，不改变Boss研究语义 |
| Python UML-11，服务用例 | 已在Python源表达外部ASR协作与后置TTS，保留UC-01～07 | V1～V4、C1；UE NPC-UC01/03对应服务UC-01/03 |
| Python UML-12a/b，领域对象/对象实例 | 已在Python源表达认证作用域、共享人格/私有状态、来源/版本/重置、稳定命令/回执及跨玩家对象例 | D1/D3、G4、M1～M4、C1～C3 |
| Python UML-14，记忆重置协作 | 已在Python源表达归属、屏障、旧写入拒绝；仍区分现行部分失败，不冒充已实现原子事务 | M3/M4、C1/C3；NPC-UC07/服务UC-07 |
| Python UML-15，指令与回执协作 | 已在Python源表达持久去重/回放、确认、条件触发和乱序合并；现有枚举与目标阶段分开 | G3/G4、C1/C2；NPC-UC02/04/05 |

Python图源仍在该仓`docs/design/uml/src`维护，页名/预览与修改要求见其UML索引；本仓不复制或另存一套源。

本次新增以下目标视图，图源和预览已列入上表，不增加业务UC：

1. A11部署图：玩家本机UE/GAS/BT、Boss推理进程、本地采集/播放/后置VAD，远程NPC认证/API/持久存储与同步，以及外部LLM/ASR/后置TTS。标明回环HTTP、远程HTTPS/SSE、权威位置、同一认证入口和凭据边界，不用Technical历史组件图冒充正式部署图。
2. A12a/b对话/语音生命周期时序：既有UC01/03的细化，表达排队、临时/最终转写、delta/meta、确认文本合成/有序播放、按回复打断及重试/读档后的迟到结果丢弃；生成、播放和动作结果分开。额外状态图未创建，避免机械增加图数。

A02/A05/A08游戏业务主体保持，已有重试/旧身份失效规则只需关联新交互时序；若实际恢复职责变化再局部修订。A03/A07/A09的Boss训练、冻结策略、共享执行与回退语义不因本次NPC演进改变，原DEP本地交付的待同步仍保留。Technical中的GAS及Boss共享执行图继续作为历史参考。

本次已按“后续维护”保存图源并逐页Desktop导出。Desktop 32.4.1的CLI页号从1开始，MCP页索引仍从0开始，不能沿用旧CLI页号导致错页。原14页/18个Word图面是2026-10-19基线历史计数，本次源图为17页，Word回填/分页按实际检查记录。

## 旧图处置

| 旧UE PNG | 本轮去向 |
| --- | --- |
| 01项目、02游戏用例 | 由A01/A02替代；旧版备份已清理，关键来源与修订说明见[建模与修订记录](../09_Course_Analysis/03_AIModelingEvidence.md) |
| 03游戏概念模型 | 由A05及CRC/关系分析替代；旧版备份已清理 |
| 07 RL用例、08 RL概念模型 | 由A03/A07替代；旧版备份已清理 |
| 04组件、05 GAS、06共享执行时序 | 移至[Technical技术参考](Technical/README.md)，正文已更新引用 |

旧PNG没有可核验的原提示与本地可编辑源，本轮不把它们补记为历史A1或人工审查成果。外部原Word及其9张旧内嵌图保留；Docs内副本沿用A01～A10并新增A11/A12及分页后缀，图形和运行证据分开。

## 后续维护

1. 修改规则所属GDD/契约，更新BR/UC与未决登记；影响对象或分支时同步模型与追踪。
2. 通过draw.io MCP修改src相应页，确认已落盘；不能把打开编辑器当成本地已保存。
3. 用draw.io Desktop从该源导出PNG并嵌入XML，保留源；不要只手改预览。
4. 检查关系双向读法、守卫、端点、文字和遮挡；原稿、实际审查与修订证据按真实阶段留存。

Word版本1.2共80页、22个阅读图面，按阅读需要由同源图片分段；逐页检查与实际修订见[建模与修订记录](../09_Course_Analysis/03_AIModelingEvidence.md)。分段不增加模型页，源仍为12份17页；成员语义接受与游戏/服务/实验运行验收仍待完成。
