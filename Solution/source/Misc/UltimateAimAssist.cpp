/*
* Ultimate Trainer - Aim Assist (based on Menyoo PC by MAFINS / ItsJustCurtis, GPL v3)
*
* 画面中央のFOV円の中にいるNPCから、中心に最も近い対象を選び、
* ゲームカメラを対象へ少しずつ向ける(カメラ追従方式)。
* - Story Mode専用。オンライン接続中は何もしない。
* - メニューが開いている間、死亡中、車両に乗っている間は何もしない。
* - 敵味方の判定はGTA Vの関係データを使い、判断できない場合は「不明」として扱う。
*/
#include "UltimateAimAssist.h"

#include "..\macros.h"
#include "..\Natives\natives2.h"
#include "..\Memory\GTAmemory.h"
#include "..\Menu\Menu.h"
#include "..\Menu\submenu_enum.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace UltimateTrainer
{
	namespace AimAssist
	{
		bool bEnabled = false;
		bool bAdsOnly = true;
		bool bShowFov = true;
		bool bShowTarget = true;
		bool bThroughWalls = false;

		bool bTargetHostile = true;
		bool bTargetNeutral = false;
		bool bTargetFriendly = false;
		bool bTargetUnknown = false;
		bool bTargetAnimals = false;

		namespace
		{
			constexpr double kPi = 3.14159265358979323846;
			constexpr float kRadToDeg = static_cast<float>(180.0 / kPi);

			// FOV円の半径(画面の高さに対する割合)
			constexpr float kFovSteps[] = { 0.04f, 0.08f, 0.12f, 0.18f, 0.25f, 0.35f };
			// 追従の強さ(1フレームあたりの割合。60FPS基準)
			constexpr float kStrengthSteps[] = { 0.05f, 0.12f, 0.25f, 0.45f, 0.70f, 1.00f };
			// 対象にする最大距離(m)
			constexpr float kDistanceSteps[] = { 30.0f, 60.0f, 100.0f, 150.0f };

			int g_fovIdx = 2;
			int g_strengthIdx = 2;
			int g_distIdx = 1;
			int g_boneIdx = 0; // 0:頭 1:胴体 2:腕 3:脚

			constexpr int kBoneIds[] = { 31086, 24818, 40269, 51826 }; // SKEL_Head, SKEL_Spine3, SKEL_R_UpperArm, SKEL_R_Thigh
			const char* const kBoneNames[] = { "Head", "Torso", "Arms", "Legs" };

			Ped g_target = 0;
			bool g_active = false;

			template <typename T, size_t N>
			constexpr int Count(const T(&)[N]) { return static_cast<int>(N); }

			float WrapAngle(float a)
			{
				while (a > 180.0f) a -= 360.0f;
				while (a < -180.0f) a += 360.0f;
				return a;
			}

			// 0:敵 1:中立 2:味方 3:不明 4:動物
			int Classify(Ped me, Ped ped)
			{
				if (!IS_PED_HUMAN(ped))
					return 4;

				if (IS_PED_IN_COMBAT(ped, me))
					return 0;

				const int rel = GET_RELATIONSHIP_BETWEEN_PEDS(ped, me);
				if (rel == 4 || rel == 5) return 0;
				if (rel == 3) return 1;
				if (rel >= 0 && rel <= 2) return 2;
				return 3;
			}

			bool CategoryAllowed(int category)
			{
				switch (category)
				{
				case 0: return bTargetHostile;
				case 1: return bTargetNeutral;
				case 2: return bTargetFriendly;
				case 3: return bTargetUnknown;
				case 4: return bTargetAnimals;
				}
				return false;
			}

			// 対象が有効で、FOV円の中にいるなら、中心からの距離(画面の高さ基準)を返す。無効なら負の値。
			float ScreenDistanceIfValid(Ped me, Ped ped, float fov, float aspect, float& outX, float& outY)
			{
				if (ped == me || !DOES_ENTITY_EXIST(ped) || IS_PED_DEAD_OR_DYING(ped, 1))
					return -1.0f;
				if (!CategoryAllowed(Classify(me, ped)))
					return -1.0f;

				const Vector3 b = GET_PED_BONE_COORDS(ped, kBoneIds[g_boneIdx], 0.0f, 0.0f, 0.0f);
				float sx = 0.0f, sy = 0.0f;
				if (!GET_SCREEN_COORD_FROM_WORLD_COORD(b.x, b.y, b.z, &sx, &sy))
					return -1.0f; // 画面外

				const float dx = (sx - 0.5f) * aspect;
				const float dy = sy - 0.5f;
				const float d = std::sqrt(dx * dx + dy * dy);
				if (d > fov)
					return -1.0f;

				if (!bThroughWalls && !HAS_ENTITY_CLEAR_LOS_TO_ENTITY(me, ped, 17))
					return -1.0f;

				outX = sx;
				outY = sy;
				return d;
			}

			void DrawFovCircle(float fov, float aspect)
			{
				constexpr int kPoints = 64;
				const float s = 0.0022f;
				for (int i = 0; i < kPoints; i++)
				{
					const float a = static_cast<float>(i) * static_cast<float>(2.0 * kPi) / static_cast<float>(kPoints);
					const float x = 0.5f + (fov * std::cos(a)) / aspect;
					const float y = 0.5f + fov * std::sin(a);
					DRAW_RECT(x, y, s / aspect, s, 255, 255, 255, 140);
				}
			}

			void DrawTargetMarker(float sx, float sy, float aspect)
			{
				const float s = 0.008f;
				DRAW_RECT(sx, sy, s / aspect, s, 255, 60, 60, 220);
			}
		}

		void Tick()
		{
			g_active = false;

			if (!bEnabled)
			{
				g_target = 0;
				return;
			}
			// Story Mode専用: オンライン接続中は動作しない
			if (NETWORK_IS_GAME_IN_PROGRESS() || NETWORK_IS_SESSION_STARTED())
			{
				g_target = 0;
				return;
			}
			// メニューが開いている間は動作しない
			if (Menu::activeSubmenu != SUB::CLOSED)
			{
				g_target = 0;
				return;
			}

			const Ped me = PLAYER_PED_ID();
			if (!DOES_ENTITY_EXIST(me) || IS_PED_DEAD_OR_DYING(me, 1) || IS_PED_IN_ANY_VEHICLE(me, 0) || !IS_PED_ARMED(me, 4))
			{
				g_target = 0;
				return;
			}

			const bool ads = IS_DISABLED_CONTROL_PRESSED(0, 25) || IS_CONTROL_PRESSED(0, 25); // INPUT_AIM
			if (bAdsOnly && !ads)
			{
				g_target = 0;
				return;
			}
			g_active = true;

			const float aspect = GET_ASPECT_RATIO(false);
			const float fov = kFovSteps[g_fovIdx];

			if (bShowFov)
				DrawFovCircle(fov, aspect);

			float tx = 0.0f, ty = 0.0f;

			// 現在の対象がまだ有効なら維持し、無効になったら中心に最も近い対象へ切り替える
			if (g_target != 0 && ScreenDistanceIfValid(me, g_target, fov, aspect, tx, ty) < 0.0f)
				g_target = 0;

			if (g_target == 0)
			{
				std::vector<Entity> peds;
				const Vector3 myPos = GET_ENTITY_COORDS(me, 1);
				GTAmemory::GetPedHandles(peds, myPos, kDistanceSteps[g_distIdx]);

				float best = 1000.0f;
				for (const Entity e : peds)
				{
					float x = 0.0f, y = 0.0f;
					const float d = ScreenDistanceIfValid(me, e, fov, aspect, x, y);
					if (d >= 0.0f && d < best)
					{
						best = d;
						g_target = e;
						tx = x;
						ty = y;
					}
				}
			}

			if (g_target == 0)
				return;

			if (bShowTarget)
				DrawTargetMarker(tx, ty, aspect);

			// カメラを対象の部位へ向ける
			const Vector3 b = GET_PED_BONE_COORDS(g_target, kBoneIds[g_boneIdx], 0.0f, 0.0f, 0.0f);
			const Vector3 camPos = GET_GAMEPLAY_CAM_COORD();
			const Vector3 camRot = GET_GAMEPLAY_CAM_ROT(2);

			const float dx = b.x - camPos.x;
			const float dy = b.y - camPos.y;
			const float dz = b.z - camPos.z;
			const float horiz = std::sqrt(dx * dx + dy * dy);
			if (horiz < 0.01f)
				return;

			const float targetYaw = -std::atan2(dx, dy) * kRadToDeg;
			const float targetPitch = std::atan2(dz, horiz) * kRadToDeg;

			const float dYaw = WrapAngle(targetYaw - camRot.z);
			const float dPitch = targetPitch - camRot.x;

			// フレーム時間に依存しない追従量
			const float dt = GET_FRAME_TIME();
			const float strength = kStrengthSteps[g_strengthIdx];
			const float f = 1.0f - std::pow(1.0f - strength, dt * 60.0f);

			SET_GAMEPLAY_CAM_RELATIVE_HEADING(GET_GAMEPLAY_CAM_RELATIVE_HEADING() + dYaw * f);
			SET_GAMEPLAY_CAM_RELATIVE_PITCH(GET_GAMEPLAY_CAM_RELATIVE_PITCH() + dPitch * f, 1.0f);
		}

		void CycleFov() { g_fovIdx = (g_fovIdx + 1) % Count(kFovSteps); }
		void CycleStrength() { g_strengthIdx = (g_strengthIdx + 1) % Count(kStrengthSteps); }
		void CycleBone() { g_boneIdx = (g_boneIdx + 1) % Count(kBoneIds); g_target = 0; }
		void CycleDistance() { g_distIdx = (g_distIdx + 1) % Count(kDistanceSteps); }

		std::string GetFovText()
		{
			char buf[32];
			std::snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(kFovSteps[g_fovIdx] * 100.0f + 0.5f));
			return std::string(buf);
		}

		std::string GetStrengthText()
		{
			char buf[32];
			std::snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(kStrengthSteps[g_strengthIdx] * 100.0f + 0.5f));
			return std::string(buf);
		}

		std::string GetBoneText()
		{
			return std::string(kBoneNames[g_boneIdx]);
		}

		std::string GetDistanceText()
		{
			char buf[32];
			std::snprintf(buf, sizeof(buf), "%d m", static_cast<int>(kDistanceSteps[g_distIdx]));
			return std::string(buf);
		}

		std::string GetTargetStateText()
		{
			if (!bEnabled) return "Off";
			if (!g_active) return "Standby";
			return g_target != 0 ? "Locked" : "Searching";
		}
	}
}
