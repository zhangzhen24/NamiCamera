# Nami Camera 相机系统

一个功能强大的模块化虚幻引擎相机系统，支持模式堆栈、处理器扩展和运行时调整。

## 核心特性

- **模式堆栈系统**：支持多相机模式同时激活，自动混合过渡
- **统一处理器架构**：通过 Processor 统一管理持久效果和临时效果
- **计算器模式**：Mode 内部使用 Calculator 计算视图各组成部分
- **动画集成**：通过动画通知实现时间轴驱动的相机处理
- **管线化处理**：结构化的相机计算管线，6 阶段处理
- **调试可视化**：内置调试绘制和日志系统

## 快速开始

### 1. 基础设置

1. 在你的 Pawn/Character 上添加 `UNamiCameraComponent`
2. 设置 `ANamiPlayerCameraManager` 作为 PlayerCameraManagerClass
3. 配置默认相机模式（如 `UNamiThirdPersonCamera`）

**C++ 设置示例：**

```cpp
// 在 PlayerController 中
AMyPlayerController::AMyPlayerController()
{
    PlayerCameraManagerClass = ANamiPlayerCameraManager::StaticClass();
}

// 在 Character 中
AMyCharacter::AMyCharacter()
{
    // 创建相机组件
    CameraComponent = CreateDefaultSubobject<UNamiCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(RootComponent);

    // 设置默认相机模式
    CameraComponent->DefaultCameraMode = UNamiThirdPersonCamera::StaticClass();
}
```

### 2. 使用相机模式

相机模式定义相机的行为方式。可以在运行时推送和弹出模式：

```cpp
// 推送新的相机模式（如瞄准模式）
FNamiCameraModeHandle Handle = CameraComponent->PushCameraMode(UMyAimingMode::StaticClass());

// 完成后弹出模式
CameraComponent->PopCameraMode(Handle);
```

### 3. 添加处理器（Processor）

处理器为相机提供效果扩展。系统提供两种类型的处理器：

**持久处理器**（Persistent Processor）：持续生效，直到手动移除

```cpp
// 添加相机震动处理器
UNamiCameraShakeProcessor* ShakeProcessor = NewObject<UNamiCameraShakeProcessor>(this);
CameraComponent->AddProcessor(ShakeProcessor);

// 移除处理器
CameraComponent->RemoveProcessor(ShakeProcessor);
```

**可混合处理器**（Blendable Processor）：自动混入混出的临时效果

```cpp
// 播放电影效果（自动混入）
UNamiCameraBlendableProcessor* Processor = CameraComponent->PlayProcessor(UMyCinematicProcessor::StaticClass());

// 停止处理器（自动混出）
CameraComponent->StopProcessor(Processor);
```

## 系统架构

### 极简两层模型（新人只需理解 2 个概念）

```
┌────────────────────────────────────────┐
│  CameraMode（相机模式）                 │  ← 选择相机类型
│    - ThirdPerson, DualFocus, FPS 等    │
└────────────────────────────────────────┘
                    ↓
┌────────────────────────────────────────┐
│  CameraProcessor（相机处理器）          │  ← 添加/播放效果
│    - AddProcessor()  持久效果           │
│    - PlayProcessor() 临时效果           │
└────────────────────────────────────────┘

【高级用户额外了解】

┌────────────────────────────────────────┐
│  Calculator（计算器）                   │  ← Mode 内部实现
│    - 隐藏在 Mode 内部，新人无需了解     │
└────────────────────────────────────────┘
```

### 使用场景对照

| 场景 | API | 说明 |
|------|-----|------|
| 选择相机类型 | `SetCameraMode(ThirdPersonMode)` | 基础设置 |
| 添加碰撞检测 | `AddProcessor(CollisionProcessor)` | 持久生效 |
| 添加震动配置 | `AddProcessor(ShakeProcessor)` | 持久生效 |
| 技能释放FOV变化 | `PlayProcessor(SkillFOVProcessor)` | 播放一次 |
| 过场动画相机 | `PlayProcessor(CinematicProcessor)` | 播放一次 |

### 相机管线

每帧通过以下阶段计算相机视图：

1. **预处理**：初始化上下文，验证组件
2. **模式堆栈**：计算并混合所有激活模式
3. **持久处理器**：应用持久效果（震动、碰撞检测）
4. **可混合处理器**：应用临时参数修改
5. **控制器同步**：更新 PlayerController 旋转
6. **平滑过渡**：从当前位置平滑过渡到目标位置
7. **后处理**：调试绘制、日志输出

## 目录结构

```
Source/NamiCamera/
├── Public/
│   ├── Animation/          # 动画通知集成
│   ├── Calculators/        # 计算器（Mode 内部使用）
│   │   ├── Target/         # 目标计算器
│   │   ├── Position/       # 位置计算器
│   │   ├── Rotation/       # 旋转计算器
│   │   └── FOV/            # FOV 计算器
│   ├── CameraModes/        # 相机模式
│   ├── Components/         # 相机组件（入口点）
│   ├── Data/               # 数据结构（View、State、Context）
│   ├── DevelopSetting/     # 开发设置
│   ├── Interfaces/         # 接口
│   ├── Logging/            # 日志类别和宏
│   ├── Math/               # 相机数学工具
│   ├── Processors/         # 处理器系统
│   │   ├── Persistent/     # 持久处理器
│   │   └── Blendable/      # 可混合处理器
│   └── Stat/               # 性能统计
└── Private/                # 实现文件
```

## 创建自定义内容

### 自定义相机模式

1. 继承 `UNamiComposableCameraMode`（最简单）或 `UNamiCameraModeBase`
2. 重写 `CalculatePivotLocation()` 定义相机看向的位置
3. 在类默认值中配置混合设置

```cpp
UCLASS()
class UMyCustomMode : public UNamiCameraModeBase
{
    GENERATED_BODY()

protected:
    virtual FVector CalculatePivotLocation_Implementation(float DeltaTime) override
    {
        // 返回相机应该聚焦的位置
        return GetCameraComponent()->GetOwnerPawn()->GetActorLocation();
    }
};
```

### 自定义持久处理器

1. 继承 `UNamiCameraPersistentProcessor`
2. 重写 `Process()` 修改相机视图

```cpp
UCLASS()
class UMyPersistentProcessor : public UNamiCameraPersistentProcessor
{
    GENERATED_BODY()

protected:
    virtual void Process_Implementation(
        FNamiCameraView& InOutView,
        float DeltaTime) override
    {
        // 在这里修改 InOutView
        InOutView.CameraLocation += FVector(0, 0, 100); // 示例：向上偏移
    }
};
```

### 自定义可混合处理器

1. 继承 `UNamiCameraBlendableProcessor`
2. 重写 `CalculateProcessorParams()` 提供参数修改

```cpp
UCLASS()
class UMyBlendableProcessor : public UNamiCameraBlendableProcessor
{
    GENERATED_BODY()

protected:
    virtual FNamiCameraProcessorParams CalculateProcessorParams_Implementation(
        float DeltaTime) override
    {
        FNamiCameraProcessorParams Params;
        Params.ArmRotationOffset = FRotator(0, 45, 0); // 旋转 45 度
        Params.ArmRotationBlendMode = ENamiCameraProcessorBlendMode::Additive;
        Params.MarkArmRotationModified();
        return Params;
    }
};
```

## 核心概念

### 模式堆栈

- 多个模式可同时激活
- 每个模式有混合权重（0-1），基于混合配置
- 最终视图是所有激活模式的加权平均
- 可随时推送/弹出模式

### 处理器（Processor）

**持久处理器**（Persistent Processor）：
- 通过 `AddProcessor()` 添加
- 持续生效直到 `RemoveProcessor()` 移除
- 适用于：碰撞检测、震动、锁定目标等

**可混合处理器**（Blendable Processor）：
- 通过 `PlayProcessor()` 播放
- 自动混入/混出，支持配置时间
- 可由曲线驱动（速度、时间、自定义参数）
- 动画通知集成，实现时间轴控制
- 支持玩家输入打断
- 适用于：技能 FOV、过场动画、临时效果

### 计算器（Calculator）

- Mode 内部使用，负责计算视图各组成部分
- 新人无需了解，高级用户可自定义
- 类型：
  - **TargetCalculator**：计算 PivotLocation
  - **PositionCalculator**：计算 CameraLocation
  - **RotationCalculator**：计算 CameraRotation
  - **FOVCalculator**：计算 FOV

### 状态分离

输入和输出的清晰分离：
- **输入状态** (`FNamiCameraState`)：我们在看什么（目标、速度、玩家输入）
- **输出视图** (`FNamiCameraView`)：相机在哪里（位置、旋转、FOV）
- **管线上下文**：管线各阶段间传递的数据

## 调试功能

在 **项目设置 > Plugins > NamiCamera** 中配置：

- **日志开关**：分类控制日志输出（效果、模式混合、输入打断等）
- **屏幕日志**：输出调试信息到屏幕
- **DrawDebug**：可视化 PivotLocation、CameraLocation、相机方向等

## 性能说明

- 系统设计为仅在游戏线程运行（无异步/并行处理）
- 所有处理在每帧 `GetCameraView()` 中完成
- Processor 查找已优化为内部 Map
- 轻量级模式切换，使用对象池

## 版本信息

- **版本**：2.0
- **作者**：秋 (Qiu)
- **引擎兼容**：Unreal Engine 5.x

## 许可证

Copyright Qiu, Inc. All Rights Reserved.
