# 任务追踪列表 (task.md)

## 当前任务：Chaos 车辆局域网联机改造与移动同步测试

- [x] **1. 车辆 Pawn 复制与网络同步机制改造 (`TestAllControlPawn`)**
  - [x] 启用 Actor 网络复制：`bReplicates = true`、`AActor::SetReplicateMovement(true)`
  - [x] 调整网络更新频率：`SetNetUpdateFrequency(66.0f)`、`SetMinNetUpdateFrequency(33.0f)`（采用 UE5.8 最新推荐 API，保证高速物理运动平滑）
  - [x] 重置车辆（翻车自救与按键重置）改造为 `ServerResetVehicle` 服务端可靠 RPC，彻底防止客户端位置被服务器瞬移回拉
  - [x] 添加刹车状态属性同步（`bReplicatedBraking` 与 `OnRep_Braking`），使局域网联机双方能实时观察对方刹车灯

- [x] **2. 玩家控制器客户端适配与仪表盘修复 (`TestAllControlPlayerController`)**
  - [x] 重写 `AcknowledgePossession` 与 `OnRep_Pawn`，动态恢复 `VehiclePawn` 引用，彻底解决客户端 UI 仪表盘速度与挡位不刷新的单人模式遗留 Bug
  - [x] 限制 `OnPawnDestroyed` 仅在权威端（`HasAuthority()`）执行重生与拥有

- [x] **3. GameMode 多人起跑格排布与防重叠生成 (`TestAllControlGameMode`)**
  - [x] 重写 `SpawnDefaultPawnFor_Implementation`，实现标准赛车起跑格（Grid Start）排布算法
  - [x] 多玩家生成自动沿起跑线错开：每排向后退 6 米、左右相隔 4 米，彻底避免两辆刚体物理车同点生成导致碰撞穿模被炸飞

- [x] **4. 编译构建验证**
  - [x] 编译 `TestAllControlEditor Mac Development`（成功，耗时 4.41s，零警告零报错）

- [x] **5. 输出局域网联机与编辑器 PIE 双人测试指引**
  - [x] 编辑器一键双开测试（Play As Listen Server + 2 Players）
  - [x] 局域网跨机器测试指南（`open <Map>?listen` 与 `open <IP>`）
