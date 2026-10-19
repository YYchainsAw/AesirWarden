# 已知问题与限制

> 2026-10-18静态核对。以下是源码发现或交付限制，未在本次运行复现；尚未执行的目标检查见[开发检查清单](01_VerificationMatrix.md)，业务未决项见[课程基线](../09_Course_Analysis/README.md)。

| ID | 问题 / 限制 | 状态与依据 | 处理和复测 |
| --- | --- | --- | --- |
| KI-01 | Policy Client在检查action_id前ResetFailures，连续缺失/越界动作可能每次重新从1计数，达不到阈值 | 待修复，静态发现：[PolicyClient](../../Source/AesirWarden/Private/AI/Boss/AesirBossPolicyClientComponent.cpp) | 对齐BR-RL-03；完成RL-FAULT-01a..e，验证完整合法响应才清零、三连错切BT |
| KI-02 | GameMode只在BeginPlay枚举敌人，动态Boss可能漏计，普通敌人可能影响胜利条件 | 胜利判定待收口：[GameMode](../../Source/AesirWarden/Private/Framework/AesirCombatPrototypeGameMode.cpp) | 明确目标Boss登记，复测MATCH-01及01a/b，覆盖实际生成方式 |
| KI-05 | 默认地图/Game/Aesir/Maps/L_OpenWorld及第三方依赖未完整进入公开仓库，公开克隆不能直接运行 | 已知交付限制：[DefaultEngine.ini](../../Config/DefaultEngine.ini)、[.gitignore](../../.gitignore) | 按[工程速查](../05_Development_Guide/01_ProjectQuickReference.md)恢复资产，BUILD-01另验实际包 |
| KI-06 | EndBossEpisode只结束遥测，不停PolicyClient；后续决策在无活动Episode时会自动新建段，结束记录不等于结束推理 | 静态生命周期风险，需核对Blueprint流程接线：[BossAIController](../../Source/AesirWarden/Private/AI/Boss/AesirBossAIController.cpp) | 对齐BR-RL-02，终局/返回/恢复取消请求并失效身份；MATCH-01b、C-08b、RL-FAULT-01f及RL-EPISODE-01复测 |

完整工程、第三方资产和原始实验数据独立备份，正式构建/实验记录代码提交、资产快照与模型版本。NPC阶段回执、语音上下文和服务模型校验等尚未闭环的目标及协议差异分别保留在[NPC交界](../02_Design_Doc/TDD/05_NetworkNote.md)和[RL契约](../08_RL_Research/01_BossRLContract.md)，不把所有未开发目标登记成已复现Bug。

新增问题写现象、最短复现步骤和影响范围；修复后保留解决日期及实际复测结果。任何本次文档修订都不将以上状态改为“已通过”。
