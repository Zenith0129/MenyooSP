/*
* Ultimate Trainer - Memory Monitor (based on Menyoo PC by MAFINS / ItsJustCurtis, GPL v3)
*
* 測定するもの:
*  - GTA Vプロセスのメモリ(ワーキングセットとプライベート使用量)
*  - Windowsの物理メモリ(合計と空き)
* 測定できない値は推測せず、表示しない。GTA V内部のメモリ管理情報は、この版では取得しない。
* プロセスとシステムの値は別々に表示し、合算しない(二重計上を避ける)。
*/
#include "UltimateMemMonitor.h"

#include <Windows.h>
#include <Psapi.h>

#include <cstdio>
#include <deque>

#pragma comment(lib, "psapi.lib")

namespace UltimateTrainer
{
	namespace MemMonitor
	{
		bool bDisplay = false;
		bool bWarnings = true;

		namespace
		{
			struct Sample
			{
				ULONGLONG tick;
				ULONGLONG procWorkingSet;
				ULONGLONG procPrivate;
				ULONGLONG sysAvail;
				ULONGLONG sysTotal;
			};

			constexpr ULONGLONG SAMPLE_INTERVAL_MS = 200;   // 毎秒5回
			constexpr size_t MAX_SAMPLES = 3000;            // 約10分
			constexpr size_t GROWTH_WINDOW = 25;            // 約5秒

			std::deque<Sample> g_samples;
			ULONGLONG g_lastTick = 0;
			Level g_level = Level::Normal;

			constexpr double MB = 1024.0 * 1024.0;

			// 直近約5秒の増加速度(MB/秒)。サンプル不足の場合は false を返す。
			bool GrowthMBps(double& out)
			{
				if (g_samples.size() <= GROWTH_WINDOW)
					return false;
				const Sample& newest = g_samples.back();
				const Sample& older = g_samples[g_samples.size() - 1 - GROWTH_WINDOW];
				const double seconds = static_cast<double>(newest.tick - older.tick) / 1000.0;
				if (seconds <= 0.0)
					return false;
				out = (static_cast<double>(newest.procPrivate) - static_cast<double>(older.procPrivate)) / MB / seconds;
				return true;
			}

			Level Evaluate(const Sample& s)
			{
				const double availMB = static_cast<double>(s.sysAvail) / MB;
				const double availPct = s.sysTotal ? (static_cast<double>(s.sysAvail) * 100.0 / static_cast<double>(s.sysTotal)) : 100.0;

				if (availPct < 3.0 || availMB < 512.0)
					return Level::Emergency;
				if (availPct < 6.0 || availMB < 1024.0)
					return Level::Danger;

				double growth = 0.0;
				const bool hasGrowth = GrowthMBps(growth);
				if (availPct < 12.0 || availMB < 2048.0 || (hasGrowth && growth > 150.0))
					return Level::Caution;

				return Level::Normal;
			}
		}

		void Tick()
		{
			const ULONGLONG now = GetTickCount64();
			if (g_lastTick != 0 && (now - g_lastTick) < SAMPLE_INTERVAL_MS)
				return;
			g_lastTick = now;

			PROCESS_MEMORY_COUNTERS_EX pmc = {};
			pmc.cb = sizeof(pmc);
			if (!GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc)))
				return; // 測定できなかった場合は、サンプルを追加しない(推測値を入れない)

			MEMORYSTATUSEX ms = {};
			ms.dwLength = sizeof(ms);
			if (!GlobalMemoryStatusEx(&ms))
				return;

			Sample s;
			s.tick = now;
			s.procWorkingSet = static_cast<ULONGLONG>(pmc.WorkingSetSize);
			s.procPrivate = static_cast<ULONGLONG>(pmc.PrivateUsage);
			s.sysAvail = static_cast<ULONGLONG>(ms.ullAvailPhys);
			s.sysTotal = static_cast<ULONGLONG>(ms.ullTotalPhys);

			g_samples.push_back(s);
			while (g_samples.size() > MAX_SAMPLES)
				g_samples.pop_front();

			g_level = Evaluate(s);
		}

		bool HasSample()
		{
			return !g_samples.empty();
		}

		Level GetLevel()
		{
			return g_level;
		}

		const char* GetLevelName()
		{
			switch (g_level)
			{
			case Level::Caution: return "Caution";
			case Level::Danger: return "Danger";
			case Level::Emergency: return "Emergency";
			default: return "Normal";
			}
		}

		std::string FormatBytes(unsigned long long bytes)
		{
			const double mb = static_cast<double>(bytes) / MB;
			char buf[64];
			if (mb < 1024.0)
				std::snprintf(buf, sizeof(buf), "%.0f MB", mb);
			else
				std::snprintf(buf, sizeof(buf), "%.2f GB", mb / 1024.0);
			return std::string(buf);
		}

		std::string GetProcessText()
		{
			if (g_samples.empty())
				return "N/A";
			const Sample& s = g_samples.back();
			return FormatBytes(s.procWorkingSet) + " (private " + FormatBytes(s.procPrivate) + ")";
		}

		std::string GetSystemText()
		{
			if (g_samples.empty())
				return "N/A";
			const Sample& s = g_samples.back();
			return FormatBytes(s.sysAvail) + " / " + FormatBytes(s.sysTotal);
		}

		std::string GetGrowthText()
		{
			double growth = 0.0;
			if (!GrowthMBps(growth))
				return "--";
			char buf[64];
			std::snprintf(buf, sizeof(buf), "%+.1f MB/s", growth);
			return std::string(buf);
		}

		std::string GetHistoryText()
		{
			char buf[96];
			std::snprintf(buf, sizeof(buf), "%u / %u samples", static_cast<unsigned>(g_samples.size()), static_cast<unsigned>(MAX_SAMPLES));
			return std::string(buf);
		}
	}
}
