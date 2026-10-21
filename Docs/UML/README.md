# UML源与预览

2026-10-21采纳的[远程NPC/本地Boss部署扩展](../02_Design_Doc/TDD/06_Deployment.md)另按DEP编号追踪。本图集继续覆盖原课程业务与本机联调分析；玩家/存档认证、外部ASR协作和缓存同步目标未由本次Markdown编辑回填到draw.io/PNG，不能据旧图声称远程方案已完整建模或运行验收。相关目标分析见两仓SDD/部署正文，历史图源和预览保持同源。

本轮基线为[COURSE-20261018-R1](../09_Course_Analysis/README.md)。模型按10个主题保留10份本地`.drawio`源，其中RL用例、RL分析类和游戏状态机分为多页，共14页。页面通过draw.io MCP写入，再由draw.io Desktop逐页导出带XML的`.drawio.png`。主题数和页数按本项目表达需要选定，不是老师要求固定数量。

先看A08a主流程及A08b恢复子状态机，再看A05及[重试对象分析](../09_Course_Analysis/01_RetryObjectAnalysis.md)理解职责；课程整合内容见[Word需求文档修订副本](../09_Course_Analysis/Word/AesirWarden需求分析文档.docx)。模型表达需求与分析，不表示同名类已实现。用户已退回上一版版式，当前按直线及斜直线为主重排；本次返修的检查、人工语义审查和最终接受状态见[过程证据](../09_Course_Analysis/03_AIModelingEvidence.md)。

## 当前分析模型

本轮14页模型已回填[Docs内Word副本](../09_Course_Analysis/Word/AesirWarden需求分析文档.docx)，图标识使用A01～A10及分页后缀；外部原件保留。下表的阅读图号用于索引，具体Word图标识及章节以修订副本为准。UC、BR、对象约束和测试映射见[逐项追踪](../09_Course_Analysis/02_Traceability.md)。

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

共同状态：10份源的14页已通过draw.io MCP保存并由Desktop逐页导出，做过同源、编号、链接与AI目视检查。关键问题、真实反馈和检查范围已合并到[建模与修订记录](../09_Course_Analysis/03_AIModelingEvidence.md)；这些检查不代表用户或同伴已接受全部模型。

用例、类及状态图以直线和斜直线为主，时序消息保持水平。A03/A07/A08通过分页减少交叉，不改变需求覆盖和关系语义。多页文件需在Desktop中逐页导出，不能用旧单页PNG代替；每页预览的文件名见上表。

## 语义与来源边界

- A05只对本课程单Boss关卡声明多重性，不把所有游戏推广为只有一个遭遇；普通关联不承诺结束对象时删除历史记录。
- A06保留DEC-03：具体技能数量未冻结。普通指令可不引用技能，施法必须引用获准技能；0..*配条件约束，不表示所有指令均可零技能。
- A07一条评估运行对应一个冻结策略条件；选模依据仅来自Selection样本，独立样本不反用于本次选模。BT段可无模型部署，回退战局独立归组。
- A08重试成功直接回探索起点，跳过菜单/开场；Boss入场完成才建新遭遇。恢复菜单失败后的“重试”指再次恢复菜单。
- A09阈值3、合法响应清零和BT保持到终局由本轮用户确认；周期/超时沿用契约配置，不是实测延迟。
- A10阶段回执是目标模型，当前协议枚举与目标仍有差异，见[服务交界](../02_Design_Doc/TDD/05_NetworkNote.md)。事实事件幂等键与执行去重键分别处理；有限缓存不承诺跨重启永久恰好一次。

NPC模型依据课程报告和已登记服务版本建立课程视图，不无说明覆盖Python仓库的权威模型。输入版本与未决项见[需求来源](../09_Course_Analysis/README.md#需求来源)。

## 旧图处置

| 旧UE PNG | 本轮去向 |
| --- | --- |
| 01项目、02游戏用例 | 由A01/A02替代；旧版备份已清理，关键来源与修订说明见[建模与修订记录](../09_Course_Analysis/03_AIModelingEvidence.md) |
| 03游戏概念模型 | 由A05及CRC/关系分析替代；旧版备份已清理 |
| 07 RL用例、08 RL概念模型 | 由A03/A07替代；旧版备份已清理 |
| 04组件、05 GAS、06共享执行时序 | 移至[Technical技术参考](Technical/README.md)，正文已更新引用 |

旧PNG没有可核验的原提示与本地可编辑源，本轮不把它们补记为历史A1或人工审查成果。外部原Word及其9张旧内嵌图保留；Docs内修订副本已按当前语义采用14页模型，使用A01～A10及分页后缀登记图标识。

## 后续维护

1. 修改规则所属GDD/契约，更新BR/UC与未决登记；影响对象或分支时同步模型与追踪。
2. 通过draw.io MCP修改src相应页，确认已落盘；不能把打开编辑器当成本地已保存。
3. 用draw.io Desktop从该源导出PNG并嵌入XML，保留源；不要只手改预览。
4. 检查关系双向读法、守卫、端点、文字和遮挡；原稿、实际审查与修订证据按真实阶段留存。

Word副本已回填并做过实际分页检查，过程见[建模与修订记录](../09_Course_Analysis/03_AIModelingEvidence.md)。A05/A06及两张时序图按同源图片裁切为有重叠的上下阅读图面，共18个图面；14页模型源及Desktop导出图没有因此增加或重画。成员语义接受与游戏/服务/实验运行验收仍待完成。
