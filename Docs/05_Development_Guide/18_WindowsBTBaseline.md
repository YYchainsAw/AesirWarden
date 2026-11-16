# Windows BT 基线构建与验证

本说明对应 [#8](https://github.com/YYchainsAw/AesirWarden/issues/8)。固定入口为 `/Game/Main`，目标为 Win64 Development。UE 5.8.4 CL58902052 是本次执行环境；正式验收还需基础操作、完整 BT 战斗、连续重试和目标机器记录。

## 资产与源码

先恢复具有使用许可的完整 `Content`，包括 `Main.umap`、World Partition 外部 Actor、角色动作和 UI 依赖。公开仓库有意排除这些私人/第三方资产；仅拉取 Git 和 LFS 不足以恢复第一关。构建记录应保存代码提交、未提交改动，以及本地资产清单的 SHA256。

营火的 `AS_SSH_Sit_Loop` 和 `AS_SSH_Sit_End` 由 C++ 默认软引用按需载入。首次包漏收这两个动画，导致开始游戏后回到菜单。`Config/DefaultEditor.ini` 的 `[AlwaysCookMaps]` 以完整包路径把它们加入 Cook；恢复工程仍需提供对应本地资产。

资产清单、完整日志和包保存在 `Saved` 下，不进入公开仓库。证据索引只提交版本、结果、摘要哈希和相对路径。重试修复由 [PR #33](https://github.com/YYchainsAw/AesirWarden/pull/33) 提供；验收连续重试的包必须纳入该修复并记录实际提交。

## 构建

在工程根目录用 PowerShell 执行：

```powershell
py Scripts/Project/build_windows_baseline.py --dry-run
py Scripts/Project/build_windows_baseline.py --engine 'D:/Program Files/Epic/UE_5.8'
```

脚本调用 `RunUAT.bat BuildCookRun`，构建 Editor 和 Game 目标，Cook `/Game/Main`，生成 Pak/IoStore、压缩并归档到 `Saved/Builds/WindowsBT`。可用 `--project`、`--engine`、`--map`、`--output` 显式指定恢复位置。每次执行单独保存命令、引擎版本、源码状态、退出码和日志到 `Saved/Verification/Windows/<UTC>/`。

首次构建默认完整 Cook；同一恢复工程的后续构建可加 `--iterate` 复用兼容的 Cook 数据。更换资产快照或需要排查缓存影响时重新执行完整构建。

`build_cook_run_succeeded` 仅表示 UAT 返回成功；`cold_start_verified` 仍为 false。正式启动使用包根目录的 `AesirWarden.exe`，不传 `.uproject`，不依赖 UnrealEditor 进程。

## 包内流程冒烟

Development 包可运行 `Aesir.Match.Smoke Victory|Defeat`。入口只在启用开发测试的非 Shipping 构建编译，普通启动不会触发测试。运行：

```powershell
py Scripts/Project/verify_windows_baseline.py --package Saved/Builds/WindowsBT
```

脚本执行包内真正的 Game 可执行文件，分别冷启动 Main，验证菜单开始、原生 Boss 入场、一次性胜败判定、结果页焦点和终局后伤害保护，并收集退出码、PASS/FAIL 标记及日志哈希。截图路径以游戏日志为准，通常位于包自身的 `Saved/Verification/MainMatch`。NPC 连接被限定到不可用的本机测试端口，子进程不继承 Bearer 凭据。

测试通过 GAS 调整生命值触发死亡，并调用原生入场入口。它验证包内流程接线；真实操作走到入口、正常攻击打完整场 BT 战斗、连续两次重试、返回后重新开始、菜单/结果退出、帧率和第二台机器启动仍需单独记录。PPO 和 NPC 的最终验收分别归 #18、#22。

## 证据

保存 UAT 结果、资产清单、包文件 SHA256、运行日志与截图，并在 [BUILD-01](../06_Test_Doc/01_VerificationMatrix.md) 写明实际覆盖范围。失败时记录最先导致失败的错误和复现命令，修复后保留前后两份日志；不将尚未执行的正式检查改为通过。

2026-11-16：修复后生成 44 个文件、约 8.65 GiB 的本地包，Cook 共 2,899 个包、0 错误。Win11 / RTX 4060 Laptop GPU / D3D12 上分别冷启动胜利和失败用例，验证结果页只创建一次，重试按钮取得焦点，两次目标 Boss Episode 均记录为 `BehaviorTree`。离屏结果截图仍偏暗，界面可读性留待实机检查。构建版本、包哈希和两次运行见[证据索引](../07_Release/Evidence/WindowsBTBaseline20261116.json)。
