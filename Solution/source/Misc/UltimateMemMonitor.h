/*
* Ultimate Trainer - Memory Monitor (based on Menyoo PC by MAFINS / ItsJustCurtis, GPL v3)
*/
#pragma once

#include <string>

namespace UltimateTrainer
{
	namespace MemMonitor
	{
		enum class Level { Normal = 0, Caution, Danger, Emergency };

		extern bool bDisplay;   // HUDに数値を表示する
		extern bool bWarnings;  // 警告(Caution/Danger/Emergency)を表示する

		// 毎フレーム呼ぶ。内部で約200msごと(毎秒5回)に測定する。
		void Tick();

		bool HasSample();
		Level GetLevel();
		const char* GetLevelName();

		// 1,024MB未満はMB、それ以上はGBで表示する
		std::string FormatBytes(unsigned long long bytes);

		std::string GetProcessText();  // GTA Vプロセスのメモリ
		std::string GetSystemText();   // Windows全体の空きメモリ
		std::string GetGrowthText();   // 直近約5秒のメモリ増加速度
		std::string GetHistoryText();  // 履歴の保持状況(最大3,000サンプル、約10分)
	}
}
