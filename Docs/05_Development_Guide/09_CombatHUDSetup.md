# 战斗 HUD 接入与调整

本轮沿用 `/Game/Aesir/UI/HealthBars/WBP_CombatHUD`，将布局留在 UMG Designer，由原生父类 `AesirCombatHUDWidget` 同步玩家生命、防御压力、Boss 生命和已配置技能的冷却。界面采用透明底、细金线和白色可染色图标；不显示尚未实现的魔法或耐力资源。

本机已完成素材导入、HUD 升级和完整编译，直接打开原 `WBP_CombatHUD` 即可调整。下节脚本供其他工作区恢复或日后维护使用，不需要为了查看本轮结果再执行一次。

## 现有 HUD 升级与素材恢复

恢复入口为 [setup_combat_hud.py](../../Scripts/Editor/setup_combat_hud.py)。已有玩家蓝图继续创建同一个 `WBP_CombatHUD`，不会另外添加第二个 HUD 或修改地图。

1. 停止 PIE/Simulate，保存已有 `WBP_CombatHUD` 和 `BP_AesirPlayerCharacter`。按[工程速查](01_ProjectQuickReference.md)关闭 UE，完整编译 `AesirWardenEditor Win64 Development` 后重新打开。
2. 确认本地 Emberhold 免费包已解压到下文指定目录，并保留已有 `F_MenuSerif` 字体及 Kenney 边框贴图。
3. 在支持 Python 的 UE 编辑器中，将 Output Log 的输入模式切换到 **Python**，执行：

```python
import sys, unreal
from pathlib import Path
sys.path.insert(0, str(Path(unreal.Paths.project_dir()) / "Scripts" / "Editor"))
import setup_combat_hud
setup_combat_hud.setup_combat_hud()
```

4. 确认日志出现 `COMBAT_HUD_SETUP_PASS`，并检查 `Saved/HUD/Install/manifest.json`。脚本导入 6 个本地图标至 `/Game/Aesir/UI/Art/Emberhold`：`T_EH_heart`、`T_EH_shield`、`T_EH_fire`、`T_EH_potion`、`T_EH_sword`、`T_EH_lightning`。这些图标采用 UI RGBA、无 Mipmap、保留 Alpha 的设置。
5. 打开原有 HUD 调整 Designer，不必重接玩家创建 HUD 的 Graph。

**第一次从旧父类升级会安装新的 Designer 布局。** 原有事件/函数 Graph 和 `PB_PlayerHealth` 变量 GUID 保留，迁移时编译 HUD 与玩家蓝图，只保存 HUD；玩家蓝图的磁盘文件保持原样。之后再次调用默认入口会保留已经调整的布局。若控件名称或类型不符合要求，脚本会停止，先检查日志修复对应控件。

每次执行前会把原 HUD、玩家蓝图及存在的 `.uexp/.ubulk` 复制到 `Saved/HUD/BeforeInstall/<时间戳>/`，并保存带 SHA256 的 `manifest.json`。如需恢复，**关闭 UE**，按照该 manifest 的原始 `source` 路径复制回对应备份包及其附属文件，再重新打开编辑器检查。

只有明确要放弃当前 Designer 排版、重新安装默认布局时才使用：

```python
setup_combat_hud.setup_combat_hud(rebuild_layout=True)
```

这会替换当前 Designer 树，不用于日常刷新或修改颜色。正常排版直接编辑蓝图并保存即可。

## 打开与调整界面

1. 本轮包含新的原生父类和蓝图属性。先按上节完成完整编译和本地资产恢复。
2. 在内容浏览器打开 `/Game/Aesir/UI/HealthBars/WBP_CombatHUD`。在 **Class Settings → Parent Class** 确认父类为 `AesirCombatHUDWidget`，进入 **Designer** 编辑。
3. 先将预览尺寸设为 `1920×1080`。整体移动一个区域时选择它的容器，调整 Canvas Slot 的 **Anchors、Alignment、Position、Size**；容器内的间隔使用子控件的 Padding 调整。保留屏幕边缘的安全距离，再切换到其他分辨率检查。
4. 调整文本时，在对应 Text Block 的 **Appearance → Font** 中修改字体、字号和字形，并检查中文显示。修改装饰 Image 的 Brush、Tint 和透明度可以替换边框风格。
5. 编译并保存蓝图，运行关卡查看。Designer 的模拟比例只用于排版，实际游戏从 GAS 属性读取数值。

原生类通过控件名称寻找需要更新的条、文字和容器。可以调整它们的位置、大小、字体和装饰，但保留本节下文列出的功能控件名称及类型。直接在 Designer 改名、删除或换成不同类型，会使对应自动更新失效。

| 必须保留的名称 | 类型 | 用途 |
| --- | --- | --- |
| `PB_PlayerHealth` | Progress Bar | 玩家当前生命。 |
| `PB_PlayerHealthTrail` | Progress Bar | 受伤后稍晚追随的生命残影，作为当前生命条的底层。 |
| `PB_GuardPressure` | Progress Bar | 防御压力。 |
| `Txt_PlayerHealth` | Text Block | 当前生命 / 最大生命。 |
| `Txt_GuardState` | Text Block | “防御压力”或“破防”状态文字。 |
| `BossPanel` | 容器控件 | Boss 区域的整体显隐。 |
| `PB_BossHealth` | Progress Bar | Boss 当前生命。 |
| `Txt_BossName` | Text Block | Boss 名称。 |
| `HB_Abilities` | Horizontal Box | 运行时技能槽容器。 |

`PB_PlayerHealth` 的 **Is Variable** 保持开启以兼容旧生命 Graph。其他控件无需开启此选项也能自动更新；若要在自己的蓝图逻辑中引用它们，可按需开启。

默认布局以 `1920×1080` 为设计尺寸，顶层为 `HUDSafeArea → HUDCanvas`：

| 选择的容器 | 默认位置与尺寸 | 推荐调整方式 |
| --- | --- | --- |
| `PlayerVitals` | 左上锚点，位置 `(72,64)`，尺寸 `436×116`。 | 整体移动生命与防御区，修改此容器的 Canvas Slot。 |
| `PlayerHealthFrame` | 玩家区内位置 `(44,24)`，尺寸 `380×22`。 | 调整生命条总尺寸，保留内部残影、填充、边框的叠放关系。 |
| `GuardPressureFrame` | 玩家区内位置 `(44,65)`，尺寸 `300×10`。 | 调整防御条长度和粗细。 |
| `BossPanel` | 底部中间锚点，Alignment `(0.5,1)`，位置 `(0,-64)`，尺寸 `720×78`。 | 修改距底边的距离和整体宽度；同步调整内部名称、生命条和分割线。 |
| `HB_Abilities` | 右下锚点，Alignment `(1,1)`，位置 `(-72,-64)`，Auto Size 开启。 | 修改距右下角的距离；默认无技能时隐藏。 |

这些是 Designer 设计单位，实际显示还受视口和项目 DPI 缩放影响。资源条保留局部半透明暗色轨道以便在亮场景中阅读，区域容器本身不绘制整块背景。

## 颜色与设计预览

选中 **Class Defaults**，搜索 `Aesir | HUD`：

| 属性 | 用途 |
| --- | --- |
| `Health Color` / `HealthColor` | 玩家与 Boss 的运行时生命填充颜色。 |
| `Guard Color` / `GuardColor` | 正常防御压力填充颜色，默认为金色。 |
| `Broken Guard Color` / `BrokenGuardColor` | 玩家带有 `state.guard_broken` 时的防御压力颜色。 |
| `Preview Health Ratio` | Designer 模拟生命比例，范围 `0～1`，默认 `0.72`。 |
| `Preview Guard Ratio` | Designer 模拟防御压力比例，范围 `0～1`，默认 `0.28`。 |
| `Preview Boss` | 是否在 Designer 中显示 Boss 区域；不代表运行时自动寻找 Boss。 |

生命和防御条的颜色由原生代码更新，统一在上述属性中修改。为同一 Progress Bar 再添加另一套 Percent 绑定、Tick 更新或填充颜色绑定，可能覆盖运行结果。装饰、静态标题和边框颜色仍可在 Designer 单独调整；`Txt_GuardState` 的文本和状态颜色由运行逻辑更新。

正常防御条表示 `GuardPressure / MaxGuardPressure`，是受防御打击后累积的压力：**条越满，越接近破防**。它不是剩余耐力，也不表示还有多少时间可以防御。破防状态取实际 Gameplay Tag `state.guard_broken`；破防期间显示满条并切换为破防颜色，状态退出后恢复实际压力比例。不能只把预览条拉满就当成运行时破防测试。

## 玩家数据与菜单流程

HUD 自动跟随创建它的 Player Controller 当前持有的 `Owning Pawn`；角色附身变化时会重新检查和绑定数据源。生命、最大生命、防御压力和最大防御压力通过 GAS 属性变化事件同步，不必在关卡蓝图重复轮询或逐帧设置条百分比。`Refresh HUD Sources` 可在自定义流程中手动请求重新绑定，正常运行已有定时刷新。

菜单和起身演出仍由现有 `AesirLevelFlowDirector` 管理 HUD 的隐藏与恢复。保留流程控制器 `GameplayHUDWidgetClasses` 中的 `WBP_CombatHUD` 配置；不要另外添加一套菜单按钮中创建、删除或强制显示战斗 HUD 的逻辑。若复制成新的 HUD 类，需要同步配置该列表和原有创建入口。

## 指定 Boss 与解除绑定

自动模式由 **Class Defaults → Follow Locked Boss**（`bFollowLockedBoss`）控制，默认开启。玩家锁定的目标必须由项目的 Boss AI Controller 控制，自动模式才显示其 Boss 条；普通敌人不会因为被锁定就显示为 Boss。它不扫描整张地图，也不因远处存在一个 Boss 就自动显示血条。

如果以后要在进入遭遇区域时持续显示 Boss 条，可在已有流程蓝图中调用 HUD 的公开函数：

1. 取得当前玩家正在显示的 `WBP_CombatHUD` 实例引用。如果创建入口没有保存引用，可用 **Get All Widgets Of Class**，Class 选 `WBP_CombatHUD`，从结果中取属于当前玩家的实例，转换为 `AesirCombatHUDWidget`。
2. 在这个实例上调用 **Set Boss Target**，`Target` 传关卡里选定的 Boss，`Display Name` 传希望显示的名称。参数必须是实际 Actor 引用，不能只填资产类。
3. 遭遇结束或需要解除固定显示时，调用同一实例的 **Clear Boss Target**。传空 Target 给 `Set Boss Target` 也表示清除固定目标。

`SetBossTarget(Target, DisplayName)` 是独立于锁定系统的固定绑定，期间改变锁定目标不会替换它。`ClearBossTarget()` 解除固定绑定后，若 `bFollowLockedBoss` 开启，下一次刷新会恢复自动跟随当前锁定的 Boss；因此“清除固定绑定”不保证 Boss 条始终隐藏。若需要完全手动管理 Boss 条，将 `Follow Locked Boss` 关闭。

目标必须在当前世界有效，具有项目生命属性且仍然存活；没有有效生命来源或目标死亡时不显示 Boss 条。自动模式默认名称为“首领”，固定绑定时才使用传入的非空 `Display Name`。

这些函数提供接线入口。本轮不自动创建遭遇触发器，也不扩展 `M1BossEncounter`；展示时机和目标由关卡流程明确指定。

## 技能槽与冷却

`Ability Entries` 默认是空数组，技能区域随之隐藏。只有已经实现、已授予玩家且有真实输入的技能才应添加配置；填一条配置不会创建、授予或执行 Gameplay Ability。

在 **Class Defaults → Aesir | HUD | Abilities → Ability Entries** 添加元素：

| 字段 | 填写方式 |
| --- | --- |
| `Label` | 技能显示名。 |
| `Input Hint` | 与真实输入绑定一致的键位提示；它只显示文字，不绑定按键。 |
| `Icon` | 已导入的 Texture2D 图标，可使用本地 Emberhold 白色 PNG 图标。 |
| `Cooldown Tag` | 该技能实际使用的冷却 Gameplay Tag；拼写与项目配置及 Gameplay Effect 保持一致。 |

当前可用于联调的是测试治疗能力及 `Cooldown.Player.TestHeal`，不是一套正式魔法栏。只有确认该测试能力已授予当前角色，并核对 `IA_TestHeal` 的当前按键后，才添加相应槽位。槽位显示只是读取该 Tag 对应的 GAS 冷却状态，不会自动设置冷却时长或创建冷却 Gameplay Effect。

带有限时 Gameplay Effect 的冷却显示遮罩和剩余整秒；冷却 Tag 存在但取不到有限到期时间时显示“…”而不编造倒计时。这种情况需要检查技能的 Tag 与 Gameplay Effect 配置。

正式技能尚未接入时保持数组为空。将来逐个加入实际技能后，再完善图标、中文名称和键位。动态生成的技能槽由代码管理，Designer 中调整技能容器的位置和大小；不要用静态假槽掩盖尚未实现的技能。

## 第三方来源与本地素材

使用的是作者 VictorGabrielF 发布的 [Emberhold Free](https://victorgabrielf.itch.io/emberhold-free-dark-fantasy-ui-kit-with-matching-sounds)，没有购买完整版。原作者许可允许无限个人及商业项目使用、修改、染色和缩放，无需署名；禁止把原始或修改后的文件作为素材包、模板或素材合集再分发，也禁止用于训练模型。完整条款以下载包内 `LICENSE.txt` 为准。

本地素材根目录：`SourceArt/UI/Emberhold/emberhold-free-v1.0/emberhold-free/`。原 ZIP、README、LICENSE 和来源记录保留在本机。作者页面标注 AI 协助代码，并说明图形为程序绘制/矢量几何，非 AI 生成图片。

**公开仓库不分发 Emberhold 原始文件和导入贴图。** 这些第三方文件由忽略规则留在用户本地；公开提交仅包含自制 HUD、恢复脚本和许可来源记录。用户本地素材完整时可以正常恢复贴图和 HUD 外观；仅克隆公开仓库不等于取得第三方文件，应先自行从原作者免费页面下载，并遵守许可。已有 [Kenney Fantasy UI Borders](https://kenney.nl/assets/fantasy-ui-borders) 使用 CC0，边框风格可继续搭配。

## 检查步骤与验证范围

1. 在 Designer 预览不同生命、防御压力比例；调整尺寸后确认文字、条和边框没有重叠或裁切。
2. 在关卡 PIE 中，从菜单点击开始：菜单及起身阶段不显示战斗 HUD，进入探索后恢复。
3. 使玩家实际受伤、恢复生命和防御受击，检查 GAS 数值与 HUD 一致；实际触发破防后检查颜色及状态文字恢复。
4. 有 Boss 的测试场景中分别检查锁定/解除锁定、固定 Boss 绑定、清除固定绑定，以及锁定普通敌人时不显示 Boss 条。
5. 默认无技能槽；如配置测试治疗，检查实际施放后冷却显示、冷却结束及键位提示，并保留其测试用途说明。
6. 如项目有换角色/重新附身流程，检查 HUD 绑定新角色的数据，离开关卡后没有重复 HUD。

以上为完整手动验收步骤。2026-10-21 本轮实际完成的验证如下，未覆盖项继续按需检查：

| 检查 | 结果与本地证据 |
| --- | --- |
| Editor 完整编译 | `AesirWardenEditor Win64 Development` 成功，包含 UHT、新 HUD 类、编辑器迁移库与测试。引擎头文件存在既有弃用提示。 |
| 免费素材与原 HUD 迁移 | `COMBAT_HUD_SETUP_PASS`；HUD 和玩家蓝图编译通过；玩家蓝图磁盘 SHA256 保持一致。备份及报告见 `Saved/HUD/BeforeInstall/`、`Saved/HUD/Install/manifest.json`。 |
| 五项自动测试 | `Aesir.HUD.SafeRatio`、`Live.PlayerVitals`、`Live.HiddenAndRebind`、`Live.BossTarget`、`Live.ConfiguredAbility` 全部通过，0 失败。最小原生角色夹具未装武器，会产生已知提示。报告 `Saved/HUD/Automation/index.json`，日志 `Saved/Logs/CombatHUDTests.log`。 |
| Main 真实流程与显示 | `HUD_MAIN_SMOKE_PASS`；1920×1080 Standalone 中只有原 HUD 一个实例，真实 Start 按钮启动，菜单和起身隐藏、探索恢复。当前玩家生命比例与 GAS 一致；临时 60% 生命、40% 防御压力及真实 Boss 固定绑定展示后恢复原数值，不保存地图或资产。日志 `Saved/Logs/CombatHUDMainSmoke.log`。 |
| 画面检查 | 已查看 `Saved/HUD/Preview/Gameplay.png`、`VitalsAndBoss.png`：透明容器、图标、文字、条和边框正常，没有 UI 相互裁切。菜单隐藏证据 `Menu.png`。预览截图中的开发屏幕提示仅在该独立测试进程中关闭。 |

复现真实流程可在独立 Main 游戏进程中运行 `Aesir.HUD.Smoke`。要确保得到完整 1920×1080 截图，使用 `-ResX=1920 -ResY=1080 -ForceRes`；清爽截图可使用 `-ExecCmds="DisableAllScreenMessages,Aesir.HUD.Smoke"`。该命令会点击开始、临时改变运行世界里的生命/压力并还原，最后结束游戏会话；不用于正常游玩。

原 `Aesir.Menu.Smoke` 还校验默认菜单 Logo 的黄金分割位置，当前用户手调布局不满足该旧位置断言。本轮使用独立 HUD 检查并保留用户菜单，不把旧位置断言作为 HUD 验收条件。

本轮没有声称覆盖所有分辨率、正式 Boss 遭遇流程、完整技能体系、Windows 打包或性能验收。Boss 固定绑定已在真实 Main 展示；自动锁定模式仍应在选定的 Boss 场景中按上述步骤验证。
