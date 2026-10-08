# 碰撞与查询配置

> 状态：C++ 查询静态核对；Blueprint/关卡碰撞预设待编辑器检查 · 2026-09-29

当前 [战斗组件](../../Source/AesirWarden/Private/Combat/AesirCombatComponent.cpp)使用武器插槽端点和球形查询检查 `ECC_Pawn`，玩家镜头遮挡查询涉及 Pawn、WorldDynamic、WorldStatic；Boss Dodge 间隙检查使用 `ECC_Visibility`。当前 INI 未看到可在此轮确认的项目自定义碰撞通道配置。

加入新通道/预设前先写下用途、发起者、目标类型、响应矩阵、预期忽略对象与性能影响；再在配置、C++、Blueprint 和测试中统一。守防、命中窗口、投射物及不可防御攻击分别测试友军/自身忽略、遮挡、重复命中与边界距离。具体资产碰撞设置以 UE 编辑器检查为准，不能仅凭 C++ 推断。
