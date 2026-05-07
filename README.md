# IoT Monitoring Camera & LINE Bot System
XIAO ESP32S3 Sense を使用した、日時指定検索・グループ対応型 見守りカメラシステム

## 🚀 概要
外出先から自宅や現場の様子を確認するためのIoTシステムです。
単なる「最新画像の取得」だけでなく、過去の膨大なデータから「特定の日時」をピンポイントで検索して取得できるのが特徴です。

## 🎬 デモ (Demo)
<p align="center">
  <img src="https://github.com/user-attachments/assets/4f23a17a-5f8e-489f-ae21-2d43fe8ac3a7" width="300" alt="Bot Demo">
  <br>
  <i>LINEトーク画面からの時刻指定検索と画像取得の様子</i>
</p>

## ✨ 主な機能
- **自動撮影・クラウド保存**: マイコンが定期的に撮影し、AWSクラウドへ自動アップロード。
- **LINE連携**: 使い慣れたLINEからコマンドひとつで画像をリクエスト。
- **高度な検索機能**: 
  - `get` 送信で最新画像を取得。
  - 日時（例：`2026-03-23T07:00`）を打つことで、その時刻以降の画像を即座に検索。
- **グループトーク対応**: LINEの仕様制限（グループでのリッチメニュー非表示）を、テキスト解析ロジックにより克服。

## 🛠 技術スタック (Tech Stack)
### Hardware
- **XIAO ESP32S3 Sense** (Camera, Wi-Fi)
- Arduino IDE (C++)

### Backend (AWS Serverless)
- **AWS Lambda** (Python 3.x)
- **Amazon API Gateway**
- **Amazon DynamoDB** (NoSQL Database)

### Frontend / API
- **LINE Messaging API**

## 💡 技術的な工夫・苦労した点
### 1. グループチャットにおけるUXの向上
LINEの仕様上、グループトークでは「リッチメニュー（固定ボタン）」が表示されません。
本システムでは、ユーザーが入力した文字列をLambda側で動的に判定し、日時フォーマットであればクエリパラメータとして処理するロジックを実装することで、グループ内でも利便性を損なわない設計にしました。

### 2. 高速な時系列データ検索
DynamoDBのソートキーに `ISO 8601` 形式のタイムスタンプを採用。
`begins_with` や `gte`（以降）といった条件を使い分けることで、秒単位まで指定しなくても、ユーザーが意図した時間帯の画像を高速に抽出できるように最適化しました。

## 🔧 セットアップ (Optional)
1. `CameraData` テーブルを DynamoDB に作成（Partition Key: `device_id`, Sort Key: `timestamp`）。
2. Lambdaに関数を作成し、環境変数を設定。
3. LINE DevelopersでWebhookを設定。
