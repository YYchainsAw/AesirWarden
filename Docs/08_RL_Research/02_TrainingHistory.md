# 模型来源与训练记录

以下为 2026-09-29 核对的模型快照。训练源码、原始 manifest 和模型保存在独立的 [aesir-ai-service 仓库](https://github.com/YYchainsAw/aesir-ai-service)及其本地模型目录；本页用于定位 UE 接入的候选模型。

## 模型与来源

下列路径均相对 Python 仓库。UE Observation/Action 定义见[跨仓契约](01_BossRLContract.md)。

| 内容 | 来源 | 快照说明 |
| --- | --- | --- |
| 模拟器与奖励 | `rl/boss/sim.py`、`rl/boss/rewards.py` | `boss-sim-005`、`boss-reward-003` |
| 训练产物 | `models/rl/boss/ppo_boss_schema_v4_sim005_seed_{0,1,2}.manifest.json` 及对应模型 | 三组 seed 的配置和产物；训练曲线与复现命令仍需整理 |
| 部署候选 | `models/rl/boss/deployment/boss_policy_schema_v4_sim005.zip` | 来自 seed 1 |
| 选模记录 | 同目录 `boss_policy_schema_v4_sim005.selection.json` | 候选来源、选模条件和哈希 |

2026-09-29 本机重新计算的候选 ZIP SHA-256 与 selection 文件一致：

`dd79ceca0e0f079634364754d1959212b0cdb357439ab9ef8e36d3ba4ed64add`

模型文件不在 UE 仓库。换机器或交付前，补齐模型获取位置、依赖版本、启动/复现命令和训练曲线。

## 选模数据的含义

selection 记录使用 `base_seed=50000`，在 aggressive、defensive、evasive 三种画像上各跑 300 局，共 900 局。Boss 胜率为 0.8433，分画像分别为 0.553、0.977、1.0。这些数字用于候选选择；独立测试需要新的测试集合，UE 中 `BT_AesirBoss` 的表现另行对照。

目前此记录没有 UE 成组对照的原始 Episode 和统计结果。正式实验按[评估方案](03_EvaluationProtocol.md)执行，并保存原始请求/响应、动作结果、Episode、配置和统计输出。

## 更新与归档

新模型接入时更新候选来源，记录模型/模拟器/奖励/Schema 版本及选模条件。正式实验或交付包保存精确产物路径、SHA-256、生成命令、日期和保管位置，原始文件保持原样。

UE 动作执行与故障回退的检查见[开发检查清单](../06_Test_Doc/01_VerificationMatrix.md)，缺陷状态见[已知问题](../06_Test_Doc/02_KnownIssues.md)。
