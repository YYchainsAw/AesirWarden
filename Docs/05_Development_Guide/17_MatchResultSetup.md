# 第一关目标 Boss 与结果页

对应 [UE #15](https://github.com/YYchainsAw/AesirWarden/issues/15) 和 MATCH-01。当前 Main 使用 `BP_AesirBoss3`，即 2026-10-29 已记录玩家与 Alice 实际交战的实例。该实例带有 Actor Tag `Aesir.MatchBoss`，由 GameMode 在载入后登记。其他敌人的死亡不会判胜。

动态 Boss 可在生成、生命组件就绪后调用 `RegisterMatchBoss`，也可由关卡入口传给 `BeginBossCombat`。切换活目标前调用 `UnregisterMatchBoss`；同一目标可重复登记。GameMode 仅在找到唯一标签目标时自动登记。终局待确认或已发布后拒绝更改登记。

胜败汇总在死亡帧之后完成。若指定 Boss 和玩家在同一帧内死亡，结果为失败。结果固定后，GameMode 停止攻击、移动和 AI 决策，拒绝后续战斗伤害，并用同一结果结束目标 Boss 的 Episode。Main 的本地 BossCombat 与结算可以在 NPC 服务不可用时运行。

结果页沿用 `WBP_CombatResult`。原生父类显示结果，并接管重试、返回菜单与退出按钮。重试和返回菜单均按当前地图的完整包名重新载入。重试在营火起点等地图准备完成后直接进入探索，返回菜单恢复营火视角、坐姿和按钮焦点。Main 的目标 Boss 等遭遇入口接受后才启动 BT 和创建 Episode。

重试等待期间锁定玩家控制。起点准备超时且菜单可用时回到营火菜单，同一世界收到引擎的载图失败通知时允许从原结果页再试。Windows cooked 包由 #8 验收。

## 恢复与复测

Main 的外部 Actor 包来自私有工程快照。恢复旧快照后，在 UE Python 中运行 `Scripts/Editor/setup_main_match_boss.py`，为指定实例补回标签。脚本先核对实例路径和标签冲突，再备份并保存该实例的包。需要迁移旧版结果页时，运行 `Scripts/Editor/setup_match_results.py`。

完成 `AesirWardenEditor Win64 Development` 构建后，从工程根目录执行：

```powershell
py -3.11 Scripts/Boss/verify_match_results.py
```

规则测试在隔离世界运行。两个 Main 场景分别启动游戏进程，调用开始入口，进入指定遭遇，通过 GAS 修改生命触发死亡，再检查结果、输入焦点、唯一结果页和 Episode。每个 Main 场景保存实际渲染截图。运行索引和日志位于 `Saved/Verification/Match/`。

重试专项使用同一脚本：

```powershell
py -3.11 Scripts/Boss/verify_match_results.py --only Retry
```

专项在同一进程实际重载 Main 三次。前两次用结果页重试按钮，第三次用返回菜单按钮，随后从营火菜单重新开始。每次检查起点、血量、GAS 冷却、HUD、输入与相机，并验证进战斗前无活动 Episode。测试从原生入口开启战斗，通过 GAS 触发失败；手动走到 Boss 和完整对打仍由 #4、#6 联合验收。

2026-11-16，重试专项在 #32 基线上通过。四轮均从营火起点 `(-2230, -1600, 164.01)`（UE 厘米）开始，玩家以 150 HP、Boss 以 500 HP 恢复。三个旧世界都在卸载时核查了 GameMode 对玩家及 Boss 死亡事件的解绑。四次首次入场分别建立独立的 BT Episode。两次重试和返回菜单各保存一张离屏截图，重试截图整体较暗。[日志、截图及源码哈希](../07_Release/Evidence/MatchRetry20261116.json)

主动注入载图失败、真实 PPO/NPC 迟到回包及 Windows 包待验。

2026-11-16 在 UE 5.8.4 构建并通过六项规则用例、Main 胜利和失败场景。六项规则用例共记录 13 条已知警告，来自测试敌人无武器网格及项目未指定 GameplayCue 搜索路径。两次 Main 都只发布一次结果，重试按钮取得焦点，原 Episode 正确结束，结算后的 GAS 伤害未改变血量。[日志、截图和文件哈希索引](../07_Release/Evidence/MatchResults20261116.json)

结果页离屏截图的文字对比度仍偏低。完整手打战斗和 Windows 包分别由 #4 与 #8 验收。
