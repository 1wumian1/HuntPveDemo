# Hunt PVE Demo

这是一个面向 C++ 期末大作业的 UE5 第一人称 PVE 原型。当前版本提供一套纯 C++ 兜底玩法，即使还没有导入美术资源，也可以在默认 GameMode 下跑通核心流程。

## 操作方式

- `Enter` 或鼠标左键：主菜单开始游戏
- `WASD`：移动
- 鼠标：视角
- 鼠标左键：射击
- `R`：换弹
- `1` 或鼠标滚轮上：切换手枪/步枪
- `F`：交互线索/撤离点
- `H`：使用回血
- `G`：使用炸药
- `E`：暗视

## 玩法流程

1. 启动后在主菜单选择 Start Game。
2. 在原型地图中寻找并交互 3 个发光线索。
3. 线索收集完成后 Boss 会出现。
4. 击杀 Boss 后进入放逐倒计时，期间持续刷小怪。
5. 倒计时结束后前往绿色撤离点，进入范围即可胜利。

## 已实现模块

- `AHuntGameMode`：主流程状态机、线索计数、Boss 解锁、放逐、撤离胜负。
- `AHuntPlayerCharacter`：第一人称移动、血量、回血、炸药、暗视、脚步声入口。
- `AHuntWeaponBase`：手枪/步枪、射线检测、弹匣、备用弹药、换弹、命中伤害。
- `AHuntEnemyBase`：小怪追踪玩家、近战攻击、受击死亡。
- `AHuntBossEnemy`：高血量 Boss，死亡后触发放逐。
- `AHuntClueActor`：可交互发光线索。
- `AHuntSpawner`：普通阶段和放逐阶段刷怪。
- `AHuntExtractionPoint`：撤离点触发胜利。
- `AHuntHUD`：C++ Canvas HUD，显示血量、线索、武器弹药、放逐倒计时、胜负。

## 素材替换建议

当前 C++ 原型使用基础形状和灯光表达线索、撤离点和测试地图。导入素材后建议创建蓝图子类替换表现层：

- `BP_HuntPlayerCharacter`：绑定第一人称手臂、动画、受击镜头震动。
- `BP_HuntWeapon_Pistol` / `BP_HuntWeapon_Rifle`：绑定枪模、枪声、换弹音效、命中音效。
- `BP_HuntEnemy` / `BP_HuntBoss`：绑定僵尸/怪物模型、行走/攻击/死亡动画。
- `BP_HuntClueActor`：绑定线索模型、发光材质和收集音效。
- `BP_HuntExtractionPoint`：绑定撤离点标识、绿色光柱和撤离音效。

## 免费素材来源

- Fab / Epic Marketplace：搜索 `free FPS weapon`、`zombie`、`monster`、`village`、`farm`、`horror props`。
- Mixamo：下载人形敌人的行走、奔跑、攻击、死亡动画，再在 UE 中 Retarget。
- Freesound：枪声、脚步声、爆炸、受伤和回血音效，优先选择 CC0 或允许署名使用的资源。
- itch.io：搜索 `free gun sound pack`、`horror ambience`、`footstep sounds`。

## GitHub 提交说明

项目已准备 `.gitignore` 和 `.gitattributes`，并建议使用 Git LFS 管理 `.uasset`、`.umap`、`.fbx`、音频和图片。不要提交 `Binaries/`、`Intermediate/`、`Saved/`、`DerivedDataCache/`、`.vs/`。

推荐提交节奏：

- `Initial Unreal project setup`
- `Add Hunt PVE gameplay loop`
- `Add player weapons and items`
- `Add enemies boss and banish phase`
- `Add HUD and feedback`
- `Import prototype assets`
