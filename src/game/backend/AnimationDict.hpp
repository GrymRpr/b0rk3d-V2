#pragma once
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <mutex>

namespace YimMenu
{
	struct AnimationEntry
	{
		std::string dictionary;
		std::vector<std::string> anims;

		bool operator==(const AnimationEntry& other) const
		{
			return dictionary == other.dictionary && anims == other.anims;
		}
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AnimationEntry, dictionary, anims);

	class AnimationDict
	{
	private:
		std::map<std::string, std::vector<std::string>> m_AllAnimations;
		std::filesystem::path GetAnimationDictFilePath();
		bool m_AniDictToggle = false;
		std::recursive_mutex m_mutex;
		std::string m_lastError;

		static AnimationDict& GetInstance()
		{
			static AnimationDict instance{};
			return instance;
		}

		AnimationDict();

		bool FetchAnimationDictImpl();
		bool SaveAnimationDictImpl();
		bool SaveAniDictToggleImpl();
		bool LoadAniDictToggleImpl();

	public:
		static std::map<std::string, std::vector<std::string>>& GetAllAnimations()
		{
			return GetInstance().m_AllAnimations;
		}

		static bool FetchAnimationDict()
		{
			return GetInstance().FetchAnimationDictImpl();
		}

		static bool SaveAnimationDict()
		{
			return GetInstance().SaveAnimationDictImpl();
		}

		static bool AniDictToggle();
		static void SetAniDictToggle(bool state);

		static bool SaveAniDictToggle();
		static bool LoadAniDictToggle();

		static const std::string& GetLastError()
		{
			return GetInstance().m_lastError;
		}
	};
}