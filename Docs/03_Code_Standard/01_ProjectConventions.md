# 个人开发约定

这些约定用于保持自己写的代码、蓝图和资料容易查找；实际操作见[工程速查](../05_Development_Guide/01_ProjectQuickReference.md)。

## 代码与资产命名

- 运行模块为 `AesirWarden`，导出宏为 `AESIRWARDEN_API`。头文件放 `Source/AesirWarden/Public/<Domain>/`，实现放对应 `Private/<Domain>/`，include 使用模块相对路径，如 `AI/Boss/AesirBossActionComponent.h`。
- 类型沿用 Unreal 前缀：`A` Actor、`U` UObject/组件、`F` 值类型、`E` 枚举、`I` 接口；领域名称与现有 `AAesirBossAIController`、`UAesirBossActionComponent` 等保持一致。
- 自有资产放在 `Content/Aesir/`，按 `Abilities`、`AI`、`Animations`、`Characters`、`Framework`、`Input`、`Maps`、`UI`、`Weapon` 分类。
- 常用前缀：`BP_` 蓝图类、`BT_` 行为树、`BB_` 黑板、`GA_` 技能、`GE_` 效果、`IA_` 输入动作、`IMC_` 输入映射、`WBP_` UI、`L_` 地图。名称包含用途与拥有者，例如 `GA_Boss_Dodge`、`GE_Cooldown_Boss_Dodge`。
- 资产重命名在 UE 编辑器中操作，检查引用和重定向后再运行。第三方资源保留来源信息，实际使用的素材列入[来源清单](../01_Project_Overview/05_CreditsAndProvenance.md)。

## C++ 与蓝图职责

- Blueprint 负责资产配置、动画表现、UI 接线和可视化调整；BT/RL 共用的动作规则与合法性检查集中在 C++/GAS。
- 复杂 Graph 拆为职责清楚的函数或组件。可调数值集中为属性，注明单位和有效范围；GameplayTag、Ability ID 使用现有映射。
- 事件绑定与解绑成对；重试、关卡切换和结果 UI 防止重复创建。异步回调检查 Encounter/Episode 是否仍有效，旧请求不能影响下一局。
- 调整反射接口或 Blueprint 父类后，重启编辑器并检查受影响资产；具体操作见[编译与迭代](../05_Development_Guide/01_ProjectQuickReference.md#编译与迭代)。
- 改动外部协议字段、动作整数值或观测含义时，同步[Boss RL 契约](../08_RL_Research/01_BossRLContract.md)与 Python 端。

## 日志与测试记录

- 错误日志写明发生了什么及原因；需要追踪时带上请求、动作或 Episode ID。区分服务失败、协议错误、动作拒绝、GAS 激活失败和流程重置失败。
- 沿用现有 typed Outcome 与稳定原因码，如 `http_failure`、`protocol_mismatch`、`invalid_action`，避免把不同问题都写成“失败”。
- 日常自测只记日期、实际结果和异常；正常迭代无需逐次填写版本清单。尚未解决的问题集中在[已知问题](../06_Test_Doc/02_KnownIssues.md)。
- 正式发布和 RL 对照实验再保存对应代码、资产、服务、模型与配置版本，附日志或原始数据；公开材料移除令牌、秘密配置等内容。

## 文档与图片

- 说明放 Markdown，数值表和原始数据用合适的表格或数据文件；同一规则只维护一份，其他页面链接引用。
- 当前文档沿用 `NN_PascalCase.ext` 与入口 `README.md`；编号只表示目录内阅读顺序，删除后不必补齐编号。
- UML 集中在[Docs/UML](../UML/README.md)：`src/*.drawio`为可编辑源，`png/*.drawio.png`为同源预览；旧组件/GAS/实现时序放`Technical/`供开发参考。
- 本轮制图通过draw.io MCP修改页面，再由draw.io Desktop导出嵌入XML的PNG，源文件继续保留。不再以PlantUML作为当前图源。规则改动时同步BR/UC、模型和追踪，再刷新预览，不能只改图片。
- 导出后核对文字、多重性、箭头、异常出口和遮挡；[建模证据](../09_Course_Analysis/03_AIModelingEvidence.md)区分AI预审、实际成员审查、自动检查与实机结果。历史原稿保持原样，不补造人工接受。
- 改名或移动文件时同步更新当前引用；固定到历史提交的证据链接保留当时路径。
