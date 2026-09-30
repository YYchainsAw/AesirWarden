# 开发环境搭建

> 状态：本机工程配置核对；跨机器复现步骤待另一台 Windows 机器验证 · 2026-09-29

1. 安装与 [uproject](../../AesirWarden.uproject)匹配的 Unreal Engine 5.8、Windows C++ 编译工具链和 Rider 或 Visual Studio。打开工程前确认所需插件可用：GameplayAbilities、StateTree、GameplayStateTree、MotionWarping、GASToolsets；ModelingToolsEditorMode 仅编辑器使用。
2. 获取项目源码、配置和有权使用的 `Content` 资产。公开仓库不含第三方原始素材、包含外部动作数据的 Animation Sequence 与模板地图，单独克隆不能运行完整 Boss 战；完整工程须从本地或受限访问的项目副本恢复。生成工程文件并编译 `AesirWardenEditor`；首次运行在编辑器核对 Blueprint 父类、GameMode 和输入映射。生成的 `Binaries/`、`Intermediate/`、`DerivedDataCache/` 为本地产物。
3. 仅在测试 PPO 时另行准备 `aesir-ai-service`、Python 虚拟环境及相应训练依赖，启动 Boss 推理端口 `8012` 并核对模型 Schema/哈希。BT 模式的本地战斗不应依赖该服务。命令服务 `8011` 是后续同伴功能。
4. 首次运行按 [COMBAT-01](../06_Test_Doc/VerificationMatrix.md)和 [BT-01](../06_Test_Doc/VerificationMatrix.md)做冒烟，记录 UE/插件版本、目标地图、启动参数与日志。

公开仓库的 `.gitignore` 默认排除 `Content`，允许项目系统 Blueprint 和自行配置的 AnimBP、Blend Space、Montage；包含外部动作数据的 Animation Sequence 仍被排除。`.gitattributes` 为允许提交的 UE 二进制资产配置 Git LFS。完整可运行工程的备份与公开仓库分开管理。
