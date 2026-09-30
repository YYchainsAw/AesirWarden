# Blueprint 组织规则

> 状态：开发约定；并非对现有所有 Blueprint 的审计结论 · 2026-09-29

1. Blueprint 负责资产配置、动画/表现、UI 接线及需要可视化调整的流程；跨 BT/RL 共用的动作合法性和可复现实验契约集中在 C++/GAS。
2. 复杂 Graph 拆成按职责命名的函数或组件；公开变量写明默认值、单位与范围。GameplayTag 和 Ability ID 使用项目映射，避免在多张图里散落字符串常量。
3. 事件绑定和解绑成对；重试、关卡切换与结果 Widget 创建必须防重复。异步服务回调要验证当前 Encounter/Episode 是否仍有效。
4. 对更改过的 Blueprint 保存父类、关键变量、函数/事件、依赖资产和 PIE 结果；二进制资产存在不能证明 Graph 行为正确。
5. 与 C++ 公共 API 或存档结构有关的 Blueprint 改动，先评估兼容/重定向，再写到[验证矩阵](../06_Test_Doc/VerificationMatrix.md)。
