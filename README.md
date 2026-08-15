# GameJoy — 丛林食物链

> Unreal Engine 5.8 · C++ 工程

一个"越吃越强"的丛林生存游戏原型。核心循环:**狩猎 → 吞食 → 成长 → 解锁更强的猎物**。所有生物(包括玩家)共享同一套感知与决策规则——按体型判断敌友,再决定追逐、逃跑还是对峙。

本仓库当前交付这条循环的**第一层地基**:统一体型数值、视锥+视线感知、以及所有生物共用的 StateTree 行为大脑。伏击/冲刺、能量成长曲线、随机能力等机制建立在它之上,留待后续迭代。

---

## 核心设计

| 概念 | 说明 |
| --- | --- |
| **Size(体型)** | 每个实体(动物 + 玩家)只带一个体型标量,作为整个食物链的唯一"货币"。捕食关系只由 Size 之比决定,与物种无关。 |
| **B 规则捕食判定** | 可配置阈值 `T`(默认 1.2)把体型关系分三档:`A/B ≥ T` → A 视 B 为**猎物**;`B/A ≥ T` → A 视 B 为**天敌**;两者之间为**灰色地带**,互为**势均力敌**,谁都吃不动谁。 |
| **越大越慢** | 移动速度与体型反相关(单调递减曲线):大块头强壮但迟缓,小体型脆弱但灵活。 |
| **视锥 + 视线感知** | 生物只能看见正前方视锥内、且无遮挡的实体。朝向、盲区、遮挡由此成立。走 **EQS + C++ StateTree**,**不使用 AIPerception**(沿用代码库既有 AI 范式)。 |
| **统一生物大脑** | 一套 StateTree 被所有物种共用,仅靠 Size 数值区分行为——加新动物 = 派生 `ACreature` + 配置 Size/外观,零新逻辑。 |
| **玩家也是一个实体** | 第三人称玩家实现同一套 `ICreatureSized` 接口,被动物用完全相同的规则感知与分类。 |

### 生物行为状态(StateTree)

```
              感知到猎物(明显更小)
   WANDER ─────────────────────────▶ CHASE ──▶ 接触触发捕食判定(占位)
   游荡 ◀─────────────────────────── 目标脱离感知

   WANDER ─── 感知到天敌(明显更大)──▶ FLEE ──▶ 天敌脱离感知 → WANDER

   WANDER ─── 感知到势均力敌(灰色地带)──▶ STANDOFF(警戒/绕圈,不吃不逃)
                                          退出:脱离感知→WANDER
                                                重分类→CHASE/FLEE
                                                逼近个人空间→FLEE
```

---

## 代码结构

新系统位于 [Source/GameJoy/Jungle/](Source/GameJoy/Jungle/),与现有游戏变体并列,结构对称:

| 路径 | 职责 |
| --- | --- |
| [Jungle/Interfaces/CreatureSized.h](Source/GameJoy/Jungle/Interfaces/CreatureSized.h) | `ICreatureSized` UInterface,暴露 `GetCreatureSize()`,让任意实体(含玩家)参与食物链。 |
| [Jungle/CreatureSizeStatics.h](Source/GameJoy/Jungle/CreatureSizeStatics.h) | 纯函数 `ClassifyBySize(MySize, OtherSize, T) → {Prey, Peer, Predator}`。 |
| [Jungle/CreaturePerceptionData.h](Source/GameJoy/Jungle/CreaturePerceptionData.h) | DataAsset:捕食阈值 T、视锥角、视距等感知参数的单一数据源。 |
| [Jungle/Creature.h](Source/GameJoy/Jungle/Creature.h) | `ACreature : ACharacter`(abstract),持有 Size、按曲线映射速度。所有动物由它派生。 |
| [Jungle/AI/CreatureAIController.h](Source/GameJoy/Jungle/AI/CreatureAIController.h) | `ACreatureAIController : AAIController`,持 `UStateTreeAIComponent` 运行大脑。 |
| [Jungle/AI/CreatureStateTreeUtility.h](Source/GameJoy/Jungle/AI/CreatureStateTreeUtility.h) | StateTree C++ 任务/条件:感知、分类、WANDER/CHASE/FLEE 决策。 |
| [Jungle/AI/EnvQueryContext_Creatures.h](Source/GameJoy/Jungle/AI/EnvQueryContext_Creatures.h) | EQS Context:视锥内、无遮挡、按体型可捕食/为天敌的最近生物。 |

设计上**沿用现有 AI 范式**([Source/GameJoy/Variant_Combat/AI/](Source/GameJoy/Variant_Combat/AI/)):AIController 挂 StateTree、任务/条件用 C++ 结构体、空间查询用 EQS,并复用其中的 `FStateTreeSetCharacterSpeedTask`、`FStateTreeFaceActorTask` 等。

### 其它内容

工程基于第三人称模板,另含三套游戏变体,可作参考:

- [Variant_Combat/](Source/GameJoy/Variant_Combat/) — 战斗玩法(含 `ICombatDamageable` 血量/伤害、AI、UI)。丛林 AI 即仿其范式。
- [Variant_Platforming/](Source/GameJoy/Variant_Platforming/) — 平台跳跃玩法。
- [Variant_SideScrolling/](Source/GameJoy/Variant_SideScrolling/) — 横版卷轴玩法。

---

## 规格驱动开发(OpenSpec)

本项目用 [OpenSpec](https://github.com/openspec) 管理需求。规格与设计文档位于 [openspec/](openspec/):

- [openspec/changes/creature-perception-foundation/proposal.md](openspec/changes/creature-perception-foundation/proposal.md) — 为什么做、做什么、范围内外。
- [openspec/changes/creature-perception-foundation/design.md](openspec/changes/creature-perception-foundation/design.md) — 技术决策(D1–D7)、风险权衡、落地顺序。
- [openspec/changes/creature-perception-foundation/tasks.md](openspec/changes/creature-perception-foundation/tasks.md) — 实现任务清单与进度。
- `specs/` 下三份能力规格:`creature-size`、`creature-perception`、`creature-ai`(WHEN/THEN 场景验收)。

**当前进度(creature-perception-foundation)**:C++ 地基基本落地——接口、分类纯函数、`ACreature` 基类、AIController、EQS Context、感知任务、WANDER/CHASE/FLEE 任务、玩家接入 `ICreatureSized` 均已完成。**待做**:Size→速度 CurveFloat 与生物 StateTree 资产的编辑器接线、STANDOFF 状态、迟滞防抖、以及 greybox 测试关的逐条 spec 验收。详见 [tasks.md](openspec/changes/creature-perception-foundation/tasks.md)。

---

## 上手

**环境**:Unreal Engine 5.8、支持 C++ 的 IDE(Rider / Visual Studio / Xcode)。

1. 克隆仓库后,右键 `GameJoy.uproject` → **Generate project files**(或首次双击 `.uproject` 时按提示编译)。
2. 用 IDE 打开生成的解决方案,编译 `GameJoy` 与 `GameJoyEditor` 目标。
3. 双击 `GameJoy.uproject` 启动编辑器。

已启用插件:`StateTree`、`GameplayStateTree`、`ModelingToolsEditorMode`、`ModelContextProtocol`、`MCPClientToolset`。

> **注**:`Binaries/`、`Intermediate/`、`Saved/`、`DerivedDataCache/`、`Build/` 为引擎生成目录,已在 `.gitignore` 中排除,首次打开工程时会自动重建。
