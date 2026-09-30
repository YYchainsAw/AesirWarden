# 输入映射清单

> 状态：资产存在性核对；具体键位/手柄绑定须在 UE 编辑器核对 · 2026-09-29

| 映射/动作资产 | 预期用途 |
| --- | --- |
| `IMC_AesirLocomotion` | 移动与视角等基础输入 |
| `IMC_AesirCombat` | 战斗输入组 |
| `IA_LightAttack`、`IA_HeavyAttack` | 轻击、重击 |
| `IA_Evade`、`IA_Guard`、`IA_LockOn` | 闪避、防御、锁定 |
| `IA_PushToTalk`、`IA_ToggleCompanionChat` | 后续同伴命令入口 |
| `IA_TestHeal` | 测试能力，正式构建是否保留待决定 |

以上资产可在 [Content/Aesir/Input](../../Content/Aesir/Input) 定位。[玩家输入绑定源码](../../Source/AesirWarden/Private/Characters/Player/AesirPlayerCharacter.cpp)可定位多个 BindAction；键位、触发类型、映射优先级和 UI 焦点尚需编辑器导出核对。变更时更新本表并跑 [COMBAT-01](../06_Test_Doc/VerificationMatrix.md)。
