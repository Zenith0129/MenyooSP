/*
* Ultimate Trainer - Aim Assist (based on Menyoo PC by MAFINS / ItsJustCurtis, GPL v3)
* Story Mode専用。オンライン接続中は動作しない。
*/
#pragma once

#include <string>

namespace UltimateTrainer
{
	namespace AimAssist
	{
		extern bool bEnabled;
		extern bool bAdsOnly;        // true: ADS(エイム)中のみ
		extern bool bShowFov;        // 画面中央のFOV円を表示
		extern bool bShowTarget;     // 現在の対象にマーカーを表示
		extern bool bThroughWalls;   // 壁越しの対象選択を許可

		extern bool bTargetHostile;
		extern bool bTargetNeutral;  // 民間人など
		extern bool bTargetFriendly;
		extern bool bTargetUnknown;  // 関係を判断できない対象
		extern bool bTargetAnimals;

		// 毎フレーム呼ぶ
		void Tick();

		// メニュー用(押すたびに段階を切り替える)
		void CycleFov();
		void CycleStrength();
		void CycleBone();
		void CycleDistance();

		std::string GetFovText();
		std::string GetStrengthText();
		std::string GetBoneText();
		std::string GetDistanceText();
		std::string GetTargetStateText();
	}
}
