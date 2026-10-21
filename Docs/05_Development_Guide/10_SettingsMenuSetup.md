# 设置菜单接入与 UMG 调整

设置页使用独立蓝图 `/Game/Aesir/UI/Menus/WBP_SettingsMenu`，原生父类为 `AesirSettingsMenuWidget`。从营火主菜单点击现有“设置”按钮即可进入；关闭后返回原主菜单，焦点回到“设置”。进入设置时整个主菜单临时隐藏，因此原 Logo 和按钮不会叠在设置页之上。

设置页沿用 `F_MenuSerif` 字体和已有 Kenney 金色边框、分割线，面板和按钮中心透明。下拉列表展开时使用局部深色底，便于阅读选项。原主菜单的 Designer 布局、地图和玩家蓝图均不需要重新摆放或接线。

## 第一次恢复设置页资产

本轮新增了原生类和配置字段，先按[工程速查](01_ProjectQuickReference.md)完整编译 `AesirWardenEditor Win64 Development`，再重新打开 UE。如果当前工作区已经包含并能打开 `WBP_SettingsMenu`，直接进入下一节调整。

其他工作区需要创建资产时，使用 [setup_settings_menu.py](../../Scripts/Editor/setup_settings_menu.py)。停止 PIE/Simulate，保存已编辑的设置蓝图，在支持 Python 的 UE 编辑器 Output Log 中切换到 **Python**，执行：

```python
import sys, unreal
from pathlib import Path
sys.path.insert(0, str(Path(unreal.Paths.project_dir()) / "Scripts" / "Editor"))
import setup_settings_menu
setup_settings_menu.setup_settings_menu()
```

日志应出现 `SETTINGS_MENU_ASSET_PASS`，报告位于 `Saved/UI/SettingsMenu/asset_setup.json`。脚本首次创建完整、可编辑的 Designer 控件树；再次运行会验证现有名称、类型、父类和编译结果，保留已调整的布局。存在同名但父类不兼容的资产、未保存的蓝图或缺少必要控件时会停止，不覆盖原资产。

如果资产已经存在，执行前会把 `WBP_SettingsMenu.uasset` 及存在的 `.uexp/.ubulk` 备份至 `Saved/UI/SettingsMenu/BeforeSetup/<时间戳>/`，附带 SHA256 manifest。需要恢复时关闭 UE，按 manifest 中的 `source` 路径复制回对应备份文件，再重新打开检查。

原主菜单原生父类的 `Settings Menu Class` 默认指向 `WBP_SettingsMenu`。复制设置页为其他蓝图时，在主菜单 **Class Defaults → Aesir | Main Menu → Settings Menu Class** 选择新类即可；新类仍须继承 `AesirSettingsMenuWidget`。已有 `Btn_Settings` 自动绑定打开操作，无需再添加一套 Create Widget。

## 页面功能与保存规则

| 页面 | 当前功能 |
| --- | --- |
| 显示 | 全屏、无边框窗口、窗口；系统支持的分辨率；垂直同步；30、60、120、无限制帧率。 |
| 画质 | 整体画质、阴影、纹理、特效，各支持低、中、高、极高、影视。单项与整体预设不一致时显示“自定义”。 |
| 声音 | 总音量，范围 0%～100%。 |
| 操作 | 鼠标灵敏度，范围 0.10～3.00；反转鼠标 Y 轴。 |

无边框模式使用桌面分辨率，分辨率选择器会禁用。列表保留当前帧率上限和分辨率，避免打开设置时把已有自定义值自动改成列表第一项。

拖动滑杆、切换下拉项和勾选开关只编辑本次会话的待应用值，点击“应用”后才修改游戏。音量和灵敏度在编辑阶段也不会即时改变。

- 只改音量、操作、帧率、垂直同步或画质时，“应用”立即生效并保存。
- 改变窗口模式或分辨率时，“应用”先预览显示设置，并显示 **15 秒**倒计时。“保留”才提交全部待应用项并保存；“还原”、Esc、超时或移除设置界面都会恢复先前显示模式。
- 取消显示预览时，其他待应用项继续留在设置页，方便继续调整，尚未生效。还原以开始预览前的实际窗口状态为准；设置页打开期间手动改变窗口尺寸，不会被未编辑过的旧分辨率覆盖。
- 修改后点击“返回”会询问是否放弃；“继续调整”保留修改，“放弃修改”返回主菜单。
- “恢复默认”只填入待应用默认值，还需点击“应用”。默认总音量 100%、灵敏度 1.00、不反转 Y、整体画质“高”、60 FPS、无边框桌面分辨率。

鼠标灵敏度和 Y 轴反转作用于玩家视角输入。当前操作页提供这两项，按键重绑定和独立手柄灵敏度需要后续功能扩展。声音页提供整个游戏设备的总音量，音乐、环境和音效的独立混音需要另行设计 SoundClass/SoundMix。

Esc 或手柄 B 对应“返回”；已有放弃确认时对应“继续调整”；显示确认时对应“还原”。手柄 LB/RB 在四个分类之间切换，确认弹窗开启时不切换页面。

## 在 Designer 调整

1. 打开 `/Game/Aesir/UI/Menus/WBP_SettingsMenu`，在 **Class Settings → Parent Class** 确认 `AesirSettingsMenuWidget`。
2. 在 **Designer** 将预览设为 `1920×1080`。选中 `SettingsSurface` 可整体移动设置面板；默认 Canvas Slot 的 Anchors/Alignment 均为 `(0.5,0.5)`、Position 为 `(0,0)`、Size 为 `1280×760`。
3. 在 `SettingsContent` 中调整标题、分类、分割线和底部操作区。四页位于 `SettingsPages`，选中相应子页即可排版。
4. 字体在 Text Block 的 **Appearance → Font** 中调整；边框在对应 Image 的 **Brush / Color and Opacity** 中调整。按钮常态、悬停、按下的透明度和边线在 **Style** 中调整。
5. `ConfirmPanel` 是 `SettingsContent` 的同级控件。预览弹窗可临时切换其 Visibility 检查排版，检查后恢复 **Collapsed**，再编译、保存并运行关卡。

默认下拉框高 `72`、字号 `30`，Content Padding 和 Button Style 的 Normal/Pressed Padding 的纵向值均为 `0`。Slate 会叠加这两层内边距，SourceHanSerif 的完整中文行高较大；缩小控件时需同时检查所选文字与展开列表，避免文字底部被裁掉。

保留功能控件的名称、类型和四页顺序，运行时即可沿用原生自动绑定。可以移动控件和更换外观；不要给同一个值再添加第二套 Percent/Value 绑定或 Tick 更新，否则可能覆盖会话中的待应用值。

| 名称 | 类型 | 用途 |
| --- | --- | --- |
| `SettingsContent` | 容器 | 普通设置内容；弹窗时隐藏。 |
| `SettingsPages` | Widget Switcher | 子页顺序为显示、画质、声音、操作。 |
| `Page_Display` / `Page_Quality` / `Page_Audio` / `Page_Controls` | Canvas Panel | 默认四页内容容器。 |
| `Btn_TabDisplay` / `Btn_TabQuality` / `Btn_TabAudio` / `Btn_TabControls` | Button | 分类切换。 |
| `Txt_TabDisplay` / `Txt_TabQuality` / `Txt_TabAudio` / `Txt_TabControls` | Text Block | 分类标题与选中颜色。 |
| `Cmb_WindowMode` / `Cmb_Resolution` / `Cmb_FrameLimit` | Combo Box String | 显示设置。 |
| `Chk_VSync` | Check Box | 垂直同步。 |
| `Cmb_OverallQuality` / `Cmb_ShadowQuality` / `Cmb_TextureQuality` / `Cmb_EffectsQuality` | Combo Box String | 画质设置。 |
| `Sld_MasterVolume` / `Sld_MouseSensitivity` | Slider | 音量与灵敏度。 |
| `Chk_InvertY` | Check Box | 反转鼠标 Y。 |
| `Txt_VolumeValue` / `Txt_SensitivityValue` | Text Block | 显示待应用数值。 |
| `Txt_SettingsStatus` | Text Block | 尚未应用、已保存等提示。 |
| `Btn_Defaults` / `Btn_Apply` / `Btn_Back` | Button | 恢复默认、应用、返回。 |
| `ConfirmPanel` | 容器 | 显示保留/还原或放弃确认。 |
| `Txt_Confirmation` | Text Block | 确认文字和显示倒计时。 |
| `Btn_ConfirmYes` / `Btn_ConfirmNo` | Button | 弹窗操作。 |
| `Txt_ConfirmYes` / `Txt_ConfirmNo` | Text Block | 根据弹窗类型切换操作名称。 |

按钮事件由原生父类自动连接，功能控件不必额外开启 **Is Variable**。如果自己的蓝图 Graph 需要引用某个控件，可按需开启。`SettingsPages` 保持四页，确认层保持在其上层，避免切换页面时连确认弹窗一起切走。

## 配置与蓝图扩展

设置后端为 `AesirGameUserSettings`，`DefaultEngine.ini` 中的 `[/Script/Engine.Engine] GameUserSettingsClassName` 指向 `/Script/AesirWarden.AesirGameUserSettings`。更改该配置后需重启进程，旧引擎设置实例不会在运行时自动替换。

音量、灵敏度、Y 轴反转和 UE 原有显示/画质项都通过 `GameUserSettings.ini` 保存，不写入玩家进度存档。具体生成目录由运行目标决定，通常位于 `Saved/Config/WindowsEditor/` 或 `Saved/Config/Windows/`。UE 5.8 的 `GGameUserSettingsIni` 是配置缓存键，实际文件路径从对应 `FConfigBranch::IniPath` 解析，不能直接作为磁盘文件名。

需要自定义设置入口时，可 `Create Widget` 这个类，设置 Owning Player，绑定 `On Closed` 后添加到视口。`Begin Settings Session` 在正常 Construct 时自动执行，已关闭的实例重新开始会话时会读取已应用值；在正在编辑的会话中重复调用会保留暂存修改和确认状态，不会丢掉玩家尚未应用的设置。Blueprint 可通过 `Get Pending Settings` / `Set Pending Settings` 批量编辑，再调用 `Request Apply`，保留相同的确认和保存流程。

设置页本身不接管关卡进度、营火演出或战斗 HUD。主菜单流程继续保持 UI 输入、禁止角色移动/转向、Boss 等待和 HUD 隐藏；返回菜单仍需点击“开始游戏”才播放原有起身和镜头切换。

## 验证入口

以下是维护入口；具体通过结果以本机最新日志为准。

| 验证 | 入口与检查 |
| --- | --- |
| 会话自动化 | `Automation RunTests Aesir.Settings`：真实 `WBP_SettingsMenu`，真实开关/下拉框/滑杆事件与数值文字、待应用值、放弃/继续、恢复默认、保存后重载、显示保留/还原、界面移除时还原、真实 15 秒超时。 |
| Main 实机检查 | 单独运行 `/Game/Main`，1920×1080，控制台执行 `Aesir.Settings.Smoke`。真实“设置”按钮进入四页并截图，正常返回、未应用修改的放弃确认、菜单 Logo/HUD 显隐、焦点、营火镜头和输入隔离。 |

自动化 fixture 会在修改设置之前通过配置分支解析真实路径，备份 `GameUserSettings` 与 `EditorSettings` 的磁盘字节、配置内存及保存/运行时修改层，结束时还原。`EditorSettings` 的位置可能在用户配置目录；测试不假定它在项目 `Saved` 下。原文件存在时另保留副本于 `Saved/Settings/BeforeAutomation/`，日志 `SETTINGS_CONFIG_BACKUP` / `SETTINGS_CONFIG_RESTORED` 记录实际路径和恢复结果。超时用例实际等待约 15 秒，不通过改写计时器跳过确认过程。

Main 检查需以独立 `UnrealEditor.exe ... -game -windowed -ResX=1920 -ResY=1080` 进程运行。命令完成后自动退出该游戏进程，在编辑器或 PIE 中会拒绝执行。此检查只编辑后放弃，不执行应用、不修改地图或玩家进度；通过标记为 `SETTINGS_MAIN_SMOKE_PASS`，截图在 `Saved/Settings/Preview/`：`Display.png`、`Quality.png`、`Audio.png`、`Controls.png`、`Discard.png`，另有 `result.txt`。

手动检查显示模式保留与超时时，请在独立游戏窗口中进行，观察预览、倒计时和原模式恢复。声音需实际播放环境音检查音量；进入游戏后转动鼠标检查灵敏度和反转。自动截图能验证控件和流程，听感、操作手感以及不同显示器上的独占全屏表现需在对应设备上确认。

## 2026-10-21 本机验证记录

| 检查 | 结果与本地证据 |
| --- | --- |
| 完整编译与蓝图 | `AesirWardenEditor Win64 Development` 编译、链接通过。`WBP_SettingsMenu` 编译 0 错误并保存，报告 `Saved/UI/SettingsMenu/asset_setup.json`。 |
| 共用流程回归 | `Aesir` 共 41 项通过、0 失败，包含 Menu 16、CameraGuide 11、HUD 5、Settings 9；报告 `Saved/Settings/AutomationFinal/index.json`。其中 24 项带测试场景/配置警告，不能视为零警告报告。 |
| 最终设置测试 | 调整字体布局及配置恢复 fixture 后，`Aesir.Settings` 9 项通过、0 失败、0 警告；报告 `Saved/Settings/AutomationPreserved/index.json`，日志 `Saved/Logs/SettingsPreservedTests.log`。包含真实控件事件、保存重载、预览确认/取消/销毁、15 秒真实超时、窗口先被外部调整后的回滚。 |
| 配置恢复 | 测试结束后核对项目和用户目录的设置文件。引擎启动单独更新的最近打开时间在关闭辅助进程后恢复；三个受核对文件 SHA256 与运行前一致，记录 `Saved/Settings/BeforeFinalVerification/result.json`。 |
| Main 实机界面 | 独立 `/Game/Main`、1920×1080 通过 `SETTINGS_MAIN_SMOKE_PASS`。四个分类、返回和丢弃流程通过；逐张检查透明边框、字体与数值，下拉框文字完整。最新日志 `Saved/Logs/SettingsMainSmoke.log`，五张截图 `Saved/Settings/Preview/`。 |

本轮未进行 cooked Windows 包验收。原主菜单用户布局、地图和玩家蓝图未重新保存；此次新建的设置蓝图在排版修复前备份于 `Saved/UI/SettingsMenu/BeforeTypographyFix/`。
