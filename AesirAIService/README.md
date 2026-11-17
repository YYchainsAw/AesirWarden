# 本地 Boss 推理程序

服务源码位于 https://github.com/YYchainsAw/aesir-ai-service 。游戏通过 `UAesirBossProcessSubsystem` 管理冻结 Windows onedir 推理包。

把完整发行目录放到工程的 `ThirdParty/BossPolicy/Windows/`，保留 `aesir-boss-policy.exe`、`release-manifest.json` 和 `_internal/`。Win64 Game 构建会将该目录作为 NonUFS 文件放到游戏可执行文件旁的 `BossPolicy/`。

启动游戏时加 `-AesirBossManaged`，或在 `[Aesir.BossProcess]` 中设置 `Enabled=true`。编辑器下也可用 `-AesirBossExecutable="D:/path with spaces/aesir-boss-policy.exe"` 指定完整本地包。

UE 异步校验配置中的 EXE 和清单 SHA256，启动专属进程，再核对 `/health` 的 release、model、SHA256 和 v4/26/9 身份。校验和服务启动期间继续显示菜单。Policy Client 等待就绪后请求决策，服务失败时转入 BT。原有 Boss 蓝图的 RL 开关仍决定使用哪种策略。

`StartupTimeoutSeconds` 默认 120 秒，可在 1 到 300 秒间配置。当前冻结包在监听前会校验完整文件树并加载 CPU 模型，冷启动耗时受存储速度影响。

重试和返回菜单会结束旧局的请求，GameInstance 持有的进程可跨关卡复用。游戏退出时回收自己启动的进程。进程输出保存在 `Saved/Logs/BossProcess-*.log`，UE 日志记录启动 PID、就绪身份及失败原因。

`DefaultGame.ini` 当前固定的是 2026-10-23 研发快照。更换发行包时，按可信发行记录更新 `ExecutableSHA256` 和 `ManifestSHA256`。正式模型候选和真实 RL 战斗继续由 AI #96、UE #5/#18 验收。
