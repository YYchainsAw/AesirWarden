# 参考、素材与贡献来源

> 来源基线：2026-09-29；营火素材记录补充于 2026-10-20。资产作者、版本、许可和路径待逐项补齐。

公开展示前，每项关键视觉/音频资产应记录：资产名、作者/平台、获取日期、许可及允许用途、项目内路径、本人修改内容、展示材料中的署名写法。没有许可依据的资产不得被本页标为可公开发布。

| 类型 | 当前可说明的边界 | 待补证据 |
| --- | --- | --- |
| 官方动画数据 | Epic Game Animation Sample 属旧工程方案；当前玩家不再使用其 Motion Matching 移动链路 | 若以后重新使用，补具体资产列表、版本、许可链接 |
| 当前玩家角色与动画 | Greystone 模型及第三方动画被重定向并接入传统动画状态机；本人工作与素材原作者须分开说明 | 原始素材作者、商店页面、许可证、重定向产物映射与个人修改记录 |
| Unreal 技术与样例 | GAS、Behavior Tree、Enhanced Input、Motion Warping 等为引擎能力 | 标明参考样例；说明本人完成的具体整合与交互 |
| 营火扫描模型 | [Campfire — Quixel Megascans / Fab](https://www.fab.com/listings/7e84e0a2-e57a-4421-a127-ed747846480c)；用户导入，菜单 BP 已保存并通过本轮实机检查 | 获取与许可记录 |
| 营火 Niagara 特效 | [Niagara Examples Pack — Epic Games / Fab](https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600)；样例 `NS_Fire`、派生采样盘和局部材质已接入并通过本轮实机检查 | 获取与许可记录；最终性能与 cooked 包检查 |
| 其他第三方资产 | 场景、角色、动画、音效和特效按实际使用情况登记 | 作者、商店/下载链接、许可版本、项目内路径与修改内容 |
| 游戏系统 | Boss Action/GAS、BT、Observation、Telemetry 等源码可在本工程定位 | 每项个人实现和协作贡献的署名确认 |
| Python 模型与服务 | 独立仓库负责训练和推理；候选结果与游戏内对照不同 | 模型 manifest、脚本版本、原始实验记录 |
| 生成式 AI | 本轮用于需求交叉核对、对象分析草稿、draw.io模型制作和AI交叉预审 | [真实过程索引](../09_Course_Analysis/03_AIModelingEvidence.md)；本人的规则决定、模型人工审查、个人/队友贡献和运行验证分别记录，未发生的部分保持待完成 |

## 营火素材导入与整合边界

两项素材由用户从 Fab 导入；2026-10-20 已完成菜单 BP 配置保存、重新加载检查和实机菜单检查，火焰正常显示，坐姿、起身与游戏控制交接通过。实际获取日期、账号领取记录及许可版本尚未核对。

| 原始来源 | 导入后的原始项目路径 | 本人整合边界与当前状态 |
| --- | --- | --- |
| Quixel Megascans 的 Campfire，Fab 商品 ID `7e84e0a2-e57a-4421-a127-ed747846480c` | 目录：`/Game/Fab/Megascans/3D/Campfire_uenpfeifa/High/uenpfeifa_tier_1/`；模型：`StaticMeshes/uenpfeifa_tier_1`；原材质与贴图保留在同目录的 `Materials/`、`Textures/` | 原作者提供扫描模型、材质与贴图。本人整合为给既有菜单 Actor 添加模型组件、配置摆放与缩放、关闭道具碰撞并隐藏临时木石。BP 已保存，本轮实机检查通过。 |
| Epic Games 的 Niagara Examples Pack，Fab 商品 ID `0e188eca-4e54-4fb2-a9ed-d8b8a565e600` | 原样例目录：`/Game/NiagaraExamples/`；使用系统：`/Game/NiagaraExamples/FX_Misc/NS_Fire` | 原作者提供 Niagara 系统及其材质、贴图和样例实现。本人整合为添加特效组件、调整火势与烟量、配置显式网格采样和局部曝光补偿，保留既有 Firelight、坐姿、起身与相机流程。BP 已保存，本轮实机检查通过。 |

Epic 派生资产保存在 `/Game/CustomAssets/Campfire/`，原始样例包保留不改：`SM_CampfireEmissionDisk` 复制自 `/Game/NiagaraExamples/StaticMesh/SM_Disk`，启用 CPU 访问并作为隐藏采样组件；`M_MenuCampfireFlames` 和 `MI_MenuCampfireFlames` 分别复制自样例 `Materials/MasterMaterials/M_Flames` 和 `Materials/MI_Flames`。本地母材质仍使用 Epic 的 `MF_ExposureCompensation`，将两个全局 MPC 输入改接局部标量 `Menu Fire Exposure Mix = 1`、`Menu Fire Exposure EV = 0`，由 `User.Flame Material` 引用本地实例，场景曝光保持原设置。采样盘与派生材质的原始制作均归 Epic Games，个人贡献仅为上述整合和调整。

接入脚本 [setup_campfire_art.py](../../Scripts/Editor/setup_campfire_art.py) 已执行并保存 `/Game/Aesir/Framework/Flow/BP_AesirCampfireStart`。保存记录见 `Saved/StartMenu/CampfireArt/installation.json`，重新加载检查见同目录 `verification.json`；实机记录为 `Saved/Logs/CampfireArtFinalSmoke.log` 的 `START_MENU_MAIN_SMOKE_PASS`（2026-10-20 14:11:59，Asia/Shanghai）。本次通过不代表完成 cooked Windows 包或正式性能验收。展示署名可写为“Campfire: Quixel Megascans；Niagara Examples Pack: Epic Games；菜单场景摆放、参数调整与流程整合：本人”。

## 玩法与美术参考

目前没有整理完成的具体作品清单。新增参考时记录作品及链接、参考点、采用的设计原则和本人改动；素材来源继续登记在上表，避免重复维护。
