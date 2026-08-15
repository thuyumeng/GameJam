## 1. 体型接口与分类 (creature-size)

- [x] 1.1 新建目录 `Source/GameJoy/Jungle/`(AI/、Interfaces/ 子目录),与现有变体并列
- [x] 1.2 定义 `ICreatureSized` UInterface(仿 `ICombatDamageable`),暴露 `GetCreatureSize()`
- [x] 1.3 创建阈值/参数数据源(DataAsset,含捕食阈值 T 默认 1.2,及视锥角/视距等感知参数)
- [x] 1.4 创建纯函数 `ClassifyBySize(MySize, OtherSize, T) -> {Prey, Peer, Predator}`(C++ static/BlueprintCallable)
- [ ] 1.5 创建 `Size → MaxWalkSpeed` 的单调递减 CurveFloat 资产
- [ ] 1.6 用几组样例 Size 验证三档分类(明显更大/接近/明显更小),改阈值结果随之变化

## 2. 生物基类与 AIController (C++)

- [x] 2.1 创建 `ACreature : ACharacter`(abstract,仿 `ACombatEnemy`),实现 `ICreatureSized`,暴露 `Size`
- [x] 2.2 Size 变化时读 CurveFloat 更新 `CharacterMovement.MaxWalkSpeed`(即时生效,无需重生成)
- [x] 2.3 创建 `ACreatureAIController : AAIController`,持 `UStateTreeAIComponent`(仿 `ACombatAIController`),`OnPossess` 启动 StateTree
- [ ] 2.4 编译通过;放一个派生蓝图到关卡,确认可被 AIController 附身并移动(不接大脑逻辑)

## 3. 感知:EQS + StateTree 任务/条件 (creature-perception)

- [x] 3.1 创建 EQS 查询与 Context(仿 `EnvQueryContext_Player/Danger`):候选生物 → 视锥角过滤 → 视线 LOS Trace
- [x] 3.2 在 `CreatureStateTreeUtility` 中新建感知任务/条件:对候选调用 `ClassifyBySize`,输出「当前猎物目标 / 是否存在天敌 / 势均力敌对象 + 最后已知位置」
- [ ] 3.3 验证视线遮挡:被草丛/岩石挡住即感知不到,移出遮挡后恢复
- [ ] 3.4 验证盲区(正后方)与超出视距的目标不被感知

## 4. 生物大脑 StateTree (creature-ai)

- [ ] 4.1 创建生物 StateTree 资产,定义 WANDER / CHASE / FLEE / STANDOFF 四状态,由 `ACreatureAIController` 运行
- [x] 4.2 WANDER:无感知目标时游荡/闲逛(`FStateTreeWanderTask`:随机可达点→停留→再走;StateTree 已接)
- [x] 4.3 CHASE:感知到猎物→追逐(引擎 `FStateTreeMoveToTask` 绑 `PreyTarget`);目标脱离感知→返回 WANDER;〔前往最后已知位置/接触捕食判定待补〕
- [x] 4.4 FLEE:感知到天敌→朝远离方向移动(`FStateTreeFleeTask`:直线背离+navmesh 投影+周期重算);天敌脱离感知→返回 WANDER〔C++ 完成,编辑器 FLEE 状态接线+PIE 验证待做〕
- [ ] 4.5 STANDOFF:势均力敌→停止接近并警戒,复用 `FStateTreeFaceActorTask` 面向对方;退出条件:脱离感知→WANDER、重分类→CHASE/FLEE、逼近个人空间→FLEE
- [ ] 4.6 复用 `FStateTreeSetCharacterSpeedTask` 等现成任务;为 STANDOFF 及各转移加入迟滞/最小停留时间,避免抖动
- [ ] 4.7 确认所有派生物种共用同一 StateTree,仅 Size 不同即表现不同行为

## 5. 玩家接入

- [x] 5.1 `AGameJoyCharacter`(或派生的丛林玩家)实现 `ICreatureSized` 并持有 Size,进入 EQS 候选集
- [ ] 5.2 验证动物用与动物完全相同的视锥/视线/分类规则对待玩家(玩家小→被追,玩家大→被逃/对峙)

## 6. 测试关卡与验收

- [ ] 6.1 在 greybox 关卡放置可挡视线的遮挡物(草丛/岩石),约定其碰撞设置
- [ ] 6.2 放置若干不同 Size 的派生动物 + 玩家,逐条走查 spec 场景(捕食阈值、灰色地带对峙、越大越慢、盲区/遮挡、体型变化打破对峙)
- [ ] 6.3 验证"加新动物 = 派生 `ACreature` + 配置 Size/外观,零新逻辑"
