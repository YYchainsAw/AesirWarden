# INI 配置索引

> 状态：当前文件静态核对 · 2026-09-29

| 文件 | 当前作用 | 修改后验证 |
| --- | --- | --- |
| [DefaultEngine.ini](../../Config/DefaultEngine.ini) | 启动地图、GameMode、Renderer/DX12 等 | 冷启动、PIE、Windows 包、画质和帧时间 |
| [DefaultGame.ini](../../Config/DefaultGame.ini) | 项目 ID 等通用项目设置 | 项目标识与打包元数据 |
| [DefaultInput.ini](../../Config/DefaultInput.ini) | Enhanced Input 默认类、轴设置 | 键鼠/手柄输入资产与运行绑定 |
| [DefaultGameplayTags.ini](../../Config/DefaultGameplayTags.ini) | Ability、Cooldown、Data、Event、State 标签 | GAS 激活、Tag 查询、蓝图引用与旧存档兼容 |
| [DefaultEditor.ini](../../Config/DefaultEditor.ini) | 当前为空 | 仅在确需团队统一编辑器设置时添加 |

记录任何新增配置的目的、默认值、受影响平台和回滚方式。运行生成的 `Saved/Config` 不作为项目规范来源；测试时须确认实际生效值。
