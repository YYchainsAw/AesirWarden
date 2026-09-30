# RL 证据登记

> 状态：已定位条目与待补项 · 2026-09-29。路径为仓库相对逻辑路径；Python 服务仓库与 UE 工程目前分开保存。

| ID | 来源 | 当前可核对事实 | 验证级别 / 后续动作 |
| --- | --- | --- | --- |
| RL-C01 | UE `Source/AesirWarden/Public/AI/Boss/AesirBossObservationTypes.h` 与 `Private/.../AesirBossObservationTypes.cpp`；Python `rl/boss/contract.py` | Schema v4、26 特征、9 动作静态一致 | 源码；需同一 UE Episode 的原始请求核对顺序 |
| RL-A01 | UE `AesirBossActionComponent`、`AesirBossDecisionTypes.h` 与 Boss GA 资产 | typed Action/GAS 入口存在 | 源码/资产；各动作运行结果待核 |
| RL-T01 | Python `models/rl/boss/ppo_boss_schema_v4_sim005_seed_{0,1,2}.manifest.json` | 三组 sim005 训练 manifest 存在 | 产物定位；训练曲线/复现命令待归档 |
| RL-M01 | Python `models/rl/boss/deployment/boss_policy_schema_v4_sim005.selection.json` 与 zip | seed 1 候选；SHA-256 与本机 zip 相符；900 局为选模集 | 原始 manifest；独立测试待做 |
| RL-F01 | UE `AesirBossPolicyClientComponent.cpp` | 响应协议检查与 BT 回退入口存在 | 静态发现失败计数问题；故障注入待做 |
| RL-E01 | UE 成组 BT–PPO Episode | 尚无本轮可核验原始数据 | 不建结果结论，待运行后补路径/哈希/报告 |

原始文件保持原样；若复制到发布研究包，记录精确相对路径、SHA-256、生成命令、日期和保管位置。
