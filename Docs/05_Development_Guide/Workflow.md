# 开发工作流

> 状态：团队/个人统一流程 · 2026-09-29

1. 选一个可验证目标，标注 P0/P1/后续，写清预期玩法或实验问题。
2. 查 [GDD/TDD](../02_Design_Doc/TDD/Architecture.md)、接口契约、相关资产和 [KnownIssues](../06_Test_Doc/KnownIssues.md)；影响结构的取舍写 ADR。
3. 创建/修改 C++ 类与模块依赖，编译成功后再派生和配置 Blueprint/GA/GE/输入资产。资产命名按 [AssetNaming](../03_Code_Standard/AssetNaming.md)。
4. 以一条最小端到端路径验证：输入或策略选择 → GAS 合法执行 → 可见反馈 → 遥测/结果。记录实际测试，而非只检查日志或资产存在。
5. 同步文档、测试状态和来源清单；把未完成或回归风险留在 KnownIssues。形成可复现提交后再扩展下一功能。

RL 相关改动还需冻结 Schema、Action、Reward、模型哈希和评估种子；参见[评估方案](../08_RL_Research/EvaluationProtocol.md)。用户通常亲自修改 Blueprint 和代码；文档任务不应附带未授权的游戏逻辑改动。
