## ADDED Requirements

### Requirement: 统一生物行为大脑

所有动物物种 SHALL 共用同一套 StateTree 行为大脑,物种间的行为差异 SHALL 仅通过 Size 数值(及其它可配置参数)体现,而非各自独立的状态逻辑。大脑 SHALL 至少包含 WANDER、CHASE、FLEE、STANDOFF 四个状态。

#### Scenario: 不同物种共用同一大脑
- **WHEN** 一只小型动物与一只大型动物同时存在
- **THEN** 两者运行同一套 StateTree,仅因 Size 不同而表现出不同行为

#### Scenario: 新增物种无需新行为逻辑
- **WHEN** 引入一个新物种,仅配置其 Size 与外观
- **THEN** 该物种直接复用现有大脑,无需编写新的状态逻辑

### Requirement: 游荡状态

生物在未感知到任何相关实体时 SHALL 处于 WANDER 状态,在环境中进行非目标性的移动/闲逛。

#### Scenario: 无目标时游荡
- **WHEN** 生物的视锥内没有可感知的实体
- **THEN** 生物进入或保持 WANDER 状态

### Requirement: 追逐猎物

当生物感知到被分类为**猎物**的实体时,SHALL 转入 CHASE 状态并朝该猎物移动。当追逐目标脱离感知(移出视锥或被遮挡)时,生物 SHALL 放弃追逐并返回 WANDER。

#### Scenario: 发现猎物则追逐
- **WHEN** 生物感知到一个被分类为猎物的实体
- **THEN** 生物转入 CHASE 状态并向该实体移动

#### Scenario: 猎物脱离感知则放弃
- **WHEN** 正在被追逐的猎物移出视锥或被遮挡而脱离感知
- **THEN** 生物放弃追逐并返回 WANDER 状态

#### Scenario: 追上后触发捕食判定
- **WHEN** 生物在 CHASE 中接触到猎物
- **THEN** 系统触发捕食判定(本阶段为占位判定,伏击/冲刺细节留待后续)

### Requirement: 逃离天敌

当生物感知到被分类为**天敌**的实体时,SHALL 转入 FLEE 状态并朝远离天敌的方向移动。当天敌脱离感知时,生物 SHALL 停止逃跑并返回 WANDER。

#### Scenario: 发现天敌则逃跑
- **WHEN** 生物感知到一个被分类为天敌的实体
- **THEN** 生物转入 FLEE 状态并向远离该天敌的方向移动

#### Scenario: 天敌脱离感知则平静
- **WHEN** 天敌移出感知范围
- **THEN** 生物停止逃跑并返回 WANDER 状态

### Requirement: 灰色地带对峙

当生物感知到被分类为**势均力敌**的实体时,SHALL 转入 STANDOFF 状态:停止直接接近,保持警戒(面向对方,可绕圈),既不捕食也不逃跑。STANDOFF SHALL 在平衡被打破时结束——包括对方脱离感知、任一方 Size 变化导致重新分类、或对方逼近至个人空间内触发逃跑。

#### Scenario: 势均力敌进入对峙
- **WHEN** 生物感知到一个被分类为势均力敌的实体
- **THEN** 生物转入 STANDOFF 状态,停止接近并保持警戒,不捕食也不逃跑

#### Scenario: 体型变化打破对峙
- **WHEN** 对峙中任一方的 Size 发生变化,使双方重新分类为猎物/天敌关系
- **THEN** 对峙结束,较大一方转入 CHASE,较小一方转入 FLEE

#### Scenario: 逼近个人空间触发逃跑
- **WHEN** 对峙中的一方逼近至另一方的个人空间距离内
- **THEN** 被逼近方从 STANDOFF 转入 FLEE

#### Scenario: 对方离开则解除对峙
- **WHEN** 对峙对象脱离感知范围
- **THEN** 生物解除警戒并返回 WANDER 状态
