# Python 仓库与跨仓 Issue 审查

> 历史审查记录：现存任务资料按 T1–T30 维护，包含 M1/M2 的 17 项和 NPC 的 13 项 T18–T30。旧批次清单、发布中间记录、删除索引和本地备份已按用户确认移除；正文中的旧文件名仅作历史记录。当前安排见 [M1/M2 清单](07_M1M2Issues.md)、[NPC 当前清单](08_CompanionIssues.md)、[统一 Project](09_GitHubProject.md)和[里程碑](10_GitHubMilestones.md)。

> 2026-10-06。本次为源码/文档审查及一项直接函数边界检查，不表示 HTTP 服务、真实模型、训练、UE 或 Windows 包已通过运行验收。

审查仓库：[YYchainsAw/aesir-ai-service](https://github.com/YYchainsAw/aesir-ai-service)，当前 `main` commit：`1678dd6a467bc2fd9c077124ede527a82958e332`。使用独立临时快照审查，没有修改或推送 Python 代码。

UE 公开 `main` commit：`facba39a3daef0e7904902c92497f84dccaa1654`。本地流程接入指南和部分 develop 实现尚未同步到公开 main；Issue 应明确这一区别，不引用不存在的公开文件链接。

## 事实与对应任务

| 发现 | 证据与边界 | 对应 Issue 追踪编号 |
| --- | --- | --- |
| Boss RL 与 NPC 指令服务分离 | [rl/README.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/rl/README.md)、[SDD v1.1](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/aesir-agent-sdd-v1.1.md) 的 OUT-06；Boss 独立服务已有实现，不从零重复建设 | D05、D13、D14 |
| v4/26 特征/9 动作契约与服务入口存在 | `rl/boss/contract.py`、`inference.py`、`inference_app.py`、`scripts/rl/boss/serve.py`；默认 Boss 端口 8012，冻结 MaskablePPO、GAS 保留执行权 | D05、D13、D14 |
| 公开克隆缺少模型、selection 和训练产物 | [.gitignore](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/.gitignore) 排除 `models/`；默认部署路径依赖外部模型。未获取历史本机模型，不确认其当前 SHA 或可用性 | D01、D10、D13 |
| 模型启动缺少对应 manifest/SHA 校验 | [inference.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/rl/boss/inference.py) 按文件路径加载，健康元数据主要为文件名和路径；需建立可追踪、可移植的版本与启动基线 | D13 |
| 非有限观测缺少明确拒绝 | `make_observation()` 仅比较上下界；直接传入第 0 位为 `float('nan')` 的 26 维向量后仍返回 NaN。HTTP validator 同样未明确 `isfinite`，但本轮未执行 HTTP 请求 | D14 |
| 现有 HTTP 测试使用假模型 | [test_inference.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/tests/rl/boss/test_inference.py) 覆盖正常回显、state_locked、长度错误，不构成真实模型或 UE 联调证据 | D14、D05 |
| 独立 eval CLI 仅支持规则策略 | [eval.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/scripts/rl/boss/eval.py) 没有冻结 PPO 模型参数；PPO 比较目前在训练脚本中，不能用重新训练代替独立测试 | D15、D10 |
| 评估聚合后不导出逐局原始数据 | [evaluation.py](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/rl/boss/evaluation.py) 先构造 EpisodeMetrics 再返回聚合报告，CLI 只输出报告；尚无本轮可归档逐局数据 | D15、D10、D12 |
| 训练脚本已有复现框架 | `train.py` 已记录版本、参数、画像、种子、检查点、TensorBoard 和周期选模；无需重做。正式独立评估仍须与训练/调参/选模种子分离 | D10、D15 |
| Python todo 中部分 UE 任务属于旧工程或后续范围 | [todo.md](https://github.com/YYchainsAw/aesir-ai-service/blob/1678dd6a467bc2fd9c077124ede527a82958e332/docs/planning/todo.md) 的 EXT-01/02 与旧训练记录需按新 UE 工程重核；不把同伴订单闭环或待批准对话 RL 并入本次单 Boss P0 | 后续另批 |

## 首批调整

保留 D01–D12；D10 已在 Python 仓库创建为 `06_AesirAIServiceIssue16.md`（本地备份已由用户移除）。新增 D13 冻结模型交付/启动基线（[#17](https://github.com/YYchainsAw/aesir-ai-service/issues/17)）、D14 HTTP 契约边界与故障样例（[#18](https://github.com/YYchainsAw/aesir-ai-service/issues/18)）、D15 冻结 PPO 独立评估入口和逐局导出（`07_AesirAIServiceIssue19.md`（本地备份已由用户移除））。合计 UE 11 条、Python 4 条；M1 十条、M2 五条，均已通过 `gh` 创建并关联统一 Project。

关键跨仓依赖为 D01 → D13、D02 → D14、D05 → D13/D14、D10 → D09/D15、D12 → D10。箭头在此表示左侧验收依赖右侧完成。D13/D14 为 D05 的跨仓子任务；D15 为 D10 的实现子任务。父任务负责集成/研究验收，子任务负责具体 Python 交付。

上述首批创建记录已按用户确认删除；现行任务和同步记录见 [M1/M2 清单](07_M1M2Issues.md)及[当前任务记录](12_CurrentIssues.json)。运行与实验条件仍需在后续任务中冻结，不填写预测结果或未经执行的通过状态。

## NPC 工作线增补

2026-10-07 按用户确认的分工进一步审查 app/ NPC、最新日志与人格包路线，建立独立 N 系列里程碑和 11 条任务。详细完成程度、直接边界检查和下一步见 [NPC 工作审查](17_CompanionWorkReview.md)，现行 NPC 任务链接见 [NPC 当前清单](08_CompanionIssues.md)。
