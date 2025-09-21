#include "Stats.hpp"

#include "game/rdr/Natives.hpp"

namespace YimMenu::Stats
{
	bool IsValid(joaat_t BaseId, joaat_t PermutationId)
	{
		StatId statid{};
		statid.BaseId = BaseId;
		statid.PermutationId = PermutationId;
		return STATS::STAT_ID_IS_VALID(&statid);
	}

	void SetInt(joaat_t BaseId, joaat_t PermutationId, int value)
	{
		StatId statid{};
		statid.BaseId = BaseId;
		statid.PermutationId = PermutationId;
		STATS::STAT_ID_SET_INT(&statid, value, TRUE);
	}

	void IncrementInt(joaat_t BaseId, joaat_t PermutationId, int value)
	{
		StatId statid{};
		statid.BaseId = BaseId;
		statid.PermutationId = PermutationId;
		STATS::_STAT_ID_INCREMENT_INT(&statid, value);
	}

	void SetBool(joaat_t BaseId, joaat_t PermutationId, bool value)
	{
		StatId statid{};
		statid.BaseId = BaseId;
		statid.PermutationId = PermutationId;
		STATS::STAT_ID_SET_BOOL(&statid, value, TRUE);
	}

	void SetFloat(joaat_t BaseId, joaat_t PermutationId, float value)
	{
		StatId statid{};
		statid.BaseId = BaseId;
		statid.PermutationId = PermutationId;
		STATS::STAT_ID_SET_FLOAT(&statid, value, TRUE);
	}

	void IncrementFloat(joaat_t BaseId, joaat_t PermutationId, float value)
	{
		StatId statid{};
		statid.BaseId = BaseId;
		statid.PermutationId = PermutationId;
		STATS::_STAT_ID_INCREMENT_FLOAT(&statid, value);
	}

	void SetDate(joaat_t BaseId, joaat_t PermutationId, Date* value)
	{
		StatId statid{};
		statid.BaseId = BaseId;
		statid.PermutationId = PermutationId;
		STATS::STAT_ID_SET_DATE(&statid, value, TRUE);
	}

	int GetInt(joaat_t BaseId, joaat_t PermutationId)
	{
		int value{};
		StatId statid{};
		statid.BaseId = BaseId;
		statid.PermutationId = PermutationId;
		STATS::STAT_ID_GET_INT(&statid, &value);
		return value;
	}

	bool GetBool(joaat_t BaseId, joaat_t PermutationId)
	{
		BOOL value{};
		StatId statid{};
		statid.BaseId = BaseId;
		statid.PermutationId = PermutationId;
		STATS::STAT_ID_GET_BOOL(&statid, &value);
		return value;
	}

	float GetFloat(joaat_t BaseId, joaat_t PermutationId)
	{
		float value{};
		StatId statid{};
		statid.BaseId = BaseId;
		statid.PermutationId = PermutationId;
		STATS::STAT_ID_GET_FLOAT(&statid, &value);
		return value;
	}

	Date GetDate(joaat_t BaseId, joaat_t PermutationId)
	{
		Date value{};
		StatId statid{};
		statid.BaseId = BaseId;
		statid.PermutationId = PermutationId;
		STATS::STAT_ID_GET_DATE(&statid, &value);
		return value;
	}
}