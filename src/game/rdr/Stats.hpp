#pragma once
#include "util/Joaat.hpp"

namespace YimMenu::Stats
{
	struct StatId
	{ 
		alignas(8) joaat_t BaseId, PermutationId;
	};

	struct Date
	{
		alignas(8) int Year, Month, Day, Hour, Minute, Second, Millisecond;
	};

	extern bool IsValid(joaat_t BaseId, joaat_t PermutationId);
	extern void SetInt(joaat_t BaseId, joaat_t PermutationId, int value);
	extern void IncrementInt(joaat_t BaseId, joaat_t PermutationId, int value);
	extern void SetBool(joaat_t BaseId, joaat_t PermutationId, bool value);
	extern void SetFloat(joaat_t BaseId, joaat_t PermutationId, float value);
	extern void IncrementFloat(joaat_t BaseId, joaat_t PermutationId, float value);
	extern void SetDate(joaat_t BaseId, joaat_t PermutationId, Date* value);
	extern int GetInt(joaat_t BaseId, joaat_t PermutationId);
	extern bool GetBool(joaat_t BaseId, joaat_t PermutationId);
	extern float GetFloat(joaat_t BaseId, joaat_t PermutationId);
	extern Date GetDate(joaat_t BaseId, joaat_t PermutationId);
}