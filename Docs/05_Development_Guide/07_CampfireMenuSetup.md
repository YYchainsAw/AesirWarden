# Main 起点营火菜单接手指南

> 2026-10-20：菜单框架、玩家坐姿/起身、同步安全绕行镜头与透明黄金分割布局。完整第一关流程仍见 [05_LevelFlowGuide.md](05_LevelFlowGuide.md)；本轮不代表 M1 完成。各次增量的验证状态见文末，不沿用首版菜单测试结果。

## 打开后先调整哪里

本轮使用已迁入的 `/Game/Main`，在现有 PlayerStart 附近集成营火菜单，默认启动地图改为 Main，保留原玩家与 Boss 布置。集成结果以文末验收记录为准。

| 资产 | 用途与调整入口 |
| --- | --- |
| `/Game/Aesir/UI/Menus/WBP_CampfireMainMenu` | 打开 UMG Designer，调整 Logo、开始游戏、设置、制作人员、退出游戏与详情子页的布局和样式。原生父类为 `AesirMainMenuWidget`。 |
| `/Game/Aesir/Framework/Flow/BP_AesirCampfireStart` | 营火、菜单相机与流程控制器合在一个 Actor 中。可修改 BP 默认外观，也可在 Main 中调整实例位置及相机。原生父类为 `AesirCampfireStart`。 |

菜单左侧留给场景与角色，右侧提供可操作框架。**保留 `Btn_Start`、`Btn_Settings`、`Btn_Credits`、`Btn_Quit`、`Btn_Back` 的 Button 类型和名称**，即可沿用 C++ 自动绑定；不必再写 OnClicked 调用链。还应保留 `MenuActions`、`MenuDetails` 两个页面容器，以及 `Txt_Status`、`Txt_DetailsTitle`、`Txt_DetailsBody` 三个 TextBlock 名称，供加载状态和子页切换使用。自定义动画、声音、字体、标题与边距均可在 Designer 中调整。

开始按钮等待起点 World Partition 流送就绪后启用；加载期间状态文字提示准备中，退出仍可使用。菜单与开场阶段锁定玩家游戏输入，隐藏配置在 `Gameplay HUD Widget Classes` 中的战斗 HUD；进入探索后恢复。缺少 BP 布局时 C++ 也有基础菜单，但正式美术调整应在 WBP 中完成。

## 透明黄金分割菜单布局

当前布局以整个游戏视口为参照：主纵轴在屏幕宽度的 **61.8034%**，Logo 中心在屏幕高度的 **38.1966%**；四个主按钮按“开始游戏 → 设置 → 制作人员 → 退出游戏”排列在 Logo 下方，并与 Logo 共用纵轴。以 1920×1080 为例，两条参考线约为 `X=1186.6`、`Y=412.5`。这些位置使用 Canvas Anchors 表达，不是仅适用于一种分辨率的绝对坐标。

面板容器不绘制底色，按钮只绘制 Kenney `panel_border_002` 的透明中心边框，Logo 下方使用 `divider_fade_003` 左右镜像组合；悬停、按下和禁用状态只改变边框颜色。营火与地图应能透过所有面板和按钮中心看到。文字采用米白色、细暗色描边和轻微阴影，保证场景较亮时仍可识别；它们不构成面板背景。

按钮统一使用 **55 号思源宋体简体中文 Regular**：`/Game/Aesir/UI/Fonts/F_MenuSerif`，Font Face 为 `/Game/Aesir/UI/Fonts/FF_SourceHanSerifSC_Regular`。Logo 保留图片 `/Game/Aesir/UI/Art/Logo/logo`；字体通过衬线、颜色与 Logo 的古典方向协调，不改变图片里的定制字形。字体原文件与授权保存在 `SourceArt/UI/Fonts/SourceHanSerif/`。

在 WBP 的 **Designer → Hierarchy** 中选择下列控件，在右侧 Details 调整：

| 控件 | 调整入口与当前值 |
| --- | --- |
| `Img_Logo` | `Slot (Canvas Panel Slot)`：Anchors `(0.618034, 0.381966)`、Alignment `(0.5, 0.5)`、Position `(0, 0)`；宽度 640，按原图比例设置高度。改大或改小时同时保持宽高比例，避免把 Logo 压成细条。 |
| `MenuDivider` | 与 Logo 使用相同 Anchor，Alignment `(0.5, 0)`；宽 400、高 30，Position Y 为 Logo 半高加 8。内部两个 Image 的 Tint 可以统一调整分隔线颜色。 |
| `MenuPanelSize` | 与 Logo 使用相同 Anchor，Alignment `(0.5, 0)`、Auto Size 开启；`Width Override=500`，Position Y 为 Logo 半高加 56。调整这里会同时移动四按钮和详情子页。 |
| `Btn_Start_Size`、`Btn_Settings_Size`、`Btn_Credits_Size`、`Btn_Quit_Size` | `Height Override=88`，Vertical Box Slot 的下间距为 8；改变它们可调整按钮高度和间隔。 |
| `Btn_Start` 等按钮 | `Appearance → Style` 下的 Normal/Hovered/Pressed/Disabled 均使用透明边框纹理，`Draw As=Box`、Margin 四边为 `0.25`；修改 Tint 可调边框颜色。不要换成带实心中心的 Panel 纹理。 |
| `Txt_Start`、`Txt_Settings`、`Txt_Credits`、`Txt_Quit` | `Appearance → Font`：Font 为 `F_MenuSerif`、Typeface 为 `Regular`、Size 为 `55`；可统一修改颜色、描边和阴影。`Render Transform → Translation Y=-18` 补偿此字体较大的行高，让可见汉字在边框内居中；换字体后可在此微调，不必缩小字号。 |
| `MenuDetails` | Designer 中临时把 Visibility 从 Collapsed 改为可见，并隐藏 `MenuActions`，便于调整子页。完成后将 `MenuDetails` 恢复为 Collapsed。 |

设置和制作人员当前是**可返回的透明子页框架**：点击后隐藏四个主按钮，显示标题、说明文字与“返回”按钮；返回、Esc 或手柄返回键恢复主菜单。设置项尚未连接画质、音量或键位配置，制作名单也仍是占位文案。

若要改运行时子页内容，在 WBP 的 Graph 中添加 **On Settings Opened** 或 **On Credits Opened** 事件，用 `Txt_DetailsTitle`、`Txt_DetailsBody` 的 `Set Text` 设置自己的文案，或扩展子页里的控件。原生按钮事件会先填写默认文案，再调用这两个事件；只在 Designer 改这两个 TextBlock 的预览文字会被运行时默认值覆盖。保留 `Btn_Back` 可以继续使用原生返回逻辑。

## 配置玩家坐姿和起身

动画播放在 **PlayerStart 生成的真实玩家角色** 上，沿用玩家原来的 `ABP_AesirPlayer`。不需要额外摆放一个展示角色，不需要制作新的 Montage，也不需要在开始按钮里接 `Play Animation` 或 `Delay`。C++ 在运行时把动画序列转换为 Montage；默认点击开始时同时播放起身并启动绕行镜头，动画与镜头都完成后才交还控制。

完整编译结束后重新打开 UE，按以下顺序检查和调整：

1. 打开 `/Game/Main`，在世界大纲中选择 `Aesir Campfire Start`，确认场景中只有一个营火流程控制器。
2. 在内容浏览器打开 `/Game/Aesir/Framework/Flow/BP_AesirCampfireStart`，点击 **Class Defaults（类默认值）**，在 Details 中搜索 `Sit`。确认启用 `Use Player Sit Animations`，`Menu Sit Loop Animation` 为 `AS_SSH_Sit_Loop`，`Stand Up Animation` 为 `AS_SSH_Sit_End`。这些属性也可在地图实例上覆盖；若实例曾修改过，以实例值为准。
3. `Player Animation Slot` 保持 `DefaultSlot`。现有 `ABP_AesirPlayer` 已有该 Slot，通常无需改图；若替换了动画蓝图，确认全身 `DefaultSlot` 的姿势能传到最终输出，且后面的 Control Rig/脚部 IK 没有把坐姿改回站姿。不要为此重建整套移动状态机。
4. 双击 `AS_SSH_Sit_Loop` 和 `AS_SSH_Sit_End` 预览动作，确认都适用于当前玩家 Skeleton。前者应为坐姿循环，后者应从相同坐姿起身。菜单直接进入循环，`AS_SSH_Sit_Start` 不是本流程的必填资产。
5. 回到 Main，移动和旋转 **PlayerStart** 来确定玩家坐在地面的地方及朝向，再移动营火 Actor 和 `MenuCamera` 来构图。只移动营火 Actor 不会移动真实玩家出生点。以坐姿双脚、臀部和起身后的脚底为准检查地面高度，避免让角色坐进地形或正对火焰。
6. 在 BP 的 **Components** 中选择新增的 `OpeningCamera`，按下文检查安全绕行设置。完整编译并重开编辑器后，原 BP 会继承这个原生组件，不需要手工添加第二个组件，也不需要重新制作按钮事件。
7. 保存 BP 和地图，启动 PIE。菜单中应持续坐着；点击开始一次后，菜单关闭，起身与镜头绕行同时开始，起身结束时接回玩家相机，然后才能移动和战斗。再按一次开始不应重复触发起身。若回到菜单并提示演出暂时无法开始，先按下文检查相机端点、路径和碰撞。

| Details 中的设置 | 默认值 | 调整作用 |
| --- | --- | --- |
| `Use Player Sit Animations` | 开启 | 启用真实玩家坐姿循环与起身流程；关闭时沿用直接交接流程。 |
| `Menu Sit Loop Animation` | `AS_SSH_Sit_Loop` | 菜单期间循环播放的动画序列，可替换为同一 Skeleton 的其它坐姿。 |
| `Stand Up Animation` | `AS_SSH_Sit_End` | 开始后的单次起身动画；默认绕行镜头读取这段 Montage 的实时进度，同步完成交接。 |
| `Player Animation Slot` | `DefaultSlot` | 原动画蓝图接收动态 Montage 的 Slot 名，必须与 AnimGraph 的 Slot 一致。 |
| `Sit Blend Time` | `0.2` 秒 | 姿势切换时的混合时间，先保留默认值，再根据角色动作微调。 |
| `Stand Up Play Rate` | `1.0` | 起身速度；增大会加快，减小会变慢。 |
| `Opening Blend Time` | `0.6` 秒 | 旧模式下起身和可选开场序列结束后切回玩家相机的时间；默认同步绕行模式不再叠加这段混合。 |
| `Opening Sequence Actor` | `None` | 默认保持为空以启用同步绕行。若显式指定 Sequence，则沿用“先起身 → Sequence → Blend”模式。 |

菜单动画期间代码临时忽略根运动，防止循环改变玩家位置；退出菜单演出后恢复原设置。不要依赖起身动画中的根位移把玩家送到另一处，应该把 PlayerStart 放在预期的可操作位置。若换动作后出现明显滑步，需要调整动画或摆位，而不是在按钮里叠加位移。

## 开始按钮后的流程

默认流程为 `MainMenu`（坐姿循环、玩家和 AI 等待）→ 点击开始 → `Opening`（起身与安全绕行镜头同步，输入仍锁定）→ 起身完成且镜头接回玩家 → `Exploration`（恢复输入、HUD 和 AI）。

`WBP_CampfireMainMenu` 的 `Btn_Start` 已通过原生父类调用 `RequestStartGame()`，不需要添加第二条 OnClicked 播放动画或镜头逻辑。起身使用 Montage 完成回调推进流程；镜头读取该 Montage 的实时进度，不使用手填动画时长的 Delay。更换起身动画或播放速度后，不必同步修改延迟值。动画与镜头均完成、起点流送也就绪后，流程才允许进入探索。动画、Slot 或 AnimInstance 配置无效时查看 Output Log 中的提示，先修复配置再验收画面。

同步模式下，起身中断、角色或相机引用丢失、路径受阻都会取消演出，停止自己的菜单动画与临时镜头，恢复菜单与坐姿，继续锁定游戏输入；若角色暂时失去控制器附身，会先等待附身恢复，再呈现菜单。`SkipOpeningCinematic()` 会先请求安全完成剩余路径，完成检查后才交还操作；不是直接在任意位置放开输入。退出关卡时同样清理自己的临时状态。若另外编写蓝图演出，避免同时控制同一个角色的动画、相机和位置。

### 调整同步起身镜头

完整编译 `AesirWardenEditor Win64 Development` 并重开 UE 后，打开 `BP_AesirCampfireStart`，在 **Components** 中选择 `OpeningCamera`，在 Details 的 **Opening Camera** 分类调整下表属性。Main 中选中营火实例后，也可展开同名组件设置实例覆盖；已有黄色重置箭头时，以实例覆盖值为准。

| 属性 | 默认值 | 调整作用 |
| --- | --- | --- |
| `Enabled` | 开启 | 启用同步安全绕行。关闭时使用原有起身后 Blend 模式，该模式不提供本组件的绕行与距离检查。 |
| `Min Player Clearance` | `100 cm` | 从相机探测球外缘到角色保守保护范围的最小净距；最低为 100 cm，可调大。不是从角色中心量 100 cm。 |
| `Camera Probe Radius` | `12 cm` | 环境碰撞探测球半径，同时计入玩家净距；范围 5～50 cm。增大会更保守，也可能使狭窄路线不可用。 |
| `Orbit Direction` | `Auto` | 自动先尝试较短方向，再尝试另一侧；`Left` / `Right` 固定绕行方向。固定方向受阻时不会自动换边。 |
| `Arc Height` | `60 cm` | 路线中段优先增加的高度，起终点不变。若优先高度受阻，会继续尝试更高的候选弧线。 |
| `Ease Curve` | `None` | 默认使用平滑缓入缓出。可选 Float Curve，以横轴 0～1 表示动画进度、纵轴 0～1 表示镜头路线进度，建议连续单调递增并经过 `(0,0)`、`(1,1)`；代码限制端点、取值范围与前进方向。 |

摆位顺序：先调 **PlayerStart** 确定真实玩家位置和朝向，再调营火 `MenuCamera` 的菜单构图，最后在玩家角色 BP 中检查实际使用的相机与 SpringArm 终点。默认路线从当前菜单视角出发，绕角色到达真实玩家相机；它读取实际相机的 FOV、SpringArm 碰撞和滞后结果，不把营火 Actor 的相机当作第二个玩家。

保护范围以角色 Capsule 为基础，并保守覆盖可见的骨骼模型、静态模型及附属装备外缘，横向按圆柱避让；角色起身过程中会更新。因此相机在角色头顶很高，也不能从保护范围内部抄近路。默认要求是**相机探测球外缘距离这整个保护范围至少 1 米**，通常比距角色中心 1 米更远。菜单相机与玩家相机两个端点也必须满足要求；若玩家 SpringArm 太短或被墙压近角色，先移动场景位置、菜单相机或调整玩家相机，不能靠减小安全距离解决。

路线开始前做完整预检，运行中对后续位移做玩家距离检查与环境球形 Sweep；正常播放与自然结束还检查上一实际镜头位置到下一实际位置的直线段，避免大卡顿时两帧间的弦切入人物或墙壁。环境检查使用 **Camera** 碰撞通道，所以应阻挡相机的墙体、地形和道具需在碰撞设置中阻挡该通道。没有配置碰撞的纯装饰模型不会自动成为阻挡物。全部候选路线都受阻时返回菜单，状态提示“开场演出暂时无法开始，请重试”；Output Log 中 `Campfire: synchronized camera aborted` 后会给出端点净距、挡路 Actor 或动画配置原因。优先调整摆位、绕行方向和弧高，再重试。

无需在关卡里手动摆放过渡 Camera Actor：组件只在演出期间创建临时相机，成功交接、取消或退出时清理。若要恢复现有 Sequence 演出，设置 `Opening Sequence Actor` 并关闭它的 Auto Play；或者显式关闭 `OpeningCamera.Enabled`。这两种兼容模式继续使用原来的 `Opening Blend Time`，不适用本段安全绕行保证。

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

默认未指定 `Opening Sequence Actor`，`OpeningCamera.Enabled` 开启，点击开始后起身与绕行同时进行，动画结束时匹配并接回玩家相机，随后进入 `Exploration`，不再额外等待约 0.6 秒。若显式关闭该组件，或指定开场 Sequence，则使用原有起身后交接模式及 `Opening Blend Time`；Sequence Actor 必须关闭 Auto Play，并自行配置角色绑定和 Camera Cuts。

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

### 显式重建黄金分割布局

[setup_golden_menu.py](../../Scripts/Editor/setup_golden_menu.py) 用于重新生成本指南的透明菜单。**它会覆盖 `WBP_CampfireMainMenu` 的现有 Designer 布局**，适合首次接入或主动恢复这套布局。日常直接在 UMG 调整即可；正常启动游戏、导入该脚本模块，以及上面的 `create_start_menu_assets()` 都不会触发这次重建，已有布局仍被保留。

需要重建时，先保存 WBP 并停止 PIE，在 UE Python 控制台显式执行：

```python
import sys, unreal
from pathlib import Path
sys.path.insert(0, str(Path(unreal.Paths.project_dir()) / "Scripts" / "Editor"))
import setup_golden_menu
setup_golden_menu.setup_golden_menu()
```

脚本先检查已有菜单、透明 Logo 和两张 Kenney 纹理，在 `Saved/StartMenu/BeforeGoldenLayout/<时间戳>/` 备份已保存的 WBP 与 Logo；随后导入思源宋体 Font Face、建立菜单 Font、设置 Logo 的 UI 纹理参数，重建并保存 WBP，Designer 预览设为 1920×1080。运行报告位于 `Saved/StartMenu/GoldenLayout/manifest.json`。需要恢复旧布局时，关闭编辑器后用该次备份中的 `WBP_CampfireMainMenu.uasset` 恢复原资产路径，再重新打开项目。

也可保存并关闭交互式编辑器后，按脚本头部示例以正常编辑器模式打开 Entry 并直接执行文件；直接执行会调用同一个重建函数。命令形式为：

```text
UnrealEditor-Cmd.exe AesirWarden.uproject /Engine/Maps/Entry -EnablePlugins=PythonScriptPlugin -ExecutePythonScript=".../Scripts/Editor/setup_golden_menu.py" -NullRHI -Unattended
```

将命令中的程序、项目和脚本路径替换为本机完整路径。这里需要正常编辑器初始化 Slate：UE 5.8 的 Font Face 导入会访问 Slate，`-run=pythonscript` commandlet 没有该初始化，会触发字体导入断言，因此本脚本使用 `-ExecutePythonScript`。避免同时从两个编辑器进程保存这些资产。

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

### 透明黄金分割菜单增量

| 检查 | 本轮结果与证据 |
| --- | --- |
| Editor 完整编译 | `AesirWardenEditor Win64 Development` 编译与链接通过；日志 `Saved/Logs/GoldenMenuBuild.log`。引擎头文件仍有既有弃用 API 提示。 |
| 字体导入、WBP 重建与备份 | 通过，WBP 编译 0 错误并保存；报告 `Saved/StartMenu/GoldenLayout/manifest.json`、日志 `Saved/Logs/GoldenMenuAssets.log`。改动前的用户布局另保存在 `Saved/StartMenu/BeforeGoldenLayout/20261020-171821/`。 |
| 设置、制作人员、返回与开始就绪状态 | `Aesir.Menu` 共 9 项通过，0 失败、0 警告；`WidgetActions` 覆盖原生菜单与保存的 WBP 两种类，验证子页、返回和开始就绪状态；报告 `Saved/StartMenu/GoldenLayout/Automation/index.json`。 |
| Main 实机布局、透明效果与起身交接 | 1920×1080 Standalone 通过 `START_MENU_PRESENTATION_PASS` 与 `START_MENU_MAIN_SMOKE_PASS`；实际 Logo 中心 `(0.6180, 0.3820)`，四按钮共轴、55 号且均在屏幕内。目视确认字体、Logo 比例及边框中心透出地图；菜单等待 10 秒内玩家 150 HP 不变、Boss 保持等待，起身后恢复输入和 HUD；日志 `Saved/Logs/GoldenMenuMainSmoke1920.log`，截图 `Saved/StartMenu/GoldenLayout/Preview/`。 |

本次截图通过进程参数 `-ForceRes -ResX=1920 -ResY=1080 -RenderOffscreen` 指定分辨率；截图进程临时执行 `DisableAllScreenMessages` 隐藏开发提示，未修改地图或项目渲染配置。该轮设置仍为占位子页；后续已接入独立的显示、画质、总音量与鼠标设置，当前操作见[设置菜单](10_SettingsMenuSetup.md)。键位重绑定尚未接入；本轮也未进行 cooked Windows 包验收。

### 同步起身与安全绕行镜头增量

| 检查 | 本轮结果与证据 |
| --- | --- |
| Editor 完整编译与新组件继承 | `AesirWardenEditor Win64 Development` 编译、链接通过；`Saved/Logs/OpeningCameraBuild.log`。已有营火 BP 和 Main 实例继承 `OpeningCamera` 并参与实机演出；本轮未重存地图或营火 BP。 |
| 自动化：起身与镜头双门禁、距离检查、跳过、受阻恢复和销毁清理 | 最终 16 项通过，0 失败、0 警告；新增慢速、快速、跳过、销毁、开始前阻挡、途中动态阻挡、动画中断 7 项 `CameraFlow` 回归；报告 `Saved/StartMenu/OpeningCamera/AutomationFinal/index.json`、日志 `Saved/Logs/OpeningCameraAutomationFinal.log`。 |
| Main 实机：起身与绕行同步、至少 100 cm 净距、终点匹配与输入/HUD 恢复 | 1920×1080 Standalone 三次均通过 `START_MENU_CAMERA_PASS`、`START_MENU_MAIN_SMOKE_PASS`；观察到的最小人物/装备保护范围净距均为 `155.16 cm`，已扣除相机探测球半径。最终版 1.5 倍速检查终点位置、旋转、FOV 误差均为 0；菜单等待 10 秒内玩家 150 HP 不变，Boss Episode 保持等待，完成后恢复输入和 HUD。 |
| 动画速度变化与用户最终镜头构图 | 标准 `1.0` 倍速约 `1.848 s`、`0.5` 倍速约 `3.349 s`、最终版 `1.5` 倍速约 `1.344 s` 完成同步交接。日志依次为 `Saved/Logs/OpeningCameraMainSmoke.log`、`OpeningCameraMainSlow.log`、`OpeningCameraMainFast.log`；原始截图位于 `Saved/StartMenu/OpeningCamera/Normal/`、`Slow/`、`Fast/`。中间帧和终点已目视检查，最终美术节奏仍可在组件参数中调整。 |

标准与慢速实机记录来自加入帧间直线段补充检查前的构建，最终版另外通过全部 16 项回归及快速实机检查。实机日志仍包含队友服务离线等原有提示，未阻塞本次流程；本轮不是 cooked Windows 包或正式性能验收。

### 2026-10-26 队友开发分支中的菜单恢复

营火功能此前保存在 `codex/campfire-start-menu@9dd9414`，未包含于队友分支最初使用的 main `17a8fef`。从该 main 分支测试时会缺少 `BP_AesirCampfireStart` 及其原生父类，日志报告 Main 外部 Actor 的 class descriptor 无法找到。已将菜单功能合入 `codex/companion-issue-19`，恢复 Main 默认地图，同时保留 #19 的 scoped 聊天与探索执行；用户本地地图、营火实例和正在修改的聊天 Controller 蓝图均未重存。

完整工程 Editor 构建、16 项菜单自动化和 6 项队友专项回归通过。Main Standalone 再次通过菜单等待 10 秒、真实开始按钮、坐姿/起身、安全镜头与输入/HUD 交接，两个 Boss 在菜单期间保持等待；最终日志为 `Saved/Logs/CompanionMenuRestoreMainFinal.log`。详细版本、报告和边界见[队友文本接入的集成复验](11_CompanionTextSetup.md#营火菜单集成复验)。
