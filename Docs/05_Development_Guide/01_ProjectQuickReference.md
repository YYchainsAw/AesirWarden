# 工程速查

这里集中记录当前 UE 工程的启动、编译与配置位置。日常任务见[开发路线](06_DevelopmentRoadmap.md)，第一关蓝图接线见[流程接入](05_LevelFlowGuide.md)。

## 环境与工程恢复

| 项目 | 当前配置 |
| --- | --- |
| 引擎与模块 | [AesirWarden.uproject](../../AesirWarden.uproject)：UE 5.8，运行模块 `AesirWarden` |
| 开发工具 | Windows C++ 工具链，Rider 或 Visual Studio |
| 运行插件 | `GASToolsets`、`GameplayAbilities`、`StateTree`、`GameplayStateTree`、`MotionWarping` |
| 编辑器插件 | `ModelingToolsEditorMode`，仅 Editor 目标启用 |
| 目标平台 | Windows，默认 DX12；30 FPS 为目标，正式判据见[评估方案](../08_RL_Research/03_EvaluationProtocol.md) |
| 启动地图 | `GameDefaultMap`、`EditorStartupMap` 均为 `/Game/Aesir/Maps/L_OpenWorld` |
| 默认 GameMode | `/Game/Aesir/Framework/BP_AesirGameMode` |

**公开仓库不包含完整可运行资产。** 地图、第三方原始素材和包含外部动作数据的 Animation Sequence 未完整收录，单独克隆不能直接运行完整 Boss 战。项目自制 Blueprint、GAS、BT、UI 及部分动画配置资产按当前白名单跟踪；允许提交的 UE 二进制资产使用 Git LFS。

完整工程恢复仍依赖本地或受限访问副本，以下内容尚待实际备份后补齐：

- 完整工程/`Content` 备份位置：**待补**。
- 对应代码与资产版本、恢复日期和首次运行结果：**待补**。

恢复后生成工程文件，编译 `AesirWardenEditor`，再检查地图、Blueprint 父类、GameMode 和输入绑定。`Binaries/`、`Intermediate/`、`DerivedDataCache/` 是可重新生成的本地产物。

## 编译与迭代

1. 修改 C++ 后编译 `AesirWardenEditor Win64 Development`，再打开编辑器检查相关资产并运行目标关卡。
2. 改动 `UCLASS/USTRUCT/UENUM/UPROPERTY`、组件默认子对象、序列化字段、Blueprint 父类或公共枚举时，先关闭 UE，完整编译后重启；修改 `Build.cs` 或插件依赖后重新生成工程文件。
3. 普通函数实现可使用编辑器中的快速编译流程；功能收口时重启编辑器并做完整构建、PIE 验证。
4. 编译失败先定位第一个错误。蓝图变量丢失、组件重复或默认值异常时，保存日志和受影响资产信息，回到已知良好版本检查后再继续保存资产。

只在确认缓存损坏、备份可用且路径明确时清理中间产物。编译通过后仍需检查实际输入、动作、UI 与关卡流程。

## 服务连接

下表是当前源码默认值，供开发联调；正式目标为本地Boss和远程HTTPS NPC，配置入口、玩家身份、缓存、模型分发和断网要求见[部署说明](../02_Design_Doc/TDD/06_Deployment.md)。当前两个NPC Subsystem仍需补可配置地址及认证，不能只修改INI即宣称已接入正式服务器。

| 模式 | 当前 UE 默认地址 | 运行要求 |
| --- | --- | --- |
| BT Boss | 无外部策略服务 | 本地 BT 战斗可独立运行 |
| PPO Boss | `http://127.0.0.1:8012` | 准备 `aesir-ai-service`、Python 环境与兼容模型；按[Boss RL 契约](../08_RL_Research/01_BossRLContract.md)核对观测、动作和模型 |
| NPC 聊天与命令 | `http://127.0.0.1:8011` | 两个 UE Subsystem 当前使用此默认地址，联调时与 Python 服务实际端口对齐 |

## 配置文件

| 文件 | 负责什么 | 改动后重点检查 |
| --- | --- | --- |
| [DefaultEngine.ini](../../Config/DefaultEngine.ini) | 启动地图、GameMode、渲染和 DX12 | 冷启动、PIE、画质与帧时间 |
| [DefaultGame.ini](../../Config/DefaultGame.ini) | 项目 ID 等通用设置 | 项目标识、打包元数据 |
| [DefaultInput.ini](../../Config/DefaultInput.ini) | Enhanced Input 默认类、轴设置 | 实际键位、映射上下文与运行绑定 |
| [DefaultGameplayTags.ini](../../Config/DefaultGameplayTags.ini) | Ability、Cooldown、Data、Event、State 标签 | 技能激活、Tag 查询、蓝图引用 |
| [DefaultEditor.ini](../../Config/DefaultEditor.ini) | 资产预览场景的灯光、环境等 SharedProfiles | 受影响的资产预览效果 |

`Saved/Config` 是运行时生成配置；排查问题时确认实际生效值，项目默认值在 `Config/Default*.ini` 维护。

## 输入与碰撞

输入资产在 [Content/Aesir/Input](../../Content/Aesir/Input)，绑定入口见[玩家角色](../../Source/AesirWarden/Private/Characters/Player/AesirPlayerCharacter.cpp)。

| 资产 | 用途 |
| --- | --- |
| `IMC_AesirLocomotion`、`IMC_AesirCombat` | 基础移动/视角与战斗映射 |
| `IA_LightAttack`、`IA_HeavyAttack` | 轻击、重击 |
| `IA_Evade`、`IA_Guard`、`IA_LockOn` | 闪避、防御、锁定 |
| `IA_PushToTalk`、`IA_ToggleCompanionChat` | 同伴语音与聊天入口 |
| `IA_TestHeal` | 测试能力，发布前决定是否移除 |

具体键位、手柄绑定、触发类型和映射优先级在 UE 中核对。改动菜单/聊天后，同时检查 UI 焦点和返回游戏后的输入恢复。

[战斗组件](../../Source/AesirWarden/Private/Combat/AesirCombatComponent.cpp)的武器球形查询检查 `ECC_Pawn`；玩家镜头查询涉及 Pawn、WorldDynamic、WorldStatic；Boss Dodge 间隙检查使用 `ECC_Visibility`。当前 INI 未配置项目自定义碰撞通道，Blueprint 和关卡碰撞预设仍需编辑器核对。

改动碰撞时检查命中对象、自身/友军忽略、遮挡、重复命中与边界距离；新通道的用途和响应关系要在配置、C++、Blueprint 中一致。

## 日常验证与打包

每次先完成一条可玩的路径，例如“输入 → 技能执行 → 可见反馈”，再扩展功能。日常记录只写日期、结果和异常，未解决项留在[已知问题](../06_Test_Doc/02_KnownIssues.md)；测试方法查[验证矩阵](../06_Test_Doc/01_VerificationMatrix.md)。

正式 Windows 包按[发布检查](../07_Release/01_ReleaseChecklist.md)验证冷启动、胜败、连续重试、返回菜单和服务断开；RL 对照实验按[评估方案](../08_RL_Research/03_EvaluationProtocol.md)冻结条件。此时再保存对应代码、资产、构建配置、服务/模型版本及日志或原始数据。
