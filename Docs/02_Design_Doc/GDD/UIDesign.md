# UI 与流程设计

> 状态：目标流程；资产存在不等于接线已验证 · 2026-09-29

## 最小答辩流程

`安全入口 → Boss 战 → Victory/Defeat → 结果页 → Retry/返回`。结果页应显示明确结局和下一步操作；重试恢复玩家/Boss 状态、清除旧 Widget 与事件绑定并创建新的 Episode。菜单、新游戏/继续和持久检查点属于下一层完善，不得作为当前已完成流程展示。

当前可定位 [WBP_CombatResult](../../../Content/Aesir/UI/Results/WBP_CombatResult.uasset)、[WBP_CombatHUD](../../../Content/Aesir/UI/HealthBars/WBP_CombatHUD.uasset)及一次性 [OnCombatEnded](../../../Source/AesirWarden/Private/Framework/AesirCombatPrototypeGameMode.cpp) 结算事件。尚未在此轮通过 PIE 核查 Widget 显示、按钮功能、输入焦点和打包行为。

## 交互原则

- Boss 的不可防御攻击应有独立、可辨认的提示；结果页不能遮挡或误触重试。
- 战斗 HUD 表达玩家生命、Boss 状态和必要的符文冷却；只有功能实际接入后才显示相应模块。
- RL/BT 模式、模型 ID、回退原因属于实验/调试界面或遥测；普通玩家只需要可理解的战斗反馈。
- 后续应补主菜单、设置、检查点无效提示和键鼠/手柄焦点路径；每条流程由[验证矩阵](../../06_Test_Doc/VerificationMatrix.md)追踪。
