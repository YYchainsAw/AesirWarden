# Main 起点营火菜单接手指南

> 2026-10-20：菜单首版框架。完整第一关流程仍见 [05_LevelFlowGuide.md](05_LevelFlowGuide.md)；本轮不代表 M1 完成。

## 打开后先调整哪里

本轮使用已迁入的 `/Game/Main`，在现有 PlayerStart 附近集成营火菜单，默认启动地图改为 Main，保留原玩家与 Boss 布置。集成结果以文末验收记录为准。

| 资产 | 用途与调整入口 |
| --- | --- |
| `/Game/Aesir/UI/Menus/WBP_CampfireMainMenu` | 打开 UMG Designer，调整右侧标题、开始、退出、状态文字的布局和样式。原生父类为 `AesirMainMenuWidget`。 |
| `/Game/Aesir/Framework/Flow/BP_AesirCampfireStart` | 营火、菜单相机与流程控制器合在一个 Actor 中。可修改 BP 默认外观，也可在 Main 中调整实例位置及相机。原生父类为 `AesirCampfireStart`。 |

菜单左侧留给场景与角色，右侧先提供可操作框架。**保留 `Btn_Start`、`Btn_Quit`、`Txt_Status` 三个控件名及对应类型 `Button`、`Button`、`TextBlock`**，即可沿用 C++ 自动绑定；不必再写 OnClicked 调用链。自定义动画、声音、字体、标题、背景与边距均可在 Designer 中调整。

开始按钮等待起点 World Partition 流送就绪后启用；加载期间状态文字提示准备中，退出仍可使用。菜单与开场阶段锁定玩家游戏输入，隐藏配置在 `Gameplay HUD Widget Classes` 中的战斗 HUD；进入探索后恢复。缺少 BP 布局时 C++ 也有基础菜单，但正式美术调整应在 WBP 中完成。

## 营火与相机

在 `BP_AesirCampfireStart` 的组件面板中调整：

- `MenuCamera`：菜单构图的位置与旋转；当前 C++ 构建时将相机 FOV 设为 50。
- `FlameCard` / `FlameCardCross`：两张交叉火焰面片；用 `Flame Height` 调整体火焰高度。
- `Firelight`：颜色、位置与衰减范围；亮度用 `Firelight Intensity`，闪烁用 `Animate Firelight`。
- `Firewood` / `StoneRing`：引擎基础几何体组成的临时木柴和石圈。可换 Mesh/材质；实例阵列由 C++ Construction 重建，逐个移动实例不会长期保留。要自由摆放正式木石，可隐藏占位组件，新增自己的 Mesh 组件。

本地可复用素材为 `/Game/CustomAssets/FX/Candles_FX/SM_FlameCard_01_N1` 蜡烛火焰面片；**尚无已确认可直接使用的完整营火 Niagara 系统**。本轮面片加基础木石用于验证菜单，最终火焰、烟、火星、木柴与音效仍需美术替换。C++ 不硬编码依赖该第三方素材，缺失时仍保留几何框架。

可选外部素材：[Epic 免费 Niagara Examples Pack](https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600)。本轮未新增下载依赖，是否替换为其中的火焰效果由后续美术调整决定。

场景中只保留一个流程控制器，避免同时放置旧 `BP_AesirLevelFlowDirector` 与新营火 Actor。营火 Actor 关闭 `Is Spatially Loaded`，其 `StartupStreamingSource` 在菜单与相机交接期间维持起点加载，进入探索后交回玩家流送源。此处还未实现按墙缝切换房间的区域流送。

## 当前流程边界

默认未指定 `Opening Sequence Actor`，开始后约 0.6 秒 Blend 到玩家相机并进入 `Exploration`。可调整 `Opening Blend Time`；若后续指定开场 Sequence，关闭该 Sequence Actor 的 Auto Play，并自行配置角色绑定和 Camera Cuts。

当前 PlayerStart 靠近已放置的 Boss；Boss/队友控制器在菜单和开场阶段等待，到 `Exploration` 才启动各自逻辑。这是本轮避免菜单期间提前战斗的边界，**尚不是完整的 BossIntro 触发、演出结束后开战与 Episode 生命周期方案**。角色坐在火边、起身动画和跳过后的最终姿态均未制作；需要用户后续配置动画/Sequence。

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

## 本轮验收记录

| 检查 | 结果 |
| --- | --- |
| Editor 完整编译与 BP 创建/编译 | 2026-10-20 通过；字体采用可序列化的 Engine Roboto UFont，资产生成复验 0 错误、0 警告。 |
| 自动化：菜单输入、重复开始、初始化和交接 | 4 项通过，0 失败、0 警告；覆盖原生/蓝图菜单、按钮事件、开始门禁、输入路由、独立输入锁保留、销毁清理和字体资产引用。报告：`Saved/StartMenu/AutomationFinal/index.json`。 |
| Main 实机 smoke：起点菜单 → 开始 → 探索 | Standalone `-game` 通过：1 个 BossController 在菜单期间保持未启动 Episode，150 HP 保持 10 秒；实际开始按钮完成相机、游戏输入与 HUD 交接。日志：`Saved/Logs/StartMenuMainSmokeFinal.log`。 |
| Main 退出按钮 | 单独启动并点击实际退出按钮通过，进程退出码 0；日志：`Saved/Logs/StartMenuQuitSmoke.log`。 |
| 用户美术调整：镜头、真实火焰效果、UMG 样式、坐姿/起身 | 待用户编辑与实际画面确认 |

1280×720 实机截图保存在 `Saved/StartMenu/Preview/MainMenu.png` 和 `MainStarted.png`。本轮验收为 Editor/Standalone 功能检查，不是 cooked Windows 包或正式性能验收。队友服务离线时仍有原有 HTTP 连接警告；本轮菜单开始未受其阻塞。
