# 存档与重试设计

> 状态：目标设计；当前工程未发现 `USaveGame` 或完整持久存档实现 · 2026-09-29

## 最小答辩重试

Boss 战入口保存临时可恢复快照：玩家生命/能力状态、Boss 初始状态、选定策略模式、必要场景标识。重试应清理旧角色/Widget/委托/Policy Client 状态并创建新 Episode。此快照可先在内存或关卡初始化流程实现，不必将完整持久存档作为 P0 前置。

## 后续持久数据

版本化存档包含经验、下次技能点阈值、未用技能点、五项属性等级（每项 1–9）、已解锁能力、拥有符文、两枚装备符文和武器定义 ID。瞬时 Health、GuardPressure、冷却剩余时间不能直接替代这些进度字段；加载时先验证版本与 ID，再转换为运行时 GAS 初值。

## 验证

至少测试新游戏、有效/无效检查点、退出后继续、死亡重试、新 Episode ID、重复死亡事件不二次结算、动态生成 Boss 的登记。当前 [GameMode](../../../Source/AesirWarden/Private/Framework/AesirCombatPrototypeGameMode.cpp)只在 BeginPlay 收集当时存在的敌人，若改为战斗中生成须重做登记与胜利判定。
