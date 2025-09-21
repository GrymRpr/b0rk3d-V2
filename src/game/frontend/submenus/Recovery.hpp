#pragma once
#include "core/frontend/manager/UIManager.hpp"
#include "util/Joaat.hpp"

namespace YimMenu::Submenus
{
	struct StatId
	{ 
		alignas(8) joaat_t BaseId, PermutationId;
	};

	struct Date
	{
		alignas(8) int Year, Month, Day, Hour, Minute, Second, Millisecond;
	};

	bool IsValid(joaat_t BaseId, joaat_t PermutationId);
	void SetInt(joaat_t BaseId, joaat_t PermutationId, int value);
	void IncrementInt(joaat_t BaseId, joaat_t PermutationId, int value);
	void SetBool(joaat_t BaseId, joaat_t PermutationId, bool value);
	void SetFloat(joaat_t BaseId, joaat_t PermutationId, float value);
	void IncrementFloat(joaat_t BaseId, joaat_t PermutationId, float value);
	void SetDate(joaat_t BaseId, joaat_t PermutationId, Date* value);
	int GetInt(joaat_t BaseId, joaat_t PermutationId);
	bool GetBool(joaat_t BaseId, joaat_t PermutationId);
	float GetFloat(joaat_t BaseId, joaat_t PermutationId);
	Date GetDate(joaat_t BaseId, joaat_t PermutationId);

	class Recovery : public Submenu
	{
	public:
		Recovery();
	};
}