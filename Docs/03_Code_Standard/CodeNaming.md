# C++ 命名与模块约定

> 状态：从当前工程整理的工作规范 · 2026-09-29

- 运行模块名为 `AesirWarden`，导出宏为 `AESIRWARDEN_API`。新公开头文件放 `Source/AesirWarden/Public/<Domain>/`，实现放对应 `Private/<Domain>/`；include 使用模块相对路径，如 `AI/Boss/AesirBossActionComponent.h`。
- 遵循 Unreal 类型前缀：`A` Actor、`U` UObject/组件、`F` 值类型、`E` 枚举、`I` 接口。项目已有 `AAesirBossAIController`、`UAesirBossActionComponent`、`FAesirBossObservation`、`EAesirBossAction`；新类型保持领域名称一致。
- Blueprint 可见接口使用清晰 Category；可调数值写为 `EditDefaultsOnly` 等属性并给范围，避免散落硬编码。外部协议字段与枚举整数值须进入[RL 契约](../08_RL_Research/BossRL_Contract.md)，改动时升版本。
- 旧类名 `AesirCombatPrototypeGameMode`、`AesirCombatPrototypeCharacter` 是迁移遗留标识；文档引用真实类名，不因名称不统一而擅自改代码或重定向资产。
- 修改公共头、GAS 标签、Ability 映射或 Python 协议时，在提交说明写出受影响 Blueprint、资产、Schema 与验证项。
