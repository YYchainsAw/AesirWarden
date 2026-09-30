# 训练与选模记录

> 状态：核对独立 Python 仓库现有源码、模型和 selection manifest · 2026-09-29。**本页不是最终评估报告。**

## 当前可定位产物

Python 服务仓库 `aesir-ai-service` 的 `rl/boss/contract.py` 为 Schema v4/26 维/9 动作，`rl/boss/sim.py` 为 `boss-sim-005`，`rl/boss/rewards.py` 为 `boss-reward-003`。模型目录有 sim005 的 seed 0、1、2 三组训练产物和对应 manifest。部署候选 `models/rl/boss/deployment/boss_policy_schema_v4_sim005.zip` 来自 seed 1；selection 文件记载 SHA-256：

`dd79ceca0e0f079634364754d1959212b0cdb357439ab9ef8e36d3ba4ed64add`

2026-09-29 重新计算本机 zip SHA-256 与 selection 文件一致。该本机路径与模型文件目前**不在 AesirWarden 工程仓库**；跨机器复现前需要可移植的获取方式、依赖版本和产物索引。

## 数据边界

selection 记录以 `base_seed=50000` 在 aggressive、defensive、evasive 三画像上各跑 300 局，共 900 局。记录中的 Boss 胜率为 0.8433，分画像分别为 0.553、0.977、1.0。这些数字只用于**候选选择**，不能再用作独立测试，也不能与 UE 中 `BT_AesirBoss` 的成绩直接比较。

待做：冻结未参与训练/调参/选模的测试集合；导出训练曲线与失败样本；在 UE 用同一 Boss/GAS/遭遇规则执行 BT 与 PPO 成组对照；记录延迟、帧时间、拒绝、回退和可读性。方案见[EvaluationProtocol](EvaluationProtocol.md)。
