# 位置触发镜头引导接入

> 2026-10-21：可复用的 `BP_CameraGuideTrigger`。触发位置和展示目标由关卡编辑者自行指定；本次不在 Main 中替用户摆放引导点。验证结果见文末。

## 在关卡里放置并指定目标

1. 完整编译 `AesirWardenEditor Win64 Development` 后重新打开 UE。在内容浏览器找到 `/Game/Aesir/Framework/Camera/BP_CameraGuideTrigger`，拖入目标关卡。
2. 将这个 Actor 移动到希望触发引导的位置。在组件面板选中 `TriggerBox`，调整 `Box Extent`，使范围覆盖玩家需要经过的位置。默认半范围为 `(200,200,150) cm`，即完整尺寸约 `400×400×300 cm`。触发 Box 为只查询的 Pawn 重叠区域；保留它的重叠事件和碰撞设置。
3. 在希望展示的地方放置一个 **Target Point**，或直接使用已有场景 Actor。选中引导 Actor，在 Details 中将 `Target Actor` 指向该对象，并按需要设置 `Target Offset`。
4. 保持 `Aim Mode=Actor Target`，保存地图后 PIE。真实玩家进入触发范围，镜头会平滑转向目标、停留，再恢复鼠标和右摇杆操作。
5. 调整 `Turn Duration` 和 `Hold Duration` 控制转向速度与展示时间。默认结束后保留展示方向；如希望先转回触发前的方向，启用 `Return To Original View`。

这套蓝图已经通过原生父类绑定重叠事件，不必在关卡蓝图或 Event Graph 再接一套 `BeginOverlap`、Timeline、`Set Control Rotation` 或输入锁，否则会产生重复驱动。

## 可编辑参数

| 参数 | 默认值 | 用途 |
| --- | --- | --- |
| `Enabled` | 开启 | 是否接受新的触发。 |
| `Aim Mode` | `Actor Target` | 看向指定场景目标；`Fixed Direction` 则使用方向箭头。 |
| `Target Actor` | 未指定 | 展示目标，通常选择自己放置的 Target Point。 |
| `Target Offset` | `(0,0,0) cm` | 世界坐标偏移，加在目标 Actor 原点上；如 `Z=200` 表示看向目标原点上方 2 米，不随目标自身旋转。 |
| `Turn Duration` | `1.2 s` | 从玩家当前朝向平滑转向目标所需的时间。 |
| `Hold Duration` | `1.5 s` | 转向后保持展示的时间。 |
| `Trigger Once` | 开启 | 成功开始一次即消耗，即使途中取消也保留已触发状态；失败的开始尝试不消耗。 |
| `Lock Movement` | 关闭 | 开启时暂时屏蔽玩家行走输入；该选项不暂停世界、敌人或战斗。 |
| `Return To Original View` | 关闭 | 开启时展示后再转回触发前保存的视角方向，再归还控制。 |
| `Return Duration` | `1.0 s` | 启用返回视角时，返回阶段的转向时间。 |

若只需看向固定方向，可将 `Aim Mode` 改为 `Fixed Direction`，旋转 `DirectionArrow` 组件或整个引导 Actor。此模式不需要指定 Target Actor；箭头世界旋转表示镜头的目标朝向，不表示展示物体的位置。俯仰角仍受当前 PlayerCameraManager 的允许范围限制。

关闭 `Trigger Once` 后，自动触发仍按玩家胶囊进入范围的事件执行：展示结束后需先退出、再进入，不会站在范围内循环播放。玩家出生时已经在范围内会做一次初始检查；若此时仍在菜单或开场，该次检查被拒绝，进入探索后不会自动补播，需要退出再进入。离开触发框不会提前取消已经启动的短引导。一次性触发器如需重新使用，可在蓝图中调用 `Reset Trigger`；它会先取消当前引导再清除已触发状态，不会立刻自动重播。

## 运行时边界

引导继续使用玩家已有的 `FollowCamera / SpringArm`，短时间控制 `Control Rotation`。它不创建新的演出相机，也不切换整个输入模式；引导结束默认保留新朝向，随后由玩家继续操作。

开始前会检查本地玩家、存活状态、当前游戏视角及关卡阶段。项目存在关卡流程控制器时，只在 `Exploration` 接受引导；菜单、营火起身和其他相机演出期间不会启动。同时只有一个引导持有该玩家的镜头控制权。引导期间，项目的锁定镜头辅助和锁定切换会让位，避免两段逻辑同时修改视角；已有敌人锁定并未被删除，结束后锁定辅助会恢复，镜头可能重新看向敌人。因此环境展示点优先放在非战斗路段。

输入锁按持有者配对释放。正常结束、取消、玩家受伤或死亡、目标失效、相机或附身变化，以及关卡退出会清理本次持有的状态，不会使用全局 Reset 解开其他系统的输入锁。取消时保留当前视角，不强制执行返回动画；`Return To Original View` 只用于正常展示结束。

`Target Actor` 使用软引用。它不会为了引导自动同步加载远处的 World Partition 区域；目标尚未加载时不启动，也不消耗一次性触发。应把触发区放在目标已经能够加载并看见的位置，或使用固定方向模式。

SpringArm 的原有碰撞行为继续生效。它可能因墙壁缩臂，这个方向引导不提供营火开场镜头的“人物外缘至少 1 米”路径约束。若某处需要镜头离开玩家沿轨道飞行，应另用演出镜头方案。

## 资产恢复

脚本入口为 [setup_camera_guide.py](../../Scripts/Editor/setup_camera_guide.py)。在完整编译并打开支持 Python 的编辑器后，可显式执行：

```python
import sys, unreal
from pathlib import Path
sys.path.insert(0, str(Path(unreal.Paths.project_dir()) / "Scripts" / "Editor"))
import setup_camera_guide
setup_camera_guide.create_camera_guide_asset()
```

脚本保留已有蓝图默认值与 Graph，不自动放置或保存任何地图；通常只需运行一次，之后直接在关卡里复制并调整引导 Actor。资产检查报告为 `Saved/CameraGuide/asset_setup.json`。

## 蓝图扩展与排查

可绑定 `On Guide Started`、`On Guide Finished` 和 `On Guide Cancelled` 派发器，例如同步显示提示字幕或音效。`Try Start Guide` 可从其他蓝图显式请求启动，它与重叠触发使用相同校验；`Cancel Guide` 可提前归还控制；`Is Guiding`、`Get Stage`、`Get Progress` 和 `Get Last Failure` 可用于运行时查看状态。

若进入范围没有反应，依次检查 `Enabled`、`Has Triggered`、玩家胶囊与 `TriggerBox` 的重叠、目标是否有效且已加载，以及当前是否在 Exploration、玩家是否受击或被其他相机/输入锁接管。失败尝试不自动重试；修复配置后退出再进入，或显式调用 `Try Start Guide`。

## 本轮验收

| 检查 | 结果与证据 |
| --- | --- |
| 原生编译、蓝图创建与重新加载 | `AesirWardenEditor Win64 Development` 完整编译与链接通过，`Saved/Logs/CameraGuideBuild.log`；蓝图创建并编译 0 错误，`Saved/Logs/CameraGuideAssets.log`、`Saved/CameraGuide/asset_setup.json`。另在自动化独立进程重新加载实际 BP 并完成 Fixed Direction 引导。 |
| 位置触发、转向与展示结束归还视角 | 1920×1080 Main Standalone 通过 `CAMERA_GUIDE_MAIN_SMOKE_PASS`，实际 BP 由真实玩家胶囊重叠启动，Actor Target 与 Fixed Direction/返回模式均完成，原有相机、臂长、FOV 和输入恢复。日志 `Saved/Logs/CameraGuideMainSmoke.log`，原始截图 `Saved/CameraGuide/Preview/GuidedTarget.png`、`ControlReturned.png`；测试目标和触发框均为临时对象，未保存地图。 |
| 一次性/重复、互斥和输入锁清理 | `Aesir.CameraGuide` 11 项与 `Aesir.Menu` 16 项共 27 项通过，0 失败；覆盖实际 BP、转向停留、返回、重叠过滤、一次性/重置、互斥和保留其他系统输入锁。报告 `Saved/CameraGuide/Automation/index.json`，日志 `Saved/Logs/CameraGuideAutomation.log`。 |
| 目标失效、受击、销毁和流程/相机接管拒绝 | 自动化的 `TargetLost`、`Interrupted`、`Destroyed`、`Rejection` 通过；角色伤害/死亡检查沿用玩家存活状态与健康变化校验。开始失败不消耗一次触发，取消释放自身计数且不重置其他系统的输入锁。 |

自动化中 11 个引导用例带有已定位的测试夹具警告：原生测试角色未配置武器，首个 GAS 初始化另提示项目未配置 GameplayCue 路径；27 项均无失败，原有菜单 16 项无警告。Main 实机仍有原有队友服务离线等日志，未阻塞本次验证。本轮未进行 cooked Windows 包或正式性能验收；用户最终展示目标与触发位置尚由用户在关卡内选择。
