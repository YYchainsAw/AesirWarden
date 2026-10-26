# NPC v0.4 跨仓接入基线

2026-10-23对齐：本页记录两仓交接版本、支持边界及UE待办。任务状态以GitHub issue为准；字段、错误码、状态机和完整样例只在服务契约维护。M1完整BT流程、M2本地Boss仍按[开发计划](../../05_Development_Guide/06_DevelopmentRoadmap.md)推进，NPC接入可并行准备。

## 锁定版本与资料

| 仓库 / 内容 | 本次核对基线 | 结论 |
| --- | --- | --- |
| UE `AesirWarden` | `main@ea43abcdee13b9be4e5aebd1775d1ce176974557` | `CompanionChatSubsystem`发送0.3到`/v1/agent/step`，只有限定探索动作接线；未接Bearer、scoped绑定、恢复与持久队列 |
| 服务 `aesir-ai-service` | `main@b641b2f075512f61936de2552884b47af1166051` | DEP2作用域、DEP3服务端请求/回执、DEP4 SQL/Bearer/持久确认与恢复已合入；UE真实执行和正式发布仍待验 |
| 文本入口修复候选 | `a0869b8106196956416d7626c46ae1cbd3b3606b`，本轮分支提交，尚未合并main | 实际文本联调使用包含此修复的服务版本；上行`b641b2f`仍是协议审计基线，不能将候选写为main已合入 |
| Boss发行基础设施 | [服务PR #95](https://github.com/YYchainsAw/aesir-ai-service/pull/95) | 尚未合并到上述main；不能作为锁定服务版本已有功能或第五步完成证据 |
| Boss正式模型 | [服务 #96](https://github.com/YYchainsAw/aesir-ai-service/issues/96) | 仍在研发；开发验证包不代表模型、Windows发行与UE对照完成 |

交接实施或验收时再记录实际两仓提交、游戏构建、后端模式、环境配置标识及模型SHA；本表是文档核对基线，不自动移动到最新main。文本修复候选在Windows适用核心回归为1493 passed、11 skipped（5项MySQL、5项POSIX、1项Whisper），本轮未运行UE；这些结果不作为实际MySQL或UE通过证据。

以下链接固定到本次服务提交，避免读到未来协议后误认为UE已兼容：

- [作用域v0.4契约](https://github.com/YYchainsAw/aesir-ai-service/blob/b641b2f075512f61936de2552884b47af1166051/docs/protocols/npc-scope-contract-v0.4-draft.md)、[请求与执行反馈增量](https://github.com/YYchainsAw/aesir-ai-service/blob/b641b2f075512f61936de2552884b47af1166051/docs/protocols/npc-loop-contract-v0.4.md)、[持久化与恢复增量](https://github.com/YYchainsAw/aesir-ai-service/blob/b641b2f075512f61936de2552884b47af1166051/docs/protocols/npc-persistence-contract-v0.4.md)。
- [scoped开发指南](https://github.com/YYchainsAw/aesir-ai-service/blob/b641b2f075512f61936de2552884b47af1166051/docs/guides/scoped-development.md)、[SQL/Bearer操作与恢复指南](https://github.com/YYchainsAw/aesir-ai-service/blob/b641b2f075512f61936de2552884b47af1166051/docs/guides/persistent-development.md)。
- [golden目录](https://github.com/YYchainsAw/aesir-ai-service/tree/b641b2f075512f61936de2552884b47af1166051/data/golden)中`scoped/`、`loop/`、`recovery/`、`sync/`；完整字段同时核对同提交schema及实际实例OpenAPI，不从本页简表自行拼接请求。
- [服务验证记录](https://github.com/YYchainsAw/aesir-ai-service/blob/b641b2f075512f61936de2552884b47af1166051/docs/logs/verification-records.md)：分别看`scope-20261021`、`loop-20261022`、`persist-20261022`、`schema-gate-20261022`。服务测试、实际MySQL受控演练与UE运行分别记证据。

## 已实现端点及限制

下面所有路径前缀均为`/v1/scoped`，JSON版本显式为`0.4`。升级必须实现新信封和生命周期，不能把0.4字段追加到旧0.3端点。

| 方法 / 路径 | 服务支持与UE接入用途 |
| --- | --- |
| `GET /context`；`POST /runs/bind`、`/runs/end` | 发现服务实例、控制revision和NPC记忆epoch；以CAS显式绑定/结束同档run。普通业务不能隐式激活run |
| `POST /encounters/control` | 显式begin/end，带预期遭遇revision/ID；与UE真实遭遇生命周期对齐 |
| `POST /agent/step` | 文本与规则心跳返回none或directive；首期使用已支持目录的探索follow/wait与已实现即时战术建议 |
| `POST /companion/chat` | 非流式文本，返回mock/llm/fallback来源；不是SSE入口 |
| `POST /world/events`、`/combat/events` | 同版世界事件模型和共同事实身份；不要将同一事实经别名上报两次计分 |
| `POST /requests/cancel`；`GET /requests/{id}`、`/directives/{id}` | 取消、当前实例生命周期及最小反馈查询；查询不返回可执行payload或延长指令有效期 |
| `POST /tactical/executions` | 原directive_id放入receipt.order_id，关联原request/scope/encounter；accepted/executed/rejected/expired/cancelled保持现有含义 |
| `GET /recovery/state`、`/recovery/operations/{id}`、`/recovery/directives/{id}` | SQL自有权威快照或原run历史确认；旧实例绑定可失效，返回结果只供恢复/核对，不驱动动作 |
| `GET /sync/state`；`POST /sync/facts` | SQL权威版本、generation/epoch及真实离线事实分项确认，不上传整份本地关系总分或重放旧动作 |
| `POST /recovery/receipts` | SQL历史回执归档，仅匹配数据库已发布原指令；不补执行、不恢复旧记忆 |
| `GET /console/state`、`/console/memory`；`POST /console/memory/reset` | 有独立权限的调试/管理接口，普通游戏凭据不自动得到管理权限 |

scoped SSE和语音尚未实现，严格模式明确拒绝；Bearer始终禁用legacy，不能为语音或聊天降级到无作用域旧路径。外部ASR V1仍待适配，VAD/TTS/顺序播放与打断后置。条件登记/本地监听尚无完整跨端契约；不满足或未知条件返回`CONDITION_NOT_SUPPORTED`，已满足的已知眩晕条件可按快照产生即时建议。旧`TacticalOrderComponent`存在条件订单结构不代表scoped条件执行已贯通。

## 身份、绑定和时间的客户端职责

1. 将可配置服务地址与Boss本地地址分开。受控联调通过回环/SSH转发；正式HTTPS、独立发布副本、OS隔离与容量门禁仍需第六步验收。服务仍单worker，不因SQL存在启用多worker。
2. 运维按真实玩家及已授权save签发短期Bearer，UE以`Authorization`发送，不把token入库、日志或打包为共享身份。`npc:use`是业务权限；console读取、reset和文档读取分权。没有公网注册/发token接口。角色ID和`party.player`不是玩家凭据。
3. 认证后发现context，保存实际game/player/save/companion、service_instance、control_revision和memory_epoch；显式bind取得run_binding_id。同档所有NPC共享活动run，各NPC有自己的epoch。战斗begin/end还需遭遇CAS。切号/切档/重试/返回先使旧本地回调失效，再按实际生命周期处理控制请求。
4. 请求按对应schema携带绑定、epoch与会话/遭遇关联；session省略时默认run，encounter按战斗关联。需要快照的入口携带当前快照与timing，chat.world_context可选；回执和取消没有world_context/timing，不能通用附加这些字段（extra=forbid）。含timing的逻辑请求首次发送时保存单调时钟，重试保留原request_id、语义载荷、快照采集基准和首次发送时刻，只累计capture_age_ms/request_age_ms。响应`timing.valid_for_ms`从首次发送起算，回包或重试不重置TTL；首次计时丢失的旧响应不得执行。
5. 收到响应先核对版本、归属、instance/binding/epoch/encounter、request/snapshot及剩余预算，再更新当前UI或登记动作；Action/GAS启动前重查目标、能力、距离、资源、冷却与动作锁。按directive_id去重实际启动，服务请求完成不能作为UE已执行证明。
6. 替代意图使用新的逻辑request_id，另发取消控制请求指向旧ID；服务`cancel_requested=false`可能表示已开始提交。UE始终丢弃已作废回调，上游可能仍完成计算或计费。

## 最小文本闭环与恢复验收

先用规则/mock完成受控文本闭环，再启用真实LLM并独立采样。以下是已有检查的执行切片，运行结果仍统一写[验证矩阵](../../06_Test_Doc/01_VerificationMatrix.md)，当前全部UE待验。

| 执行切片 | 必须保存的证据 / 通过边界 | 复用检查 |
| --- | --- | --- |
| 认证与控制 | 两玩家同角色/同名档互不访问；无效/到期/撤销凭据拒绝；discover→CAS bind→begin/end→end可追溯，旧控制不重新激活旧局 | NPC-SCOPE-01、NPC-EVO-06、DEP-UE-04 |
| 文本到动作到回执 | 当前角色的闲聊与合法follow/wait分流正确；相同逻辑重试保持directive身份；UE实际移动/等待后回真实反馈；选择已实现战术能力时才做对应GAS验证 | NPC-CHAT-01、NPC-COMMAND-01、NPC-RECEIPT-01、NPC-EVO-04 |
| 取消、重复与迟到 | 同会话排队/替代可定位；生成中重试/返回后释放旧回包不更新新局；重复建议不重复启动，乱序accepted不倒退终态，冲突终态拒绝 | C-08b、NPC-EVO-01/04/06、DEP-UE-02 |
| 真实拒绝与断网 | 无效目标/能力、过期、冷却、忙碌明确拒绝；NPC断网不阻塞菜单、退出、本地Boss与已实现基础队友行为；unknown不换ID自动重做 | NPC-COMMAND-01、NPC-RECEIPT-01、DEP-UE-02 |
| 重启与权威恢复 | SQL落库后回包前终止进程；重启后重新discover/bind，只读确认原操作/指令，旧payload不执行；缓存按恢复快照收敛 | NPC-EVO-06、DEP-UE-04 |
| 事实与回执队列 | 稳定fact/receipt身份、归属和原run/encounter持久保留；持久确认后按分项移除；STATE_VERSION_CONFLICT先拉权威状态，再用新批次ID保留原事实ID；队列满额/过期可见且退出不等网络 | NPC-EVENT-01、NPC-EVO-05/06、DEP-UE-04/06 |
| reset屏障 | 尚未确认且原epoch已过时的新事实，其记忆分项skipped_reset，关系独立确认且不双计；已经持久确认的重复事实返回原ack，原状态可能仍为recorded/processed，但不再次写入记忆或计分。无活动上下文的not_applied_missing_context不记为恢复成功；旧回执只能归档原事实 | NPC-EVO-05/06、DEP-UE-06 |

202和stored只表示回执记录，不证明启动/命中/动画完成；executed仅UE自报已执行。完整started/hit/completed扩展另定契约。未来条件登记后再执行DEP-UE-03完整场景，不能用即时建议通过代替“断网后本地条件触发”。真实端到端耗时按固定硬件、后端、并发和样本记录，规则/mock与HTTP单点结果不替代SC-011或NFR达标。

## AIRI学习项与现有实现的对应

来源为2026-10-21学习清单，固定AIRI `6876b0dcd94006f1092a2e0bd869ebdcddc7d50c`；完整20项职责仍见[演进页](07_NPCInteractionEvolution.md)。本页分别记录借鉴目标与Aesir已实现子集。

| 学习项 | 服务基线已应用部分 | 当前UE / 后续余项 |
| --- | --- | --- |
| D1/D2/D4 | 请求/上游尝试身份、同会话排队、取消、预算与终态；提交前重验 | UE旧回调作废、真实执行去重和端到端关联；scoped流式尚未实现 |
| G1/G3/G4 | 本地/云端分工、稳定directive、最小回执合并 | 当前世界/GAS复核与真实回执；条件登记/监听另行补齐 |
| C1/C2 | Bearer/存档授权、SQL事务、持久逻辑身份与确认 | UE凭据/缓存接入；正式HTTPS与容量等发布门禁 |
| M2/M4/C3 | 原事实身份、epoch屏障、权威快照/版本、真实事实与历史回执归档 | UE本地队列、分项移除与冲突恢复；通用主张/来源纠错、任意分支读档/跨设备合并未由此完成 |
| D3/G2/C4 | 已有快照/仲裁/节流及新增文本阶段追踪和预算 | 完整来源上下文治理、事件合并及跨ASR/LLM/UE阶段采样继续完善 |
| V1～V4/M1/M3 | 已有ASR抽象、分层记忆和关系规则继续复用 | 外部ASR首期适配；VAD/TTS/打断及索引按需读对照后置；稳定条目纠错与受控遗忘按实际需求补齐 |

这些学习项已进入两仓目标文档，服务端也落实了第一批及部分恢复机制；当前缺口集中在UE消费这些契约并完成真实运行证据。不能据服务测试关闭整组AT-EVO、DEP-UE或M5。

## 跨仓任务关联

进行中、阻塞与完成状态在下列issue维护；本页保留契约和依赖，不另建状态看板。

| 工作包 | issue / 接入边界 |
| --- | --- |
| AIRI 20项学习点汇总追踪 | [服务 #103](https://github.com/YYchainsAw/aesir-ai-service/issues/103)，汇总D1～C4各项已实现子集、跨仓工作包及待验证边界；不替代各issue验收 |
| UE v0.4公共接入基础 | [UE #24](https://github.com/YYchainsAw/AesirWarden/issues/24)，统一配置、凭据、绑定、epoch、时间预算与旧回调作废，供M5各链路复用，不阻塞M1/M2 |
| UE聊天与探索文本 | [UE #19](https://github.com/YYchainsAw/AesirWarden/issues/19)，使用#24公共基础接通当前角色对话和已支持探索建议 |
| UE实际动作及回执 | [UE #20](https://github.com/YYchainsAw/AesirWarden/issues/20)，真实本地动作、执行去重与原指令回执，保持现有五种结果含义 |
| UE事件、离线队列与reset | [UE #21](https://github.com/YYchainsAw/AesirWarden/issues/21)，真实事实/回执持久队列、版本冲突及分项确认，复用SQL恢复契约 |
| UE跨端总验收 | [UE #22](https://github.com/YYchainsAw/AesirWarden/issues/22)，按固定两仓提交保存正常、重复、迟到、故障及恢复证据 |
| 文本入口修复 | [服务 #97](https://github.com/YYchainsAw/aesir-ai-service/issues/97)，候选提交见上表；实际联调记录包含修复的服务提交，尚未合并前不改写main状态 |
| 外部ASR首期 | [服务 #98](https://github.com/YYchainsAw/aesir-ai-service/issues/98)，首期按键输入及正式scoped适配，不能由#56流式ASR/VAD任务代替 |
| DEP6正式发布 | [服务 #99](https://github.com/YYchainsAw/aesir-ai-service/issues/99)，HTTPS、独立发布副本、隔离及真实容量/恢复门禁 |
| 上下文、事件、条件与追踪 | [服务 #100](https://github.com/YYchainsAw/aesir-ai-service/issues/100)，细化D3/G2/G3/C4余项，条件登记须先完成跨端契约 |
| 记忆来源、纠错、按需检索与后续游标 | [服务 #101](https://github.com/YYchainsAw/aesir-ai-service/issues/101)，完整来源/更新与读取对照，真实cursor能力后续设计；当前state_version不冒充消息流游标 |
| legacy SSE缺陷 | [服务 #102](https://github.com/YYchainsAw/aesir-ai-service/issues/102)，独立跟踪旧流式路径，不纳入本轮文本入口修复或推定scoped SSE已支持 |
