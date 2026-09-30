# 目标与范围

> 状态：目标基线；实现状态为 2026-09-29 静态核对，未做本次 PIE/打包测试。

## 答辩交付优先级

| 层级 | 范围 | 目前证据边界 |
| --- | --- | --- |
| P0 | 单 Boss 完整遭遇；BT 与冻结 PPO 使用同一高层 Action、GAS 合法执行器；可追踪 Episode；胜负、结果、重试；Windows 稳定演示 | Boss Action、BT、Observation v4/26 维、9 动作、Telemetry、GameMode 一次性结算源码存在；新工程的端到端 PPO、重试和打包仍未验证 |
| P0 研究 | 版本化 Observation/Action/Reward、训练配置和随机种子、BT 对照、玩家画像、延迟与失败案例 | 旧项目有训练/选模记录；当前工程尚无独立 UE BT–PPO 成组结果 |
| P1 | 主菜单、新游戏/继续、短引导、Boss 入口检查点、表现打磨 | 当前源码/资产未形成可确认的完整闭环；在 P0 稳定后实现 |
| 后续 | 普通敌人、多敌协作 MARL、文本/语音同伴真实 GAS 指令闭环、扩展符文/装备与内容 | 保留设计，不作为本轮完成项 |

## 明确不纳入当前验收

不把模拟器 `RuleBossPolicy` 当作 UE 行为树成绩；不把冻结策略称为在线学习；不以精选视频或胜率单独证明可读性、公平性和多样性。暂不引入逐帧 RL 控制、商业游戏体量内容或无测量依据的自制算法基础设施。

## 变更规则

范围变化记录在本页及 [Changelog](../CHANGELOG.md)；影响架构且难以逆转的决定写入 [Decisions](../02_Design_Doc/TDD/Decisions/README.md)。功能状态只能由当前工程源码/资产及运行证据提升，参见[验证矩阵](../06_Test_Doc/VerificationMatrix.md)。
