## Why

丛林游戏的核心是一条"越吃越强"的食物链循环:狩猎 → 吞食 → 成长 → 解锁更强的猎物。这套循环要转起来,首先需要一层地基——所有生物(包括玩家)都能**感知周围、按体型判断敌友、并作出追逐或逃跑的决策**。本提案只交付这层地基;伏击/冲刺、能量成长曲线、随机能力等花活都建立在它之上,因此必须先把它做扎实、做通用。

## What Changes

- 引入统一的 **Size(体型)数值**:每个实体(动物 + 玩家)只带一个体型标量,作为整个食物链的唯一货币。
- 引入 **B 规则的捕食判定**:必须"明显更大"(体型比达到阈值 ~1.2x)才能捕食;中间区间为**灰色地带**,双方谁都吃不动对方。
- 引入 **越大越慢**:移动速度与体型反相关,大块头强壮但迟缓,小体型脆弱但灵活。
- 引入 **视锥 + 视线感知**:生物只能看见正前方视锥内、且无遮挡的实体。感知通过 **EQS 查询 + C++ StateTree 任务/条件**实现(沿用代码库现有 AI 范式,不引入 AIPerception)。朝向、盲区、遮挡从此成立。
- 引入 **统一生物大脑(StateTree)**:一套状态机被所有物种共用,只靠 Size 数值区分行为——
  - `WANDER` 游荡 → 感知到实体后按体型分类
  - `CHASE` 追逐(目标明显更小)
  - `FLEE` 逃跑(目标明显更大)
  - `STANDOFF` 对峙(体型接近,落在灰色地带 → 警戒/绕圈,等待平衡被打破)
- **玩家也是这套感知规则下的一个实体**:动物用同样的方式感知并判断玩家。

**范围之外(本次不做,留给后续 change):**
- 大块头如何追到更快的小猎物(伏击 / 冲刺机制)——捕食命中先用占位判定。
- 能量成长曲线的具体数值设计。
- 随机能力清单与效果。
- 战斗/血量系统(灰色地带暂为"警戒对峙",不含互相伤害)。

## Capabilities

### New Capabilities

- `creature-size`: 统一的体型数值模型——Size 标量、B 规则的捕食阈值与灰色地带区间划分、体型↔速度的反相关关系。
- `creature-perception`: 基于视锥 + 视线的感知——生物如何探测其他实体(含朝向、盲区、遮挡),并将感知到的实体按体型分类为猎物 / 势均力敌 / 天敌。
- `creature-ai`: 所有生物共用的 StateTree 行为大脑——WANDER / CHASE / FLEE / STANDOFF 四状态及其基于体型分类的转移规则。

### Modified Capabilities

<!-- 无现有 spec;这是项目的第一批 capability。 -->

## Impact

- **引擎/工程**:Unreal Engine 5.8,**GameJoy 为 C++ 工程**(Runtime 模块 `GameJoy`,已依赖 `AIModule`、`UMG`)。第三人称模板 + Combat/Platforming/SideScrolling 三套变体。已启用 **StateTree** 与 **GameplayStateTree** 插件。
- **沿用现有 AI 范式**(仿照 `Variant_Combat/AI/`):
  - AIController 挂 `UStateTreeAIComponent` 跑 StateTree(仿 `ACombatAIController`)。
  - StateTree 的任务/条件用 C++ 结构体实现(仿 `CombatStateTreeUtility` 里的 `FStateTree*Task`/`*Condition`)。
  - 空间查询用 **EQS**(仿现有 `EnvQueryContext_Player` / `EnvQueryContext_Danger`)。
  - **不使用 AIPerception**(全代码库 0 处使用)。
- **新增 C++**(建议置于新目录 `Source/GameJoy/Jungle/` 或 `Variant_Jungle/`,与现有变体并列):
  - `ACreature : ACharacter` 基类——持有 `Size`,实现体型↔速度逻辑(仿 `ACombatEnemy`;abstract)。所有动物由它派生。
  - `ACreatureAIController : AAIController`——持 `UStateTreeAIComponent`(仿 `ACombatAIController`)。
  - `ICreatureSized` UInterface——暴露 Size 供任意实体(含玩家)被查询/分类(仿 `ICombatDamageable` 接口范式)。
  - `CreatureStateTreeUtility`——新的 StateTree 任务/条件(感知、分类、CHASE/FLEE/STANDOFF 决策);**复用**现有 `FStateTreeSetCharacterSpeedTask`、`FStateTreeFaceActorTask` 等。
  - EQS 查询/Context——"视锥内、无遮挡、按体型可捕食/为天敌的最近生物"。
- **新增资产**:生物 StateTree 资产(四状态 + 转移)、Size→速度 CurveFloat、阈值 DataAsset。
- **玩家角色**:第三人称 `AGameJoyCharacter` 接入 `ICreatureSized`(带 Size),使其在动物眼中等同于一个实体。(注:第三人称天然适配"看着自己变大",不再受第一人称视角限制。)
- **关卡/环境**:视线遮挡依赖场景遮挡物(草丛、岩石等),`LevelPrototyping/` 需能提供遮挡体。
- **风险**:EQS 每次查询 + 视线 Trace 有成本;需控制查询频率与同屏生物数(见 design.md)。
