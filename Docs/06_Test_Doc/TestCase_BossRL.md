# Boss RL 与 BT 对照用例

> 状态：测试设计；2026-09-29 未运行 UE 成组对照。

1. **契约**：同一战局导出 UE 26 维 Observation、请求/响应 ID、Schema、Action ID、Availability 与执行结果；逐索引对照 Python `contract.py`。
2. **合法性**：BT 与 PPO 分别请求全部配置动作；验证目标、眩晕、死亡、忙碌、冷却和能力缺失返回 typed Outcome，不绕过 GAS。
3. **回退**：分别停止服务、超时、返回坏 JSON、错误 Schema、缺失与越界 Action；记录连续失败计数、BT 接管、下一次动作和终局。
4. **成组对照**：运行前固定模型、Boss/地图、玩家画像、种子、局数、顺序和统计方法；相同条件分别执行 BT 与 PPO，保留全部 Episode。
5. **成本**：在 Windows 目标构建记录决策延迟、HTTP 耗时、平均/1% low/极端帧率；区分加载尖峰和战斗期。

结果和原始证据填入[验证矩阵](VerificationMatrix.md)、[证据登记](../08_RL_Research/EvidenceRegister.md)及后续正式报告。选模 900 局不可代替本用例。
