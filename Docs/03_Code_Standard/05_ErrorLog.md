# 日志与错误记录

> 状态：项目工作约定 · 2026-09-29

日志至少区分：Boss 策略请求失败、协议/Schema 错误、Action 拒绝、GAS 激活失败、Episode 终局、结果 UI/重试失败。每条可复现错误应包含时间、构建/模型版本、Episode ID（有则填）、策略来源、动作或请求 ID、稳定原因码；避免只写笼统的“失败”。

当前 [Boss Action Outcome](../../Source/AesirWarden/Public/AI/Boss/AesirBossDecisionTypes.h)已有 typed 结果；[Policy Client](../../Source/AesirWarden/Private/AI/Boss/AesirBossPolicyClientComponent.cpp)记录 `invalid_observation`、`http_failure`、`protocol_mismatch`、`missing_action`、`invalid_action` 等原因。新增原因码先确定含义、触发点与回退动作，再同步测试用例。

公开的日志、截图、遥测和报告不得包含本机秘密配置、令牌或不必要的个人路径。正式实验保存原始日志与配置哈希；摘要只引用原始证据，不替代它。
