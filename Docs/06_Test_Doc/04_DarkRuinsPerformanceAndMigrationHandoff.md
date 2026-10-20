# DarkRuins 地图、性能测试与优化交接报告

交接日期：2026-10-20，Asia/Shanghai。当前状态核对至主项目 10:53–10:57 的 Standalone 运行记录；性能数据主要采于 10 月 19–20 日。本文供同一台电脑上的另一段会话接续，原始 CSV、Trace、地图资源均保留。

## 1. 接手时先读这些结论

1. 用户希望将 DarkRuins 样例作为 AesirWarden 的游戏地图。地图已有 **World Partition、Nanite 和 HLOD Layer 资源**，不需要从零设计地图流送系统；这些资源的存在也不等于整图性能已经验收。
2. 已完成源项目的静止/跑图 CSV、分辨率定位、MegaLights 开关与采样对照、候选预设、Development 打包复测、Unreal Insights 停顿定位、完整供电新基线、查询缓冲 1/2 对照。
3. **源项目独立 Development 包**在完整供电、1080p、High、MegaLights 2 采样、85% 渲染比例下，最新正向两轮平均约 **54.75 / 55.94 FPS**。它们支持继续推进地图集成，不能直接记作迁入主项目后的成绩，也不能声称稳定 60 FPS。
4. 已确认的目标长帧涉及 **GPU 遮挡查询结果同步，以及一处 VSM 资源创建/内存提交**。尚无证据支持“先写自定义地图加载算法”是当前首要方案。PSO 和地图磁盘 I/O 不能只凭邻近计数就被认定为根因。
5. `r.NumBufferedOcclusionQueries 2` 只有小幅改善线索，正向平均 FPS 差约 **2.18%**，仍出现 55 ms 长帧；保留为临时测试候选，未证明修复卡顿。`r.Shadow.Virtual.DynamicDeallocationMode 2` **尚未进行 A/B**，不要写成已经完成的优化。
6. 地图已迁入 AesirWarden。本轮迁移清单 **11,655 个文件全部通过源/目标 SHA-256 核验**，地图外部 Actor/Object 齐全。首次迁移 OOM 已通过旧迁移流程恢复。
7. 用户已确认导入和运行成功。最新主项目 Standalone 日志进一步证明 **玩家与 BT Boss 已交战、双方伤害生效**；不应再要求从零重复放置 Boss。本局遥测结果为 `Aborted`，尚不证明胜败、重试和返回菜单闭环通过。
8. 主项目当前试玩条件与源项目性能基线不同：**抗锯齿仍为 0、最新启动参数为 1280×720、MegaLights 采样为 4**，未发现完整应用 1080p/TSR/2 采样/85%/查询缓冲 2 的证据。主项目尚无新的正式性能 CSV/Development 包基线。

## 2. 工程与证据位置

| 用途 | 当前电脑上的路径 |
| --- | --- |
| 主项目 | `D:\YYchainsaw\Unreal-Projects\AesirWarden` |
| 源样例项目 | `D:\YYchainsaw\Unreal-Projects\DarkRuinsMegascansSample` |
| 主地图包名 | `/Game/Main.Main`，文件为主项目 `Content\Main.umap` |
| 引擎 | `D:\Program Files\Epic\UE_5.8`，测试识别 UE 5.8.0-55116800 |
| 源项目性能包及启动器根目录 | `D:\YYchainsaw\Unreal-Projects\DarkRuinsMegascansSample\Saved\PerformanceBuilds\DarkRuins_Dev_20261019` |
| 源项目打包测试 CSV | 上述根目录的 `RunData\Saved\Profiling\CSV` |
| 源项目打包测试日志 | 上述根目录的 `RunData\PackagedDevelopment.log` |
| 早期编辑器 Standalone CSV | 源项目 `Saved\Profiling\CSV` |
| 派生报告、分析 JSON、图表、分析脚本 | `C:\Users\YYchainsaw\.codex\visualizations\2026\10\19\01a15223-fa2e-78d1-b5db-e210fb07ad5a` |
| 原有测试操作说明 | [03_DarkRuinsPerformanceTest.md](D:/YYchainsaw/Unreal-Projects/AesirWarden/Docs/06_Test_Doc/03_DarkRuinsPerformanceTest.md) |

这些是本机路径。若交接给另一台电脑，需要额外带走所需原始数据和资产；仅传本文不会携带约 2 GB 的 Trace、原始 CSV 或地图。

## 3. 测试过程与为什么选择当前预设

硬件识别为 Ryzen 7 8845H、RTX 4060 Laptop GPU、约 8 GB 专用显存、32 GB 级别系统内存。笔记本供电条件在后期改变；没有记录实时功率、温度或时钟，因此不把跨阶段提升全部归功于电源。

### 3.1 最初三段跑图：确认问题类型

2026-10-19，同一编辑器启动的 Standalone 进程。1080p 输出、100% 渲染比例、MegaLights 4 采样，多数画质组为 High，但 ShadingQuality=3；不是后续每个画质组均为 2 的正式包。

| 用户确认的场景 | 采集时长/秒 | 平均 FPS | 1% low FPS | P99/ms | 最长帧/ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| 静止转镜头 | 29.6 | 28.48 | 12.95 | 49.25 | 107.81 |
| 正常跑图 | 125.3 | 29.11 | 13.99 | 46.32 | 113.04 |
| 原路快速跑动返回 | 42.2 | 30.35 | 13.53 | 43.87 | 105.11 |

GPU 平均约 31.5–33.5 ms，接近 30 FPS 的 33.3 ms 预算；GameThread 平均约 4–5 ms。静止段也有约 **5.06 秒一次**的周期尖峰，因此不能把所有卡顿解释成跨区磁盘加载。该周期现象的全部原因尚未证实；后续测试部分尖峰减弱或未复现，不等同已经找到并修复统一根因。

正常跑图和返回轮有流送/清理活动，但这些是瞬时采样，不是独立加载单元总数。编辑器同时运行时上报显存预算约 5955 MB，部分峰值只剩几十 MB，促使后续关闭编辑器测独立包。

详见[初始三段分析](C:/Users/YYchainsaw/.codex/visualizations/2026/10/19/01a15223-fa2e-78d1-b5db-e210fb07ad5a/DarkRuins_CSV_Analysis_20261019.md)。

### 3.2 单变量定位与预设选择

以下几组主要是固定镜头、同进程的编辑器 Standalone 对照。它们用于判断方向，不与后续打包版和不同供电条件混算成累计收益。

| 对照 | 平均 FPS | GPU 平均/ms | 支持的决定 |
| --- | --- | --- | --- |
| 内部渲染比例 100% → 70% | 27.28 → 43.34 | 35.23 → 21.65 | 此视角明显受渲染分辨率影响；70% 是定位条件，最终候选用 85% |
| MegaLights 开 → 关 | 27.39 → 17.07 | 35.09 → 56.97 | 保留 MegaLights；关闭后常规灯光/阴影路径更贵 |
| MegaLights 4 → 2 采样 | 27.39 → 30.88 | 35.09 → 30.95 | 2 采样可作为性能档候选，不能预期总 GPU 成本减半 |

补充边界：

- 比例 70% 表示内部宽高约为原来的 70%，像素约 49%，输出仍为 1080p；它包含画质取舍。
- MegaLights 关闭前后的 GPU 截图也显示 Shadow Depths、Lights 等成本上升。不能因 MegaLights 自身约 6 ms，就推断关闭会净省 6 ms；父子统计和异步队列也不能相加。
- 关闭 MegaLights 的预热阶段出现过 VSM 页池和灯数溢出、PSO 等待警告。它们不全在正式 30 秒样本内，不可把预热切换停顿记入样本长帧数。
- 用户对 2 采样反馈为“阴影增加了非常少量的噪点、闪烁或拖影”。这只是一轮观察，不是所有灯光、运动和战斗场景的完整画质验收。
- 2 采样对照后段有性能漂移，间隔内也切换过 MegaLights。保留此条件下的收益方向，不承诺每个场景固定提升 12.7%。

对应报告：[比例对照](C:/Users/YYchainsaw/.codex/visualizations/2026/10/19/01a15223-fa2e-78d1-b5db-e210fb07ad5a/DarkRuins_Resolution_AB_20261019.md)、[MegaLights 开关](C:/Users/YYchainsaw/.codex/visualizations/2026/10/19/01a15223-fa2e-78d1-b5db-e210fb07ad5a/DarkRuins_MegaLights_AB_20261019.md)、[采样数对照](C:/Users/YYchainsaw/.codex/visualizations/2026/10/19/01a15223-fa2e-78d1-b5db-e210fb07ad5a/DarkRuins_Samples_AB_20261019.md)。

### 3.3 候选预设、打包复测与完整供电

候选预设逐步确定为：1080p 输出、High、MegaLights 开启、2 采样、85% 渲染比例、VSync/动态分辨率关闭、不限帧。

- 10/19 编辑器 Standalone 跑图：平均 **36.30 FPS**，1% low **23.22 FPS**，最长 **147.80 ms**。约 95.75% 帧在 33.3 ms 内，仍有三个明显长帧和静态网格准备等待线索。
- 10/19 两份完整 Development 打包路线约 **37.78 / 37.99 FPS**。第二份正式统计保留约 77 ms 首帧，路线中还有约 46.60 / 49.99 ms 长帧。这两份之间曾重启进程，不能称严格同进程热缓存复测。
- 10/20 用户报告电脑接上完整电源后，新基线达到 **54.75 FPS**。相对历史参考约 44% 的总变化还混有缓存、路线、进程及系统状态差异，不能写成“接电优化提升 44%”。

对应报告：[候选预设跑图](C:/Users/YYchainsaw/.codex/visualizations/2026/10/19/01a15223-fa2e-78d1-b5db-e210fb07ad5a/DarkRuins_Preset_Run_20261019.md)、[首轮打包对照](C:/Users/YYchainsaw/.codex/visualizations/2026/10/19/01a15223-fa2e-78d1-b5db-e210fb07ad5a/DarkRuins_Packaged_Run_20261019.md)、[打包重复路线](C:/Users/YYchainsaw/.codex/visualizations/2026/10/19/01a15223-fa2e-78d1-b5db-e210fb07ad5a/DarkRuins_Repeat_Run_20261019.md)。

## 4. 最新源地图性能基线与查询缓冲对照

这三轮都是源项目 Windows Development 独立包、D3D12/SM6、RTX 4060 Laptop、1920×1080 窗口、各画质组 High（2）、MegaLights=1、采样=2、SP=85%、VSync=0、DynamicRes=0、MaxFPS=0。用户报告完整供电。三轮同一游戏进程中完成。

| 指标 | A：缓冲 1 正向 | B：缓冲 2 返回 | C：缓冲 2 正向 |
| --- | ---: | ---: | ---: |
| CSV 时间标签 | 20261020_090715 | 20261020_091556 | 20261020_092447 |
| 有效帧数 | 6558 | 6719 | 6773 |
| FrameTime 合计/秒 | 119.790 | 118.952 | 121.082 |
| 平均 FPS | 54.75 | 56.49 | 55.94 |
| 1% low FPS | 44.77 | 45.72 | 46.02 |
| P99/ms | 21.57 | 20.58 | 20.69 |
| 最长帧/ms | 35.05 | 55.33 | 55.34 |
| >33.3 / >50 / >100 ms 帧数 | 1 / 0 / 0 | 1 / 1 / 0 | 1 / 1 / 0 |
| GPU 平均/ms | 17.12 | 16.55 | 16.75 |
| RT Visibility 聚合等待平均/ms | 9.49 | 9.25 | 9.12 |
| 有效显存使用峰值/MB | 5851.73 | 5898.92 | 5899.86 |
| 有效同帧最小显存预算余量/MB | 801.82 | 775.88 | 781.09 |

源包运行日志在 08:55:59 明确设置缓冲 1，在 09:15:37.770 改为 2，之后本次对照期间未见更改。C 与 A 正向路线大体重合，但累计距离为 272.0 / 252.7 米，镜头、速度和预热仍有差异。

**可以确认：**C 相比 A 平均 FPS 高约 2.18%，1% low 高约 2.78%，P99 低约 4.08%，Visibility 平均等待减少约 0.37 ms。**不能确认：**这些小幅差异全部由缓冲参数造成，或等待和长帧已经消失。

C 的 55.34 ms 出现在录制后 **0.87 秒**，前两帧有 45.05 ms Visibility 等待，随后 RT 53.43 ms，再后表现为长帧。附近有 PSO miss，但无 GC，CSV 无法确定具体根因。该帧保留正式统计；首尾各去两秒后最长为 28.96 ms，只作为边界敏感性补充。B 的 55.33 ms 则发生在约 110.59 秒，不能将两者自动套用为旧 Trace 同一种原因。

C 中只有约 **15.46%** 的帧在 16.7 ms 内。多数帧有 30 FPS 预算余量，但没有稳定 60 FPS 证据，未来战斗、特效、传送和全地图仍需单独验证。

B 有 2 个、C 有 1 个显存预算与使用同时记 0 的样本，仅从显存有效样本中排除，FrameTime 全部保留。零值不是“显存余量耗尽”；两分钟记录也不足以证明长期泄漏或换页。

用户对查询缓冲 2 的“没有注意到明显异常”反馈来自 **B 返回轮**，不能扩展为 C、新主项目和全地图高速移动均已验收。

原始 CSV：

- [A：缓冲 1 正向](D:/YYchainsaw/Unreal-Projects/DarkRuinsMegascansSample/Saved/PerformanceBuilds/DarkRuins_Dev_20261019/RunData/Saved/Profiling/CSV/Profile(20261020_090715).csv)
- [B：缓冲 2 返回](D:/YYchainsaw/Unreal-Projects/DarkRuinsMegascansSample/Saved/PerformanceBuilds/DarkRuins_Dev_20261019/RunData/Saved/Profiling/CSV/Profile(20261020_091556).csv)
- [C：缓冲 2 正向](D:/YYchainsaw/Unreal-Projects/DarkRuinsMegascansSample/Saved/PerformanceBuilds/DarkRuins_Dev_20261019/RunData/Saved/Profiling/CSV/Profile(20261020_092447).csv)

完整报告：[完整供电基线](C:/Users/YYchainsaw/.codex/visualizations/2026/10/19/01a15223-fa2e-78d1-b5db-e210fb07ad5a/DarkRuins_Power_Baseline_20261020.md)、[缓冲 2 返回](C:/Users/YYchainsaw/.codex/visualizations/2026/10/19/01a15223-fa2e-78d1-b5db-e210fb07ad5a/DarkRuins_Occlusion_Return_20261020.md)、[同方向缓冲对照](C:/Users/YYchainsaw/.codex/visualizations/2026/10/19/01a15223-fa2e-78d1-b5db-e210fb07ad5a/DarkRuins_Occlusion_Forward_20261020.md)。

![源地图同方向缓冲对照；保留全部有效帧](C:/Users/YYchainsaw/.codex/visualizations/2026/10/19/01a15223-fa2e-78d1-b5db-e210fb07ad5a/DarkRuins_OcclusionForward_20261020.png)

## 5. Unreal Insights 已确认的停顿链

原始 Trace 为源项目 10/19 的详细诊断轮，不是无追踪开销的帧率成绩。RouteStart=192.926 秒、RouteEnd=314.685 秒，书签内有 4539 个完整 Game Frame，最大 47.266 ms。

| 距 RouteStart | Game Frame | 已确认的主要时间线 |
| --- | ---: | --- |
| 53.12 秒 | 47.27 ms | GT 等 RT 约 43.42 ms；RT 等可见性任务 15.06 ms，对应 Worker 等 GPU 查询结果 15.97 ms；随后 VSM 纹理创建 18.41 ms，其中 CommitReservedResource 18.23 ms |
| 116.51 秒 | 41.78 ms | RT 可见性等待 28.65 ms，Worker 查询结果等待 29.16 ms |
| 118.51 秒 | 41.13 ms | RT 可见性等待 29.70 ms，Worker 查询结果等待 31.24 ms |

解释与限制：

- 跨线程、父子 scope 与异步队列有包含和重叠关系，表中时间不能直接相加。
- 晚段 GPU `BeginOcclusionTests` 自身约 0.02 ms。这里的长耗时是 **等结果可用/队列推进**，不是遮挡算法计算本身花 30 ms。
- 最大帧附近的 8 个计算 PSO 任务开始于主要查询等待与 VSM 提交结束之后，且标记为 Precached。因此不能仅凭 CSV 的 compute miss=8 将前面的停顿归因于 PSO。
- 对这三个目标帧，地图磁盘 I/O 没有出现在已确认的主要阻塞链中；这不证明所有历史卡顿或全部地图加载都没有 I/O 成本。
- 没有完整 query/fence ID 依赖，不能指认某个 GPU pass 为唯一根因。GPU 时序误差约可到 0.42 ms，不作亚毫秒因果断言；内存标签分析部分有错误，未用于内存归因。

原始证据：[DarkRuins_Hitch.utrace](D:/YYchainsaw/Unreal-Projects/DarkRuinsMegascansSample/Saved/PerformanceBuilds/DarkRuins_Dev_20261019/TraceData/20261019_164232_251/DarkRuins_Hitch.utrace)，同目录 `Analysis` 中保留 Hitch53、Late116、Late118、PsoEvents、RouteSync 导出。[完整 Trace 报告](C:/Users/YYchainsaw/.codex/visualizations/2026/10/19/01a15223-fa2e-78d1-b5db-e210fb07ad5a/DarkRuins_Trace_Analysis_20261019.md)。

尚未执行的 VSM 单变量候选是 `r.Shadow.Virtual.DynamicDeallocationMode 2`。本机实现中，2 禁止因没有 VSM 需求而动态释放物理池；默认 0 在 MegaLights 开启时允许该释放。保留池会持续占显存，不能消除首次创建，而且尚未证明此前分配是动态释放后重建造成。**没有做这项 A/B，也未把它写进项目默认。**

关键源码：[查询缓冲默认与说明](<D:/Program Files/Epic/UE_5.8/Engine/Source/Runtime/Core/Private/HAL/ConsoleManager.cpp:4258>)、[查询结果等待](<D:/Program Files/Epic/UE_5.8/Engine/Source/Runtime/Renderer/Private/SceneVisibility.cpp:3298>)、[VSM 动态释放模式](<D:/Program Files/Epic/UE_5.8/Engine/Source/Runtime/Renderer/Private/VirtualShadowMaps/VirtualShadowMapArray.cpp:2381>)。

## 6. 参数状态：已测、候选与尚未实施

| 项目 | 交接状态 |
| --- | --- |
| World Partition / Nanite / HLOD | 源地图原有能力；迁移文件完整，不等于所有区域和 HLOD 构建均验收 |
| MegaLights | 开关对照支持保留；主项目已开启项目支持 |
| MegaLights 2 采样 | 源包已用的性能档候选，有少量噪点/拖影反馈；最新主项目日志仍为 4 |
| 85% 内部渲染比例 | 源包候选预设，不是改变输出分辨率；最新主项目未确认设为 85% |
| TSR | 源包测试使用；主项目配置仍 AA=0，尚未对齐 |
| 查询缓冲 2 | 源包运行时测试候选，收益有限，未冻结项目默认 |
| VSM DynamicDeallocationMode=2 | 只提出方案，尚未执行 A/B |
| 关闭 VSM 保留资源机制、全局降阴影/关 Lumen | 没有作为已验证方案实施 |
| 修改 WP 网格尺寸/加载范围、自研预测加载 | 没有证据要求优先实施；需要对应场景的加载/激活 Trace 后决定 |
| PSO 预缓存优化 | 有邻近活动线索，未证明所有长帧由 PSO 引起，未声称已修复 |

不要复制源项目整套 Config 覆盖主项目。输入、GAS、游戏模式、服务及已有工程设置需要保留。源配置的遗留 `r.Nanite.AllowTessellation` 在本机 UE 5.8 源码未找到注册，不应机械照搬；真正 `r.Nanite.Tessellation` 默认已为 1。

## 7. 迁移经过、恢复与完整性证明

两项目均 UE 5.8。迁移前检查整个源/目标 Content，相对路径未发现冲突。

首次迁移在 10/20 09:49 左右崩溃：新迁移流程加载并重新保存大量资源，源编辑器 UsedVirtual 约 65.54 GiB，系统可用提交余量只剩 0.17 GiB，再申请 1197.5 MiB 失败。它是 **编辑器迁移峰值问题**，不能解释成运行地图需要 65 GB 内存或 GPU 显存不足。未由本会话修改 Windows 分页文件。

恢复时在源编辑器执行：

```text
AssetTools.UseNewPackageMigration 0
```

随后重新迁移 Main 的完整依赖到主项目 Content，对本次残留同路径文件覆盖。旧流程对普通 `/Game` 包采用直接复制，降低加载/重存需求；不泛化为所有插件或跨挂载点迁移都绝不加载资源。

10:00:43 的核验结果：

| 检查项 | 结果 |
| --- | --- |
| 本轮清单文件 | 11,655，源/目标 SHA-256 全部相同 |
| 缺失 / 大小不同 / 哈希不同 / 校验中变动 | 0 / 0 / 0 / 0 |
| 核验字节 | 24,756,516,116 |
| Main 外部 Actor / Object | 8,811 / 89，全部到位 |
| Main.umap、两个 HLODLayer、8 个 DataLayer | 全部到位 |
| 70 张已迁入地图所对应外部包 | 10,136 个均存在、大小一致 |

源 Content 的部分 AssetZoo、Previewer、未选中辅助关卡/素材不在这次依赖清单内，不能以“全源 13,800 文件没有全部出现”判本次失败。

[SHA-256 核验 JSON](C:/Users/YYchainsaw/.codex/visualizations/2026/10/19/01a15223-fa2e-78d1-b5db-e210fb07ad5a/DarkRuins_Migration_Verification_20261020.json)固定记录的是迁移结束时点；此后用户在地图中增加玩法 Actor，不要求继续与源项目保持字节一致。[当时迁移核验报告](C:/Users/YYchainsaw/.codex/visualizations/2026/10/19/01a15223-fa2e-78d1-b5db-e210fb07ad5a/DarkRuins_Migration_Verification_20261020.md)中的“未启用插件/未运行”是 10:00 的历史状态，已被下一节的新记录更新。

地图采用外部 Actor 存储，不能仅备份或移动 Main.umap。项目 `.gitignore` 忽略绝大多数第三方 Content、外部 Actor/Object 和部分地图，**只创建 Git 分支不能完整备份迁入地图**。主项目 .uproject、DefaultEngine.ini 及文档当前有用户未提交修改，另有 UML 改动；接手时保留，不清理或回退无关工作。

## 8. 主项目当前状态与最新试玩证据

### 8.1 已落地配置与尚未对齐的条件

- `AesirWarden.uproject` 已显式启用 WaterExtras；它依赖 Water，最新日志确认二者都加载。不要因 .uproject 没有另列 Water，就误判水插件未启用。
- [DefaultEngine.ini](D:/YYchainsaw/Unreal-Projects/AesirWarden/Config/DefaultEngine.ini:70) 中 MegaLights 项目支持已为 True；DX12/SM6、Lumen HWRT、VSM、虚拟纹理已开启。
- 同文件第 107 行仍是 `r.AntiAliasingMethod=0`。第 4/14 行的编辑器启动地图和默认游戏地图仍为 `/Game/Aesir/Maps/L_OpenWorld.L_OpenWorld`，尚未改为 Main；全局 GameMode 为 BP_AesirGameMode，Main 运行也确实使用它。
- 最新 Standalone 启动日志为 1280×720、采样 4，未见后续将输出改为 1920×1080、SP=85%、缓冲=2 的命令证据。因此当前试玩不能视作此前测试条件已完整复现。
- Shader/DDC 正在正常复用同一 E 盘缓存。此前一批有 820 次查询，其中 600 次 DDC 命中、153 个实际编译任务；新项目/新渲染配置需要补充对应变体，不意味着所有资源全部重新编译。正式计时前需等后台派生与网格编译完成。

### 8.2 玩家与 BT Boss 已有有效交战

最新证据是 [AesirWarden_2.log](D:/YYchainsaw/Unreal-Projects/AesirWarden/Saved/Logs/AesirWarden_2.log)，10:53:24–10:57:17，为编辑器可执行文件的 **Standalone -game**，不是 cooked Development 包。

- Main 的 World Partition 初始化并开始运行，BP_AesirGameMode 接管。
- 玩家 GAS 初始化为 150 HP，BP_AesirBoss 初始化为 500 HP。
- 日志记录玩家对 Boss 造成伤害、Boss 对玩家造成伤害，双方扣血有效。
- Boss 遥测：BehaviorTree，`result=Aborted`，duration=204.44 秒，decisions=628，accepted=628 / rejected=0，damage_dealt=126，damage_received=50。
- [本局遥测 JSON](D:/YYchainsaw/Unreal-Projects/AesirWarden/Saved/AesirTelemetry/BossEpisodes/affc5a8b-43cd-4cf9-4b69-5fa9839eead4.json)。

这些数据证明基础战斗链运行过；它们不是正式 BT–PPO 对照、不是胜败结算验收，也不是战斗帧率测试。当前没有主项目新的正式性能 CSV 或独立 Development 包成绩。

### 8.3 当前日志问题与待验证风险

| 项目 | 证据与处理边界 |
| --- | --- |
| 队友服务失败 | 最新 Standalone 有战术 HTTP 502、health 超时、agent/step 连续超时，并记录回退本地 AI。需要处理服务或明确离线分支/重试策略；BT 战斗不应被在线队友服务阻塞 |
| 历史 Alice 空引用 | 旧 PIE 日志出现 AliceOrderComponent 的 BeginEncounter/EndEncounter 等空引用。最新 Standalone 未重现，但没有代码/蓝图修复证据，不宣称已经修复 |
| GameplayCue 扫描范围 | 最新日志提示没有 GameplayCueNotifyPaths，回退扫描整个 /Game。地图迁入后资源量变大，应先找到真实 GameplayCue 资产目录，再收窄扫描范围并复测启动成本；不凭空指定目录 |
| HLOD / 类导入警告 | 最新日志有 HLODBuilder 类、WorldPartitionHLODUtilities 与 AlembicLibrary 导入警告，地图随后仍运行。目标项目打包时调查；不等价于迁移文件丢失，也不因此盲目启用所有插件 |
| 网格运行时准备 | 仍有静态网格等待/构建及内存额度警告。先完成派生，再测 cooked 包，避免把开发期编译卡顿记作最终运行加载算法问题 |
| WP 敌人登记 | GameMode BeginPlay 只枚举当时已加载敌人；源码未见完整动态登记/注销机制。晚加载/卸载 Boss 可能影响计数，是代码级风险，尚未通过跨区场景复现 |
| 胜败/重试 | 现有流程 C++ 文档只覆盖菜单→开场→探索；最新局为 Aborted。Boss 入场、指定 Boss 结算、重试清理和返回菜单要分别验收，不能把枚举或资源存在当成实现完成 |

日志定位：最新 Standalone 第 1877 行 GameplayCue 扫描，第 1841–1843/1879 行类导入警告，第 2302/2311/2321 行服务失败与回退，第 2488–2489 行本局遥测。旧 PIE 空引用在 [AesirWarden.log](D:/YYchainsaw/Unreal-Projects/AesirWarden/Saved/Logs/AesirWarden.log:9004)附近，日志继续更新/轮换后应按关键字重新定位。

WP 风险源码：[AesirCombatPrototypeGameMode.cpp](D:/YYchainsaw/Unreal-Projects/AesirWarden/Source/AesirWarden/Private/Framework/AesirCombatPrototypeGameMode.cpp:29)。接入时应围绕目标 Encounter/Boss 明确登记、死亡、卸载和重试生命周期，不把整张地图所有敌人都无差别计入一场战斗。

## 9. 接续工作建议

用户已表示当前地图帧数足够，尚未接入完整特效。当前优先级是保留地图成果并推进玩法闭环，不继续为 2% 左右的差异反复录普通 CSV。

1. 先记录当前工作副本、资产与配置。分清功能试玩和性能对照预设，核对 TSR、采样、分辨率、SP、查询缓冲的实际值；不要把旧源包参数当作主项目已生效。
2. 在已有 Boss 场地继续验证锁定、导航/绕障碍、双方攻击、死亡、胜败、连续重试与返回菜单。Boss 已有交战证据，无需从零重复摆放。跟进服务超时、历史空引用及 WP 晚加载登记风险。
3. 目标流程通过后，为 **AesirWarden 主项目**制作首份 cooked Windows Development 包，关闭编辑器，建立地图/玩家/Boss 的新性能基线。角色、碰撞、动画、GAS、AI、服务状态均作为测试条件登记。
4. 逐步加入特效或新增玩法，再针对真实超预算区域/卡顿单变量排查。如果优先专项解决长帧，录新 Trace 验证调用链；旧 Trace 只能证明当时的源项目目标帧。
5. 只有新证据显示加载/激活成为主要阻塞，才调整 WP 范围、网格、HLOD、常驻引用、对象创建或预测加载。避免一开始重写已有加载系统。

现有开发入口：[当前计划](D:/YYchainsaw/Unreal-Projects/AesirWarden/Docs/05_Development_Guide/06_DevelopmentRoadmap.md)、[流程接线指南](D:/YYchainsaw/Unreal-Projects/AesirWarden/Docs/05_Development_Guide/05_LevelFlowGuide.md)、[验证矩阵](D:/YYchainsaw/Unreal-Projects/AesirWarden/Docs/06_Test_Doc/01_VerificationMatrix.md)。正式 BT–PPO 对照仍遵守既有单玩家/单 Boss 和共享执行规则，不能用本次混合试玩遥测替代研究结果。

## 10. 复测入口、命令与分析口径

### 10.1 旧源包启动器的适用范围

源包根目录已有 `Start-PerformanceTest.cmd`、`Start-HitchTrace.cmd`、`Start-PerformanceTest.ps1`。正常轮输出到 RunData；Trace 轮使用独立时间戳 TraceData，启动时含 CPU/GPU/frame/bookmark/loadtime/file/log/task 与 statnamedevents。

**这些启动器指向 DarkRuins 源项目旧包，不是迁入后的 AesirWarden。** 正常启动器显式设置采样 2、SP85 和 High，但没有设置查询缓冲 2，复测时必须查询并明确登记该值。详细 Trace 只用于诊断，不与轻量 CSV 成绩混算。

### 10.2 新主项目复现候选预设的命令模板

先确保项目级 MegaLights 支持、目标地图和必要插件正确，后台编译结束。下面用于 **Standalone/Development 游戏控制台**，不是编辑器视口。它是复测模板，本文没有自动执行或写入主项目：

```text
r.SetRes 1920x1080w
scalability 2
r.AntiAliasingMethod 4
r.MegaLights.Allowed 1
r.MegaLights.NumSamplesPerPixel 2
r.ScreenPercentage 85
r.NumBufferedOcclusionQueries 2
r.VSync 0
r.DynamicRes.OperationMode 0
t.MaxFPS 0
scalability
r.AntiAliasingMethod
r.MegaLights.Allowed
r.MegaLights.NumSamplesPerPixel
r.ScreenPercentage
r.NumBufferedOcclusionQueries
stat fps
stat unit
```

`scalability 2` 会影响/保存用户画质，按测试约定记录原值；4 对应 TSR。查询缓冲 2 是候选设置，做 1/2 对照时只改这一项，并保持其余条件。普通采集只需在路线起终点输入 `CsvProfile START` / `CsvProfile STOP`，使用新文件，保留原始数据。完整命令与内存/流送观察方法见原测试操作说明。

### 10.3 读 CSV 和图表时必须保留的规则

- 用末尾完整表头解析动态追加字段，排除重复表头和 metadata。前期行可能比最终表头短，不能以列数必须相等过滤，否则会丢掉开头几十秒。
- 平均 FPS = 1000 / mean(FrameTime)；1% low 为最慢 ceil(N×1%) 帧平均耗时倒数；P95/P99 线性插值。主表保留全部有效正 FrameTime，边界裁剪仅作敏感性补充。
- Frame、GT、RT、RHI、GPU、Visibility 及父子 GPU pass 可能包含、并行或跨帧，不能相加。Visibility 是聚合等待，不等同纯 GPU Query 时间。
- PSO OnHitch 的 -1 是不可用哨兵，不算成次数，也不当作“没有 PSO 卡顿”。相邻帧 miss 计数只提供线索。
- 显存使用必须对齐同一帧预算；预算/使用同时为零的样本标为无效遥测，保留该帧耗时。不能用 8 GB 物理容量替代当时上报的进程预算。
- 记录路径、朝向、速度、首遍/重复遍、供电、进程重启、编辑器是否运行和是否 Trace。不同条件的结果作为历史参考，不拼成单变量收益。
- CSV metadata 的 targetframerate 不是实际 FPS，也不等同运行时 t.MaxFPS。metadata 时长与 FrameTime 合计约一帧边界差异，分别注明。
- 分析主指标已做过独立 Python 交叉核验。本次交接整理复用既有分析，未重复长时间跑图、构建、全量地图哈希或改系统设置。

分析脚本在派生报告目录：`analyze_darkruins.mjs`、`report_darkruins.mjs`、`plot_darkruins.py`、`analyze_darkruins_trace.py`、`audit_darkruins_migration.py`。最新同向模式为 `--occlusion-forward`；返回模式为 `--occlusion`；供电对照为 `--power`。原始数据和既有报告保持原位，后续新增条件使用独立文件名并注明版本。

## 11. 可直接发给下一段会话的接续说明

```text
请先完整阅读：
D:\YYchainsaw\Unreal-Projects\AesirWarden\Docs\06_Test_Doc\04_DarkRuinsPerformanceAndMigrationHandoff.md

这是 DarkRuins 地图、性能测试、优化取舍和迁入 AesirWarden 的交接。
源地图独立 Development 包在完整供电、1080p、High、MegaLights 2采样、85%下约55 FPS；
查询缓冲2收益有限，VSM保留池方案尚未测试，不要先重写地图加载系统。
地图迁移清单已经SHA-256核验通过，最新主项目Standalone也已出现玩家和BT Boss双方有效伤害，
但尚未完成胜败/重试验收，也没有主项目同条件正式性能基线。
主项目当前AA仍0，最新试玩分辨率和采样也与源基线不同；请先核对当前文件和最新日志，
保留用户已有地图/配置修改，再按我这段会话提出的具体任务接续。
不要把旧源包成绩、历史报告时点和当前主项目状态混为一谈。
```
