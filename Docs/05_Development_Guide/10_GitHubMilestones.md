# GitHub 里程碑与交付目标

> 2026-10-07 · 当前 30 条 Issue，使用简短标题与两仓连续 T 编号。11 个里程碑的标题、目标、状态和截止日期保留；本批更新 4 个 NPC 里程碑的任务范围及实际链接。

**M1 = T1–T8；M2 = T9–T17；M5 = T18–T26；N2 = T27–T29；N3 = T30。** 两仓 Milestone 独立维护，同阶段通过 Project 与原生依赖关联；M3/M4/M6 暂未拆任务。

[M1/M2 任务](07_M1M2Issues.md) · [NPC 当前任务](08_CompanionIssues.md) · [统一 Project](09_GitHubProject.md)。

## AesirWarden · M1 · 完整游玩流程与 BT Boss 基线

[GitHub 里程碑](https://github.com/YYchainsAw/AesirWarden/milestone/1)；实际编号 1；Open 8，Closed 0。

### 目标

利用现有玩家移动、攻击、翻滚和可战斗的 BT Boss，完成从开始到胜败、重试和返回的完整第一关。

### 进入条件与检查点

以当前本地 develop 和资产为基础；菜单至探索的 C++ 已存在，沿用已可玩的玩家/BT 机制。

菜单与探索接线 → Boss 入口与开战 → 胜败结果 → 重试与返回 → Windows 基线

### 完成条件

- [ ] 配置现有流程控制器、菜单、相机、输入与 HUD；自然结束/跳过、重复点击有一致交接。
- [ ] 从第一关起点可到达 Boss；入口只触发一次，开战前不决策/伤害，正式开战时启动 BT 与 Episode。
- [ ] 仅指定 Boss 死亡触发通关，玩家死亡触发失败；终局只结算一次，正确停止决策并显示结果。
- [ ] 重试从第一关开头重新游玩，默认跳过菜单/开场；至少连续两次重试，生命、冷却、敌人、输入、UI、委托和入口状态正确重置。
- [ ] 返回菜单后可再次开始；Windows 包冷启动通过基础操作、BT、胜败、重试、返回流程并保存证据。
- [ ] 归档 UE 代码版本、完整资产快照、构建配置/哈希、日志与工程恢复方法。

### 交付与关闭

BT Windows 基线包、操作/恢复说明、开始/胜败/重试/返回录屏和日志、BT Episode 与版本索引齐备后关闭。

### 范围与协作

先完成可玩闭环，允许占位镜头和表现；正式美术/演出在 M6。PPO、推理服务、训练和故障回退归 M2，玩家新增技能与数值完善归 M4，NPC 接入归 M5。

责任：YYchainsAw 负责 UE 流程与验收。
统一 Project：https://github.com/users/YYchainsAw/projects/2

2026-10-07 按用户确认目标调整。截止日期未设置；全部验收条件待执行。已有实现按当前版本继续，Issue 数量和历史测试数不代替实际交付证据。

### 任务编号与推进顺序

**M1：T1–T8，共 8 项，全部在 UE 仓库。**

T 编号为两仓统一任务顺序，GitHub 原生 Issue 编号保留。可提前准备的工作允许并行，最终验收按原生依赖执行。

- [T1 · 整理 BT 工程与资产恢复说明](https://github.com/YYchainsAw/AesirWarden/issues/1)（AesirWarden#1）
- [T2 · 接通菜单、开场与探索流程](https://github.com/YYchainsAw/AesirWarden/issues/14)（AesirWarden#14）
- [T3 · 验证玩家基础操作](https://github.com/YYchainsAw/AesirWarden/issues/3)（AesirWarden#3）
- [T4 · 接通 Boss 入口与 BT 开战](https://github.com/YYchainsAw/AesirWarden/issues/6)（AesirWarden#6）
- [T5 · 验证 BT Boss 战斗与结算](https://github.com/YYchainsAw/AesirWarden/issues/4)（AesirWarden#4）
- [T6 · 接通胜败判定与结果页](https://github.com/YYchainsAw/AesirWarden/issues/15)（AesirWarden#15）
- [T7 · 接通重试与返回菜单](https://github.com/YYchainsAw/AesirWarden/issues/7)（AesirWarden#7）
- [T8 · 验收 BT Windows 包](https://github.com/YYchainsAw/AesirWarden/issues/8)（AesirWarden#8）

BT 工程恢复和重试可独立推进，PPO 与推理服务在 M2 验收。

## AesirWarden · M2 · Boss 系统完善与 RL 实战闭环

[GitHub 里程碑](https://github.com/YYchainsAw/AesirWarden/milestone/5)；实际编号 5；Open 5，Closed 0。

### 目标

完善现有 Boss 动作、状态和 GAS 执行规则，让 Python 冻结 RL 策略在真实 UE 中完成战斗，故障时由 BT 接管。

### 进入条件与检查点

以 M1 可玩流程为集成基线；Boss 规则盘点、服务样例和训练资料准备可提前。Python 仓库 M2 独立维护服务交付。

Boss 动作/规则收口 → Python 环境与模型一致性 → 真实推理服务 → RL 生命周期与 UE 联调 → 打包及故障回归

### 完成条件

- [ ] 记录九动作和实际 GA 映射、目标/距离、忙碌/受击/死亡、冷却、动作可用掩码及 typed Outcome；补齐约定缺口，未配置动作明确禁用/拒绝。
- [ ] UE 规则与 Python 模拟器/奖励/观测/动作版本对齐，现有候选模型兼容性有依据；变更后需要重训时归档新训练和选模资料。
- [ ] 真实冻结模型服务可复现启动，模型获取、SHA、manifest 和 v4/26/9 兼容性有校验及失败记录。
- [ ] RL 开战、终局、重试与返回对齐 M1；旧请求、定时器和目标不会进入下一局，过场不计 Episode 时长。
- [ ] BT/PPO 使用同一执行器，保存同一次请求/响应、动作执行、伤害和终局，冻结 PPO 完成真实 UE Episode。
- [ ] 修复 KI-01；离线、超时、坏 JSON、协议/Schema 错误和缺失/越界动作有注入证据，三次连续错误触发 BT 接管。
- [ ] Windows 包验证 PPO、BT 回退、胜败、连续重试与返回，保存构建/资产/模型/服务版本。

### 交付与关闭

Boss 动作规则清单、Python M2 交付、真实 PPO Episode、故障与生命周期记录及 Windows 回归齐备后关闭；服务单测不能单独关闭 UE M2。

### 范围与协作

本阶段聚焦现有单 Boss 与冻结推理；新增招式/阶段由盘点明确清单，避免预设额外内容。正式成组实验在 M3；在线学习另行立项。

责任：YYchainsAw 负责 UE Boss 和 Python Boss RL。
统一 Project：https://github.com/users/YYchainsAw/projects/2

2026-10-07 按用户确认目标调整。截止日期未设置；全部验收条件待执行。已有实现按当前版本继续，Issue 数量和历史测试数不代替实际交付证据。

### 任务编号与推进顺序

**M2：T9–T17，共 9 项：UE 5 项，Python 4 项。**

UE 仓库负责 T9、T14–T17；Python 仓库负责 T10–T13。两仓里程碑分别验收各自任务，UE 汇总实际战斗与 Windows 交付。

T 编号为两仓统一任务顺序，GitHub 原生 Issue 编号保留。可提前准备的工作允许并行，最终验收按原生依赖执行。

- [T9 · 完善 Boss 动作与战斗规则](https://github.com/YYchainsAw/AesirWarden/issues/16)（AesirWarden#16）
- [T10 · 同步 UE 与 RL 训练规则](https://github.com/YYchainsAw/aesir-ai-service/issues/29)（aesir-ai-service#29）
- [T11 · 验证模型并整理训练记录](https://github.com/YYchainsAw/aesir-ai-service/issues/30)（aesir-ai-service#30）
- [T12 · 校验模型并启动推理服务](https://github.com/YYchainsAw/aesir-ai-service/issues/17)（aesir-ai-service#17）
- [T13 · 验证 HTTP 接口与故障处理](https://github.com/YYchainsAw/aesir-ai-service/issues/18)（aesir-ai-service#18）
- [T14 · 修复连续失败计数与 BT 回退](https://github.com/YYchainsAw/AesirWarden/issues/2)（AesirWarden#2）
- [T15 · 接通 RL 开战、结束与重试](https://github.com/YYchainsAw/AesirWarden/issues/17)（AesirWarden#17）
- [T16 · 验证 UE 与 RL 模型完整战斗](https://github.com/YYchainsAw/AesirWarden/issues/5)（AesirWarden#5）
- [T17 · 验收 RL 与回退 Windows 包](https://github.com/YYchainsAw/AesirWarden/issues/18)（AesirWarden#18）

## AesirWarden · M3 · RL 对照评估与复现证据

[GitHub 里程碑](https://github.com/YYchainsAw/AesirWarden/milestone/2)；实际编号 2；Open 0，Closed 0。

### 目标

回答冻结 PPO 与 UE BT 的战况响应、行为差异和运行成本，交付可复核、可复现的研究结果。

### 进入条件与检查点

M1/M2 的选定战斗构建稳定后正式采集；方案和工具可提前准备。

冻结方案/版本 → 独立模拟器测试 → UE 同条件采集 → 统计与失败案例 → 复现归档

### 完成条件

- [ ] 正式运行前固定代码、模型 SHA、资产、机器、数值、技能、初态、画像、种子、每条件局数、顺序及排除/超时规则。
- [ ] 冻结战况响应性、行为多样性和性能指标；Windows 30 FPS 目标对应硬件和判断方式在采集前登记。
- [ ] 使用未参与训练/调参/选模的独立数据；历史 900 局仅作为选模集，模拟器 RuleBossPolicy 结果单独标识。
- [ ] UE BT/PPO 在相同 Boss/GAS/场地/画像/结算规则下完成预定条件，保存所有成功、失败、超时和排除 Episode。
- [ ] 由原始数据生成胜负区间、时长、伤害、动作/重复、拒绝/回退、延迟、帧表现、分画像结果及失败案例。
- [ ] 归档原始数据、manifest、模型、分析命令、报告、训练资料与版本索引，更新验证矩阵及证据登记。

### 交付与关闭

实验按预定方案执行，结果和限制可复现后关闭；改善有限或未发现改善也是有效结论。

### 范围与协作

默认单玩家对单 Boss。M4/M5/M6 改变研究条件时保持冻结研究版本或重采受影响条件；没有体验研究时不据胜率宣称公平性已证明。

责任：YYchainsAw 负责 UE 对照、总体分析和 Python RL 研究。
统一 Project：https://github.com/users/YYchainsAw/projects/2

2026-10-07 按用户确认目标调整。截止日期未设置；全部验收条件待执行。已有实现按当前版本继续，Issue 数量和历史测试数不代替实际交付证据。

### 当前任务安排

保留本阶段目标与验收边界。用户批准先聚焦 M1/M2，后期已有 Issue 已导出备份后删除；正式推进本阶段时再按实际剩余工作拆分。

## AesirWarden · M4 · 玩家数值、技能与手感

[GitHub 里程碑](https://github.com/YYchainsAw/AesirWarden/milestone/6)；实际编号 6；Open 0，Closed 0。

### 目标

完善玩家数值、约定技能和操作手感，形成规则一致、可回归的玩家战斗系统。

### 进入条件与检查点

沿用现有移动、攻击、翻滚、属性和 GAS 机制；先盘点并冻结本阶段技能与数值清单。

机制盘点 → 数值/技能范围 → 实现与接线 → 手感调整 → 战斗回归

### 完成条件

- [ ] 属性初值、伤害/减伤、冷却规则及来源可查；AttributeSet、GE、旧组件、UI 和重试重置一致。
- [ ] 约定技能的输入、目标、效果、冷却、取消/受击、动画和反馈逐项实现。
- [ ] 攻击连段缓冲、翻滚衔接、命中/无敌窗口、锁定和相机经过实测调整，保存参数与具体问题前后记录。
- [ ] BT/PPO 场景的伤害、防御、死亡、胜负和重试回归通过；说明对冻结模型和实验版本的影响。

### 交付与关闭

明确数值规则、约定技能清单、实际回归及手感问题处理记录齐备后关闭。

### 范围与协作

不把“补齐所有技能”作为无限范围。成长、装备、持久存档和新增资源机制待需求明确后另行设计，可与 M2 协调推进。

责任：YYchainsAw 负责 UE 玩家系统。
统一 Project：https://github.com/users/YYchainsAw/projects/2

2026-10-07 按用户确认目标调整。截止日期未设置；全部验收条件待执行。已有实现按当前版本继续，Issue 数量和历史测试数不代替实际交付证据。

### 当前任务安排

保留本阶段目标与验收边界。用户批准先聚焦 M1/M2，后期已有 Issue 已导出备份后删除；正式推进本阶段时再按实际剩余工作拆分。

## AesirWarden · M5 · AI 队友聊天与战斗联调

[GitHub 里程碑](https://github.com/YYchainsAw/AesirWarden/milestone/4)；实际编号 4；Open 4，Closed 0。

### 目标

让一个 AI 队友在当前 UE 工程中聊天、响应指令、实际参与战斗并反馈执行结果。

### 进入条件与检查点

复用现有 Companion/BT、HTTP 子系统和 Python NPC 服务；协议、端口及真实能力清单先对齐。

协议与能力目录 → 聊天/探索 → 战斗指令执行 → 执行回执 → 全流程回归

### 完成条件

- [ ] 启用一个 NPC，角色 ID、协议、端口和真实 UE 能力清单明确；未实现技能明确拒绝。
- [ ] 文本聊天、对话 UI 焦点、探索跟随/等待有效；基础语音记录实际服务条件和降级。
- [ ] 实际目标与约定技能执行有效；条件触发、取消/替换、冷却、死亡和遭遇结束有明确处理。
- [ ] 请求/指令 ID、UE 校验、执行、完成/失败/取消和单条/批量回执串联，接受不等于完成。
- [ ] Python M5 服务证据可引用，故障降级与胜败/重试/返回后旧命令清理通过真实 UE 回归。

### 交付与关闭

当前构建的真实聊天和战斗演示、服务验收、执行回执与版本证据齐备后关闭。

### 范围与协作

YYchainsAw 负责 UE 同伴与接入，队友负责 Python NPC 服务。N2/N3 的人格包/跨游戏扩展独立推进，M5 不阻塞 M1/M2/M3。

责任：YYchainsAw 主责 UE，dyh 支持 Python NPC 联调。
统一 Project：https://github.com/users/YYchainsAw/projects/2

2026-10-07 按用户确认目标调整。截止日期未设置；全部验收条件待执行。已有实现按当前版本继续，Issue 数量和历史测试数不代替实际交付证据。

### 任务编号与推进顺序

**M5：T18–T26，共 9 项：Python 5 项，UE 4 项。**

本仓负责 T23–T26（UE 接入与实机验收）；Python 仓负责 T18–T22（服务交付）。最终以 T22 汇总服务验收、T26 汇总真实 UE 联调。

M5 不作为 Boss M1/M2 的前置，队友可并行推进；实际 UE 联调使用 T8 的 BT Windows 基线，不等待 RL、N2 或 N3。

T 编号为统一 Project 任务编号，GitHub 原生 Issue 编号保留。依赖约束最终验收，准备工作可并行；已有实现与本次验收证据分别记录。

- [T18 · 对齐 NPC 协议与联调配置](https://github.com/YYchainsAw/aesir-ai-service/issues/31)（aesir-ai-service#31）

- [T19 · 修复 NPC 指标漏采](https://github.com/YYchainsAw/aesir-ai-service/issues/32)（aesir-ai-service#32）

- [T20 · 验证 NPC 状态与时间边界](https://github.com/YYchainsAw/aesir-ai-service/issues/33)（aesir-ai-service#33）

- [T21 · 提供 NPC 联调与故障样例](https://github.com/YYchainsAw/aesir-ai-service/issues/34)（aesir-ai-service#34）

- [T22 · 验收 NPC 服务与人格表现](https://github.com/YYchainsAw/aesir-ai-service/issues/35)（aesir-ai-service#35）

- [T23 · 接通队友聊天与探索指令](https://github.com/YYchainsAw/AesirWarden/issues/19)（AesirWarden#19）

- [T24 · 接通队友战斗与执行回执](https://github.com/YYchainsAw/AesirWarden/issues/20)（AesirWarden#20）

- [T25 · 接通队友事件与流程重置](https://github.com/YYchainsAw/AesirWarden/issues/21)（AesirWarden#21）

- [T26 · 验收队友聊天与战斗联调](https://github.com/YYchainsAw/AesirWarden/issues/22)（AesirWarden#22）

## AesirWarden · M6 · 综合演示与交付

[GitHub 里程碑](https://github.com/YYchainsAw/AesirWarden/milestone/3)；实际编号 3；Open 0，Closed 0。

### 目标

整合完整第一关、约定玩家技能、RL Boss 和 AI 队友，交付稳定、可启动和可展示的 Windows 版本。

### 进入条件与检查点

冻结本次演示选择的 M1/M2/M4/M5 交付范围与版本，保留 M3 的研究构建和证据。

演示范围/版本 → 正式表现 → Windows 集成回归 → 发布资料与证据

### 完成条件

- [ ] 正式菜单、必要开场/入场/收尾、短引导、UI/音画完成；自然结束和跳过的控制权一致。
- [ ] 同一 Windows 包可演示完整流程、约定玩家技能、RL Boss 和 NPC，胜败、连续重试与返回通过。
- [ ] 两条 Python 服务的启动、模型/人格资料和离线行为可复现，目标机器的性能按冻结判据复核。
- [ ] 包、两仓 commit、完整资产快照、模型哈希、依赖、演示视频、来源/许可、启动说明和 M3 研究资料有对应索引。

### 交付与关闭

明确版本的最终演示包、操作说明、集成回归与成果索引齐备后关闭。

### 范围与协作

M1/M2/M3 可先独立交付。正式表现改变研究条件时保留研究版本，或以新版本补验证并重采受影响条件。

责任：YYchainsAw 负责 UE 整合，Python 两条线配合。
统一 Project：https://github.com/users/YYchainsAw/projects/2

2026-10-07 按用户确认目标调整。截止日期未设置；全部验收条件待执行。已有实现按当前版本继续，Issue 数量和历史测试数不代替实际交付证据。

### 当前任务安排

保留本阶段目标与验收边界。用户批准先聚焦 M1/M2，后期已有 Issue 已导出备份后删除；正式推进本阶段时再按实际剩余工作拆分。

## aesir-ai-service · M2 · Boss RL 训练、模型与推理联调基线

[GitHub 里程碑](https://github.com/YYchainsAw/aesir-ai-service/milestone/1)；实际编号 1；Open 4，Closed 0。

### 目标

为 UE M2 提供规则匹配、可移植且可追踪的 Boss RL 训练产物和真实推理服务。

### 进入条件与检查点

复用现有 rl/boss、训练脚本、三种子 sim005 产物和独立 8012 服务，准备工作可与 UE 流程并行。

规则/版本对齐 → 候选兼容与训练资料 → 模型交付校验 → 服务与故障样例 → UE 联调支持

### 完成条件

- [ ] 动作语义、伤害/防御/冷却、时间尺度、观测和掩码与 UE 选定规则一致；差异与 simulation/reward/schema 修订可追踪。
- [ ] 既有候选兼容性有依据；需重训时保留训练参数、种子、检查点、选模集与模型 manifest，不复用旧模型冒充新规则策略。
- [ ] 模型获取、SHA、shape/action space/manifest 校验与失败原因明确；真实服务可按说明启动，正常及 state_locked 响应有记录。
- [ ] 共享观测与 HTTP 覆盖有限值、维度、Schema、标识回显和动作边界。
- [ ] 故障样例覆盖断连、超时、坏 JSON、协议/Schema 和缺失/越界动作，可供 UE 验证三次失败后 BT 接管。

### 交付与关闭

规则对齐记录、可访问模型/训练索引、真实服务记录与故障样例交付后关闭仓库里程碑；UE M2 仍需真实 Episode 和 Windows 验收。

### 范围与协作

Boss RL 保持与 app/ NPC 服务分离；正式独立测试和成组研究归 M3，沿用离线训练与冻结推理。

责任：YYchainsAw 负责 Python Boss RL。
统一 Project：https://github.com/users/YYchainsAw/projects/2

2026-10-07 按用户确认目标调整。截止日期未设置；全部验收条件待执行。已有实现按当前版本继续，Issue 数量和历史测试数不代替实际交付证据。

### 任务编号与推进顺序

**M2：T9–T17，共 9 项：UE 5 项，Python 4 项。**

UE 仓库负责 T9、T14–T17；Python 仓库负责 T10–T13。两仓里程碑分别验收各自任务，UE 汇总实际战斗与 Windows 交付。

T 编号为两仓统一任务顺序，GitHub 原生 Issue 编号保留。可提前准备的工作允许并行，最终验收按原生依赖执行。

- [T9 · 完善 Boss 动作与战斗规则](https://github.com/YYchainsAw/AesirWarden/issues/16)（AesirWarden#16）
- [T10 · 同步 UE 与 RL 训练规则](https://github.com/YYchainsAw/aesir-ai-service/issues/29)（aesir-ai-service#29）
- [T11 · 验证模型并整理训练记录](https://github.com/YYchainsAw/aesir-ai-service/issues/30)（aesir-ai-service#30）
- [T12 · 校验模型并启动推理服务](https://github.com/YYchainsAw/aesir-ai-service/issues/17)（aesir-ai-service#17）
- [T13 · 验证 HTTP 接口与故障处理](https://github.com/YYchainsAw/aesir-ai-service/issues/18)（aesir-ai-service#18）
- [T14 · 修复连续失败计数与 BT 回退](https://github.com/YYchainsAw/AesirWarden/issues/2)（AesirWarden#2）
- [T15 · 接通 RL 开战、结束与重试](https://github.com/YYchainsAw/AesirWarden/issues/17)（AesirWarden#17）
- [T16 · 验证 UE 与 RL 模型完整战斗](https://github.com/YYchainsAw/AesirWarden/issues/5)（AesirWarden#5）
- [T17 · 验收 RL 与回退 Windows 包](https://github.com/YYchainsAw/AesirWarden/issues/18)（AesirWarden#18）

## aesir-ai-service · M3 · 独立模拟器评估与 RL 复现证据

[GitHub 里程碑](https://github.com/YYchainsAw/aesir-ai-service/milestone/2)；实际编号 2；Open 0，Closed 0。

### 目标

为 UE M3 研究提供冻结 PPO 独立评估工具、逐 Episode 数据及训练/选模复现资料。

### 进入条件与检查点

工具可提前准备，正式运行等待 UE 冻结方案与适用模型完成。

独立评估入口 → 原始导出 → 独立测试 → 训练证据归档

### 完成条件

- [ ] 规则策略和已冻结 PPO 可通过独立 CLI 评估，无需重新训练，支持画像、种子与输出目录。
- [ ] 逐 Episode 原始数据、失败/超时/排除和 manifest 可回查，聚合可重算。
- [ ] 不复用训练、调参或历史 900 局选模数据，按冻结方案完成独立模拟器测试。
- [ ] 现有训练配置、模型 SHA、manifest、复现命令、曲线/检查点和失败样本有保管索引。

### 交付与关闭

工具、独立数据和复现资料可核验后关闭；供 UE 总体报告引用。

### 范围与协作

模拟器结果不代替 UE BT 实战；不以胜率提高作为关闭条件。

责任：YYchainsAw 负责 Python Boss RL。
统一 Project：https://github.com/users/YYchainsAw/projects/2

2026-10-07 按用户确认目标调整。截止日期未设置；全部验收条件待执行。已有实现按当前版本继续，Issue 数量和历史测试数不代替实际交付证据。

### 当前任务安排

保留本阶段目标与验收边界。用户批准先聚焦 M1/M2，后期已有 Issue 已导出备份后删除；正式推进本阶段时再按实际剩余工作拆分。

## aesir-ai-service · M5 · NPC 服务基线与 UE 聊天、战斗联调支持

[GitHub 里程碑](https://github.com/YYchainsAw/aesir-ai-service/milestone/3)；实际编号 3；Open 5，Closed 0。

### 目标

冻结现有 NPC 服务基线，为 UE M5 聊天、战斗执行和回执联调提供协议、样例与验收证据。

### 进入条件与检查点

人格、记忆、关系、事件、自主行为和降级已有实现，从当前 commit 的基线与剩余边界继续。

服务/协议基线 → 指标与边界 → 故障样例 → 服务验收 → 真实 UE 联调支持

### 完成条件

- [ ] 当前 NPC 功能按当前 commit 验收，原始证据、未测边界和历史测试数量版本有说明。
- [ ] 协议 0.3、端口、真实 UE 能力目录、指令和单条/批量回执对齐。
- [ ] 修正 game_id 分层指标漏采，核验场景、节流、关系冷却和时间跳变。
- [ ] 交付联调样例、故障回放和服务证据，支持 UE 的实际技能执行与回执；Mock 不记作 UE 通过。

### 交付与关闭

服务基线、边界回归和 UE 联调支持资料齐备后关闭仓库阶段；整个 M5 还需 UE 实际演示。

### 范围与协作

用户负责 UE 同伴，队友负责 Python NPC。N2/N3 独立服务扩展保持原路线，Boss RL 由用户负责。

责任：dyh 主责 Python NPC，YYchainsAw 负责 UE 接入。
统一 Project：https://github.com/users/YYchainsAw/projects/2

2026-10-07 按用户确认目标调整。截止日期未设置；全部验收条件待执行。已有实现按当前版本继续，Issue 数量和历史测试数不代替实际交付证据。

### 任务编号与推进顺序

**M5：T18–T26，共 9 项：Python 5 项，UE 4 项。**

本仓负责 T18–T22（NPC 服务）；UE 仓负责 T23–T26（游戏接入与实机验收）。最终以 T22 汇总服务验收、T26 汇总真实 UE 联调。

M5 不作为 Boss M1/M2 的前置，队友可并行推进；实际 UE 联调使用 T8 的 BT Windows 基线，不等待 RL、N2 或 N3。

T 编号为统一 Project 任务编号，GitHub 原生 Issue 编号保留。依赖约束最终验收，准备工作可并行；已有实现与本次验收证据分别记录。

- [T18 · 对齐 NPC 协议与联调配置](https://github.com/YYchainsAw/aesir-ai-service/issues/31)（aesir-ai-service#31）

- [T19 · 修复 NPC 指标漏采](https://github.com/YYchainsAw/aesir-ai-service/issues/32)（aesir-ai-service#32）

- [T20 · 验证 NPC 状态与时间边界](https://github.com/YYchainsAw/aesir-ai-service/issues/33)（aesir-ai-service#33）

- [T21 · 提供 NPC 联调与故障样例](https://github.com/YYchainsAw/aesir-ai-service/issues/34)（aesir-ai-service#34）

- [T22 · 验收 NPC 服务与人格表现](https://github.com/YYchainsAw/aesir-ai-service/issues/35)（aesir-ai-service#35）

- [T23 · 接通队友聊天与探索指令](https://github.com/YYchainsAw/AesirWarden/issues/19)（AesirWarden#19）

- [T24 · 接通队友战斗与执行回执](https://github.com/YYchainsAw/AesirWarden/issues/20)（AesirWarden#20）

- [T25 · 接通队友事件与流程重置](https://github.com/YYchainsAw/AesirWarden/issues/21)（AesirWarden#21）

- [T26 · 验收队友聊天与战斗联调](https://github.com/YYchainsAw/AesirWarden/issues/22)（AesirWarden#22）

## aesir-ai-service · N2 · S2–S4 人格包与跨游戏最小接入

[GitHub 里程碑](https://github.com/YYchainsAw/aesir-ai-service/milestone/4)；实际编号 4；Open 3，Closed 0。

### 目标

收口已有 S2，实现人格包迁移、S3 游戏档案/canonical schema 和 S4 L0/L3 自有跨游戏接入。

### 进入条件与检查点

沿用队友已登记 S2–S4 路线，复用现有人格包 loader/validator。

S2 迁移/归属 → S3 档案/schema → S4 能力门控 → 自有样例验收

### 完成条件

- [ ] 迁移 Alice/Bruno 配置，校验角色唯一性、未知能力等级拒绝和冲突报告。
- [ ] 游戏档案、canonical schema、capability/directive 子集与版本化样例完成，一进程一游戏。
- [ ] L0 只对话/记忆/关系，L3 按游戏能力启用；不足时明确降级/拒绝。
- [ ] 自有 demo-vn/narrator 验证按配置新增角色/游戏，状态和产物按游戏/角色隔离。

### 交付与关闭

自有人格包与最小跨游戏复用证据齐备后关闭。

### 范围与协作

独立服务扩展，Project 阶段 N2；与 UE M5 可并行，不作为 Boss M1/M2/M3 前置。

责任：dyh 负责 Python NPC 服务扩展。
统一 Project：https://github.com/users/YYchainsAw/projects/2

2026-10-07 按用户确认目标调整。截止日期未设置；全部验收条件待执行。已有实现按当前版本继续，Issue 数量和历史测试数不代替实际交付证据。

### 任务编号与推进顺序

**N2：T27–T29，共 3 项，全部在 Python 仓库。**

围绕现有 S2 基础完成自有人格包与 L0/L3 最小跨游戏服务；本阶段不要求完成第二款游戏。

T 编号为统一 Project 任务编号，GitHub 原生 Issue 编号保留。依赖约束最终验收，准备工作可并行；已有实现与本次验收证据分别记录。

- [T27 · 完成人格包迁移与校验](https://github.com/YYchainsAw/aesir-ai-service/issues/36)（aesir-ai-service#36）

- [T28 · 发布游戏档案与协议 Schema](https://github.com/YYchainsAw/aesir-ai-service/issues/37)（aesir-ai-service#37）

- [T29 · 接通 L0/L3 跨游戏服务](https://github.com/YYchainsAw/aesir-ai-service/issues/38)（aesir-ai-service#38）

## aesir-ai-service · N3 · S5 外部人格包守卫与接入门禁

[GitHub 里程碑](https://github.com/YYchainsAw/aesir-ai-service/milestone/5)；实际编号 5；Open 1，Closed 0。

### 目标

在外部人格包接入前完成 S5 数据校验、实际 checksum 和可追踪的审核门禁。

### 进入条件与检查点

保留 N2 自有包基线，外部包接入前完成本阶段。

lint/冲突检查 → checksum 验证 → 审核门禁 → 正常/异常回归

### 完成条件

- [ ] schema、白名单/配额、注入样例、归属与版本冲突 lint 有可追踪报告。
- [ ] checksum 实际计算验证，修改/损坏包明确拒绝。
- [ ] 包版本、哈希、冲突报告和实际审核结果留存，未经通过的外部包不加载。
- [ ] 正常自有包、异常包和降级通过回归，保留 N2 可复现基线。

### 交付与关闭

外部包校验和审核门禁有实际证据后关闭。

### 范围与协作

独立服务扩展，Project 阶段 N3；门禁是外部包接入条件，不扩展当前 Boss 交付范围。

责任：dyh 负责 Python NPC 服务扩展。
统一 Project：https://github.com/users/YYchainsAw/projects/2

2026-10-07 按用户确认目标调整。截止日期未设置；全部验收条件待执行。已有实现按当前版本继续，Issue 数量和历史测试数不代替实际交付证据。

### 任务编号与推进顺序

**N3：T30，共 1 项，在 Python 仓库。**

沿用现有 S5 目标，补齐 checksum、lint 与审核加载门禁；前置为 T29，不扩展在线平台。

T 编号为统一 Project 任务编号，GitHub 原生 Issue 编号保留。依赖约束最终验收，准备工作可并行；已有实现与本次验收证据分别记录。

- [T30 · 完成外部人格包校验](https://github.com/YYchainsAw/aesir-ai-service/issues/39)（aesir-ai-service#39）
