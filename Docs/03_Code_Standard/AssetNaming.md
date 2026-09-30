# Unreal 资产命名

> 状态：基于现有目录的工作规范 · 2026-09-29

项目自有资产放在 `Content/Aesir/`，按 `Abilities`、`AI`、`Animations`、`Characters`、`Framework`、`Input`、`Maps`、`UI`、`Weapon` 分区。现有前缀可参考：`BP_` Blueprint 类、`BT_` Behavior Tree、`BB_` Blackboard、`GA_` Gameplay Ability、`GE_` Gameplay Effect、`IA_` Input Action、`IMC_` Input Mapping Context、`WBP_` Widget Blueprint、`L_` Level。

命名应包含用途和拥有者，如 `GA_Boss_Dodge`、`GE_Cooldown_Boss_Dodge`，让相关 Ability/Effect/Tag 可从名称互相查找。新资源先确定归属文件夹和命名，再引用到 Blueprint 或配置。迁移第三方/官方样例前记录原始路径、许可和本地副本；不要让文档或作品集把外部源资产误标为个人原创。

当前目录还包含大量动画资源；对每个实际进入构建或展示的第三方资源，最终在[来源清单](../01_Project_Overview/05_CreditsAndProvenance.md)填写来源。重命名已有 `.uasset` 后须检查软引用与重定向并在编辑器验证。
