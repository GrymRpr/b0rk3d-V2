#include "AnimationDict.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <cstdlib>

using json = nlohmann::json;

namespace YimMenu
{
	std::filesystem::path AnimationDict::GetAnimationDictFilePath()
	{
		const char* appdata = std::getenv("APPDATA");
		if (!appdata) {
			return std::filesystem::current_path() / "AniDict.json";
		}
		std::filesystem::path dir = std::filesystem::path(appdata) / "Terminus-b0rk3d";
		std::filesystem::create_directories(dir); // ensure exists
		return dir / "AniDict.json";
	}

	static std::filesystem::path GetAniDictTogglePath()
	{
		const char* appdata = std::getenv("APPDATA");
		if (!appdata) {
			return std::filesystem::current_path() / "AniDict.toggle";
		}
		std::filesystem::path dir = std::filesystem::path(appdata) / "Terminus-b0rk3d";
		std::filesystem::create_directories(dir);
		return dir / "AniDict.toggle";
	}

	AnimationDict::AnimationDict()
	{
		LoadAniDictToggleImpl();
		FetchAnimationDictImpl();
	}

	bool AnimationDict::FetchAnimationDictImpl()
	{
		std::scoped_lock lock(m_mutex);
		m_lastError.clear();
		m_AllAnimations.clear();
		auto file = GetAnimationDictFilePath();
		if (!std::filesystem::exists(file))
			return true; // No file = empty dict, that's fine

		std::ifstream in(file);
		if (!in.is_open())
		{
			m_lastError = "Failed to open AniDict.json for reading";
			return false;
		}
		try
		{
			json j;
			in >> j;
			for (auto it = j.begin(); it != j.end(); ++it)
			{
				if (!it.value().is_array()) continue;
				std::vector<std::string> anims;
				for (const auto& anim : it.value())
				{
					if (!anim.is_string()) continue;
					anims.push_back(anim.get<std::string>());
				}
				m_AllAnimations[it.key()] = anims;
			}
		}
		catch (const std::exception& ex)
		{
			m_lastError = std::string("Failed to load AniDict.json: ") + ex.what();
			m_AllAnimations.clear();
			return false;
		}
		return true;
	}

	bool AnimationDict::SaveAnimationDictImpl()
	{
		std::scoped_lock lock(m_mutex);
		m_lastError.clear();
		auto file = GetAnimationDictFilePath();
		json j;
		for (const auto& [dict, anims] : m_AllAnimations)
			j[dict] = anims;
		if (j.empty())
			j = json::object(); // Always output {}

		std::ofstream out(file, std::ios::trunc);
		if (!out.is_open())
		{
			m_lastError = "Failed to open AniDict.json for writing";
			return false;
		}
		try
		{
			out << j.dump(2) << std::endl;
		}
		catch (const std::exception& ex)
		{
			m_lastError = std::string("Failed to write AniDict.json: ") + ex.what();
			return false;
		}
		return true;
	}

	bool AnimationDict::SaveAniDictToggleImpl()
	{
		auto file = GetAniDictTogglePath();
		std::ofstream out(file, std::ios::trunc);
		if (!out.is_open())
			return false;
		out << (m_AniDictToggle ? "1" : "0");
		return true;
	}

	bool AnimationDict::LoadAniDictToggleImpl()
	{
		auto file = GetAniDictTogglePath();
		if (!std::filesystem::exists(file)) {
			m_AniDictToggle = false;
			return false;
		}
		std::ifstream in(file);
		if (!in.is_open())
		{
			m_AniDictToggle = false;
			return false;
		}
		char c;
		in >> c;
		m_AniDictToggle = (c == '1');
		return m_AniDictToggle;
	}

	bool AnimationDict::AniDictToggle()
	{
		return GetInstance().m_AniDictToggle;
	}

	void AnimationDict::SetAniDictToggle(bool state)
	{
		auto& instance = GetInstance();
		instance.m_AniDictToggle = state;
	}

	bool AnimationDict::SaveAniDictToggle()
	{
		return GetInstance().SaveAniDictToggleImpl();
	}

	bool AnimationDict::LoadAniDictToggle()
	{
		return GetInstance().LoadAniDictToggleImpl();
	}
}