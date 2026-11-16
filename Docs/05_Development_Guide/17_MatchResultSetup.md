# 第一关目标 Boss 与结果页

对应 [UE #15](https://github.com/YYchainsAw/AesirWarden/issues/15) 和 MATCH-01。当前 Main 使用 `BP_AesirBoss3`，即 2026-10-29 已记录玩家与 Alice 实际交战的实例。该实例带有 Actor Tag `Aesir.MatchBoss`，由 GameMode 在载入后登记。其他敌人的死亡不会判胜。

动态 Boss 可在生成、生命组件就绪后调用 `RegisterMatchBoss`，也可由关卡入口传给 `BeginBossCombat`。切换活目标前调用 `UnregisterMatchBoss`；同一目标可重复登记。GameMode 仅在找到唯一标签目标时自动登记。终局待确认或已发布后拒绝更改登记。

胜败汇总在死亡帧之后完成。若指定 Boss 和玩家在同一帧内死亡，结果为失败。结果固定后，GameMode 停止攻击、移动和 AI 决策，拒绝后续战斗伤害，并用同一结果结束目标 Boss 的 Episode。Main 的本地 BossCombat 与结算可以在 NPC 服务不可用时运行。

结果页沿用 `WBP_CombatResult`。原生父类显示结果，并接管重试、返回菜单与退出按钮。重试和返回菜单均重新载入当前地图。重试在营火起点等待地图准备完成后自动开始并跳过开场，返回菜单停在正常启动菜单。连续重试、状态重建和 Windows cooked 包在 #7 与 #8 单独验收。

## 恢复与复测

Main 的外部 Actor 包来自私有工程快照。恢复旧快照后，在 UE Python 中运行 `Scripts/Editor/setup_main_match_boss.py`，为指定实例补回标签。脚本先核对实例路径和标签冲突，再备份并保存该实例的包。需要迁移旧版结果页时，运行 `Scripts/Editor/setup_match_results.py`。

完成 `AesirWardenEditor Win64 Development` 构建后，从工程根目录执行：

```powershell
py -3.11 Scripts/Boss/verify_match_results.py
```

规则测试在隔离世界运行。两个 Main 场景分别启动游戏进程，调用开始入口，进入指定遭遇，通过 GAS 修改生命触发死亡，再检查结果、输入焦点、唯一结果页和 Episode。每个 Main 场景保存实际渲染截图。运行索引和日志位于 `Saved/Verification/Match/`。

2026-11-16 在 UE 5.8.4 构建并通过六项规则用例、Main 胜利和失败场景。六项规则用例共记录 13 条已知警告，来自测试敌人无武器网格及项目未指定 GameplayCue 搜索路径。两次 Main 都只发布一次结果，重试按钮取得焦点，原 Episode 正确结束，结算后的 GAS 伤害未改变血量。[日志、截图和文件哈希索引](../07_Release/Evidence/MatchResults20261116.json)

离屏截图的文字对比度仍偏低。完整手打战斗、结果页后连续两次重试和返回菜单，以及 Windows 包分别留在 #4、#7 和 #8 验收。
