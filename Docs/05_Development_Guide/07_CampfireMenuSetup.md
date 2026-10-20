# Main 起点营火菜单接手指南

> 2026-10-20：菜单框架与玩家坐姿/起身交接。完整第一关流程仍见 [05_LevelFlowGuide.md](05_LevelFlowGuide.md)；本轮不代表 M1 完成。坐姿增量的验证状态见文末，不沿用首版菜单测试结果。

## 打开后先调整哪里

本轮使用已迁入的 `/Game/Main`，在现有 PlayerStart 附近集成营火菜单，默认启动地图改为 Main，保留原玩家与 Boss 布置。集成结果以文末验收记录为准。

| 资产 | 用途与调整入口 |
| --- | --- |
| `/Game/Aesir/UI/Menus/WBP_CampfireMainMenu` | 打开 UMG Designer，调整右侧标题、开始、退出、状态文字的布局和样式。原生父类为 `AesirMainMenuWidget`。 |
| `/Game/Aesir/Framework/Flow/BP_AesirCampfireStart` | 营火、菜单相机与流程控制器合在一个 Actor 中。可修改 BP 默认外观，也可在 Main 中调整实例位置及相机。原生父类为 `AesirCampfireStart`。 |

菜单左侧留给场景与角色，右侧先提供可操作框架。**保留 `Btn_Start`、`Btn_Quit`、`Txt_Status` 三个控件名及对应类型 `Button`、`Button`、`TextBlock`**，即可沿用 C++ 自动绑定；不必再写 OnClicked 调用链。自定义动画、声音、字体、标题、背景与边距均可在 Designer 中调整。

开始按钮等待起点 World Partition 流送就绪后启用；加载期间状态文字提示准备中，退出仍可使用。菜单与开场阶段锁定玩家游戏输入，隐藏配置在 `Gameplay HUD Widget Classes` 中的战斗 HUD；进入探索后恢复。缺少 BP 布局时 C++ 也有基础菜单，但正式美术调整应在 WBP 中完成。

## 配置玩家坐姿和起身

动画播放在 **PlayerStart 生成的真实玩家角色** 上，沿用玩家原来的 `ABP_AesirPlayer`。不需要额外摆放一个展示角色，不需要制作新的 Montage，也不需要在开始按钮里接 `Play Animation` 或 `Delay`。C++ 在运行时把动画序列转换为 Montage，并等待起身播放完成后交还控制。

完整编译结束后重新打开 UE，按以下顺序检查和调整：

1. 打开 `/Game/Main`，在世界大纲中选择 `Aesir Campfire Start`，确认场景中只有一个营火流程控制器。
2. 在内容浏览器打开 `/Game/Aesir/Framework/Flow/BP_AesirCampfireStart`，点击 **Class Defaults（类默认值）**，在 Details 中搜索 `Sit`。确认启用 `Use Player Sit Animations`，`Menu Sit Loop Animation` 为 `AS_SSH_Sit_Loop`，`Stand Up Animation` 为 `AS_SSH_Sit_End`。这些属性也可在地图实例上覆盖；若实例曾修改过，以实例值为准。
3. `Player Animation Slot` 保持 `DefaultSlot`。现有 `ABP_AesirPlayer` 已有该 Slot，通常无需改图；若替换了动画蓝图，确认全身 `DefaultSlot` 的姿势能传到最终输出，且后面的 Control Rig/脚部 IK 没有把坐姿改回站姿。不要为此重建整套移动状态机。
4. 双击 `AS_SSH_Sit_Loop` 和 `AS_SSH_Sit_End` 预览动作，确认都适用于当前玩家 Skeleton。前者应为坐姿循环，后者应从相同坐姿起身。菜单直接进入循环，`AS_SSH_Sit_Start` 不是本流程的必填资产。
5. 回到 Main，移动和旋转 **PlayerStart** 来确定玩家坐在地面的地方及朝向，再移动营火 Actor 和 `MenuCamera` 来构图。只移动营火 Actor 不会移动真实玩家出生点。以坐姿双脚、臀部和起身后的脚底为准检查地面高度，避免让角色坐进地形或正对火焰。
6. 保存 BP 和地图，启动 PIE。菜单中应持续坐着；点击开始一次后，菜单关闭，角色完成起身，镜头交接后才能移动和战斗。再按一次开始不应重复触发起身。

| Details 中的设置 | 默认值 | 调整作用 |
| --- | --- | --- |
| `Use Player Sit Animations` | 开启 | 启用真实玩家坐姿循环与起身流程；关闭时沿用直接交接流程。 |
| `Menu Sit Loop Animation` | `AS_SSH_Sit_Loop` | 菜单期间循环播放的动画序列，可替换为同一 Skeleton 的其它坐姿。 |
| `Stand Up Animation` | `AS_SSH_Sit_End` | 开始后的单次起身动画，完成后才继续开场流程。 |
| `Player Animation Slot` | `DefaultSlot` | 原动画蓝图接收动态 Montage 的 Slot 名，必须与 AnimGraph 的 Slot 一致。 |
| `Sit Blend Time` | `0.2` 秒 | 姿势切换时的混合时间，先保留默认值，再根据角色动作微调。 |
| `Stand Up Play Rate` | `1.0` | 起身速度；增大会加快，减小会变慢。 |
| `Opening Blend Time` | `0.6` 秒 | 起身和可选开场序列结束后，切回玩家相机所用的时间。 |
| `Opening Sequence Actor` | `None` | 可选的额外开场演出；仅实现营火坐起时保持为空。 |

菜单动画期间代码临时忽略根运动，防止循环改变玩家位置；退出菜单演出后恢复原设置。不要依赖起身动画中的根位移把玩家送到另一处，应该把 PlayerStart 放在预期的可操作位置。若换动作后出现明显滑步，需要调整动画或摆位，而不是在按钮里叠加位移。

## 开始按钮后的流程

`MainMenu`（坐姿循环、玩家和 AI 等待）→ 点击开始 → `Opening`（单次起身，输入仍锁定）→ 可选 `Opening Sequence Actor` → 相机交接 → `Exploration`（恢复输入、HUD 和 AI）。

`WBP_CampfireMainMenu` 的 `Btn_Start` 已通过原生父类调用 `RequestStartGame()`，不需要添加第二条 OnClicked 播放动画逻辑。起身使用 Montage 完成回调推进流程，不使用手填动画时长的 Delay；更换起身动画或播放速度后，不必同步修改一个延迟值。动画、Slot 或 AnimInstance 配置无效时查看 Output Log 中的提示，先修复配置再验收画面。

起身动画中断、跳过开场或退出关卡时，代码负责停止自己的菜单动画并清理临时状态，避免输入和根运动设置留在菜单状态。若另外编写蓝图演出，避免同时控制同一个角色的动画和位置。

## 营火与相机

2026-10-20 已把两套素材接入并保存到现有营火 BP，新进程重编译、Main 实例检查与实机菜单流程均通过。具体证据见文末本次增量记录。

在 `BP_AesirCampfireStart` 的组件面板中调整：

- `MenuCamera`：菜单构图的位置与旋转；当前 C++ 构建时将相机 FOV 设为 50。
- `CampfireScannedBase`：导入的 Quixel Campfire 扫描模型；相对位置与缩放可在组件 Details 中调整。
- `CampfireEmissionSurface`：隐藏的粒子采样圆盘，使用 Epic `SM_Disk` 的本地副本，开启 CPU Access；它是 `CampfireNiagara` 的父组件。调圆盘缩放可调整火焰覆盖面积，不应删除或重新挂回根组件，否则样例会误选占位圆柱进行采样，导致粒子不能正常运行。
- `CampfireNiagara`：Epic Niagara Examples Pack 的 `NS_Fire`，在组件 User Parameters 中调整火势和烟量。初始参数为 `Flame Scale = 0.9`、`Lifetime Scale = 0.85`、`Spawn Rate = 220`、`Smoke Spawn Scale = 0.035`；样例中拼写为 `Base Light Intentsity` 的参数设为 0，保留现有 `Firelight` 控制主火光。
- `FlameCard` / `FlameCardCross`：首版的交叉火焰占位面片，正式素材接入后隐藏；`Flame Height` 只控制这些占位面片，不控制 Niagara。
- `Firelight`：颜色、位置与衰减范围；亮度用 `Firelight Intensity`，闪烁用 `Animate Firelight`。
- `Firewood` / `StoneRing`：引擎基础几何体组成的临时木柴和石圈，正式素材接入后隐藏。实例阵列仍由 C++ Construction 重建；正式模型使用独立组件，不会被这段占位逻辑重置。

用户已导入 [Quixel Campfire](https://www.fab.com/listings/7e84e0a2-e57a-4421-a127-ed747846480c) 与 [Epic Niagara Examples Pack](https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600)。模型路径为 `/Game/Fab/Megascans/3D/Campfire_uenpfeifa/High/uenpfeifa_tier_1/StaticMeshes/uenpfeifa_tier_1`，粒子路径为 `/Game/NiagaraExamples/FX_Misc/NS_Fire`。后者包含火焰、余烬与烟；原始资产和目录不改写。已有蜡烛面片留作占位回退，C++ 不增加对导入素材的硬编码依赖。此次接入不新增声音。

派生资产位于 `/Game/CustomAssets/Campfire/`：`SM_CampfireEmissionDisk`、`M_MenuCampfireFlames` 和 `MI_MenuCampfireFlames`，沿用原素材的许可与归属，按现有规则留在本地。`CampfireNiagara` 的 `User.Flame Material` 指向该材质实例；它保留样例火焰形状与纹理，把原样例由 Gallery 全局 MPC 驱动的曝光补偿改为局部参数 `Menu Fire Exposure Mix = 1`、`Menu Fire Exposure EV = 0`。当前室外地图曝光约 EV12.8，原材质在这里几乎看不到火焰；局部补偿让火焰清晰可见，不修改全图光照。需微调亮度时打开该 MI 调整 `Menu Fire Exposure EV`，而非提高全场景曝光。

显式接入入口为 [setup_campfire_art.py](../../Scripts/Editor/setup_campfire_art.py)。停止 PIE 后，在编辑器 Python 控制台执行：

```python
import sys
from pathlib import Path
import unreal
sys.path.insert(0, str(Path(unreal.Paths.project_dir()) / "Scripts" / "Editor"))
import setup_campfire_art
setup_campfire_art.install_campfire_art()
```

该入口创建上述本地派生资产并修改、保存现有营火 BP，首次保存前备份 BP 到 `Saved/StartMenu/CampfireArt/BeforeInstall/`；不保存地图、不改菜单布局、坐起动画或镜头摆位。重复执行保留已创建组件的美术参数，遇到同名的非本脚本组件或不同素材引用会中止。后续自行调整组件即可，不必重跑脚本。通过命令行接入时应先保存并关闭已打开的 UE，避免两个编辑器争用同一个资产。另一台电脑导入相同两套包后运行该脚本，才能重新得到这些本地派生依赖。

场景中只保留一个流程控制器，避免同时放置旧 `BP_AesirLevelFlowDirector` 与新营火 Actor。营火 Actor 关闭 `Is Spatially Loaded`，其 `StartupStreamingSource` 在菜单与相机交接期间维持起点加载，进入探索后交回玩家流送源。此处还未实现按墙缝切换房间的区域流送。

## 当前流程边界

默认未指定 `Opening Sequence Actor`，开始后先等待玩家起身，再用约 0.6 秒 Blend 到玩家相机并进入 `Exploration`。可调整 `Opening Blend Time`；若后续指定开场 Sequence，关闭该 Sequence Actor 的 Auto Play，并自行配置角色绑定和 Camera Cuts。它是在起身后播放的额外演出，当前需求无需设置。

世界大纲中的 `Sequences` 只是整理文件夹。原地图里的 `Additional_PickupShots`、`BRolls`、`QA_Perf_Flythrough` 等 Level Sequence Actor 未接到营火开始流程；无需删除它们，也无需用它们制作本轮坐起效果。`Aesir Campfire Start` 才是本轮菜单入口，`StartupStreamingSource` 是它维护起点加载的组件，`MenuCamera` 是菜单观察镜头，二者均不是额外玩家。

当前 PlayerStart 靠近已放置的 Boss；Boss/队友控制器在菜单和开场阶段等待，到 `Exploration` 才启动各自逻辑。这是本轮避免菜单期间提前战斗的边界，**尚不是完整的 BossIntro 触发、演出结束后开战与 Episode 生命周期方案**。代码负责播放和交接；坐姿对地面、朝向、营火距离、镜头遮挡仍需在自己的地图里做画面验收。

胜败结算、重试、返回菜单、完整 Windows 包验收仍按开发路线推进，不能凭菜单可点击就关闭整个 M1。

## 重建与恢复本地场景

新增原生类后先关闭 UE，完整编译 `AesirWardenEditor Win64 Development`，再打开项目。编辑器辅助脚本为 [setup_start_menu.py](../../Scripts/Editor/setup_start_menu.py)。在启用 Python Editor Script Plugin 的 UE Python 控制台执行：

```python
import sys, unreal
from pathlib import Path
sys.path.insert(0, str(Path(unreal.Paths.project_dir()) / "Scripts" / "Editor"))
import setup_start_menu
setup_start_menu.create_start_menu_assets()
```

该入口创建/验证两个 BP，保留已有 Designer 布局和 BP 调整，不加载或保存地图；直接运行脚本也只执行这一步。需要在另一份本地 Main 中恢复默认集成时，显式调用：

```python
setup_start_menu.install_in_main_at_player_start()
```

此入口加载 `/Game/Main`，确认唯一 PlayerStart，加载附近 Actor，将营火放到出生点附近并保存地图；已有单个营火时保留位置，发现其它流程控制器时中止以便检查。首次集成前地图备份与放置清单在 `Saved/StartMenu/BeforeInstall/Main.umap`、`Saved/StartMenu/MainMenuPlacement.json`。更换或恢复 WP 地图时必须保留配套 ExternalActors，不能把单个 `.umap` 当成完整场景备份。

若希望自己选点，可在已打开的地图调用 `setup_start_menu.place_start_menu(unreal.Vector(x, y, z))`，这条入口支持撤销，并把保存交给用户。

代码、脚本和项目自制的两个 BP 可进入 Git；Main、第三方素材及 `Content/__ExternalActors__` / `Content/__ExternalObjects__` 按现有忽略规则留在本地。另一台电脑仅拉取 Git 无法得到 Main，需要先准备合法来源的地图与素材，再运行恢复入口。

## 验收记录

下表为菜单首版的既有验证，不能作为坐姿/起身增量已经通过的证据。

| 检查 | 结果 |
| --- | --- |
| Editor 完整编译与 BP 创建/编译 | 2026-10-20 通过；字体采用可序列化的 Engine Roboto UFont，资产生成复验 0 错误、0 警告。 |
| 自动化：菜单输入、重复开始、初始化和交接 | 4 项通过，0 失败、0 警告；覆盖原生/蓝图菜单、按钮事件、开始门禁、输入路由、独立输入锁保留、销毁清理和字体资产引用。报告：`Saved/StartMenu/AutomationFinal/index.json`。 |
| Main 实机 smoke：起点菜单 → 开始 → 探索 | Standalone `-game` 通过：1 个 BossController 在菜单期间保持未启动 Episode，150 HP 保持 10 秒；实际开始按钮完成相机、游戏输入与 HUD 交接。日志：`Saved/Logs/StartMenuMainSmokeFinal.log`。 |
| Main 退出按钮 | 单独启动并点击实际退出按钮通过，进程退出码 0；日志：`Saved/Logs/StartMenuQuitSmoke.log`。 |
| 用户美术调整：镜头、真实火焰效果、UMG 样式 | 待用户编辑与实际画面确认 |

1280×720 实机截图保存在 `Saved/StartMenu/Preview/MainMenu.png` 和 `MainStarted.png`。本轮验收为 Editor/Standalone 功能检查，不是 cooked Windows 包或正式性能验收。队友服务离线时仍有原有 HTTP 连接警告；本轮菜单开始未受其阻塞。

### 本次正式营火素材与坐起流程增量

| 检查 | 2026-10-20 结果与证据 |
| --- | --- |
| BP 接入与保存 | 通过；原始 Quixel/Epic 资产保持原样，本地材质和采样盘副本可加载。安装记录：`Saved/StartMenu/CampfireArt/installation.json`。 |
| 独立进程重新加载、编译、Main 实例检查 | 通过，0 错误、0 警告。确认唯一流程控制器、正式组件可见、四个占位隐藏、CPU 采样盘是 Niagara 父组件、低烟参数及局部曝光材质保存正常；`Saved/Logs/CampfireArtVerifyFinal.log`、`Saved/StartMenu/CampfireArt/verification.json`。 |
| Editor 编译与菜单自动化 | `AesirWardenEditor Win64 Development` 编译通过；`Aesir.Menu` 共 7 项通过，0 失败、0 警告，覆盖菜单、自然起身、跳过与销毁清理；`Saved/StartMenu/CampfireArt/Automation/index.json`。 |
| Main 实机菜单 → 起身 → 探索 | 1280×720 Standalone 通过；10 秒内玩家 150 HP 不变、Boss Episode 不启动；起身播放完毕后恢复相机、输入与 HUD。火焰、余烬和少量烟正常可见；`Saved/Logs/CampfireArtFinalSmoke.log`，终局标记 `START_MENU_MAIN_SMOKE_PASS`。 |

本次 1280×720 原始画面保存在 `Saved/StartMenu/CampfireArt/Preview/MainMenu.png`、`MainStandingUp.png`、`MainStarted.png`。首次流程检查在起身混出阶段误判：`GetCurrentActiveMontage()` 会提前排除正混出的 Montage，而演出结束委托尚未触发；本次仅修正 smoke 检查为验证演出拥有的实际 Montage 实例，继续保留输入与 Boss 门禁检查，没有改动生产动画流程。

场景仍有 Lumen cached-lighting pre-exposure 的原有提示，队友推理服务离线时仍有 HTTP 连接警告；它们未阻塞本次营火与菜单流程检查。尚未完成 cooked Windows 包、正式帧率验收、篝火音效或最终构图美术确认。

### 删除后撤销的构造崩溃修复

2026-10-20 14:44 的编辑器日志确认触发步骤为删除营火 Actor 后执行 `TRANSACTION UNDO`。撤销恢复期间，`ACameraActor` 已恢复为有效对象，但其 `CameraComponent` 引用为空；原 `OnConstruction()` 仅检查 Camera Actor，再直接设置 FOV，因此空指针崩溃。进一步在独立编辑器进程复现确认：UE 5.8 的 ChildActorComponent 为子 Actor 和根组件设置了事务标记，却没有为 Camera Actor 的非根原生 CameraComponent 设置该标记，删除撤销后相机组件仍可能缺失。该问题与火焰参数或新增补光灯无关。

修复将菜单相机刷新集中为 `RefreshMenuCamera()`：同时检查 ChildActorComponent、Camera Actor 和 CameraComponent，组件不可用时暂不向流程发布菜单相机；在编辑器世界为有效 CameraComponent 补上 `RF_Transactional`，使其参与删除撤销。`PostRegisterAllComponents()` 也刷新相机，覆盖已保存地图加载后直接删除的情况；构造和 BeginPlay 保留刷新。此处不重建 Child Actor、不调用 Modify、不保存资产。占位组件与火光更新也补上恢复期间的有效性检查，保留已有材质、火焰参数、补光与镜头配置。

新增 `Aesir.Menu.ConstructionRecovery.Native` 与 `.Blueprint` 两项回归，检查 CameraComponent 的事务标记，并精确模拟有效 Camera Actor 的 CameraComponent 暂缺，以及 ChildActorComponent 暂缺，随后还原并验证相机/FOV 恢复。`AesirWardenEditor Win64 Development` 完整编译与链接通过；`Aesir.Menu` 共 9 项通过，0 失败、0 警告，报告为 `Saved/StartMenu/CrashFix/Automation/index.json`。另在完整编辑器的临时 Entry 世界中，对原生类和实际 BP 分别连续执行 3 次删除、撤销，共 6 次均不崩溃，CameraComponent 与 MenuCamera 引用正常恢复；记录为 `Saved/StartMenu/CrashFix/delete_undo.json`、`Saved/Logs/CampfireDeleteUndoFullEditor.log`。

对 `/Game/Main` 中已保存的营火实例另做 3 次真实删除、撤销，相机组件与菜单相机引用均恢复，Actor 及美术组件的变换、灯光颜色和强度保持一致；记录为 `Saved/StartMenu/CrashFix/saved_main_delete_undo.json`、`Saved/Logs/CampfireSavedMainUndo.log`。使用最终 DLL 的 Main Standalone 菜单 → 起身 → 探索检查也通过，玩家 150 HP 保持 10 秒、Boss 保持等待、起身后输入与 HUD 恢复；日志为 `Saved/Logs/CampfireCrashFixMainSmoke.log`，终局标记 `START_MENU_MAIN_SMOKE_PASS`。以上独立检查均未保存资产，Main、营火 BP 和火焰 MI 的 SHA256 与用户关闭编辑器后的记录相同。
