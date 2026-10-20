# 第一关流程控制器接入

> 2026-09-30：已实现 C++ 菜单、开场和探索交接；蓝图接入与 PIE 验收待完成。

2026-10-20 已新增并在 Main 接入可编辑营火菜单，当前制作入口与实测记录见[营火菜单接手指南](07_CampfireMenuSetup.md)。下文保留原有导演的手工接线说明；新 `WBP_CampfireMainMenu` 的开始/退出按钮由原生父类自动绑定。

设计依据：[第一关完整游戏流程](../02_Design_Doc/GDD/01_GameFlow.md)。本阶段覆盖 `Uninitialized → MainMenu → OpeningCinematic → Exploration`。Boss 入场、策略/Episode 启停、胜败和重试仍按后续阶段接入，枚举中的这些状态只是预留。

## 源码

- [AesirLevelFlowTypes.h](../../Source/AesirWarden/Public/Framework/Flow/AesirLevelFlowTypes.h)：原生流程状态枚举和状态变化委托。
- [AesirLevelFlowDirector.h](../../Source/AesirWarden/Public/Framework/Flow/AesirLevelFlowDirector.h)：可配置属性、蓝图入口与表现事件。
- [AesirLevelFlowDirector.cpp](../../Source/AesirWarden/Private/Framework/Flow/AesirLevelFlowDirector.cpp)：初始化、菜单、过场、相机交接与清理。

当前为本地单人流程，仅使用本地 PlayerController 0。Actor 不运行 Tick；初始化检查有 10 秒超时，等待相机 Blend 的定时器只执行一次。

## 编译与蓝图父类

1. 保存并关闭 UE，编译 `AesirWardenEditor Win64 Development`，再启动项目。原生枚举和反射属性变更后，按[工程速查](01_ProjectQuickReference.md#编译与迭代)完整构建并重启。
2. 打开 `/Game/Aesir/Framework/Flow/BP_AesirLevelFlowDirector`，通过 `File → Reparent Blueprint` 选择原生 `AesirLevelFlowDirector`。
3. 若已制作先前的蓝图流程，移除旧的 BeginPlay 初始化、相机/输入切换、定时器，以及同名 `EnterMainMenu`、`RequestStartGame`、`EnterExploration` 自定义事件。改用继承的 C++ 函数，避免两套流程同时执行。
4. 若存在与继承属性同名的蓝图变量，把引用改接继承属性后移除旧变量。流程状态采用原生 `EAesirLevelFlowState`；旧 `/Game/Aesir/Framework/Flow/E_AesirLevelFlowState` 不参与当前代码，清理前检查是否仍有资产引用。

## 场景配置

在蓝图 Class Defaults 中设置：

- `Main Menu Widget Class`：`WBP_MainMenu`。
- `Opening Blend Time`：首版默认 0.6 秒，可在实例中调整；0 表示立即交接。

在 `L_OpenWorld` 中只放置一个流程控制器，设置实例属性：

- `Menu Camera`：营火构图的 `Camera_MainMenu`（CameraActor/CineCameraActor）。
- `Opening Sequence Actor`：首版可留空，点击开始后直接 Blend 到玩家相机。以后指定地图内的开场 LevelSequenceActor，并关闭该 Actor 的 **Auto Play**。
- 保持 GameMode 正常生成并 Possess 玩家，菜单相机不需要替代玩家 Pawn。
- 已放置 Boss 的 `Auto Possess AI` 先设为 `Disabled`，保留正确的 AI Controller Class。此控制器目前不会启动 Boss；Boss 登场后的控制器生成、GAS/策略与 Episode 边界须在下一阶段接入。

缺少相机或菜单类时会记录 Error 并停止初始化；超过 10 秒仍没有本地玩家及其 Pawn 时同样停止。重复放置流程控制器时，后启动的实例会报错并拒绝接管。

开场 Sequence 的 Auto Play 若未关闭，也会报错并停止流程初始化，避免把已经自行开始播放的过场作为菜单后开场使用。

## 菜单按钮绑定

沿用 `WBP_MainMenu` 的无参数分发器 `OnStartRequested`：

```text
WBP_MainMenu：Btn_Start.OnClicked
    → Call OnStartRequested
```

无需为了防重入而永久禁用按钮：C++ 在接受第一次请求后立即离开 MainMenu，后续请求返回 false。若选择禁用按钮，应在请求失败时重新启用，避免无法再次操作。

在 `BP_AesirLevelFlowDirector` 添加继承事件 `Event On Main Menu Created`：

```text
On Main Menu Created（MenuWidget）
    → Cast to WBP_MainMenu（Object = MenuWidget）
    → Bind Event to OnStartRequested（Target = Cast 返回的 Widget）

绑定的无参数事件 HandleStartRequested
    → Request Start Game（Target = Self）
```

只在这个创建事件中绑定一次。C++ 已经创建和保存 Widget，并在此事件之后 AddToViewport、设置焦点和输入模式；蓝图无需重复创建菜单或手动调用 EnterMainMenu。

退出按钮可调用 `Quit Game`，Specific Player 使用 `Get Owning Player`。

## HUD 和场景表现

`OnPhaseChanged(PreviousState, NewState)` 是蓝图表现入口，也可绑定 `OnFlowStateChanged`：

- `MainMenu`、`OpeningCinematic`：隐藏现有战斗 HUD。
- `Exploration`：显示现有战斗 HUD，切换到游玩表现。

HUD 仍使用既有实例。流程控制器不会搜索、隐藏或重新创建项目中的所有 Widgets，也不会自动设置主角坐姿。已有 HUD 若晚于流程初始化才创建，应在创建时读取 `CurrentState` 决定初始可见性。

## 开场 Sequence 和跳过

配置开场 Sequence 后，C++ 以非循环、从起点、正常速度播放。Sequence 不额外叠加移动/视角输入锁；流程控制器在菜单和开场期间统一使用 UI Only，避免攻击输入穿透。

自然结束和停止回调进入同一个相机交接入口，防止双重完成。`SkipOpeningCinematic()` 会先用 `GoToEndAndStop()` 求值 Sequence 的结束位置，再进入同一相机 Blend。过场结束处理使用回调，Blend 使用一次性定时器。

Sequence 的角色绑定、起身动画、最终位置/朝向、动画模式恢复、Camera Cuts 和轨道的 Keep/Restore State 仍需在编辑器中配置并验证。C++ 回调不能代替这些资产设置。外部在中途直接 Stop 并不等价于跳过到结束位置；正式跳过应调用 `SkipOpeningCinematic()`。

`EnterExploration()` 是显式完成入口，正常由 C++ 在 Blend 后调用；重复调用会返回 false。提前从蓝图调用会立即完成交接，应只在确实需要直接交接的路径使用。

## 验收

2026-09-30 的构建记录：UE 5.8 `AesirWardenEditor Win64 Development` 编译与 DLL 链接结果为 `Succeeded`，反射生成代码包含流程枚举、三个入口和蓝图表现事件。蓝图父类、场景配置、按钮接线和实际游玩流程仍待编辑器检查。

以下均为待运行的检查，不代表已通过：

1. PIE 启动后显示营火镜头与菜单；WASD、视角、攻击、举盾和闪避不驱动角色，Boss 不行动。
2. 点击开始后菜单移除，约 0.6 秒后恢复角色相机与游戏输入；连续点击只产生一次状态切换。
3. `OpeningBlendTime = 0` 也能立即进入探索。
4. 加入开场 Sequence 后，自然结束和跳过分别恢复相同的玩家位置、朝向、动画状态与控制权。
5. 两次连续 PIE 不残留菜单、输入锁、定时器或 Sequence 回调。
6. 缺少相机、菜单类，以及没有玩家 Pawn 时，日志给出明确配置/超时错误。

预期日志顺序：

```text
Level flow: EAesirLevelFlowState::Uninitialized -> EAesirLevelFlowState::MainMenu
Level flow: EAesirLevelFlowState::MainMenu -> EAesirLevelFlowState::OpeningCinematic
Level flow: EAesirLevelFlowState::OpeningCinematic -> EAesirLevelFlowState::Exploration
```
