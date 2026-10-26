**DarkRuins 地图性能与加载验证流程（UE 5.8）**

2026-10-19。适用于 `D:\YYchainsaw\Unreal-Projects\DarkRuinsMegascansSample` 的 `/Game/Main`，以及迁入 AesirWarden 的关卡。本文保留初始测试操作说明；截至 2026-10-20 的实测结果、参数取舍、迁移完成状态与后续工作见[性能与地图交接报告](04_DarkRuinsPerformanceAndMigrationHandoff.md)。第 3 节的 100% 是初始定位基线，后续性能候选采用 MegaLights 2 采样和 85% 渲染比例，条件与适用范围以交接报告为准。

本机已识别 RTX 4060 Laptop GPU、约 8 GB 专用显存、32 GB 级别系统内存。样例已有 World Partition 和 HLOD Layer 资源；资源存在、编辑器能打开或 PIE 能启动，均不能替代运行时加载与帧时间验证。

AesirWarden 当前文档中的 30 FPS 是目标，最低硬件和正式低帧判据尚未冻结，见[评估方案](../08_RL_Research/03_EvaluationProtocol.md)。下文给出测试预算与排查触发条件，不将它们登记成已经通过的验收标准。

**1. 固定环境，先登记原始设置**

1. 笔记本接电，固定系统性能模式、显卡模式和散热条件。记录 GPU/驱动、CPU、内存、UE 版本、测试包版本和地图版本。
2. 测试安装在同一块本地 SSD 上。暂停下载、资源导入和其他编译任务，正式打包版测试时关闭 UE 编辑器。
3. 在样例的 `Main` 里先选好可重复的路线，记录起点、终点、相机朝向、移动速度及沿途截图。正式首遍测试前重新启动游戏进程。
4. 建议选 4–5 个固定位置：起点、开阔室外、密集建筑、狭窄通道、拟用作 Boss 场地的位置。按地图实际连通关系选择，不要求这些位置一定连接成一条路线。
5. 先截图保存原始画质、分辨率、World Settings 和 Player Controller 的流送设置。下面的 `scalability 2` **会保存用户画质设置**，结束后需要恢复原值。
6. 等待编辑器着色器编译和资源派生任务完成后再启动测试。首次使用的 PSO 仍可能在游戏内创建，需单独记录。不要为了测试删除 DDC、驱动缓存或整个 Saved 目录。

正式记录分开列出：样例/迁入关卡、Standalone/打包版、未限帧/30 FPS 限帧、首遍/重复路线、是否开启详细追踪。不同条件的数据不要混合。

**2. 启动与控制台入口**

初查：打开样例项目的 `Main`，在播放按钮旁选择 **Standalone Game**。地图如果只有自由相机，可先做场景渲染与流送基线；飞行结果不能代替真实玩家的碰撞、寻路和战斗验证。

正式复测：在编辑器的 Packaging 设置中选择 **Development**，通过 Platforms → Windows 打包；确认 `/Game/Main` 被包含。运行打包游戏时关闭编辑器，并确认实际运行的是此地图。此流程使用开发构建中的控制台与统计功能，不能假设 Shipping 包提供相同功能。

点击游戏窗口，用英文输入法按 `~`（通常是 Esc 下方按键）打开控制台，一行一行输入命令。如果按键无效，在 Project Settings → Input → Console Keys 检查绑定。需要较长输出时展开控制台，或在日志中查看。

**3. 建立统一的 1080p 高画质基线**

先输入以下查询命令，并保存输出或截图；单独输入 CVar 名称通常会显示当前值及来源：

```text
scalability
r.SetRes
r.VSync
t.MaxFPS
r.DynamicRes.OperationMode
r.ScreenPercentage
r.AntiAliasingMethod
r.TextureStreaming
```

再按以下顺序设置测试条件：

```text
r.SetRes 1920x1080w
scalability 2
r.VSync 0
t.MaxFPS 0
r.DynamicRes.OperationMode 0
r.ScreenPercentage 100
```

含义：1920×1080 窗口、各画质组 High（2）、关闭垂直同步、取消 `t.MaxFPS` 限制、关闭动态分辨率、固定主渲染比例为 100%。`scalability 2` 后再设比例，避免预设覆盖比例。

`r.SetRes` 在编辑器视口内无效，应在 Standalone/打包游戏中执行。再次查询上述设置，确认输出分辨率与渲染比例实际生效；如果项目的平滑帧率、游戏设置或驱动仍在限帧，也需固定并记录。插件提供的动态分辨率/超分辨率设置若覆盖这些参数，应在游戏设置中另外关闭或固定。

不要在这一轮顺便开启/关闭 Nanite、光追、MegaLights、阴影或改纹理池。先测上述统一条件，原样例最高画质可另外测一组。抗锯齿方法保持同一值并登记。

**4. 用 HUD 找到问题大类**

先逐条输入：

```text
stat fps
stat unit
stat unitgraph
```

| 指标 | 记录与解释 |
| --- | --- |
| Frame | 实际每帧耗时；30 FPS 对应约 33.3 ms，60 FPS 对应约 16.7 ms |
| Game | 游戏线程耗时；高时继续查 Actor、蓝图、AI、动画、物理等 |
| Draw / RHIT | 渲染线程/RHI 侧耗时；高时继续查场景提交、对象注册与渲染工作 |
| GPU | GPU 渲染耗时；高时查光照、阴影、反射、材质、分辨率等 |

这些时间存在并行、同步与等待，**不能把 Game、Draw、GPU 相加当成 Frame**。哪个数接近 Frame 只是瓶颈线索，不足以单独确认根因。

按需要逐个打开其他面板，不要同时把所有统计都叠在屏幕上：

```text
stat gpu
stat streaming
stat rhi
stat memory
stat virtualtexturememory
```

- `stat streaming`：主要看普通纹理流送需求、池占用及超预算情况，不是 World Partition 地图加载状态，也不代表全部显存。
- `stat virtualtexturememory`：虚拟纹理内存统计。样例启用了 VT，普通纹理池看起来正常，仍需检查 VT。
- `stat rhi`：RHI 资源统计，不保证等于操作系统看到的全部 GPU 内存占用。
- `stat memory`：UE 内存统计；同时在任务管理器记录游戏进程内存和 GPU 专用/共享内存。记录测试是否导致持续增长、明显换页或画面长期模糊。

如怀疑虚拟纹理池压力，另外进行一轮诊断：

```text
r.VT.Residency.Show 1
r.VT.Residency.Notify 1
```

查看各 VT 物理池的驻留/超预算提示。完成后恢复：

```text
r.VT.Residency.Show 0
r.VT.Residency.Notify 0
```

统计组一般再次输入同一条 `stat ...` 切换关闭。也可以用以下命令清除统计 HUD：

```text
stat none
```

`stat none` 仅关闭统计显示，不保证停止 CSV、UnrealStats 或 Trace 录制。正式 CSV 采集时尽量关闭无关 HUD、分区调试绘制和屏幕录制，减少测量开销。

**5. 验证 World Partition 的运行时加载、卸载**

先在编辑器里检查并截图：

- World Settings 中是否启用 **Enable Streaming**，采用哪种 Runtime Hash，以及实际分区/加载范围设置。
- Player Controller 的 **Enable Streaming Source** 是否开启；若使用额外 Streaming Source，记录它的位置、范围和启用状态。
- 抽查几个远近不同区域的场景 Actor，检查 **Is Spatially Loaded**；检查预期卸载的区域是否被常驻 Actor、关卡蓝图或跨区引用带着一起加载。
- Data Layer 的初始状态；Activated 并不自动意味着所有空间 Actor 都全图常驻。

先保留原始设置做基线。需要修改时，用地图副本或明确记录每一次修改，并重启测试实例进行同条件比较。

进入游戏后逐条输入：

```text
stat levels
wp.Runtime.ToggleDrawRuntimeHash2D
wp.Runtime.ToggleDrawStreamingSources
```

后两条是切换命令，再次输入即可关闭，**不要追加 0/1**。如果 2D 绘制不易观察，可关闭 2D 后试：

```text
wp.Runtime.ToggleDrawRuntimeHash3D
```

观察玩家移动时加载单元是否随位置变化，远离起点并超过加载范围后，原区域的单元是否进入卸载状态；返回后是否重新加载。`stat levels` 中绿色表示已加载且可见，蓝色可表示已卸载但仍在内存等待 GC。常驻管理对象和远景 HLOD 仍在是正常情况，不能要求所有远景都消失。

如需调整调试显示层级，先确认 World Settings 的 Runtime Hash 类型，只选对应一组：

RuntimeHashSet：

```text
wp.Runtime.HashSet.ShowDebugDisplayLevel 0
wp.Runtime.HashSet.ShowDebugDisplayLevelCount 4
wp.Runtime.HashSet.ShowDebugDisplayMode 0
```

RuntimeSpatialHash：

```text
wp.Runtime.ShowRuntimeSpatialHashGridLevel 0
wp.Runtime.ShowRuntimeSpatialHashGridLevelCount 4
```

不要因为不匹配的专用命令没显示，就判断 World Partition 没有工作。先用通用的 2D/3D 绘制和 `stat levels`。

验证至少覆盖正常跑图、快速返回、预期最快移动速度及可能的传送/重试。如果实际游戏包含传送，检查目标区域是否准备好再交还玩家控制，避免掉出地面、碰撞缺失或敌人延迟出现。

单元卸载不要求进程内存或显存立即下降，缓存和资源池可能保留已分配空间。结合单元状态、对象/资源报告以及多次往返后的内存趋势判断。不要在正式跑图中手动 GC 来制造“内存释放正常”的结果。

**6. 检查 HLOD 是否在替代远景**

选择远处应该可见的建筑群，停在固定位置，暂时输入：

```text
wp.Runtime.HLOD 0
```

观察哪些远景代理消失，然后恢复：

```text
wp.Runtime.HLOD 1
```

再正常靠近/远离，确认高精度场景与远景代理的切换是否合理，有无明显洞口、错误遮挡或突变。HLOD Layer 资源存在，不代表 HLOD 已构建并生效。

**关闭 HLOD 不会自动加载远处原始高精度 Actor。** 因此此操作只帮助识别代理，不能用开关前后的 FPS 差值声称 HLOD 提升了多少性能。测试构建/参数调整的收益，需要视觉覆盖相当的配置和同条件路线。

正式性能采集前恢复 HLOD，关闭刚才启用的分区调试绘制。

**7. 采集固定路线的 CSV 数据**

推荐最小测试矩阵：

| 记录 | 操作 | 建议时长/次数 |
| --- | --- | --- |
| 静止观察 | 在选定最复杂位置站立并转动镜头 | 30 秒；单独记录 |
| FirstPass | 新启动游戏进程后，首次沿固定路线正常移动 | 约 180 秒，1 遍 |
| Repeat01–03 | 同一进程中重复同一路线，固定起点、朝向、速度和路线方向 | 每遍约 180 秒，共 3 遍 |
| FastMove | 以游戏允许的最快速度跨区、快速转向 | 约 120 秒，单独记录 |
| Endurance | 反复往返并观察内存是否持续增长 | 至少 10 分钟，单独记录 |

FirstPass 只表示本进程首遍，不表示已经清空操作系统磁盘缓存、DDC 或 GPU 驱动缓存。不同设置应先完成预热，再比较重复路线；不要将第一次创建 PSO 的开销只算在其中一个配置上。

每一遍都按以下顺序执行，起止处打开控制台本身会产生干扰帧，分析时用预先约定的边界规则排除控制台操作段，并保留原始文件：

1. 到固定起点，确认设置相同，先关闭无关统计 HUD 和调试绘制。
2. 输入开始命令，关闭控制台，然后开始路线：

```text
CsvProfile START
```

3. 走到终点后打开控制台，输入：

```text
CsvProfile STOP
```

4. 等待日志出现 CSV 写入完成及实际文件路径，保存该遍的文件，并登记 FirstPass/Repeat01 等标签。不要把开始和停止两条命令连续执行而没有实际走图。

默认位置：

```text
D:\YYchainsaw\Unreal-Projects\DarkRuinsMegascansSample\Saved\Profiling\CSV\
```

CSV 至少检查 `FrameTime`（毫秒）列。其他 CPU/GPU 列取决于启用的类别和构建，不能假设默认 CSV 已含完整 GPU 分项。打包版 Saved 目录可能重定向，以该次日志报告的绝对路径为准。

建议由 CSV 计算并登记：总帧数、有效采样时长、平均 FPS、P95/P99 帧时间、最大帧时间、超过 50/100 ms 的帧数。平均 FPS 使用 `1000 / 平均 FrameTime(ms)`，不要对每帧的 `1000/FrameTime` 直接取算术平均。1% low 如需记录，统一采用“最慢 ceil(总帧数×1%) 帧的平均耗时，再以 1000 除以它”，并在报告中保留此定义。

50/100 ms 是本流程的观察阈值，不是项目已确认的发布门槛。不要只用平均 FPS 或某一张截图判断是否流畅。

**8. 单独采集卡顿的 Unreal Insights 时间线**

如果 FirstPass/Repeat 出现明显卡顿，再做一轮详细诊断，不和轻量 CSV 基线混在一起。重走出现问题的路线，先检查追踪状态：

```text
Trace.Status
```

若已经在录制到 Insights 或另一个文件，先输入 `Trace.Stop`。然后输入：

```text
Trace.File DarkRuins_Hitch01.utrace cpu,gpu,frame,bookmark,loadtime,file,log
Trace.Status
Trace.Bookmark RouteStart
```

关闭控制台，复现卡顿。到终点后输入：

```text
Trace.Bookmark RouteEnd
Trace.Stop
```

下一轮换一个文件名，避免覆盖。`Trace.File` 生成 `.utrace`，默认在项目 `Saved\Profiling`。`Trace.Status` 用于核实正在采集及启用的通道；不要只看到命令输入成功就认定已有有效记录。使用 `Trace.File`，不使用本机 UE 5.8 已标记弃用的 `Trace.Start`。

打开本机工具：

```text
D:\Program Files\Epic\UE_5.8\Engine\Binaries\Win64\UnrealInsights.exe
```

在 Insights 中通过 Open Trace File 打开对应 `.utrace`，在 Frames 中选择长帧，检查 CPU、GPU、Asset Loading 和文件 I/O 轨迹；这些通道已开启也需确认轨迹里实际有事件。

| 时间线里的证据 | 优先检查 |
| --- | --- |
| 卡顿附近有资产读取/加载，并与等待重合 | I/O、异步加载依赖、同步加载调用 |
| GameThread 在组件注册、物理状态创建等位置耗时突出 | 分区激活阶段的 Actor/组件、碰撞、实例组织 |
| 出现 PSO 创建/等待，或对应日志记录 PSO creation hitches | 首次管线创建及预缓存策略 |
| GPU 持续超预算，而非只有跨区一帧 | GPU 轨迹中的光照、阴影、材质、反射等实际热点 |
| 往返多轮后出现 GC 长帧或内存持续增长 | 对象生命周期、常驻引用、资源峰值与 GC |

事件名称和细节取决于构建；只有相关性时先标为疑似。PSO 工作可能出现在 CPU 等待中，不能把所有跨区长帧都归为磁盘慢。

需要从进程启动开始采集加载/PSO，或更详细 CPU 事件时，可使用下面的 **PowerShell 命令**。它不是游戏控制台命令；先退出本次测试实例，输出文件名每轮更换：

```powershell
New-Item -ItemType Directory -Force -Path 'D:\YYchainsaw\Unreal-Projects\DarkRuinsMegascansSample\Saved\Profiling' | Out-Null
& 'D:\Program Files\Epic\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' `
  'D:\YYchainsaw\Unreal-Projects\DarkRuinsMegascansSample\DarkRuinsMegascansSample.uproject' `
  '/Game/Main' '-game' '-windowed' '-ResX=1920' '-ResY=1080' '-log' `
  '-trace=cpu,gpu,frame,bookmark,loadtime,file,log' '-statnamedevents' `
  '-tracefile=D:\YYchainsaw\Unreal-Projects\DarkRuinsMegascansSample\Saved\Profiling\DarkRuins_Startup01.utrace'
```

此命令用编辑器可执行文件启动独立游戏，不等于 Development 打包版。它适合初查；正式包用实际游戏 exe 搭配相同追踪参数，再单独登记构建。`-statnamedevents` 增加细节，也增加开销，只用于诊断轮。

进入游戏后仍需核对画质与分辨率；本次启动已经在录制，不要再启动第二个 `Trace.File`。在目标时刻用 `Trace.Bookmark` 标记，完成后 `Trace.Stop`。

启动加载时间与跑图帧时间分开登记。加载时长应采用固定边界，例如“启动进程→可控制角色且起点资源准备好”；`GenerateStreaming` 的日志耗时、编辑器打开地图耗时不能冒充打包游戏进图时间。

**9. 采集内存报告与替代统计文件**

内存报告单独做，不在计时路线中执行。在固定位置、等待相同时间后输入：

```text
memreport -full
```

默认输出在项目 `Saved\Profiling\MemReports`，以日志路径为准。可以分别保存起点、最复杂区域、多轮往返后报告，并标明位置和轮次。此命令会刷新加载、执行 GC 和同步渲染，可能造成明显暂停和改变后续状态，不能拿报告触发的卡顿当游戏卡顿，也不能用报告后的内存下降证明自然卸载效果。

如果需要传统 Session Frontend 的统计录制，可在独立一轮使用：

```text
stat startfile
```

实际完成路线后再输入：

```text
stat stopfile
```

默认输出在项目 `Saved\Profiling\UnrealStats`，格式 `.uestats`，通过 Session Frontend → Profiler 打开。它不等同于 `.utrace`，也不要用 Unreal Insights 打开它。无需为了同一路线同时启用 CSV、UnrealStats 和详细 Trace。

**10. 用对照实验确定优化方向**

保持同一构建、路线、相机和预热方式，一轮只改一个变量；修改后先观察画面是否仍具可比性。

- 渲染压力疑似过大：单独一轮输入 `r.ScreenPercentage 70`，预热后重跑；若 GPU 耗时明显下降，说明像素/渲染相关负载值得优先检查，但不能据此认定全部瓶颈。完成后 `r.ScreenPercentage 100`。这是定位用画质变化，单独标记，不能混入原基线。
- 跨区加载疑似过慢：在副本中比较原加载范围与一个经过记录的调整值。范围过小可能加载跟不上，过大可能内存压力上升，不直接抄固定格子尺寸或范围作为答案。
- PSO 疑似卡顿：比较同进程首遍/重复遍，结合对应 PSO 日志和时间线。不要先关闭预缓存后再声称原地图有问题，也不要把所有首遍差异都归因 PSO。
- 玩法成本：迁入 AesirWarden 后，以相同场景/相机比较“仅玩家”“玩家+BT Boss”“玩家+PPO Boss”，分别登记条件。AI 对照须遵守已有研究协议，不能把新增队友、不同人数或不同地图混作策略差异。

帧时间靠近 33.3 ms 的空场景已经缺乏 30 FPS 玩法余量。先报告实际数据，讨论给战斗预留的 CPU/GPU 预算；不要把临时经验值写成已经冻结的项目需求。

**11. 限帧体验验证与收尾**

未限帧测试完成后，单独一轮输入：

```text
t.MaxFPS 30
```

实际用玩家移动、转镜头、锁定、闪避、攻击，并检查跨区、死亡重试、返回菜单再开始。观察是否有掉地面、碰撞未准备好、镜头穿模、敌人状态丢失、Boss/队友跨区消失等问题。限帧轮和未限帧容量数据分别保存；限帧后 Frame 接近 33.3 ms 可能只是正常等待，不能当作 GPU 已耗尽预算。

结束所有采集，并恢复本轮调试项：

```text
CsvProfile STOP
stat stopfile
Trace.Stop
stat none
r.VT.Residency.Show 0
r.VT.Residency.Notify 0
wp.Runtime.HLOD 1
```

只对实际打开过的分区绘制切换命令再执行一次，将其关闭；不要盲目执行切换命令，反而开启绘制。将 `r.ScreenPercentage`、`t.MaxFPS`、VSync、动态分辨率和画质恢复到第 1 步登记的原值。`stat none` 不能替代停止采集命令。

每一遍至少留下以下记录，结果未知时留空，不预填“通过”：

| 构建/地图/配置 | 轮次/路线/时长 | 平均 FPS | P95/P99 帧时间 | 最大帧时间 | >50/>100 ms 帧数 | 内存/显存峰值与趋势 | 单元加载/卸载 | 异常位置/证据文件 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 待记录 | 待记录 | — | — | — | — | — | — | — |

保存每轮 CSV、卡顿轮 `.utrace`、对应进程日志、必要的内存报告及画质/路线截图。打包版的 Saved 位置可能重定向；以该次输出日志为准，将实际绝对路径登记在记录中。

**命令核验来源**

本流程关键命令已只读核对本机 `D:\Program Files\Epic\UE_5.8\Engine\Source`：`UnrealEngine.cpp`（分辨率、Scalability 分发、MemReport）、`Scalability.cpp`（画质保存行为）、`CsvProfiler.cpp`（CSV 采集）、`TraceAuxiliary.cpp`（Trace.File/Bookmark/Stop）、World Partition 的 Subsystem、HashSet/SpatialHash DebugDraw、HLOD Runtime Subsystem，以及 VT 内存驻留统计。尚未代用户执行任何跑图、录制或验收。

- [Epic：Stat Commands](https://dev.epicgames.com/documentation/en-us/unreal-engine/stat-commands-in-unreal-engine)：统计 HUD、关卡状态、UnrealStats。
- [Epic：World Partition](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine)：流送源、空间 Actor、运行时调试。
- [Epic：Unreal Insights Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-insights-reference-in-unreal-engine-5)：Trace 通道、Trace.File 和 Trace.Stop。
- [Epic：Trace Quick Start](https://dev.epicgames.com/documentation/en-us/unreal-engine/trace-quick-start-guide-in-unreal-engine)：追踪采集与分析。
- [Epic：Virtual Texture Memory Pools](https://dev.epicgames.com/documentation/en-us/unreal-engine/virtual-texture-memory-pools-in-unreal-engine)：VT 池与驻留观察。
- [Epic：PSO Precaching](https://dev.epicgames.com/documentation/unreal-engine/pso-precaching-for-unreal-engine)：首次 PSO 创建与运行时卡顿。
