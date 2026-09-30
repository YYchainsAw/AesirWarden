# 已知问题与限制

> 状态：2026-09-29 静态核对；下表是待处理项，不代表已在新工程运行复现。

| ID | 问题/限制 | 当前依据 | 关闭条件 |
| --- | --- | --- | --- |
| KI-01 | Policy Client 在检查 `action_id` 前调用 `ResetFailures()`；连续缺失/越界动作可能每次重新从 1 计数，达不到阈值 | [源码](../../Source/AesirWarden/Private/AI/Boss/AesirBossPolicyClientComponent.cpp) | 调整逻辑后对两类错误连续注入，确认 BT 接管和 Episode 可结算 |
| KI-02 | GameMode 仅在 BeginPlay 枚举当时存在的敌人；动态生成 Boss 可能不计入胜利判定 | [源码](../../Source/AesirWarden/Private/Framework/AesirCombatPrototypeGameMode.cpp) | 固定场景限制写明，或实现动态登记并运行测试 |
| KI-03 | PPO 完整 UE Episode、同条件 BT 对照和 Windows 性能尚无本轮证据 | [验证矩阵](VerificationMatrix.md) | 完成预定实验，保留原始遥测/配置/统计报告 |
| KI-04 | 结果 Widget 资产存在，但显示、重试、返回与重复委托清理未核验 | [WBP_CombatResult](../../Content/Aesir/UI/Results/WBP_CombatResult.uasset) | PIE 与打包流程连续通过并记录 Episode ID |
| KI-05 | 默认地图指向本地 `/Game/Aesir/Maps/L_OpenWorld`，但地图及其第三方依赖不进入公开仓库；公开克隆不能直接运行 | [DefaultEngine.ini](../../Config/DefaultEngine.ini)及公开资产排除规则 | 在 README/环境指南说明恢复方式；可运行版本另行做 Windows 包验证 |
| KI-06 | 公开仓库只保存代码、文档和经筛选的项目蓝图，完整工程与实验数据仍需独立备份和版本关联 | `.gitignore`、`.gitattributes`、[证据登记](../08_RL_Research/EvidenceRegister.md) | 每次实验记录代码提交、资产快照和模型/遥测版本 |

将来发现的问题先给复现步骤、影响版本、严重度和负责人；修复后关联测试证据，不仅删除行。
