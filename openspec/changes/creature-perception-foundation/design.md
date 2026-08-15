## Context

工程为 GameJoy——Unreal Engine 5.8 的 **C++ 工程**(Runtime 模块 `GameJoy`,依赖 `Engine`/`AIModule`/`UMG`),基于第三人称模板,含 Combat、Platforming、SideScrolling 三套变体。已启用 **StateTree** 与 **GameplayStateTree** 插件。本设计为丛林食物链游戏铺第一层地基:一套所有生物(含玩家)共用的感知—决策系统。

代码库已有一整套可直接借鉴/复用的 AI 范式(`Variant_Combat/AI/`):
- `ACombatAIController : AAIController`,持 `UStateTreeAIComponent` 跑 StateTree。
- `ACombatEnemy : ACharacter`(abstract,实现接口 + 委托),由 StateTree 驱动。
- `CombatStateTreeUtility`:一批 C++ `FStateTreeTaskCommonBase` / `FStateTreeConditionCommonBase`,其中 `FStateTreeSetCharacterSpeedTask`、`FStateTreeFaceActorTask`、`FStateTreeGetPlayerInfoTask`、`FStateTreeIsInDangerCondition`(自带视锥半角)可直接复用或借范式。
- `EnvQueryContext_Player` / `EnvQueryContext_Danger`:**EQS** 用作空间查询与目标选择。
- 接口范式 `ICombatDamageable` / `ICombatAttacker`(UInterface)。
- **代码库 0 处使用 AIPerception**。

关键约束:
- **C++ 优先,沿用现有范式**:新系统用 C++ 基类 + StateTree C++ 任务/条件 + EQS,和 `Combat*` 保持一致,而非另起蓝图/AIPerception 炉灶。
- **统一大脑**:动物间不写各自独立 AI;差异仅由数据(Size 等)驱动。
- **感知走 EQS + StateTree**(用户已拍板),不引入 AIPerception。

## Goals / Non-Goals

**Goals:**
- `ACreature` C++ 基类 + 一个共用 StateTree,承载 WANDER / CHASE / FLEE / STANDOFF。
- 基于 **EQS + C++ StateTree 任务/条件**的视锥+视线感知,含遮挡(Trace)检测。
- 统一 Size 数值(经 `ICreatureSized` 接口暴露),B 规则分类(猎物/势均力敌/天敌,阈值 ~1.2),体型↔速度反相关。
- 玩家(第三人称 `AGameJoyCharacter`)实现 `ICreatureSized`,作为同一套规则下的普通实体被动物看待。
- 加新动物 = 派生 `ACreature` + 配置 Size/外观,零新逻辑。
- 尽量**复用** `CombatStateTreeUtility` 现有任务(设速度、面向目标等)。

**Non-Goals:**
- 伏击 / 冲刺 / 耐力等"大块头如何追到快猎物"的机制(捕食命中先用占位判定)。
- 能量成长曲线的数值平衡、随机能力系统。
- 生物间的战斗/互相伤害。注:Combat 变体已有 `ICombatDamageable` 血量/伤害系统,后续若把灰色地带做成"能打架",可在其上扩展——本次不做。
- 群体/协作 AI、寻路以外的高级导航。

## Decisions

### D1. 感知用 EQS + C++ StateTree 任务/条件,不用 AIPerception
- **选择**:感知实现为 StateTree 周期性驱动的 **EQS 查询**(或等价的 C++ 感知任务):候选生物 → 视锥角过滤 → 视线 LOS Trace → 按 Size 分类,输出当前 CHASE 目标 / 是否存在天敌 / 势均力敌对象。视锥半角可借 `FStateTreeIsInDangerCondition` 的 `SightConeAngle` 范式;目标信息借 `FStateTreeGetPlayerInfoTask` 范式扩成"任意生物"。
- **理由**:与代码库现有 AI(EQS + StateTree C++)100% 一致,不引入唯一的 AIPerception 异类;EQS 的 Trace 测试项天生能做视锥+视线遮挡。
- **代价**:AIPerception 白送的"感知记忆/遗忘"这里没有——需要时在 StateTree 里自存"上次见到的位置"。
- **备选**:AIPerception(Sight)——更省事、自带记忆,但成为代码库唯一异类,已否决。

### D2. 一套 StateTree,数据驱动物种差异
- **选择**:单个生物 StateTree 资产,被所有 `ACreature` 共用(经 `ACreatureAIController` 的 `UStateTreeAIComponent` 运行)。状态转移读取"感知目标的体型分类"派生结果,而非物种。
- **理由**:纯体型规则让所有物种决策逻辑同构;共用大脑使"加动物=加数据"。
- **备选**:每物种一棵 StateTree(维护爆炸);Behavior Tree(代码库统一用 StateTree,不混用)。

### D3. Size 经接口暴露,分类是纯函数
- **选择**:定义 `ICreatureSized` UInterface(仿 `ICombatDamageable`),暴露 `GetCreatureSize()`。任意实体(动物、玩家)实现它即可参与食物链。提供纯函数 `ClassifyBySize(MySize, OtherSize, T) -> {Prey, Peer, Predator}`,阈值 `T` 存单一数据源(DataAsset),默认 1.2。
- **理由**:接口让玩家与动物一视同仁、无需特例分支;分类每次现算,天然支持"变大→关系瞬间反转";阈值集中配置。
- **备选**:把 Size 直接放 `ACreature`(玩家就接不进来了);预存敌友表(违背纯体型动态性)。

### D4. 体型→速度用一条可配置曲线
- **选择**:`Size → MaxWalkSpeed` 的单调递减 `CurveFloat`;Size 变化时更新 `CharacterMovement.MaxWalkSpeed`。可复用现有 `FStateTreeSetCharacterSpeedTask` 在状态里临时改速(如逃跑加速留待后续)。
- **理由**:曲线好调、可视化;满足"体型变化即时影响速度"。
- **备选**:线性公式(够用但手感难调)。

### D5. 玩家实现 `ICreatureSized`,第三人称天然适配
- **选择**:`AGameJoyCharacter`(或其派生)实现 `ICreatureSized` 并持有 Size,进入同一套 EQS 候选集,被动物按体型分类。
- **理由**:spec 要求玩家是同一套规则下的实体;接口统一避免特例。第三人称视角天然能看到自身体型变化,解决了"看不到自己变大"的顾虑。

### D6. STANDOFF 是"等待打破平衡"的过渡态
- **选择**:STANDOFF 中停止接近、用现成 `FStateTreeFaceActorTask` 面向对方(可绕圈),不做接触判定。退出条件:目标脱离感知 → WANDER;重新分类为猎物/天敌 → CHASE/FLEE;对方进入个人空间 → FLEE。
- **理由**:灰色地带张力来自"随时可能被打破";建成过渡态而非稳定态,后续能力/伤害系统可直接挂到这些退出条件上。

### D7. 代码组织:新增独立目录,与现有变体并列
- **选择**:新代码放 `Source/GameJoy/Jungle/`(或 `Variant_Jungle/`),内部再分 `AI/`(Controller、StateTreeUtility、EQS)、`Interfaces/`(ICreatureSized),与 `Variant_Combat/` 结构对称。
- **理由**:遵循代码库按变体/特性分目录的惯例;与三套现有变体隔离,互不干扰。

## Risks / Trade-offs

- **[EQS 查询 + LOS Trace 的性能]** 每个生物周期性跑 EQS + 视线 Trace,数量大时开销上升 → 控制查询频率(非每帧)、限制查询半径与候选数、错帧调度、控制同屏生物数。
- **[无感知记忆]** EQS 无记忆,目标一断视线立刻"消失",可能显得生硬 → 在 StateTree 存"最后已知位置",CHASE 断视线后短暂前往该点再放弃;完整记忆留待后续。
- **[遮挡依赖关卡]** 视线遮挡要求场景有能挡视线的碰撞体;greybox 若无遮挡物,伏击门形同虚设 → 在 `LevelPrototyping/` 提供草丛/岩石等遮挡体并约定碰撞设置。
- **[越大越慢导致大块头吃不到快猎物]** 有意的设计张力,但地基阶段捕食命中是占位判定,可能出现"追不上→循环 CHASE 不放弃" → CHASE 加超时/失去感知即放弃;真正解法留给后续伏击/冲刺 change。
- **[对峙抖动]** 两个 STANDOFF 个体互相触发进/退可能抖动 → 转移加入迟滞(hysteresis)/最小停留时间。
- **[分类逻辑落点]** 分类/个人空间判定等抽成 C++ 纯函数或 StateTree 条件,避免散落在多处转移里。

## Migration Plan

全新系统,无既有数据迁移。落地顺序:
1. `ICreatureSized` 接口 + `ClassifyBySize` 纯函数 + 阈值 DataAsset,单元式验证三档分类。
2. `ACreature` 基类(Size + Size→速度曲线接线)+ `ACreatureAIController`(UStateTreeAIComponent),先跑通移动,不接大脑。
3. EQS 查询/Context + `CreatureStateTreeUtility` 感知任务(视锥+LOS+分类),输出目标/天敌/对峙对象。
4. 生物 StateTree(WANDER→CHASE/FLEE/STANDOFF)接感知输出,复用 SetSpeed/FaceActor 任务。
5. `AGameJoyCharacter` 实现 `ICreatureSized`。
6. greybox 测试关放遮挡物 + 若干不同 Size 的派生动物,逐条验收 spec 场景。

回滚:新代码/资产集中于独立目录,未改动现有变体;删除新增目录与资产即可回到原状。

## Open Questions

- 感知的具体载体:纯 EQS 查询,还是一个自写的 C++ StateTree 感知任务(内部做 overlap+cone+trace)?两者都行,实现时按复杂度定——倾向先 EQS 复用现有 Context 范式。
- 阈值 `T` 与视锥角/视距等参数的数据载体:单一 DataAsset vs 各自 `UPROPERTY(EditAnywhere)` 在 `ACreature` 上?倾向 DataAsset 集中管。
- "个人空间距离"是全局常量还是随 Size 缩放?(大动物个人空间更大?)
- 捕食占位判定的接口形状,如何设计才能让后续伏击/冲刺无痛替换?(建议也走 `ICreatureSized` 或新接口的一个方法)
- 玩家基类:直接给 `AGameJoyCharacter` 加接口,还是为丛林玩法派生一个新的玩家角色?
