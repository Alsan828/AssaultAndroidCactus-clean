# Assault Android Cactus

UE 5.8.3 / Windows 64bit / C++・Blueprint。
ゲームソース、設定、Blueprint、Level、ゲーム素材、クラス図の最小バックアップです。
主工程の最新 `DA_Stage_Test` を含み、元のクリーン版のテスト除外設定を使用します。

Unreal Engine 5.8.3 と Visual Studio 2022 の C++ ゲーム開発環境を用意します。
`AssaultAndroidCactus.uproject` から Visual Studio project files を生成し、
Development Editor / Win64 をビルドしてから工程を開きます。

```powershell
.\build.ps1 -EngineDir 'UE_5.8のインストール先'
```

ビルド済みファイル、UE本体、キャッシュ、テスト、履歴、重複する `Source.zip` は含めていません。
Niagara Examples Pack はゲームから参照される素材とその依存素材だけを保存しています。
`README.txt` に操作と素材の権利表記があります。同ファイルの ZIP 配布物はこのリポジトリには含みません。
