# 构建与目标平台

> 状态：配置静态核对；未在本轮打包 · 2026-09-29

当前 [uproject](../../AesirWarden.uproject)关联 Unreal Engine 5.8、运行模块 `AesirWarden`，启用 GameplayAbilities、StateTree、MotionWarping 等插件。目标平台为 Windows；[DefaultEngine.ini](../../Config/DefaultEngine.ini)中默认 RHI 为 DX12。项目 30 FPS 是性能目标，具体硬件与判据尚未冻结。

当前 `GameDefaultMap` 和 `EditorStartupMap` 均指向本地 `/Game/Aesir/Maps/L_OpenWorld`。地图资产不进入公开仓库，因此公开源码克隆不能直接启动完整战斗；发布可运行版本前，仍须在打包后从冷启动到结果/重试验证，不能仅凭编辑器 PIE 断言可发布。

构建记录应包括 UE 版本、构建配置、目标地图、Cook/打包设置、插件版本、Python 服务/模型打包方式、硬件、日志与输出哈希。裁剪或性能设置只有测得收益后才固定。
