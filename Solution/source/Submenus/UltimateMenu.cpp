/*
* Ultimate Trainer main menu (based on Menyoo PC by MAFINS / ItsJustCurtis, GPL v3)
*/
#include "UltimateMenu.h"
#include "MainMenu.h"

#include "..\Misc\UltimateMemMonitor.h"
#include "..\Misc\UltimateAimAssist.h"

#include <string>

namespace sub
{
	void UltimateMenu()
	{
		using namespace UltimateTrainer::MemMonitor;

		AddTitle("Ultimate Trainer");

		AddBreak("Aim Assist");
		AddToggle("Aim Assist", UltimateTrainer::AimAssist::bEnabled);
		AddOptionDescription("Smoothly turns the camera toward the target closest to the screen centre. Story Mode only. Needs a weapon in hand.");
		AddToggle("ADS Only", UltimateTrainer::AimAssist::bAdsOnly);
		AddToggle("Show FOV Circle", UltimateTrainer::AimAssist::bShowFov);
		AddToggle("Show Target Marker", UltimateTrainer::AimAssist::bShowTarget);
		AddToggle("Through Walls", UltimateTrainer::AimAssist::bThroughWalls);
		AddOptionDescription("Allows targets that are not directly visible.");
		{
			bool cycFov = false, cycStr = false, cycBone = false, cycDist = false;
			AddOption(std::string("FOV Size: ") + UltimateTrainer::AimAssist::GetFovText(), cycFov);
			if (cycFov) UltimateTrainer::AimAssist::CycleFov();
			AddOption(std::string("Strength: ") + UltimateTrainer::AimAssist::GetStrengthText(), cycStr);
			if (cycStr) UltimateTrainer::AimAssist::CycleStrength();
			AddOption(std::string("Target Part: ") + UltimateTrainer::AimAssist::GetBoneText(), cycBone);
			if (cycBone) UltimateTrainer::AimAssist::CycleBone();
			AddOption(std::string("Max Distance: ") + UltimateTrainer::AimAssist::GetDistanceText(), cycDist);
			if (cycDist) UltimateTrainer::AimAssist::CycleDistance();
		}
		AddOption(std::string("State: ") + UltimateTrainer::AimAssist::GetTargetStateText(), null);

		AddBreak("Aim Assist Targets");
		AddToggle("Target Hostile", UltimateTrainer::AimAssist::bTargetHostile);
		AddToggle("Target Neutral / Civilians", UltimateTrainer::AimAssist::bTargetNeutral);
		AddToggle("Target Friendly", UltimateTrainer::AimAssist::bTargetFriendly);
		AddToggle("Target Unknown", UltimateTrainer::AimAssist::bTargetUnknown);
		AddOptionDescription("Used when the game gives no clear relationship between the target and you.");
		AddToggle("Target Animals", UltimateTrainer::AimAssist::bTargetAnimals);

		AddBreak("Memory Monitor");
		AddToggle("Display Memory Info", bDisplay);
		AddOptionDescription("Shows GTA V and system memory on the HUD.");
		AddToggle("Memory Warnings", bWarnings);
		AddOptionDescription("Shows a warning when free memory gets low. It never stops anything by itself.");

		AddBreak("Current Status");
		AddOption(std::string("Level: ") + GetLevelName(), null);
		AddOption(std::string("GTA V: ") + GetProcessText(), null);
		AddOption(std::string("System free / total: ") + GetSystemText(), null);
		AddOption(std::string("Growth (5 s): ") + GetGrowthText(), null);
		AddOption(std::string("History: ") + GetHistoryText(), null);
	}
}

#include "..\Menu\submenu_switch.h"
#include "..\Menu\submenu_enum.h"
REGISTER_SUBMENU(ULTIMATE_MAIN, sub::UltimateMenu)
