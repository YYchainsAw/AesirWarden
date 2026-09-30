# 贡献与文档维护

> 状态：项目工作约定草案 · 2026-09-30。以下规则适用于公开仓库中的代码、文档和经核对的项目蓝图。

## 工作前

1. 阅读 [项目范围](01_Project_Overview/02_GoalsAndScope.md)、[架构](02_Design_Doc/TDD/Architecture.md)和[已知问题](06_Test_Doc/KnownIssues.md)。
2. 先判断改动属于答辩 P0、演示完善 P1，还是 2027 扩展。P0 以单 Boss、共用 GAS 执行、BT–PPO 对照和稳定闭环为先。
3. 修改 C++、Blueprint、UML 或实验契约时，写清影响的接口、资产、验证项和回退方式。未经项目作者确认的 Blueprint/游戏代码更改不应由文档迁移任务附带执行。

## 变更与提交

- 一次提交尽量只解决一个目的；提交说明建议用 `docs:`、`fix:`、`feat:`、`test:` 等前缀，并写明范围。
- C++ 采用 [CodeNaming](03_Code_Standard/CodeNaming.md)；资产采用 [AssetNaming](03_Code_Standard/AssetNaming.md)。不要把自动生成的 `Binaries/`、`Intermediate/`、`DerivedDataCache/` 作为源码提交。
- 行为或数据契约改变时，同步更新 GDD/TDD、[RL 契约](08_RL_Research/BossRL_Contract.md)和[验证矩阵](06_Test_Doc/VerificationMatrix.md)。
- PR 或合并审查记录应包含：目的、变动路径、验证方法、实际结果、仍未验证的风险。只有实际执行过的测试才能标为通过。

## 第三方与 AI 辅助

记录 Epic 官方素材、Marketplace/其他第三方素材、生成式 AI 和协作者的来源、许可与本人修改范围；详见[贡献与来源](01_Project_Overview/05_CreditsAndProvenance.md)。不把样例动画、外部模型或 AI 草稿声明为个人原创。
