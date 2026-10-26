# 队友 v0.4 文本接入与验证

对应 [UE #19](https://github.com/YYchainsAw/AesirWarden/issues/19)。开发基线为 UE `17a8fef`、服务 `df0f4f9aba6e805e040ad8103e76cf7b9f4c59ce`；后者已合入服务 [#97 目标角色解析修复](https://github.com/YYchainsAw/aesir-ai-service/issues/97)。本页说明本轮源码、配置和验证入口，最终运行结果在文末逐项填写。

2026-10-26 联调修正：`17a8fef` 尚未包含 `codex/campfire-start-menu@9dd9414` 的营火菜单、同步起身/镜头、设置与 HUD。队友开发分支现已合入该菜单分支，继续保留 main 为祖先。默认地图恢复为 `/Game/Main`；菜单/起身阶段 Boss 与队友等待，进入 `Exploration` 后才启动现有逻辑。完整本地工程测试时应同时使用这次集成后的代码和资产，不能只切到旧 `f9f19c7`；被 Git 忽略的 Main 地图和外部 Actor 仍须保留本地副本。

## 本轮范围

`UCompanionChatSubsystem` 使用严格 JSON `/v1/scoped`：先发现角色上下文，再以 CAS 绑定游玩，随后提交文本或心跳。回复必须匹配当前玩家/存档/角色、服务实例、游玩绑定、记忆 epoch、会话和快照；本轮探索请求的 `encounter_id` 为 null，`session_id` 使用 `run_id`。

玩家文本统一走 `/v1/scoped/agent/step`。服务负责既有意图解析和普通聊天回退，UE 不另建关键词解析器。探索执行只接受 `source=player_command` 的 `follow`、`wait`：跟随的 payload 必须且只能为 `target_id=party.player`，等待的 payload 必须为空。实际导航、存活状态、当前阶段和重复执行仍由 UE 复核。合法的其他来源回复不授权探索动作。

这仅覆盖公共 scoped 基础设施在 #19 中需要的发现、绑定、epoch、取消和旧响应隔离。[#24](https://github.com/YYchainsAw/AesirWarden/issues/24) 的完整公共交付、遭遇控制，以及 [#20](https://github.com/YYchainsAw/AesirWarden/issues/20) 的战斗接入/回执仍需分别完成；离线队列、持久恢复等 [#21](https://github.com/YYchainsAw/AesirWarden/issues/21) 范围尚待接入。scoped SSE、ASR、VAD、TTS 和条件指令登记不在本轮。

## 配置与启动

UE 从 `[Aesir.Companion]` 读取非秘密配置，随后由进程环境变量覆盖。可将本机非秘密值放在生成配置 `Saved/Config/WindowsEditor/Game.ini` 的同名节中；正式包对应 `Saved/Config/Windows/Game.ini`。默认配置见 [DefaultGame.ini](../../Config/DefaultGame.ini)。

| INI 字段 | 环境变量 | 当前默认值 / 规则 |
| --- | --- | --- |
| `ServiceBaseUrl` | `AESIR_NPC_BASE_URL` | `http://127.0.0.1:8011`；与实际服务端口一致，部署时使用已配置的 HTTPS 地址 |
| `GameId` | `AESIR_NPC_GAME_ID` | `aesir`，必须与服务配置和响应一致 |
| `SaveId` | `AESIR_NPC_SAVE_ID` | 无默认值；必须是服务已授权给当前主体的 UUID |
| `CompanionId` | `AESIR_NPC_COMPANION_ID` | `companion.alice`；角色必须已注册且场景有相应实例 |
| `RequireBearer` | — | 默认 `false`；设为 `true` 时客户端缺少凭据即停止连接 |
| 无凭据配置字段 | `AESIR_NPC_BEARER_TOKEN` | 仅从进程环境读取，不能写入 INI、蓝图、资产或提交日志 |

服务的身份模式由服务端决定。`RequireBearer=false` 仍会携带环境中已有的 token，也不会绕过服务端 Bearer 校验。开发固定身份模式中的玩家和存档须在服务配置中预登记，UE 自报一个 UUID 不会创建权限。

从已注入凭据的 PowerShell 启动 UE 时，可设置本次进程的非秘密参数：

```powershell
$env:AESIR_NPC_BASE_URL = 'http://127.0.0.1:18019'
$env:AESIR_NPC_GAME_ID = 'aesir'
$env:AESIR_NPC_SAVE_ID = '27190000-0019-4019-8019-000000000019'
$env:AESIR_NPC_COMPANION_ID = 'companion.alice'
& 'D:\Program Files\Epic\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' 'D:\YYchainsaw\.codex-worktrees\AesirWarden-companion-issue19\AesirWarden.uproject'
```

这个固定存档仅供下文隔离开发服务和 HTTP 契约测试。实际联调改用目标服务授权的存档；环境变量须在启动 UE 的进程中设置，已运行的编辑器不会继承另一个终端的修改。

## 关卡、输入与 UI 接线

1. 完整编译 `AesirWardenEditor Win64 Development` 后重启 UE。本轮包含反射字段、UI 原生类和模块依赖变更，不以 Live Coding 代替首次完整构建。
2. 在实际地图使用的 PlayerController 蓝图中，将 `Toggle Companion Chat Action` 指向 `IA_ToggleCompanionChat`，并把该 Action 映射到已由此 Controller 加载的 Enhanced Input Mapping Context。已有 Action 时按键由实际 IMC 决定；未配置 Action 时默认按 **T** 打开聊天，Esc 关闭。
3. `Companion Chat Widget Class` 留空时使用原生 `AesirCompanionChatWidget` 回退面板，提供输入、发送、关闭、重试上一条和取消请求。复用现有 WBP 时可指定原 Widget 类并继续使用 Subsystem 接口；需要继承原生绑定时，父类改为 `AesirCompanionChatWidget`，可选控件名称/类型为 `ChatInput`（EditableTextBox）、`Transcript`（TextBlock）、`CompanionName`（TextBlock）、`TranscriptScroll`（ScrollBox）。自制按钮可调用下节 API。
4. 场景至少有一个存活的玩家和目标队友。队友 `CompanionId` 与配置相同，使用 `AesirCompanionAIController`；默认 Alice 可先使用现有资产。跟随需要可行走的 NavMesh、可达路径及合适的接受半径，不能以收到 HTTP 200 代替移动结果。
5. 在探索阶段打开聊天，检查焦点落在输入框，移动、转镜头和攻击不会穿透 UI。Esc 或关闭按钮关闭聊天；再次打开应能输入。关闭时只释放聊天拥有的输入锁，过场或死亡等其他状态不应被误解锁。
6. 输入 `EIRIN follow` 和 `EIRIN wait`（也可使用服务人格配置的本角色唤醒词），分别观察跟随和停止等待。不要用针对 Alice 的唤醒词验证 Bruno 的成功命令；Bruno 应使用其已注册唤醒词，例如 `BRUNO follow`。第二套美术资产不作为协议路由检查的前置条件。

当现有流程控制器离开 `Exploration`、角色死亡或世界清理时，聊天与单局请求会失效。没有流程控制器的开发地图按探索沙盒处理；正式过场仍需让真实流程状态反映当前阶段，不能只播放动画而不切换状态。

## Blueprint / C++ 入口

通过 GameInstance 获取 `CompanionChatSubsystem`；Blueprint 使用 `Get Game Instance Subsystem`。源码入口为 [CompanionChatSubsystem.h](../../Source/AesirWarden/Public/Services/Companion/CompanionChatSubsystem.h)、[原生聊天 UI](../../Source/AesirWarden/Public/UI/AesirCompanionChatWidget.h) 和 [队友 Controller](../../Source/AesirWarden/Public/AI/Companion/AesirCompanionAIController.h)。

| 接口 / 事件 | 用途 |
| --- | --- |
| `SendChatMessage(Text)` | 发送 1～500 字符文本；连续输入进入本地有界队列，依次处理 |
| `GetSelectedCompanionId()` / `SetSelectedCompanionId(Id)` | 读/切换当前角色；使旧角色回调失效，重新取得目标角色 epoch，同一存档继续共享一个 run |
| `ConfigureSession(BaseUrl, GameId, SaveId)` | 切换服务/账号或读入另一授权存档前调用；先失效旧会话，再使用新配置，凭据仍从环境读取 |
| `RetryLastMessage()` | 仅重试当前结果未确认的原逻辑请求，返回是否接受重试 |
| `ResetConversation()` | 取消本地待处理操作、失效旧回调并尽力结束旧 run；下次操作重新发现/绑定，不清服务长期记忆和关系 |
| `OnReplyReceived` | 已通过身份、快照和有效期校验的回复 |
| `OnRequestFailed` | 连接、权限、绑定、取消或未知提交等错误提示；自制 UI 应显示此事件 |
| `OnConversationReset` | 清理旧角色/旧局的 UI 内容与本地表现 |
| `OnDirectiveReceived` | 已校验的探索建议；现有队友 Controller 做最终执行复核和去重 |

返回菜单、重新开始/重试和同档读档的入口主动调用 `ResetConversation()`；读入新存档或切换主体时调用 `ConfigureSession()` 并使用相应授权 UUID/凭据。自动阶段检查和世界清理是额外保护，不能代替尚未接通的正式流程入口。

## 请求生命周期与结果含义

文本和心跳使用以下顺序：

1. `GET /v1/scoped/context?save_id=...&companion_id=...`，取得服务实例、可信 scope、控制 revision 和角色 `memory_epoch`。
2. `POST /v1/scoped/runs/bind`，携带原 revision/预期 run 做 CAS，取得新 binding；切角色时复用同一活动 run，不为各 NPC 各建一局。
3. `POST /v1/scoped/agent/step`，仅此实时请求附本次世界快照与 timing；普通文本未识别时由服务返回对话或 `none`。
4. 本地复核有效期与阶段，仅执行受支持的玩家探索动作；`directive_id`、`request_id`、`run_id` 用于关联和去重。
5. 取消使用 `/v1/scoped/requests/cancel`；离开时尽力发送 `/v1/scoped/runs/end`。本地先失效，退出与切图不等待服务器成功。

正式客户端快照和请求预算均为 **8 秒**。请求首次发送后记录本地单调时钟；重试保留原 `request_id`、world snapshot、binding、epoch 和预算，仅累计尝试年龄。响应 `valid_for_ms` 从首次发送起算，`before_seconds` 的较短动作窗口也会收紧本地截止时间，收到缓存回包不会重新续期。HTTP 契约测试有独立的 10 秒测试预算，不代表正式客户端预算改变。

请求发送失败、超时或服务报告提交结果不确定时，不自动换 ID 重做。原生 UI 的“重试上一条”调用 `RetryLastMessage()`；超过原预算后重试失败，使用“取消请求”或 `ResetConversation()` 清掉本地等待，再明确输入新意图。服务端取消不能撤销已提交事实，本地仍会丢弃旧回调。重置会话不等于服务记忆 reset。

HTTP 200 表示服务产生了回复；服务请求 `completed` 不代表 UE 动作完成，`source=mock` 也不代表真实 LLM 验证。实际跟随/等待应同时记录 `Companion directive applied` 或失败日志中的 request/directive/agent，以及 UE 中真实位置和状态变化；战斗阶段/命中回执留给 #20。

## 构建与自动化命令

以下路径按本次独立工作树给出；在其他机器替换 `$ue`、`$project` 和服务路径。生成产物放在 `Saved`，命令不会补回未公开的地图/第三方资产。

```powershell
$ue = 'D:\Program Files\Epic\UE_5.8'
$project = 'D:\YYchainsaw\.codex-worktrees\AesirWarden-companion-issue19\AesirWarden.uproject'
& "$ue\Engine\Build\BatchFiles\Build.bat" AesirWardenEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE
$report = 'D:\YYchainsaw\.codex-worktrees\AesirWarden-companion-issue19\Saved\CompanionText\Automation'
& "$ue\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $project /Engine/Maps/Entry -unattended -nop4 -nosound -NullRHI '-ExecCmds=Automation RunTests Aesir.Companion' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$report" "-AbsLog=$report\Automation.log"
```

`Aesir.Companion.ScopedProtocol` 覆盖纯协议身份、CAS 后态、有效期、动作参数及回放；`Aesir.Companion.Lifecycle` 检查本地生命周期。HTTP 契约用例只有显式设置 `AESIR_NPC_CONTRACT_URL` 才联网；未设置时输出跳过说明，不能记作端到端通过。`/Engine/Maps/Entry` 自动化不替代真实关卡、输入和导航验收。

## 隔离 loopback HTTP 测试

使用与 `df0f4f9` 契约一致的服务源码和已安装依赖的 Python 3.12 环境；先核对服务 HEAD。本轮实际使用 PR #104 的 head `26e4b8d3f46c91f765f4ffafe901d991c03ebe32`，其代码已随 `df0f4f9` 合并，包含 #97 修复。避免使用旧 `develop-yjx` 工作副本。以下在单独终端前台运行，数据落在独立临时根。

```powershell
$serviceRepo = 'D:\YYchainsaw\.codex-worktrees\aesir-ai-service-alignment-20261023'
$python = 'D:\YYchainsaw\.codex-worktrees\aesir-ai-service-alignment-20261023\.venv\Scripts\python.exe'
$testRoot = Join-Path $env:TEMP ('aesir-npc-contract-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot | Out-Null
Set-Location -LiteralPath $serviceRepo
git rev-parse HEAD
$env:PYTHON_DOTENV_DISABLED = '1'
$env:AESIR_GAME_ID = 'aesir'
$env:AESIR_SCOPE_IDENTITY_BACKEND = 'development'
$env:AESIR_SCOPE_DEV_PLAYER_ID = '77777777-7777-4777-8777-777777777777'
$env:AESIR_SCOPE_DEV_SAVE_IDS = '["27190000-0019-4019-8019-000000000019"]'
$env:AESIR_SCOPE_STORAGE_BACKEND = 'json'
$env:AESIR_SCOPE_LEGACY_ENABLED = 'false'
$env:AESIR_INTENT_BACKEND = 'rule'
$env:AESIR_PARSER_BACKEND = 'rule'
$env:AESIR_COMPANION_BACKEND = 'mock'
$env:AESIR_ASR_BACKEND = 'mock'
$env:AESIR_MEMORY_ROOT = Join-Path $testRoot 'memory'
$env:AESIR_RELATIONSHIP_ROOT = Join-Path $testRoot 'relationship'
$env:AESIR_RECEIPTS_DIR = Join-Path $testRoot 'receipts'
$env:AESIR_DIALOGUE_SIGNALS_DIR = Join-Path $testRoot 'dialogue_signals'
$env:AESIR_AGENT_STEPS_DIR = Join-Path $testRoot 'agent_steps'
$env:AESIR_EVENT_SIGNALS_DIR = Join-Path $testRoot 'event_signals'
$env:AESIR_REQUEST_TRACES_DIR = Join-Path $testRoot 'request_traces'
& $python -m uvicorn app.main:app --host 127.0.0.1 --port 18019 --workers 1
```

其他机器需将 `$serviceRepo` 替换为相同服务版本的实际副本。`$python` 指向已有虚拟环境。保留本次 `$testRoot` 作为证据，结束后正常停止这个专用服务进程。本轮自动验证所用启动脚本另外清空子进程继承的 `AESIR_*`/`LLM_*` 配置后再设置上述值，以免混入其他实例配置；这些修改仅作用于专用测试进程。

在运行 UE 自动化的终端设置：

```powershell
$env:AESIR_NPC_CONTRACT_URL = 'http://127.0.0.1:18019'
```

随后执行上节自动化命令，或只运行 `Automation RunTests Aesir.Companion.HttpContract.ExplorationRouting`。测试仅接受带明确端口的纯 HTTP loopback origin，使用固定测试 save，检查 Alice follow、同 ID 重试复用 directive、wait、Bruno 路由隔离与关闭 run。该测试不携带 Bearer token，不能指向真实账户或 Bearer/SQL 实例。真实 LLM、身份凭据、持久事务恢复、游戏导航和 GAS 均需另记证据。

## Windows 包复验入口

恢复完整地图、外部 Actor 和合法来源素材后，才使用 UAT 构建开发包。将 `$packageMap` 指向本次实际验收地图；下面 `/Game/Main` 仅用于已恢复该地图的完整工程。

```powershell
$packageMap = '/Game/Main'
$archive = 'D:\YYchainsaw\Builds\AesirWarden-CompanionText'
& "$ue\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$project" -nop4 -platform=Win64 -clientconfig=Development -build -cook "-map=$packageMap" -stage -pak -archive "-archivedirectory=$archive" -utf8output
```

冷启动包时沿用其启动进程的配置/环境，复验聊天焦点、移动/攻击输入交接、follow/wait、连续输入、取消、切角色、旧回包、死亡/过场/切图及服务不可达。cook 成功与可执行文件存在不代替这些用例的真实运行结果。

## 验证记录（2026-10-26）

UE 开发分支 `codex/companion-issue-19`，main 基线 `17a8fef`。本轮实际服务提交为 `26e4b8d3f46c91f765f4ffafe901d991c03ebe32`（PR #104 head，已合入 `df0f4f9`），CPython 3.12.15，单 worker、loopback `18019`、规则解析/mock 对话、development 身份与 JSON 存储。独立运行数据保存在本工作树的 `Saved/Companion/ServiceRuntime`，没有使用真实账户、LLM 或 SQL。

最终 Editor 构建成功，7 项 UE 自动化全部通过，0 项失败、0 项测试警告、0 项跳过。构建仍含引擎弃用 API 与非首选 MSVC 版本提示。首次探索执行测试的临时 World 未初始化 Actor，已补齐初始化后保留全部断言重跑；以下均为最后一次运行结果。

| 检查 | 实际结果 | 版本 / 日志 / 证据 |
| --- | --- | --- |
| Editor 完整构建、链接与反射 | 通过 | `Saved/Logs/CompanionBuildFinal.log`；`AesirWardenEditor Win64 Development` |
| `Aesir.Companion.ScopedProtocol` | 4 项通过 | scope/CAS、epoch、双层快照、首次计时/短指令有效期、非法参数和回放；`Saved/Companion/AutomationFinal/index.json` |
| `Aesir.Companion.Lifecycle` | 1 项通过 | 旧/非活动回调、共享 run 切角色、会话清理、重入保护、配置拒绝和过期请求释放；同上报告 |
| `Aesir.Companion.ExplorationExecution` | 1 项通过 | 真实 transient GameWorld 中 wait 清速度、重复无第二次物理副作用、旧 generation/过期拒绝、无 NavMesh 的 follow 明确失败及 reset 隔离。尚未验证真实导航跟随位移 |
| loopback HTTP 契约 | 1 项通过，已启用 | 原生 UE HTTP → 真实 Uvicorn：Alice 闲聊/follow/同 ID retry/wait、Bruno 同 run 路由隔离、run end。`Saved/Logs/CompanionAutomationFinal.log`、上述报告及 `Saved/Companion/ServiceRuntime` |
| 真实地图 Alice 聊天、跟随/等待和输入交接 | 待执行 | 记录地图/资产、导航、请求/指令 ID、坐标/行为和录屏/日志 |
| Bruno 路由隔离 | HTTP 路由通过；真实角色资产待验 | 已验证错误唤醒词仅对话无动作、正确唤醒词只指向 Bruno，未制作第二角色资产 |
| 队列、原逻辑重试、取消及切角色/读档/返回的迟到结果 | 专项自动化部分覆盖；真实流程故障注入待验 | 已覆盖旧回调和重复指令、同 ID HTTP 重试及过期释放；未据此宣称完整地图中的断网/取消/切图全部通过 |
| Bearer/真实 LLM 联调 | 待执行 | 脱敏凭据模式、人格/协议版本、供应商与原始计时；token 不进入附件 |
| cooked Windows 包 | 待执行 | 包版本、地图/资产、启动配置、上述实际流程及故障证据 |

只有实际运行后才更新结果。服务端测试、HTTP 契约、源码检查和 UE 实际动作分别记账，不据本页创建或文档修订关闭 #19、#22 或 #24。

### 营火菜单集成复验

2026-10-26 将菜单分支 `9dd9414` 合入队友分支，集成提交 `4e212e3`；`c8421bf` 补齐当前 HUD 已引用、原存放于 develop 的六个 Emberhold 图标及其来源/授权文件。默认地图恢复为 `/Game/Main`，同时保留 #19 聊天与指令执行。没有修改本地 Main 地图、营火外部 Actor 或用户正在调整的 PlayerController 蓝图。

| 检查 | 结果与证据 |
| --- | --- |
| 隔离工作树及完整本地工程 Editor 构建 | 均通过；完整工程 `Saved/Logs/CompanionMenuRestoreBuild.log` |
| 菜单和队友集成回归 | 22 项通过，0 失败、0 测试警告、0 跳过：菜单 16 项、队友协议/生命周期/探索执行 6 项。报告 `Saved/Companion/MenuRestoreRegression/index.json`；本次未重复 HTTP/LLM 测试 |
| Main 真实 Standalone | `START_MENU_PRESENTATION_PASS`、`START_MENU_CAMERA_PASS`、`START_MENU_MAIN_SMOKE_PASS`；进程退出码 0。菜单等待 10 秒保持玩家 150 HP、两个 Boss Episode 未启动；实际开始按钮、Sit_End、安全镜头、游戏输入和 HUD 交接通过。日志 `Saved/Logs/CompanionMenuRestoreMainFinal.log` |
| 资源与本地改动 | 最终 Main 日志无 `LoadErrors`；Main 地图、营火外部 Actor 及用户 PlayerController 蓝图的 SHA256 与修复前一致。备份和清单在完整工程 `Saved/Companion/MenuRestoreBackup-a623a8fe531147788c1e78a75581e81c/` |

真实菜单截图为完整工程 `Saved/StartMenu/MainMenu.png`，起身和探索截图为同目录 `MainStandingUp.png`、`MainStarted.png`。此复验确认菜单恢复和两功能线兼容，不替代 #19 的真实导航跟随、输入操作和远端联调验收。
