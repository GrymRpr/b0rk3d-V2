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
		{0xAE47CA77, "Bounty Horde"},
		{0x761CE0AD, "Camp Resupply"},
		{0x41436136, "CU Railroad Passenger Train/Long"},
		{0x592A5CD0, "CU Railroad Passenger Train/Short"},
		{0x3260CE89, "Engine"},
		{0x0E62D710, "Ghost Train"},
		{0x5AA369CA, "Gunslinger 4"},
		{0x3EDA466D, "Handcart"},
		{0xA3BF0BEB, "Kidnapped Buyer"},
		{0xAA3E691E, "Lannahechee and Midland Railroad Cargo Train 1"},
		{0x1EF82A51, "Lannahechee and Midland Railroad Cargo Train 2"},
		{0xF9B038FC, "Lannahechee and Midland Railroad Cargo Train 3"},
		{0x0CCC2F70, "Lannahechee and Midland Railroad Coal Train"},
		{0xD233B18D, "Moving Bounty"},
		{0x2D3645FA, "PU Rail Passenger Train"},
		{0xDA2EDE2F, "PU Rail Cargo Train"},
		{0x515E31ED, "Prisoner Escort"},
		{0x487B2BE7, "Private Cornwall Train/long"},
		{0xDC9DD041, "Private Cornwall Train/short"},
		{0x3ADC4DA9, "Sleeper Train 1"},
		{0xCD2C7CA1, "Sleeper Train 2"},
		{0x0941ADB7, "S&E Cargo Train 2"},
		{0x0D03C58D, "S&E Cargo Train 1"},
		{0x6CC26E27, "S&E Cargo Train 3"},
		{0x1C043595, "S&E Passenger Train"},
		{0x3D72571D, "S&E Passenger Train with Bar"},
		{0x2D1A6F0C, "Standard RDO Train"},
		{0xBF69518F, "Trolley (Cornwall City Railway)"},
		{0x09B679D6, "Trolley (PU Railroad)"},
		{0x25E5D8FF, "United States Army Train/Long"},
		{0xC1F1DD80, "United States Army Train/Short"}
	};
}