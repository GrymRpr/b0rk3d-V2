#pragma once
#include <string>

namespace YimMenu::Data
{
	struct TrainData
	{
		uint32_t model;
		std::string name;
	};

	const TrainData g_Trains[] = {
	    {0x8EAC625C, "Appleseed"},
	    {0xF9B038FC, "Bounty Hunter"},
	    {0x0E62D710, "Ghost Train"},
	    {0x3D72571D, "Gunslinger 1"},
	    {0x5AA369CA, "Gunslinger 2"},
	    {0x3D72571D, "Gunslinger 3"},
	    {0x5AA369CA, "Gunslinger 4"},
	    {0x3EDA466D, "Handcart"},
	    {0x767DEB32, "Industry2"},
	    {0x515E31ED, "Prisoner Escort(Passenger Train)"},
	    {0x487B2BE7, "Winter Train"},
	    {0x005E03AD, "Standard Train 1"},
	    {0x0392C83A, "Standard Train 2"}, 
	    {0x0660E567, "Standard Train 3"},
	    {0x0941ADB7, "Train_vip_rescue"},
	    {0x761CE0AD, "Train_camp_resupply"},
	    {0xAE47CA77, "Train_bounty_horde"},
	    {0xD233B18D, "Train_moving_bounty_1"},
	    {0xA3BF0BEB, "Train_kidnapped_buyer"},
	    {0xA3BF0BEB, "Trolley"},

	};
}