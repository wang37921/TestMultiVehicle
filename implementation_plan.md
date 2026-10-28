# 局域网联机改造与 Chaos 车辆移动同步方案设计 (implementation_plan.md)

## 1. 深度意图理解与架构方案对比

用户需求：将当前的单人 Chaos 车辆工程改造为支持**局域网联机**，用于测试 **Chaos 物理车辆在网络下的移动同步表现**。

### 1.1 方案判断与潜在风险提示（规则 7 & 10）
常规的虚幻角色（Character）使用基于胶囊体的运动学模拟（Kinematic），同步机制极其成熟。但 **Chaos 车辆是基于刚体动力学的物理模拟（Rigid Body Physics Simulation）**，联机开发中有以下几个典型坑点：

| 关键维度 | 单人模式现状 | 局域网联机标准做法 (UE5 标准) | 潜在风险与利弊分析 |
| :--- | :--- | :--- | :--- |
| **Actor 网络复制** | `bReplicates = false`，未开启复制 | 显式开启 `bReplicates = true` 与 `SetReplicateMovement(true)` | **如果不开启**：客户端连接后看不见远端车辆，也无法收到服务器位置更新。 |
| **控制器 Possession** | 仅在 `OnPossess()` 中给 `VehiclePawn` 赋值 | 客户端通过重写 `AcknowledgePossession` / 动态更新 | **如果不处理**：`OnPossess` 是服务器独占事件，客户端不会触发，导致客户端 UI 仪表盘的车速、挡位始终为 0。 |
| **车辆出生点排布** | 场景内仅 1 个 `PlayerStart` | GameMode 重写生成位计算，按**赛车起跑格（Grid）**错落排布 | **如果不处理**：两辆物理模拟小车在同一坐标生成，发生严重的刚体碰撞重叠，瞬间产生巨大排斥力导致车辆**炸飞上天**或陷入地面。 |
| **重置车辆与翻车自救** | 客户端本地直接 `SetActorTransform` | 使用 `ServerResetVehicle` 可靠 RPC 权威重置 | **如果不处理**：客户端本地瞬移会被服务器状态判定为非法，产生剧烈的瞬移拉扯回弹（Rubberbanding）。 |
| **网络同步频率** | 默认低频（约 20~33Hz） | 设置 `NetUpdateFrequency = 66.0f`，`MinNetUpdateFrequency = 33.0f` | 高速移动物理车辆需要高频状态广播，否则其他玩家看到的车辆会有拖影、卡顿感。 |

---

## 2. 改造详细设计

### 2.1 车辆 Pawn：`ATestAllControlPawn`
1. **构造函数**：
   ```cpp
   bReplicates = true;
   SetReplicateMovement(true);
   NetUpdateFrequency = 66.0f;
   MinNetUpdateFrequency = 33.0f;
   ```
2. **翻车重置 RPC**：
   - 增加可靠 Server RPC：`ServerResetVehicle()`
   - 客户端触发 `DoResetVehicle()` 时，如果不是权威端（`!HasAuthority()`），则调用 `ServerResetVehicle()`；服务端权威端执行真实的位置重置与线速度/角速度清零。
3. **刹车灯多端同步**：
   - 增加 `UPROPERTY(ReplicatedUsing = OnRep_Braking) bool bReplicatedBraking`
   - 当刹车或手刹状态变化时在服务器赋值，所有客户端在 `OnRep_Braking` 中同步调用 `BrakeLights(bReplicatedBraking)`，实现两车对视时车灯实时亮灭。

### 2.2 玩家控制器：`ATestAllControlPlayerController`
1. **客户端 Possession 绑定**：
   - 重写 `AcknowledgePossession(APawn* P)`：在客户端确认拥有 Pawn 时，给 `VehiclePawn` 赋值并初始化绑定。
   - 在 `Tick` 中加入安全保护：如果 `VehiclePawn` 为空但本地 Pawn 已就绪，动态获取 `Cast<ATestAllControlPawn>(GetPawn())`。
2. **重生权限限制**：
   - 将 `OnPawnDestroyed` 里的重新生成和拥有逻辑限制在 `if (HasAuthority())` 下执行，杜绝客户端非法调用。

### 2.3 游戏模式：`ATestAllControlGameMode`
1. **起跑格（Grid Start）防重叠排布**：
   - 重写 `ChoosePlayerStart_Implementation(AController* Player)` 或在生成 Pawn 时计算偏移：
   - 维护已进入房间的玩家序列号（PlayerIndex）；
   - 基于地图首个 `PlayerStart`，以当前玩家序号计算偏移量：
     - 横向间距：左右交错偏移 3.5 米（左右交替：奇数偏左、偶数偏右）；
     - 纵向间距：每排向后退 6 米；
   - 确保加入局域网的每位玩家均有独立的起跑发车位，彻底消除碰撞穿模风险。

---

## 3. 待修改与交付文件清单

1. [`Source/TestAllControl/TestAllControlPawn.h`](file:///Users/cong/Documents/Unreal%20Projects/TestAllControl/Source/TestAllControl/TestAllControlPawn.h)
2. [`Source/TestAllControl/TestAllControlPawn.cpp`](file:///Users/cong/Documents/Unreal%20Projects/TestAllControl/Source/TestAllControl/TestAllControlPawn.cpp)
3. [`Source/TestAllControl/TestAllControlPlayerController.h`](file:///Users/cong/Documents/Unreal%20Projects/TestAllControl/Source/TestAllControl/TestAllControlPlayerController.h)
4. [`Source/TestAllControl/TestAllControlPlayerController.cpp`](file:///Users/cong/Documents/Unreal%20Projects/TestAllControl/Source/TestAllControl/TestAllControlPlayerController.cpp)
5. [`Source/TestAllControl/TestAllControlGameMode.h`](file:///Users/cong/Documents/Unreal%20Projects/TestAllControl/Source/TestAllControl/TestAllControlGameMode.h)
6. [`Source/TestAllControl/TestAllControlGameMode.cpp`](file:///Users/cong/Documents/Unreal%20Projects/TestAllControl/Source/TestAllControl/TestAllControlGameMode.cpp)

---

## 4. 联机测试步骤与指引

1. **编辑器内快捷测试（PIE 双人联机，最快验证）**：
   - 虚幻编辑器主视口顶部“运行（Play）”按钮旁点击三个点 `...`；
   - **Net Mode**：选择 `Play As Listen Server`（作为监听服务器运行）；
   - **Number of Players**：设置为 `2`；
   - 点击运行，编辑器将同时拉起两个视口窗口，左边主机右边客机，同屏进行撞车、追逐、测速与物理同步测试！
2. **局域网跨机器测试（Standalone 模式）**：
   - 主机电脑：游戏控制台按 `~` 输入 `open VehicleBasic?listen`
   - 客机电脑：游戏控制台按 `~` 输入 `open <主机IP地址>`
