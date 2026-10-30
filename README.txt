Assault Android Cactus
UE 5.8.3 / Windows 64bit / C++・Blueprint

工程を開く
1. Unreal Engine 5.8.3とVisual Studio 2022の「C++によるゲーム開発」を用意する。
2. AssaultAndroidCactus.uprojectを右クリックし、Generate Visual Studio project filesを実行する。
3. ソリューションをDevelopment Editor / Win64でビルドする。
4. AssaultAndroidCactus.uprojectを開き、タイトルLevelでPlayを押す。
初回はシェーダー処理に時間がかかる場合がある。

PowerShellでビルドする場合
.\build.ps1 -EngineDir 'UE_5.8のインストール先'
Editorを閉じた状態で実行する。

Windows実行版
別添のAssaultAndroidCactus_Windows.zipを展開し、AssaultAndroidCactus.exeを起動する。
実行版はUE EditorやVisual Studioを使わず起動できる。

ゲームの流れ
タイトルでSTART GAMEを選ぶとアリーナへ進む。
3波の敵を倒すと、最後の道具を拾う時間を経て結果を表示する。
電池は毎秒減り、0になるとMISSION FAILED。再挑戦またはタイトルへ戻れる。
倒れると3秒後に復帰する。倒れた時に電池容量の30%を追加消費する。
黄色い道具は100点、緑の電池は25を補充する。

操作
WASD / 矢印：移動
マウス：俯瞰ではカーソル位置へ照準、一人称では視線移動
左クリック長押し：攻撃
Space：AUTO / FLAME切替（1秒無敵、切替間隔1.5秒）
V：俯瞰 / 一人称切替
Esc：プレイ終了
Shift+F1：Editor内でマウスを解放
Xbox：左スティックで移動、右スティックで照準、RTで攻撃、LBで武器切替、Yで視点切替
Xbox Menu：プレイ終了
右スティックは上下反転、左右は通常方向。Dead Zoneは0.2。
メニューはマウス、上下/TabとEnter/Space、Xbox方向パッドとAで操作する。
結果画面のB/Escはタイトルへ戻る。

主な機能
移動・照準・2視点、通常弾・弾プール、火炎放射、体力・被弾無敵・ダウン復帰。
NavMeshで追跡する小型敵、射線が通ると停止して撃つ小型射撃敵。
Data Assetによる波管理、得点道具のドロップ・回収、電池、タイトル・結果・失敗画面。
追跡中の射撃と近接攻撃を行う大型敵、定位置から円形弾を撃つBoss。速度・火力強化アイテムは未実装。

フォルダ
Source：ゲームのC++とビルド設定。
Source.zip：同じSource/構成のコードZIP。展開先は工程ルート。
Config：入力・衝突・起動Level・パッケージ設定。
Content：Level、Blueprint、入力、モデル、アニメーション、材質、Niagara。
Docs/ClassDiagram.drawio：クラス設計図。未実装のクラスも含む。

主な編集箇所（Content内）
Characters/Player/BP_CactusCharacter
  親クラス：ACactusPlayerCharacter。
  Class Defaults → Player：移動、カメラ、体力、被弾無敵、武器切替。
  Weapon：主武器の間隔・速度・威力・寿命・弾クラス・Pool初期数。
  Weapon / Flamethrower：射程350cm、左右15度、1ダメージ/0.1秒。
  First Person Flame Offset：一人称の銃口位置。初期は右28cm・下18cm。
  Effects：火炎Niagaraと表示Scale・Rotation・Reference Length。
Characters/Enemies/BP_CactusEnemy / BP_CactusRangedEnemy
  Enemy / Health、Chase、Contact、Shooting、Dropsで敵の値を設定する。
  基本体力10。通常敵は接触1ダメージ/秒、射撃敵は900cm以内で1秒に1発。
Characters/Enemies/BP_CactusLargeEnemy
  体力30、速度180cm/秒。NavMeshで追跡しながら700cm以内で1.5秒に1発。
  140cm以内では接触攻撃を優先する。Contactは1ダメージ/秒。
  Enemy / ShootingのStop To Shootはfalse。Health、Chase、Contact、Shootingで調整する。
  L_TestArenaのEnemy_Large_Melee_30HPで確認できる。
Characters/Enemies/BP_CactusBoss
  体力50、移動なし。Enemy / Barrageで発射数・間隔・弾速・威力・寿命・角度を設定する。
  初期値は1.5秒ごとに16発、450cm/秒、威力1、寿命4秒。次の一周は11.25度ずらす。
  Muzzle Heightは足元からの高さ。Initial Fire Delayは出現から最初の発射までの秒数。
  L_TestArenaのEnemy_Boss_Barrage_50HPで確認できる。
Input/IMC_Player / Input/Actions：キー割り当て。
Game/BP_CactusGameMode
  Battery：容量、毎秒消費、更新間隔、ダウン時消費率。
  Screens：画面クラス、Level参照、結果までの拾得待ち時間。
Game/Stages/DA_Stage_Test
  Waves：敵クラス・数・生成間隔・Spawn Batch Size。
  Spawn Batch Sizeは1回に出す数。最大生存数に達すると空きを待つ。
  第1波は通常5体、第2波は通常8体＋射撃2体、第3波は大型1体、第4波はBoss1体。
  第3・4波はSpawn Point Indices=8、Spawn Radius Override=0で中心から出現する。大型は追跡し、Bossは固定される。
  Spawn Radius Overrideは負数でSpawnerの設定を使用、0で指定地点のみ、正数で周囲の範囲を指定する。
Maps/L_Title：起動Level。
Maps/L_WaveArena：4波のアリーナ。
  TP_Wave_Centerが第3・4波の中心生成点。初期位置は(0,0,0)。前2波は元の8地点を使う。
Maps/L_TestArena：個別の動作確認用Level。
  地形を変更した場合はBuild PathsでNavigationを再構築して保存する。
Items/BP_CactusScorePickup / BP_CactusBatteryPickup：得点と電池の補充量。
UI/WBP_Title / WBP_Result / WBP_Failure
  Designerで配置・文字、Class Defaults → Menuで色・動き・音を編集する。
  Energy Core Style / Selection Orb StyleのCore Fillは0で中心透明、1で全体表示。
Weapons/Effects/NS_CactusFlamethrower：火炎の粒子・形・色。
Weapons/Projectiles：通常弾・敵弾のBlueprintと材質。
値の変更後はBlueprintをCompileし、資産とLevelを保存する。

素材
Mannyとアニメーション：Epic Games、UE Third Personテンプレート。
火炎：Epic Games、Niagara Examples Pack。
https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600
NS_CactusFlamethrowerはNS_FireとMI_Flamesを基に調整したもの。
素材の権利は各提供元に帰属する。
