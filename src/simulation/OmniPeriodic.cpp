#include "OmniPeriodic.h"

#include "ElementCommon.h"

#include <algorithm>

namespace
{
constexpr int PeriodicEventsPerFrame = 1024;

struct ReactionBudget
{
	Simulation *simulation = nullptr;
	int tick = -1;
	int remaining = PeriodicEventsPerFrame;
};

thread_local ReactionBudget reactionBudget;

struct NobleGasProperties
{
	int dischargeChance;
	int glowLife;
	int dischargeHeat;
	int photonMask;
};

struct AlkaliProperties
{
	int waterHeat;
	float pressure;
	int fireChance;
	float oxygenThreshold;
	float boilingPoint;
};

struct AlkalineEarthProperties
{
	int waterHeat;
	float pressure;
	int fireChance;
	float waterThreshold;
	float acidThreshold;
	float oxygenThreshold;
	float boilingPoint;
	unsigned int flameColour;
};

struct BoronGroupProperties
{
	float acidThreshold;
	float oxygenThreshold;
	float boilingPoint;
	int reactionHeat;
	float pressure;
	unsigned int flameColour;
};

struct CarbonGroupProperties
{
	float acidThreshold;
	float oxygenThreshold;
	float boilingPoint;
	int reactionHeat;
	float pressure;
	int oxideProduct;
	unsigned int flameColour;
};

struct NitrogenGroupProperties
{
	float acidThreshold;
	float oxygenThreshold;
	float boilingPoint;
	int reactionHeat;
	float pressure;
	int oxideProduct;
	unsigned int flameColour;
};

struct OxygenGroupProperties
{
	float oxygenThreshold;
	float boilingPoint;
	int reactionHeat;
	float pressure;
	int oxideProduct;
	unsigned int flameColour;
};

struct HalogenProperties
{
	float hydrogenThreshold;
	float metalThreshold;
	float disinfectionThreshold;
	float boilingPoint;
	int reactionHeat;
	float pressure;
	unsigned int vapourColour;
};

struct FirstTransitionProperties
{
	float acidThreshold;
	float oxygenThreshold;
	float boilingPoint;
	int reactionHeat;
	float pressure;
	unsigned int flameColour;
};

struct SecondTransitionProperties
{
	float acidThreshold;
	float oxygenThreshold;
	float boilingPoint;
	int reactionHeat;
	float pressure;
	unsigned int flameColour;
};

struct ThirdTransitionProperties
{
	float acidThreshold;
	float oxygenThreshold;
	float boilingPoint;
	int reactionHeat;
	float pressure;
	unsigned int flameColour;
};

struct LanthanideProperties
{
	float acidThreshold;
	float oxygenThreshold;
	float boilingPoint;
	int reactionHeat;
	float pressure;
	unsigned int emissionColour;
};

struct ActinideProperties
{
	float acidThreshold;
	float oxygenThreshold;
	float boilingPoint;
	int reactionHeat;
	int decayProduct;
	int captureProduct;
	int lifeMin;
	int lifeMax;
	float decayHeat;
	float captureHeat;
	float pressure;
	unsigned int emissionColour;
};

struct SuperheavyProperties
{
	float acidThreshold;
	float oxygenThreshold;
	float boilingPoint;
	int reactionHeat;
	int decayProduct;
	int captureProduct;
	int lifeMin;
	int lifeMax;
	float decayHeat;
	float captureHeat;
	float synthesisThreshold;
	float pressure;
	unsigned int emissionColour;
	int radiationType;
};

bool ConsumeEvent(Simulation *sim)
{
	if (reactionBudget.simulation != sim || reactionBudget.tick != sim->currentTick)
	{
		reactionBudget = { sim, sim->currentTick, PeriodicEventsPerFrame };
	}
	if (reactionBudget.remaining <= 0)
	{
		return false;
	}
	--reactionBudget.remaining;
	sim->RecordOmniEvent();
	return true;
}

NobleGasProperties PropertiesFor(int type)
{
	switch (type)
	{
	case PT_HE:
		return { 12, 7, 8, 0x03F00000 };
	case PT_NE:
		return { 6, 9, 18, 0x0007C000 };
	case PT_AR:
		return { 8, 8, 14, 0x001F8000 };
	case PT_KR:
		return { 7, 9, 20, 0x000FF000 };
	case PT_XE:
		return { 2, 12, 45, 0x03FC0000 };
	case PT_RN:
		return { 5, 10, 30, 0x0003FFF0 };
	case PT_OG:
		return { 3, 12, 65, 0x03F03F00 };
	default:
		return { 16, 6, 8, 0x03FFFFFF };
	}
}

AlkaliProperties AlkaliPropertiesFor(int type)
{
	switch (type)
	{
	case PT_NA:
		return { 120, 0.8f, 4, 460.0f, 1156.0f };
	case PT_K:
		return { 220, 1.8f, 2, 410.0f, 1032.0f };
	case PT_CS:
		return { 380, 4.0f, 1, 360.0f, 944.0f };
	case PT_FR:
		return { 560, 7.0f, 1, 330.0f, 950.0f };
	default:
		return { 100, 0.5f, 6, 500.0f, 1200.0f };
	}
}

AlkalineEarthProperties AlkalineEarthPropertiesFor(int type)
{
	switch (type)
	{
	case PT_BE:
		return { 70, 0.25f, 0, 900.0f, 330.0f, 900.0f, 2742.0f, 0xFFE8F4FF };
	case PT_MAGN:
		return { 150, 0.6f, 8, 650.0f, 293.0f, MAX_TEMP, 1363.0f, 0xFFFFFFFF };
	case PT_CA:
		return { 170, 1.0f, 8, 273.0f, 273.0f, 650.0f, 1757.0f, 0xFFFF8A35 };
	case PT_SR:
		return { 250, 2.0f, 4, 273.0f, 273.0f, 580.0f, 1655.0f, 0xFFFF3030 };
	case PT_BA:
		return { 340, 3.5f, 2, 273.0f, 273.0f, 520.0f, 1500.0f, 0xFF66FF66 };
	case PT_RA:
		return { 430, 5.0f, 2, 273.0f, 273.0f, 480.0f, 1413.0f, 0xFF70FFB0 };
	default:
		return { 80, 0.25f, 0, MAX_TEMP, MAX_TEMP, MAX_TEMP, MAX_TEMP, 0 };
	}
}

BoronGroupProperties BoronGroupPropertiesFor(int type)
{
	switch (type)
	{
	case PT_B:
		return { MAX_TEMP, 1000.0f, 4200.0f, 160, 0.3f, 0xFFFFC080 };
	case PT_ALUM:
		return { 360.0f, 1100.0f, 2743.0f, 220, 0.7f, 0xFFFFFFFF };
	case PT_GA:
		return { 320.0f, 700.0f, 2673.0f, 120, 0.4f, 0xFFA0C0FF };
	case PT_IN:
		return { 330.0f, 650.0f, 2345.0f, 130, 0.5f, 0xFF89A8FF };
	case PT_TL:
		return { 293.0f, 520.0f, 1746.0f, 180, 0.8f, 0xFF66CC66 };
	case PT_NH:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 500, 1.0f, 0xFFFF80C0 };
	default:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 0, 0.0f, 0 };
	}
}

CarbonGroupProperties CarbonGroupPropertiesFor(int type)
{
	switch (type)
	{
	case PT_SLCN:
		return { MAX_TEMP, 900.0f, 4000.0f, 180, 0.3f, PT_GLAS, 0xFFDDEEFF };
	case PT_GE:
		return { 360.0f, 720.0f, 3106.0f, 130, 0.4f, PT_GLAS, 0xFF9FB8FF };
	case PT_TIN:
		return { 330.0f, 750.0f, 2875.0f, 110, 0.35f, PT_SALT, 0xFFC8D8FF };
	case PT_LEAD:
		return { 360.0f, 650.0f, 2022.0f, 90, 0.45f, PT_SALT, 0xFFB0B8C0 };
	case PT_FL:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 550, 1.0f, PT_NONE, 0xFFFF70B0 };
	default:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 0, 0.0f, PT_NONE, 0 };
	}
}

NitrogenGroupProperties NitrogenGroupPropertiesFor(int type)
{
	switch (type)
	{
	case PT_N:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 20, 0.1f, PT_NONE, 0xFFB080FF };
	case PT_P:
		return { MAX_TEMP, 320.0f, 554.0f, 340, 0.75f, PT_DUST, 0xFFE8FFB0 };
	case PT_AS:
		return { 380.0f, 620.0f, 887.0f, 160, 0.45f, PT_DUST, 0xFFB8D8FF };
	case PT_SB:
		return { 350.0f, 680.0f, 1908.0f, 130, 0.4f, PT_SALT, 0xFFC8D0FF };
	case PT_BI:
		return { 390.0f, 720.0f, 1837.0f, 100, 0.35f, PT_SALT, 0xFFD8B0FF };
	case PT_MC:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 600, 1.0f, PT_NONE, 0xFFFF6090 };
	default:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 0, 0.0f, PT_NONE, 0 };
	}
}

OxygenGroupProperties OxygenGroupPropertiesFor(int type)
{
	switch (type)
	{
	case PT_S:
		return { 390.0f, 718.0f, 300, 0.6f, PT_SMKE, 0xFFFFFF50 };
	case PT_SE:
		return { 620.0f, 958.0f, 170, 0.4f, PT_DUST, 0xFF80A0FF };
	case PT_TE:
		return { 780.0f, 1261.0f, 140, 0.35f, PT_GLAS, 0xFFD0E0FF };
	case PT_LV:
		return { MAX_TEMP, MAX_TEMP, 650, 1.0f, PT_NONE, 0xFFFF5080 };
	default:
		return { MAX_TEMP, MAX_TEMP, 0, 0.0f, PT_NONE, 0 };
	}
}

HalogenProperties HalogenPropertiesFor(int type)
{
	switch (type)
	{
	case PT_F:
		return { 293.0f, 273.0f, 273.0f, MAX_TEMP, 500, 1.4f, 0xFFDFF56A };
	case PT_CHLR:
		return { MAX_TEMP, 320.0f, 273.0f, MAX_TEMP, 300, 0.8f, 0xFF95C84B };
	case PT_BR:
		return { 380.0f, 360.0f, 290.0f, 332.0f, 220, 0.6f, 0xFF8B2F20 };
	case PT_I:
		return { 500.0f, 480.0f, 320.0f, 457.0f, 160, 0.4f, 0xFF7B3FA0 };
	case PT_AT:
		return { 520.0f, 520.0f, 340.0f, MAX_TEMP, 240, 0.5f, 0xFF4E4057 };
	case PT_TS:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, MAX_TEMP, 700, 1.0f, 0xFFFF4060 };
	default:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, MAX_TEMP, 0, 0.0f, 0 };
	}
}

FirstTransitionProperties FirstTransitionPropertiesFor(int type)
{
	switch (type)
	{
	case PT_SC:
		return { 320.0f, 750.0f, 3109.0f, 130, 0.35f, 0xFFDDEBFF };
	case PT_V:
		return { 420.0f, 900.0f, 3680.0f, 100, 0.25f, 0xFFC8D8FF };
	case PT_MN:
		return { 300.0f, 650.0f, 2334.0f, 180, 0.45f, 0xFFFFD8A0 };
	default:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 0, 0.0f, 0 };
	}
}

SecondTransitionProperties SecondTransitionPropertiesFor(int type)
{
	switch (type)
	{
	case PT_Y:
		return { 320.0f, 650.0f, 3609.0f, 140, 0.35f, 0xFFB8FF90 };
	case PT_ZR:
		return { 450.0f, 850.0f, 4682.0f, 160, 0.40f, 0xFFDDEEFF };
	case PT_NB:
		return { 550.0f, 1000.0f, 5017.0f, 100, 0.25f, 0xFFB8C8FF };
	case PT_TC:
		return { 450.0f, 850.0f, 4538.0f, 120, 0.30f, 0xFF80FFD0 };
	case PT_RU:
		return { 600.0f, 1100.0f, 4423.0f, 80, 0.20f, 0xFFD8E0FF };
	case PT_RH:
		return { 650.0f, 1050.0f, 3968.0f, 75, 0.18f, 0xFFFFE8F0 };
	case PT_PD:
		return { 500.0f, 900.0f, 3236.0f, 85, 0.20f, 0xFFE8FFF0 };
	case PT_AG:
		return { 550.0f, 1100.0f, 2435.0f, 70, 0.18f, 0xFFFFFFFF };
	case PT_CD:
		return { 300.0f, 550.0f, 1040.0f, 110, 0.30f, 0xFFFFE0A0 };
	default:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 0, 0.0f, 0 };
	}
}

ThirdTransitionProperties ThirdTransitionPropertiesFor(int type)
{
	switch (type)
	{
	case PT_HF:
		return { 700.0f, 900.0f, 4876.0f, 120, 0.30f, 0xFFDDEEFF };
	case PT_TA:
		return { 1200.0f, 1300.0f, 5731.0f, 90, 0.20f, 0xFFD8D0FF };
	case PT_RE:
		return { 800.0f, 1050.0f, 5903.0f, 80, 0.20f, 0xFFE0E8FF };
	case PT_OS:
		return { 650.0f, 450.0f, 5285.0f, 170, 0.35f, 0xFFFFFFA0 };
	case PT_IR:
		return { 1000.0f, 1200.0f, 4701.0f, 70, 0.15f, 0xFFFFF0E8 };
	default:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 0, 0.0f, 0 };
	}
}

LanthanideProperties LanthanidePropertiesFor(int type)
{
	switch (type)
	{
	case PT_LA:
		return { 293.0f, 550.0f, 3737.0f, 140, 0.35f, 0xFFB8D0FF };
	case PT_CE:
		return { 320.0f, 800.0f, 3716.0f, 130, 0.30f, 0xFFFFD080 };
	case PT_PR:
		return { 330.0f, 620.0f, 3793.0f, 125, 0.30f, 0xFFD0E8FF };
	case PT_ND:
		return { 340.0f, 650.0f, 3347.0f, 120, 0.30f, 0xFFB8D8FF };
	case PT_PM:
		return { 300.0f, 500.0f, 3273.0f, 180, 0.45f, 0xFF80FFB0 };
	case PT_SM:
		return { 350.0f, 650.0f, 2067.0f, 115, 0.30f, 0xFFD8D8FF };
	case PT_EU:
		return { 293.0f, 500.0f, 1802.0f, 150, 0.40f, 0xFFFF9070 };
	case PT_GD:
		return { 380.0f, 700.0f, 3546.0f, 110, 0.28f, 0xFF90FFE8 };
	case PT_TB:
		return { 400.0f, 720.0f, 3503.0f, 105, 0.28f, 0xFF80FF80 };
	case PT_DY:
		return { 420.0f, 750.0f, 2840.0f, 100, 0.25f, 0xFF90B8FF };
	case PT_HO:
		return { 430.0f, 780.0f, 2993.0f, 95, 0.25f, 0xFFD090FF };
	case PT_ER:
		return { 450.0f, 800.0f, 3141.0f, 90, 0.22f, 0xFFFF90C8 };
	case PT_TM:
		return { 420.0f, 720.0f, 2223.0f, 100, 0.24f, 0xFF80C8FF };
	case PT_YB:
		return { 293.0f, 450.0f, 1469.0f, 155, 0.42f, 0xFFFFE0A0 };
	case PT_LU:
		return { 500.0f, 900.0f, 3675.0f, 85, 0.20f, 0xFFDDE8FF };
	default:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 0, 0.0f, 0 };
	}
}

ActinideProperties ActinidePropertiesFor(int type)
{
	switch (type)
	{
	case PT_AC:
		return { 330.0f, 650.0f, 3471.0f, 160, PT_FR, PT_TH,
			720, 1200, 120.0f, 80.0f, 0.20f, 0xFFB8E8D0 };
	case PT_TH:
		return { 500.0f, 850.0f, 5061.0f, 100, PT_RA, PT_PA,
			1200, 2000, 100.0f, 120.0f, 0.15f, 0xFFD8E8E8 };
	case PT_PA:
		return { 450.0f, 720.0f, 4300.0f, 140, PT_URAN, PT_URAN,
			900, 1500, 140.0f, 150.0f, 0.22f, 0xFFA0D8C8 };
	case PT_NP:
		return { 400.0f, 650.0f, 4447.0f, 150, PT_PA, PT_PLUT,
			720, 1200, 160.0f, 180.0f, 0.28f, 0xFF80B890 };
	case PT_AM:
		return { 350.0f, 600.0f, 2284.0f, 170, PT_NP, PT_CM,
			600, 1000, 190.0f, 200.0f, 0.32f, 0xFFC0D078 };
	case PT_CM:
		return { 400.0f, 680.0f, 3383.0f, 180, PT_PLUT, PT_BK,
			480, 840, 220.0f, 230.0f, 0.38f, 0xFFFFB070 };
	case PT_BK:
		return { 380.0f, 640.0f, 2900.0f, 190, PT_AM, PT_CF,
			420, 720, 250.0f, 260.0f, 0.42f, 0xFFFF9080 };
	case PT_CF:
		return { 360.0f, 600.0f, 1743.0f, 210, PT_CM, PT_ES,
			300, 540, 320.0f, 320.0f, 0.70f, 0xFFFFD060 };
	case PT_ES:
		return { 340.0f, 570.0f, 1269.0f, 220, PT_BK, PT_FM,
			260, 480, 360.0f, 350.0f, 0.48f, 0xFFFF8050 };
	case PT_FM:
		return { 330.0f, 550.0f, 2200.0f, 230, PT_CF, PT_MD,
			220, 420, 400.0f, 380.0f, 0.52f, 0xFFFF6070 };
	case PT_MD:
		return { 320.0f, 520.0f, 1400.0f, 240, PT_ES, PT_NO,
			180, 360, 440.0f, 420.0f, 0.58f, 0xFFE860A0 };
	case PT_NO:
		return { 310.0f, 500.0f, 1500.0f, 250, PT_FM, PT_LR,
			150, 300, 480.0f, 460.0f, 0.64f, 0xFFC870D8 };
	case PT_LR:
		return { 300.0f, 480.0f, 2200.0f, 260, PT_MD, PT_RF,
			120, 240, 520.0f, 500.0f, 0.72f, 0xFFA080FF };
	default:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 0, NT, NT,
			0, 0, 0.0f, 0.0f, 0.0f, 0 };
	}
}

SuperheavyProperties SuperheavyPropertiesFor(int type)
{
	switch (type)
	{
	case PT_RF:
		return { 500.0f, 900.0f, 3600.0f, 280, PT_HF, PT_DB,
			360, 600, 420.0f, 250.0f, 900.0f, 0.50f, 0xFF9FC8FF, PT_PHOT };
	case PT_DB:
		return { 520.0f, 950.0f, 3400.0f, 290, PT_TA, PT_SG,
			320, 540, 450.0f, 270.0f, 1000.0f, 0.55f, 0xFF90B8FF, PT_NEUT };
	case PT_SG:
		return { 550.0f, 1000.0f, 3900.0f, 300, PT_TUNG, PT_BH,
			280, 480, 480.0f, 290.0f, 1100.0f, 0.60f, 0xFF80A8FF, PT_PHOT };
	case PT_BH:
		return { 480.0f, 850.0f, 3500.0f, 310, PT_RE, PT_HS,
			250, 430, 510.0f, 310.0f, 1200.0f, 0.65f, 0xFF9080FF, PT_PHOT };
	case PT_HS:
		return { 600.0f, 1100.0f, 3800.0f, 320, PT_OS, PT_MT,
			220, 380, 540.0f, 330.0f, 1300.0f, 0.70f, 0xFFB070FF, PT_NEUT };
	case PT_MT:
		return { 450.0f, 800.0f, 3200.0f, 330, PT_IR, PT_DS,
			190, 330, 570.0f, 350.0f, 1400.0f, 0.75f, 0xFFD070F0, PT_PHOT };
	case PT_DS:
		return { 430.0f, 750.0f, 2900.0f, 340, PT_PTNM, PT_RG,
			165, 290, 600.0f, 370.0f, 1500.0f, 0.80f, 0xFFFF70D0, PT_PHOT };
	case PT_RG:
		return { 380.0f, 650.0f, 2300.0f, 350, PT_GOLD, PT_CN,
			140, 250, 630.0f, 390.0f, 1600.0f, 0.85f, 0xFFFF80A0, PT_NEUT };
	case PT_CN:
		return { 320.0f, 500.0f, 750.0f, 360, PT_MERC, PT_NH,
			120, 220, 660.0f, 410.0f, 1700.0f, 0.90f, 0xFFFFA0C0, PT_PHOT };
	default:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 0, NT, NT,
			0, 0, 0.0f, 0.0f, MAX_TEMP, 0.0f, 0, NT };
	}
}

bool IsAlkaliMetal(int type)
{
	return type == PT_NA || type == PT_K || type == PT_CS || type == PT_FR;
}

bool IsAlkalineEarthMetal(int type)
{
	return type == PT_BE || type == PT_MAGN || type == PT_CA ||
		type == PT_SR || type == PT_BA || type == PT_RA;
}

bool IsBoronGroupElement(int type)
{
	return type == PT_B || type == PT_ALUM || type == PT_GA ||
		type == PT_IN || type == PT_TL || type == PT_NH;
}

bool IsReactiveCarbonGroupElement(int type)
{
	return type == PT_SLCN || type == PT_GE || type == PT_TIN ||
		type == PT_LEAD || type == PT_FL;
}

bool IsNitrogenGroupElement(int type)
{
	return type == PT_N || type == PT_P || type == PT_AS ||
		type == PT_SB || type == PT_BI || type == PT_MC;
}

bool IsOxygenGroupExtension(int type)
{
	return type == PT_S || type == PT_SE || type == PT_TE || type == PT_LV;
}

bool IsHalogenElement(int type)
{
	return type == PT_F || type == PT_CHLR || type == PT_BR ||
		type == PT_I || type == PT_AT || type == PT_TS;
}

bool IsFirstTransitionExtension(int type)
{
	return type == PT_SC || type == PT_V || type == PT_MN;
}

bool IsSecondTransitionExtension(int type)
{
	return type == PT_Y || type == PT_ZR || type == PT_NB ||
		type == PT_TC || type == PT_RU || type == PT_RH ||
		type == PT_PD || type == PT_AG || type == PT_CD;
}

bool IsThirdTransitionExtension(int type)
{
	return type == PT_HF || type == PT_TA || type == PT_RE ||
		type == PT_OS || type == PT_IR;
}

bool IsLanthanide(int type)
{
	return type == PT_LA || type == PT_CE || type == PT_PR ||
		type == PT_ND || type == PT_PM || type == PT_SM ||
		type == PT_EU || type == PT_GD || type == PT_TB ||
		type == PT_DY || type == PT_HO || type == PT_ER ||
		type == PT_TM || type == PT_YB || type == PT_LU;
}

bool IsActinideExtension(int type)
{
	return type == PT_AC || type == PT_TH || type == PT_PA ||
		type == PT_NP || type == PT_AM || type == PT_CM ||
		type == PT_BK || type == PT_CF || type == PT_ES ||
		type == PT_FM || type == PT_MD || type == PT_NO ||
		type == PT_LR;
}

bool IsSuperheavyTransition(int type)
{
	return type == PT_RF || type == PT_DB || type == PT_SG ||
		type == PT_BH || type == PT_HS || type == PT_MT ||
		type == PT_DS || type == PT_RG || type == PT_CN;
}

bool IsHalogenReactiveMetal(int type)
{
	return type == PT_LITH || type == PT_NA || type == PT_K ||
		type == PT_RBDM || type == PT_CS || type == PT_MAGN ||
		type == PT_CA || type == PT_SR || type == PT_BA ||
		type == PT_ALUM || type == PT_COPR || type == PT_IRON ||
		type == PT_METL;
}

bool IsDisinfectionTarget(int type)
{
	return type == PT_PATH || type == PT_SPOR || type == PT_MYCL ||
		type == PT_BIOF;
}

bool IsWaterLike(int type)
{
	return type == PT_WATR || type == PT_DSTW || type == PT_SLTW ||
		type == PT_CBNW || type == PT_WTRV;
}

void ResetReactionProduct(Particle &particle, int type)
{
	particle.life = type == PT_CAUS ? 75 : 0;
	particle.ctype = 0;
	particle.tmp = 0;
	particle.tmp2 = 0;
}

void AddBoundedPressure(Simulation *sim, int x, int y, float amount)
{
	auto &pressure = sim->pv[y / CELL][x / CELL];
	pressure = std::min(pressure + amount, MAX_PRESSURE);
}

void EmitPeriodicFire(
	Simulation *sim, int x, int y, float temperature, int chance,
	unsigned int colour = 0)
{
	if (chance <= 0)
	{
		return;
	}
	if (chance > 1 && !sim->rng.chance(1, chance))
	{
		return;
	}
	int fire = sim->create_part(-3, x, y, PT_FIRE);
	if (fire >= 0)
	{
		sim->parts[fire].temp = temperature;
		sim->parts[fire].life = 16;
		if (colour)
		{
			sim->parts[fire].dcolour = colour;
		}
	}
}

bool IsDischargeSource(int packed)
{
	if (!packed)
	{
		return false;
	}
	auto type = TYP(packed);
	return type == PT_SPRK || type == PT_ELEC || type == PT_PLSM || type == PT_LIGH;
}

bool HasLocalDischarge(int x, int y, int pmap[YRES][XRES], Simulation *sim)
{
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			if (IsDischargeSource(pmap[y + ry][x + rx]) ||
				IsDischargeSource(sim->photons[y + ry][x + rx]))
			{
				return true;
			}
		}
	}
	return false;
}

bool HasLocalPhoton(int x, int y, Simulation *sim)
{
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = sim->photons[y + ry][x + rx];
			if (packed && TYP(packed) == PT_PHOT)
			{
				return true;
			}
		}
	}
	return false;
}

bool EmitDischarge(UPDATE_FUNC_ARGS)
{
	if (parts[i].life > 0 || !HasLocalDischarge(x, y, pmap, sim))
	{
		return false;
	}
	auto properties = PropertiesFor(parts[i].type);
	if (!sim->rng.chance(1, properties.dischargeChance) || !ConsumeEvent(sim))
	{
		return false;
	}
	parts[i].life = properties.glowLife;
	parts[i].temp = std::min(parts[i].temp + float(properties.dischargeHeat), MAX_TEMP);
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = properties.photonMask;
		parts[photon].temp = parts[i].temp;
		parts[photon].life = 24;
	}
	return true;
}

bool ExchangeCryogenicHeat(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_HE || parts[i].temp >= 20.0f)
	{
		return false;
	}
	int start = sim->rng.between(0, 7);
	for (int offset = 0; offset < 8; ++offset)
	{
		int slot = (start + offset) % 8;
		int rx = (slot % 3) - 1;
		int ry = (slot / 3) - 1;
		if (slot >= 4)
		{
			++slot;
			rx = (slot % 3) - 1;
			ry = (slot / 3) - 1;
		}
		if (!InBounds(x + rx, y + ry))
		{
			continue;
		}
		auto packed = pmap[y + ry][x + rx];
		if (!packed)
		{
			continue;
		}
		auto neighbour = ID(packed);
		if (parts[neighbour].temp <= parts[i].temp + 1.0f || !ConsumeEvent(sim))
		{
			return false;
		}
		float transfer = std::min(4.0f, (parts[neighbour].temp - parts[i].temp) * 0.05f);
		parts[neighbour].temp -= transfer;
		parts[i].temp = std::min(parts[i].temp + transfer, 20.0f);
		return true;
	}
	return false;
}

bool DecayRadioactiveGas(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_RN && parts[i].type != PT_OG)
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = parts[i].type == PT_RN
			? sim->rng.between(1200, 2400)
			: sim->rng.between(45, 120);
		return false;
	}
	--parts[i].tmp;
	if (parts[i].tmp > 0)
	{
		return false;
	}
	if (!ConsumeEvent(sim))
	{
		parts[i].tmp = 1;
		return false;
	}

	auto sourceType = parts[i].type;
	auto targetType = sourceType == PT_RN ? PT_POLO : PT_RN;
	auto temperature = std::min(parts[i].temp + (sourceType == PT_RN ? 120.0f : 420.0f), MAX_TEMP);
	sim->part_change_type(i, x, y, targetType);
	parts[i].temp = temperature;
	parts[i].life = 0;
	parts[i].ctype = 0;
	parts[i].tmp2 = 0;
	parts[i].tmp = targetType == PT_RN ? sim->rng.between(1200, 2400) : 0;

	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = sourceType == PT_RN ? 0x0003FFF0 : 0x03F03F00;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool DecayFrancium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_FR || parts[i].type != PT_FR)
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sim->rng.between(180, 360);
		return false;
	}
	--parts[i].tmp;
	if (parts[i].tmp > 0)
	{
		return false;
	}
	if (!ConsumeEvent(sim))
	{
		parts[i].tmp = 1;
		return false;
	}

	auto temperature = std::min(parts[i].temp + 360.0f, MAX_TEMP);
	sim->part_change_type(i, x, y, PT_POLO);
	ResetReactionProduct(parts[i], PT_POLO);
	parts[i].temp = temperature;
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x0003FFF0;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool ReactAlkaliWithWaterOrAcid(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = AlkaliPropertiesFor(sourceType);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed)
			{
				continue;
			}
			auto neighbourType = TYP(packed);
			bool acid = neighbourType == PT_ACID;
			if (!acid && !IsWaterLike(neighbourType))
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto neighbour = ID(packed);
			float heat = acid ? properties.waterHeat * 0.65f : properties.waterHeat;
			float temperature = std::min(
				std::max(parts[i].temp, parts[neighbour].temp) + heat,
				MAX_TEMP);
			int residue = acid ? PT_SALT : PT_CAUS;
			sim->part_change_type(i, x, y, residue);
			sim->part_change_type(neighbour, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[i], residue);
			ResetReactionProduct(parts[neighbour], PT_H2);
			parts[i].temp = temperature;
			parts[neighbour].temp = temperature;
			AddBoundedPressure(sim, x, y, acid ? properties.pressure * 0.5f : properties.pressure);
			EmitPeriodicFire(sim, x, y, temperature, acid ? properties.fireChance * 2 : properties.fireChance);
			return true;
		}
	}
	return false;
}

bool OxidiseHotAlkali(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = AlkaliPropertiesFor(sourceType);
	if (parts[i].temp < properties.oxygenThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto oxygen = ID(packed);
			float temperature = std::min(
				parts[i].temp + properties.waterHeat + 240.0f,
				MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[i].temp = temperature;
			parts[oxygen].temp = temperature;
			parts[oxygen].life = 24;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.75f);
			return true;
		}
	}
	return false;
}

bool VaporiseHotAlkali(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = AlkaliPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	float temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.5f);
	return true;
}

bool UpdateAlkali(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsAlkaliMetal(sourceType))
	{
		return false;
	}
	if (DecayFrancium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseHotAlkali(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactAlkaliWithWaterOrAcid(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return OxidiseHotAlkali(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool DecayRadium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_RA || parts[i].type != PT_RA)
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sim->rng.between(900, 1800);
		return false;
	}
	--parts[i].tmp;
	if (parts[i].tmp > 0)
	{
		return false;
	}
	if (!ConsumeEvent(sim))
	{
		parts[i].tmp = 1;
		return false;
	}

	auto temperature = std::min(parts[i].temp + 240.0f, MAX_TEMP);
	sim->part_change_type(i, x, y, PT_RN);
	ResetReactionProduct(parts[i], PT_RN);
	parts[i].temp = temperature;
	parts[i].tmp = sim->rng.between(1200, 2400);
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x0003FFF0;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool ReactAlkalineEarthWithWaterOrAcid(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = AlkalineEarthPropertiesFor(sourceType);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed)
			{
				continue;
			}
			auto neighbourType = TYP(packed);
			bool acid = neighbourType == PT_ACID;
			if (!acid && !IsWaterLike(neighbourType))
			{
				continue;
			}
			auto neighbour = ID(packed);
			float threshold = acid ? properties.acidThreshold : properties.waterThreshold;
			if (std::max(parts[i].temp, parts[neighbour].temp) < threshold)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			float heat = acid ? properties.waterHeat * 0.55f : properties.waterHeat;
			float temperature = std::min(
				std::max(parts[i].temp, parts[neighbour].temp) + heat,
				MAX_TEMP);
			int residue = acid ? PT_SALT : PT_CAUS;
			sim->part_change_type(i, x, y, residue);
			sim->part_change_type(neighbour, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[i], residue);
			ResetReactionProduct(parts[neighbour], PT_H2);
			parts[i].temp = temperature;
			parts[neighbour].temp = temperature;
			AddBoundedPressure(
				sim, x, y,
				acid ? properties.pressure * 0.4f : properties.pressure);
			EmitPeriodicFire(
				sim, x, y, temperature,
				acid ? properties.fireChance * 2 : properties.fireChance,
				properties.flameColour);
			return true;
		}
	}
	return false;
}

bool OxidiseHotAlkalineEarth(UPDATE_FUNC_ARGS, int sourceType)
{
	// Magnesium retains its existing metallurgy implementation, which produces
	// typed recoverable scrap and a bright finite flame above 800 K.
	if (sourceType == PT_MAGN)
	{
		return false;
	}
	auto properties = AlkalineEarthPropertiesFor(sourceType);
	if (parts[i].temp < properties.oxygenThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto oxygen = ID(packed);
			float temperature = std::min(
				parts[i].temp + properties.waterHeat + 180.0f,
				MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[i].temp = temperature;
			parts[oxygen].temp = temperature;
			parts[oxygen].life = 24;
			parts[oxygen].dcolour = properties.flameColour;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.6f);
			return true;
		}
	}
	return false;
}

bool VaporiseHotAlkalineEarth(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = AlkalineEarthPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	float temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.flameColour;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.35f);
	return true;
}

bool UpdateAlkalineEarth(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsAlkalineEarthMetal(sourceType))
	{
		return false;
	}
	if (DecayRadium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseHotAlkalineEarth(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactAlkalineEarthWithWaterOrAcid(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return OxidiseHotAlkalineEarth(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool DecayNihonium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_NH || parts[i].type != PT_NH)
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sim->rng.between(90, 180);
		return false;
	}
	--parts[i].tmp;
	if (parts[i].tmp > 0)
	{
		return false;
	}
	if (!ConsumeEvent(sim))
	{
		parts[i].tmp = 1;
		return false;
	}

	auto temperature = std::min(parts[i].temp + 500.0f, MAX_TEMP);
	sim->part_change_type(i, x, y, PT_POLO);
	ResetReactionProduct(parts[i], PT_POLO);
	parts[i].temp = temperature;
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x03F03F00;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool CaptureBoronNeutron(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_B)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			for (auto packed : { pmap[y + ry][x + rx], sim->photons[y + ry][x + rx] })
			{
				if (!packed || TYP(packed) != PT_NEUT)
				{
					continue;
				}
				if (!ConsumeEvent(sim))
				{
					return false;
				}

				auto neutron = ID(packed);
				auto temperature = std::min(
					std::max(parts[i].temp, parts[neutron].temp) + 180.0f,
					MAX_TEMP);
				sim->part_change_type(i, x, y, PT_LITH);
				sim->part_change_type(neutron, x + rx, y + ry, PT_HE);
				ResetReactionProduct(parts[i], PT_LITH);
				ResetReactionProduct(parts[neutron], PT_HE);
				parts[i].temp = temperature;
				parts[neutron].temp = temperature;
				AddBoundedPressure(sim, x, y, 0.35f);
				return true;
			}
		}
	}
	return false;
}

bool EmbrittleAluminiumWithGallium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_GA || parts[i].temp < 302.91f)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed)
			{
				continue;
			}
			auto neighbour = ID(packed);
			auto neighbourType = TYP(packed);
			if (neighbourType != PT_ALUM &&
				(neighbourType != PT_LAVA || parts[neighbour].ctype != PT_ALUM))
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto temperature = parts[neighbour].temp;
			sim->part_change_type(neighbour, x + rx, y + ry, PT_MSCR);
			ResetReactionProduct(parts[neighbour], PT_MSCR);
			parts[neighbour].ctype = PT_ALUM;
			parts[neighbour].temp = temperature;
			parts[neighbour].tmp3 = 0;
			parts[neighbour].tmp4 = 0;
			parts[i].temp = std::min(parts[i].temp + 12.0f, MAX_TEMP);
			return true;
		}
	}
	return false;
}

bool ReactBoronGroupWithAcidOrCaustic(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_B || sourceType == PT_NH)
	{
		return false;
	}
	auto properties = BoronGroupPropertiesFor(sourceType);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed)
			{
				continue;
			}
			auto neighbourType = TYP(packed);
			bool acid = neighbourType == PT_ACID;
			bool caustic = neighbourType == PT_CAUS &&
				(sourceType == PT_ALUM || sourceType == PT_GA);
			if (!acid && !caustic)
			{
				continue;
			}
			auto neighbour = ID(packed);
			if (std::max(parts[i].temp, parts[neighbour].temp) < properties.acidThreshold)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto temperature = std::min(
				std::max(parts[i].temp, parts[neighbour].temp) +
					float(properties.reactionHeat),
				MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(neighbour, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[neighbour], PT_H2);
			parts[i].temp = temperature;
			parts[neighbour].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure);
			return true;
		}
	}
	return false;
}

bool OxidiseHotBoronGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_NH)
	{
		return false;
	}
	auto properties = BoronGroupPropertiesFor(sourceType);
	if (parts[i].temp < properties.oxygenThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto oxygen = ID(packed);
			auto oxide = sourceType == PT_B ? PT_GLAS : PT_SALT;
			auto temperature = std::min(
				parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, oxide);
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[i], oxide);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[i].temp = temperature;
			parts[oxygen].temp = temperature;
			parts[oxygen].life = 20;
			parts[oxygen].dcolour = properties.flameColour;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.4f);
			return true;
		}
	}
	return false;
}

bool VaporiseHotBoronGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_NH)
	{
		return false;
	}
	auto properties = BoronGroupPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.flameColour;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.25f);
	return true;
}

bool UpdateBoronGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsBoronGroupElement(sourceType))
	{
		return false;
	}
	if (DecayNihonium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (CaptureBoronNeutron(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (EmbrittleAluminiumWithGallium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseHotBoronGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactBoronGroupWithAcidOrCaustic(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return OxidiseHotBoronGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool DecayFlerovium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_FL || parts[i].type != PT_FL)
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sim->rng.between(60, 130);
		return false;
	}
	--parts[i].tmp;
	if (parts[i].tmp > 0)
	{
		return false;
	}
	if (!ConsumeEvent(sim))
	{
		parts[i].tmp = 1;
		return false;
	}

	auto temperature = std::min(parts[i].temp + 550.0f, MAX_TEMP);
	sim->part_change_type(i, x, y, PT_POLO);
	ResetReactionProduct(parts[i], PT_POLO);
	parts[i].temp = temperature;
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x03F03F00;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool EmbrittleColdTin(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_TIN || parts[i].type != PT_TIN)
	{
		return false;
	}
	if (parts[i].temp >= 286.0f)
	{
		parts[i].tmp = 0;
		return false;
	}
	parts[i].tmp = std::min(parts[i].tmp + 1, 120);
	if (parts[i].tmp < 120)
	{
		return false;
	}
	if (!ConsumeEvent(sim))
	{
		parts[i].tmp = 119;
		return false;
	}

	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_MSCR);
	ResetReactionProduct(parts[i], PT_MSCR);
	parts[i].ctype = PT_TIN;
	parts[i].temp = temperature;
	return true;
}

bool ReactCarbonGroupWithAcid(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_SLCN || sourceType == PT_FL)
	{
		return false;
	}
	auto properties = CarbonGroupPropertiesFor(sourceType);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_ACID)
			{
				continue;
			}
			auto acid = ID(packed);
			if (std::max(parts[i].temp, parts[acid].temp) < properties.acidThreshold)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto temperature = std::min(
				std::max(parts[i].temp, parts[acid].temp) +
					float(properties.reactionHeat),
				MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(acid, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[acid], PT_H2);
			parts[i].temp = temperature;
			parts[acid].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure);
			return true;
		}
	}
	return false;
}

bool OxidiseHotCarbonGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_FL)
	{
		return false;
	}
	auto properties = CarbonGroupPropertiesFor(sourceType);
	if (parts[i].temp < properties.oxygenThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto oxygen = ID(packed);
			auto temperature = std::min(
				parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, properties.oxideProduct);
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[i], properties.oxideProduct);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[i].temp = temperature;
			parts[oxygen].temp = temperature;
			parts[oxygen].life = 20;
			parts[oxygen].dcolour = properties.flameColour;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.35f);
			return true;
		}
	}
	return false;
}

bool VaporiseHotCarbonGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_FL)
	{
		return false;
	}
	auto properties = CarbonGroupPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.flameColour;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.25f);
	return true;
}

bool ExciteGermanium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_GE || parts[i].life > 0 ||
		!HasLocalDischarge(x, y, pmap, sim) ||
		!sim->rng.chance(1, 4) || !ConsumeEvent(sim))
	{
		return false;
	}
	parts[i].life = 10;
	parts[i].temp = std::min(parts[i].temp + 15.0f, MAX_TEMP);
	return true;
}

bool UpdateCarbonGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsReactiveCarbonGroupElement(sourceType))
	{
		return false;
	}
	if (DecayFlerovium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (EmbrittleColdTin(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseHotCarbonGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactCarbonGroupWithAcid(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (OxidiseHotCarbonGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return ExciteGermanium(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool DecayMoscovium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_MC || parts[i].type != PT_MC)
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sim->rng.between(50, 110);
		return false;
	}
	--parts[i].tmp;
	if (parts[i].tmp > 0)
	{
		return false;
	}
	if (!ConsumeEvent(sim))
	{
		parts[i].tmp = 1;
		return false;
	}

	auto temperature = std::min(parts[i].temp + 600.0f, MAX_TEMP);
	sim->part_change_type(i, x, y, PT_NH);
	ResetReactionProduct(parts[i], PT_NH);
	parts[i].tmp = sim->rng.between(90, 180);
	parts[i].temp = temperature;
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x03F03F00;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool ReactNitrogenGroupWithAcid(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_AS && sourceType != PT_SB && sourceType != PT_BI)
	{
		return false;
	}
	auto properties = NitrogenGroupPropertiesFor(sourceType);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_ACID)
			{
				continue;
			}
			auto acid = ID(packed);
			if (std::max(parts[i].temp, parts[acid].temp) < properties.acidThreshold)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto temperature = std::min(
				std::max(parts[i].temp, parts[acid].temp) +
					float(properties.reactionHeat),
				MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(acid, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[acid], PT_H2);
			parts[i].temp = temperature;
			parts[acid].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure);
			return true;
		}
	}
	return false;
}

bool OxidiseHotNitrogenGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_N || sourceType == PT_MC)
	{
		return false;
	}
	auto properties = NitrogenGroupPropertiesFor(sourceType);
	if (parts[i].temp < properties.oxygenThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto oxygen = ID(packed);
			auto temperature = std::min(
				parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, properties.oxideProduct);
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[i], properties.oxideProduct);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[i].temp = temperature;
			parts[oxygen].temp = temperature;
			parts[oxygen].life = sourceType == PT_P ? 32 : 20;
			parts[oxygen].dcolour = properties.flameColour;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.35f);
			return true;
		}
	}
	return false;
}

bool VaporiseHotNitrogenGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_N || sourceType == PT_MC)
	{
		return false;
	}
	auto properties = NitrogenGroupPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.flameColour;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.25f);
	return true;
}

bool ExciteNitrogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_N || parts[i].type != PT_N || parts[i].life > 0 ||
		!HasLocalDischarge(x, y, pmap, sim) ||
		!sim->rng.chance(1, 5) || !ConsumeEvent(sim))
	{
		return false;
	}
	parts[i].life = 12;
	parts[i].temp = std::min(parts[i].temp + 20.0f, MAX_TEMP);
	return true;
}

bool UpdateNitrogenGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsNitrogenGroupElement(sourceType))
	{
		return false;
	}
	if (DecayMoscovium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseHotNitrogenGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactNitrogenGroupWithAcid(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (OxidiseHotNitrogenGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return ExciteNitrogen(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool DecayLivermorium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_LV || parts[i].type != PT_LV)
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sim->rng.between(40, 90);
		return false;
	}
	--parts[i].tmp;
	if (parts[i].tmp > 0)
	{
		return false;
	}
	if (!ConsumeEvent(sim))
	{
		parts[i].tmp = 1;
		return false;
	}

	auto temperature = std::min(parts[i].temp + 650.0f, MAX_TEMP);
	sim->part_change_type(i, x, y, PT_FL);
	ResetReactionProduct(parts[i], PT_FL);
	parts[i].tmp = sim->rng.between(60, 130);
	parts[i].temp = temperature;
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x03F03F00;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool OxidiseHotOxygenGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_LV)
	{
		return false;
	}
	auto properties = OxygenGroupPropertiesFor(sourceType);
	if (parts[i].temp < properties.oxygenThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto oxygen = ID(packed);
			auto temperature = std::min(
				parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, properties.oxideProduct);
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[i], properties.oxideProduct);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[i].temp = temperature;
			parts[oxygen].temp = temperature;
			if (properties.oxideProduct == PT_SMKE)
			{
				parts[i].life = 45;
			}
			parts[oxygen].life = sourceType == PT_S ? 32 : 20;
			parts[oxygen].dcolour = properties.flameColour;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.35f);
			return true;
		}
	}
	return false;
}

bool VaporiseHotOxygenGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_LV)
	{
		return false;
	}
	auto properties = OxygenGroupPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.flameColour;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.25f);
	return true;
}

bool ExciteSelenium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_SE || parts[i].type != PT_SE || parts[i].life > 0 ||
		(!HasLocalDischarge(x, y, pmap, sim) && !HasLocalPhoton(x, y, sim)) ||
		!sim->rng.chance(1, 4) || !ConsumeEvent(sim))
	{
		return false;
	}
	parts[i].life = 12;
	parts[i].temp = std::min(parts[i].temp + 15.0f, MAX_TEMP);
	return true;
}

bool UpdateOxygenGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsOxygenGroupExtension(sourceType))
	{
		return false;
	}
	if (DecayLivermorium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseHotOxygenGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (OxidiseHotOxygenGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return ExciteSelenium(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool DecayRadioactiveHalogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if ((sourceType != PT_AT && sourceType != PT_TS) ||
		(parts[i].type != sourceType &&
			(parts[i].type != PT_LAVA || parts[i].ctype != sourceType)))
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sourceType == PT_TS ? sim->rng.between(35, 75) :
			sim->rng.between(240, 480);
		return false;
	}
	--parts[i].tmp;
	if (parts[i].tmp > 0)
	{
		return false;
	}
	if (!ConsumeEvent(sim))
	{
		parts[i].tmp = 1;
		return false;
	}

	auto properties = HalogenPropertiesFor(sourceType);
	auto temperature = std::min(parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
	int product = sourceType == PT_TS ? PT_MC : PT_POLO;
	sim->part_change_type(i, x, y, product);
	ResetReactionProduct(parts[i], product);
	parts[i].temp = temperature;
	if (product == PT_MC)
	{
		parts[i].tmp = sim->rng.between(50, 110);
	}
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = sourceType == PT_TS ? 0x03F03F00 : 0x0003FFF0;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool ReactFluorineWithWater(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_F || parts[i].type != PT_F || parts[i].temp < 250.0f)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || !IsWaterLike(TYP(packed)))
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}
			auto water = ID(packed);
			auto properties = HalogenPropertiesFor(sourceType);
			auto temperature = std::min(parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_ACID);
			sim->part_change_type(water, x + rx, y + ry, PT_ACID);
			ResetReactionProduct(parts[i], PT_ACID);
			ResetReactionProduct(parts[water], PT_ACID);
			parts[i].temp = temperature;
			parts[water].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure);
			return true;
		}
	}
	return false;
}

bool ReactHalogenWithHydrogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_CHLR || sourceType == PT_TS)
	{
		return false;
	}
	auto properties = HalogenPropertiesFor(sourceType);
	if (parts[i].temp < properties.hydrogenThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_H2)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}
			auto hydrogen = ID(packed);
			auto temperature = std::min(parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_ACID);
			sim->part_change_type(hydrogen, x + rx, y + ry, PT_ACID);
			ResetReactionProduct(parts[i], PT_ACID);
			ResetReactionProduct(parts[hydrogen], PT_ACID);
			parts[i].temp = temperature;
			parts[hydrogen].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.6f);
			return true;
		}
	}
	return false;
}

bool ReactHalogenWithMetal(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_TS)
	{
		return false;
	}
	auto properties = HalogenPropertiesFor(sourceType);
	if (parts[i].temp < properties.metalThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed)
			{
				continue;
			}
			auto metal = ID(packed);
			auto metalType = parts[metal].type == PT_LAVA ? parts[metal].ctype : parts[metal].type;
			if (!IsHalogenReactiveMetal(metalType))
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}
			auto temperature = std::min(parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(metal, x + rx, y + ry, PT_SALT);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[metal], PT_SALT);
			parts[i].temp = temperature;
			parts[metal].temp = temperature;
			parts[i].dcolour = properties.vapourColour;
			parts[metal].dcolour = properties.vapourColour;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.45f);
			return true;
		}
	}
	return false;
}

bool DisinfectWithHalogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_TS)
	{
		return false;
	}
	auto properties = HalogenPropertiesFor(sourceType);
	if (parts[i].temp < properties.disinfectionThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || !IsDisinfectionTarget(TYP(packed)))
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}
			auto target = ID(packed);
			auto temperature = std::min(parts[i].temp + float(properties.reactionHeat) * 0.25f, MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(target, x + rx, y + ry, PT_DUST);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[target], PT_DUST);
			parts[i].temp = temperature;
			parts[target].temp = temperature;
			return true;
		}
	}
	return false;
}

bool VaporiseHalogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_BR && sourceType != PT_I)
	{
		return false;
	}
	auto properties = HalogenPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_SMKE);
	ResetReactionProduct(parts[i], PT_SMKE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.vapourColour;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.25f);
	return true;
}

bool UpdateHalogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsHalogenElement(sourceType))
	{
		return false;
	}
	if (DecayRadioactiveHalogen(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactFluorineWithWater(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactHalogenWithHydrogen(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactHalogenWithMetal(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (DisinfectWithHalogen(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return VaporiseHalogen(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool ExciteScandium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_SC || parts[i].type != PT_SC || parts[i].life > 0 ||
		!HasLocalDischarge(x, y, pmap, sim) || !sim->rng.chance(1, 3) ||
		!ConsumeEvent(sim))
	{
		return false;
	}
	parts[i].life = 12;
	parts[i].temp = std::min(parts[i].temp + 18.0f, MAX_TEMP);
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x000FF000;
		parts[photon].temp = parts[i].temp;
		parts[photon].life = 20;
	}
	return true;
}

bool AlloyVanadiumToolSteel(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_V || parts[i].temp < 1800.0f)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_LAVA)
			{
				continue;
			}
			auto steel = ID(packed);
			if (parts[steel].ctype != PT_STEL || parts[steel].temp < 1700.0f ||
				!ConsumeEvent(sim))
			{
				continue;
			}
			auto temperature = std::max(
				1800.0f, (parts[i].temp + parts[steel].temp) * 0.5f);
			sim->part_change_type(i, x, y, PT_LAVA);
			ResetReactionProduct(parts[i], PT_LAVA);
			parts[i].ctype = PT_TSTL;
			parts[i].temp = temperature;
			ResetReactionProduct(parts[steel], PT_LAVA);
			parts[steel].ctype = PT_TSTL;
			parts[steel].temp = temperature;
			return true;
		}
	}
	return false;
}

bool DeoxidiseSteelWithManganese(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_MN || parts[i].temp < 1200.0f)
	{
		return false;
	}
	int molten = -1;
	int oxygen = -1;
	int moltenX = 0;
	int moltenY = 0;
	int oxygenX = 0;
	int oxygenY = 0;
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed)
			{
				continue;
			}
			auto neighbour = ID(packed);
			if (TYP(packed) == PT_O2 && oxygen < 0)
			{
				oxygen = neighbour;
				oxygenX = x + rx;
				oxygenY = y + ry;
			}
			else if (TYP(packed) == PT_LAVA && molten < 0 &&
				(parts[neighbour].ctype == PT_IRON || parts[neighbour].ctype == PT_STEL) &&
				parts[neighbour].temp >= 1600.0f)
			{
				molten = neighbour;
				moltenX = x + rx;
				moltenY = y + ry;
			}
		}
	}
	if (molten < 0 || oxygen < 0 || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = std::max(
		1600.0f, (parts[i].temp + parts[molten].temp) * 0.5f);
	sim->part_change_type(i, x, y, PT_LAVA);
	ResetReactionProduct(parts[i], PT_LAVA);
	parts[i].ctype = PT_STEL;
	parts[i].temp = temperature;
	ResetReactionProduct(parts[molten], PT_LAVA);
	parts[molten].ctype = PT_STEL;
	parts[molten].temp = temperature;
	sim->part_change_type(oxygen, oxygenX, oxygenY, PT_SLAG);
	ResetReactionProduct(parts[oxygen], PT_SLAG);
	parts[oxygen].temp = std::min(temperature, 1450.0f);
	AddBoundedPressure(sim, moltenX, moltenY, 0.2f);
	return true;
}

bool ReactFirstTransitionWithAcid(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = FirstTransitionPropertiesFor(sourceType);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_ACID)
			{
				continue;
			}
			auto acid = ID(packed);
			if (std::max(parts[i].temp, parts[acid].temp) < properties.acidThreshold ||
				!ConsumeEvent(sim))
			{
				continue;
			}
			auto temperature = std::min(
				std::max(parts[i].temp, parts[acid].temp) +
					float(properties.reactionHeat),
				MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(acid, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[acid], PT_H2);
			parts[i].temp = temperature;
			parts[acid].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure);
			return true;
		}
	}
	return false;
}

bool OxidiseHotFirstTransition(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = FirstTransitionPropertiesFor(sourceType);
	if (parts[i].temp < properties.oxygenThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2 || !ConsumeEvent(sim))
			{
				continue;
			}
			auto oxygen = ID(packed);
			auto temperature = std::min(
				parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_MSCR);
			ResetReactionProduct(parts[i], PT_MSCR);
			parts[i].ctype = sourceType;
			parts[i].temp = temperature;
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[oxygen].temp = temperature;
			parts[oxygen].life = 20;
			parts[oxygen].dcolour = properties.flameColour;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.35f);
			return true;
		}
	}
	return false;
}

bool VaporiseFirstTransition(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = FirstTransitionPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.flameColour;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.25f);
	return true;
}

bool UpdateFirstTransition(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsFirstTransitionExtension(sourceType))
	{
		return false;
	}
	if (AlloyVanadiumToolSteel(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (DeoxidiseSteelWithManganese(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseFirstTransition(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactFirstTransitionWithAcid(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (OxidiseHotFirstTransition(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return ExciteScandium(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool DecayTechnetium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_TC ||
		(parts[i].type != PT_TC &&
			(parts[i].type != PT_LAVA || parts[i].ctype != PT_TC)))
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sim->rng.between(600, 1200);
		return false;
	}
	--parts[i].tmp;
	if (parts[i].tmp > 0)
	{
		return false;
	}
	if (!ConsumeEvent(sim))
	{
		parts[i].tmp = 1;
		return false;
	}
	auto temperature = std::min(parts[i].temp + 180.0f, MAX_TEMP);
	sim->part_change_type(i, x, y, PT_RU);
	ResetReactionProduct(parts[i], PT_RU);
	parts[i].temp = temperature;
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x0003FFF0;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool ExciteYttrium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_Y || parts[i].type != PT_Y || parts[i].life > 0 ||
		!HasLocalDischarge(x, y, pmap, sim) || !sim->rng.chance(1, 4) ||
		!ConsumeEvent(sim))
	{
		return false;
	}
	parts[i].life = 12;
	parts[i].temp = std::min(parts[i].temp + 16.0f, MAX_TEMP);
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x0000FFF0;
		parts[photon].temp = parts[i].temp;
		parts[photon].life = 20;
	}
	return true;
}

bool ReactZirconiumWithSteam(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_ZR || parts[i].temp < 1000.0f)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || (TYP(packed) != PT_WTRV && TYP(packed) != PT_WATR))
			{
				continue;
			}
			auto water = ID(packed);
			if (std::max(parts[i].temp, parts[water].temp) < 1000.0f ||
				!ConsumeEvent(sim))
			{
				continue;
			}
			auto temperature = std::min(
				std::max(parts[i].temp, parts[water].temp) + 260.0f, MAX_TEMP);
			sim->part_change_type(i, x, y, PT_MSCR);
			ResetReactionProduct(parts[i], PT_MSCR);
			parts[i].ctype = PT_ZR;
			parts[i].temp = temperature;
			sim->part_change_type(water, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[water], PT_H2);
			parts[water].temp = temperature;
			EmitPeriodicFire(sim, x, y, temperature, 1, 0xFFFFFFFF);
			AddBoundedPressure(sim, x, y, 0.8f);
			return true;
		}
	}
	return false;
}

bool CatalyseHydrogenWithPlatinumGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if ((sourceType != PT_RU && sourceType != PT_RH) ||
		parts[i].temp < (sourceType == PT_RH ? 330.0f : 450.0f) ||
		!sim->rng.chance(1, sourceType == PT_RH ? 2 : 4))
	{
		return false;
	}
	int hydrogen = -1;
	int oxygen = -1;
	int hydrogenX = 0;
	int hydrogenY = 0;
	int oxygenX = 0;
	int oxygenY = 0;
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed)
			{
				continue;
			}
			if (TYP(packed) == PT_H2 && hydrogen < 0)
			{
				hydrogen = ID(packed);
				hydrogenX = x + rx;
				hydrogenY = y + ry;
			}
			else if (TYP(packed) == PT_O2 && oxygen < 0)
			{
				oxygen = ID(packed);
				oxygenX = x + rx;
				oxygenY = y + ry;
			}
		}
	}
	if (hydrogen < 0 || oxygen < 0 || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = std::min(parts[i].temp + 140.0f, MAX_TEMP);
	sim->part_change_type(hydrogen, hydrogenX, hydrogenY, PT_WATR);
	sim->part_change_type(oxygen, oxygenX, oxygenY, PT_WATR);
	ResetReactionProduct(parts[hydrogen], PT_WATR);
	ResetReactionProduct(parts[oxygen], PT_WATR);
	parts[hydrogen].temp = temperature;
	parts[oxygen].temp = temperature;
	parts[i].temp = std::min(parts[i].temp + 25.0f, MAX_TEMP);
	return true;
}

bool ReleasePalladiumHydrogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_PD || parts[i].type != PT_PD ||
		parts[i].tmp <= 0 || parts[i].temp < 500.0f)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry) ||
				pmap[y + ry][x + rx])
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}
			int hydrogen = sim->create_part(-1, x + rx, y + ry, PT_H2);
			if (hydrogen < 0)
			{
				return false;
			}
			parts[hydrogen].temp = parts[i].temp;
			--parts[i].tmp;
			return true;
		}
	}
	return false;
}

bool AbsorbPalladiumHydrogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_PD || parts[i].type != PT_PD ||
		parts[i].tmp >= 4 || parts[i].temp >= 500.0f)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_H2 || !ConsumeEvent(sim))
			{
				continue;
			}
			sim->kill_part(ID(packed));
			++parts[i].tmp;
			parts[i].temp = std::min(parts[i].temp + 8.0f, MAX_TEMP);
			return true;
		}
	}
	return false;
}

bool TarnishSilverWithSulfur(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_AG || parts[i].temp < 350.0f)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_S || !ConsumeEvent(sim))
			{
				continue;
			}
			auto sulfur = ID(packed);
			sim->part_change_type(i, x, y, PT_MSCR);
			ResetReactionProduct(parts[i], PT_MSCR);
			parts[i].ctype = PT_AG;
			parts[i].dcolour = 0xFF303438;
			sim->part_change_type(sulfur, x + rx, y + ry, PT_DUST);
			ResetReactionProduct(parts[sulfur], PT_DUST);
			parts[sulfur].dcolour = 0xFF303438;
			return true;
		}
	}
	return false;
}

bool ReactSecondTransitionWithAcid(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = SecondTransitionPropertiesFor(sourceType);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_ACID)
			{
				continue;
			}
			auto acid = ID(packed);
			if (std::max(parts[i].temp, parts[acid].temp) < properties.acidThreshold ||
				!ConsumeEvent(sim))
			{
				continue;
			}
			auto temperature = std::min(
				std::max(parts[i].temp, parts[acid].temp) +
					float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(acid, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[acid], PT_H2);
			parts[i].temp = temperature;
			parts[acid].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure);
			return true;
		}
	}
	return false;
}

bool OxidiseHotSecondTransition(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = SecondTransitionPropertiesFor(sourceType);
	if (parts[i].temp < properties.oxygenThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2 || !ConsumeEvent(sim))
			{
				continue;
			}
			auto oxygen = ID(packed);
			auto temperature = std::min(
				parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_MSCR);
			ResetReactionProduct(parts[i], PT_MSCR);
			parts[i].ctype = sourceType;
			parts[i].temp = temperature;
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[oxygen].temp = temperature;
			parts[oxygen].life = 20;
			parts[oxygen].dcolour = properties.flameColour;
			return true;
		}
	}
	return false;
}

bool VaporiseSecondTransition(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = SecondTransitionPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.flameColour;
	return true;
}

bool UpdateSecondTransition(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsSecondTransitionExtension(sourceType))
	{
		return false;
	}
	if (DecayTechnetium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactZirconiumWithSteam(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (CatalyseHydrogenWithPlatinumGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReleasePalladiumHydrogen(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (AbsorbPalladiumHydrogen(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (TarnishSilverWithSulfur(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseSecondTransition(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactSecondTransitionWithAcid(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (OxidiseHotSecondTransition(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return ExciteYttrium(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool CaptureHafniumNeutron(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_HF)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			for (auto packed : { pmap[y + ry][x + rx], sim->photons[y + ry][x + rx] })
			{
				if (!packed || TYP(packed) != PT_NEUT)
				{
					continue;
				}
				if (!ConsumeEvent(sim))
				{
					return false;
				}
				auto neutron = ID(packed);
				parts[i].temp = std::min(
					std::max(parts[i].temp, parts[neutron].temp) + 90.0f,
					MAX_TEMP);
				parts[i].tmp = std::min(parts[i].tmp + 1, 255);
				sim->kill_part(neutron);
				return true;
			}
		}
	}
	return false;
}

bool PassivateTantalum(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_TA || parts[i].temp < 700.0f || parts[i].life > 0)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2 || !ConsumeEvent(sim))
			{
				continue;
			}
			auto oxygen = ID(packed);
			auto temperature = std::min(
				std::max(parts[i].temp, parts[oxygen].temp) + 80.0f,
				MAX_TEMP);
			sim->part_change_type(oxygen, x + rx, y + ry, PT_GLAS);
			ResetReactionProduct(parts[oxygen], PT_GLAS);
			parts[oxygen].temp = temperature;
			parts[oxygen].dcolour = 0xFFD8D0FF;
			parts[i].life = 120;
			parts[i].temp = temperature;
			AddBoundedPressure(sim, x, y, 0.15f);
			return true;
		}
	}
	return false;
}

bool AlloyRheniumSuperalloy(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_RE || parts[i].temp < 2000.0f)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed)
			{
				continue;
			}
			auto nickel = ID(packed);
			if (TYP(packed) != PT_NICL &&
				(TYP(packed) != PT_LAVA || parts[nickel].ctype != PT_NICL))
			{
				continue;
			}
			if (std::max(parts[i].temp, parts[nickel].temp) < 2000.0f ||
				!ConsumeEvent(sim))
			{
				continue;
			}
			auto temperature = std::min(
				std::max(parts[i].temp, parts[nickel].temp) + 100.0f,
				MAX_TEMP);
			sim->part_change_type(i, x, y, PT_LAVA);
			sim->part_change_type(nickel, x + rx, y + ry, PT_LAVA);
			ResetReactionProduct(parts[i], PT_LAVA);
			ResetReactionProduct(parts[nickel], PT_LAVA);
			parts[i].ctype = PT_TSTL;
			parts[nickel].ctype = PT_TSTL;
			parts[i].temp = temperature;
			parts[nickel].temp = temperature;
			return true;
		}
	}
	return false;
}

bool OxidiseOsmiumToToxicVapour(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_OS || parts[i].temp < 450.0f)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2 || !ConsumeEvent(sim))
			{
				continue;
			}
			auto oxygen = ID(packed);
			auto temperature = std::min(
				std::max(parts[i].temp, parts[oxygen].temp) + 170.0f,
				MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SMKE);
			ResetReactionProduct(parts[i], PT_SMKE);
			parts[i].ctype = PT_OS;
			parts[i].life = 80;
			parts[i].temp = temperature;
			parts[i].dcolour = 0xFFFFFFA0;
			sim->part_change_type(oxygen, x + rx, y + ry, PT_CAUS);
			ResetReactionProduct(parts[oxygen], PT_CAUS);
			parts[oxygen].temp = temperature;
			parts[oxygen].dcolour = 0xFFFFFFA0;
			AddBoundedPressure(sim, x, y, 0.5f);
			return true;
		}
	}
	return false;
}

bool CatalysePeroxideWithIridium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_IR || parts[i].temp < 350.0f)
	{
		return false;
	}
	int peroxide[2] = { -1, -1 };
	int peroxideX[2] = { 0, 0 };
	int peroxideY[2] = { 0, 0 };
	int emptyX = -1;
	int emptyY = -1;
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed)
			{
				if (emptyX < 0)
				{
					emptyX = x + rx;
					emptyY = y + ry;
				}
				continue;
			}
			if (TYP(packed) == PT_PERO)
			{
				int slot = peroxide[0] < 0 ? 0 : (peroxide[1] < 0 ? 1 : -1);
				if (slot >= 0)
				{
					peroxide[slot] = ID(packed);
					peroxideX[slot] = x + rx;
					peroxideY[slot] = y + ry;
				}
			}
		}
	}
	if (peroxide[0] < 0 || peroxide[1] < 0 || emptyX < 0 || !ConsumeEvent(sim))
	{
		return false;
	}
	int oxygen = sim->create_part(-1, emptyX, emptyY, PT_O2);
	if (oxygen < 0)
	{
		return false;
	}
	auto temperature = std::min(parts[i].temp + 90.0f, MAX_TEMP);
	for (int slot = 0; slot < 2; ++slot)
	{
		sim->part_change_type(
			peroxide[slot], peroxideX[slot], peroxideY[slot], PT_WATR);
		ResetReactionProduct(parts[peroxide[slot]], PT_WATR);
		parts[peroxide[slot]].temp = temperature;
	}
	parts[oxygen].temp = temperature;
	parts[i].temp = std::min(parts[i].temp + 15.0f, MAX_TEMP);
	return true;
}

bool ReactThirdTransitionWithAcid(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = ThirdTransitionPropertiesFor(sourceType);
	if (sourceType == PT_TA && parts[i].life > 0)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_ACID)
			{
				continue;
			}
			auto acid = ID(packed);
			if (std::max(parts[i].temp, parts[acid].temp) < properties.acidThreshold ||
				!ConsumeEvent(sim))
			{
				continue;
			}
			auto temperature = std::min(
				std::max(parts[i].temp, parts[acid].temp) +
					float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(acid, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[acid], PT_H2);
			parts[i].temp = temperature;
			parts[acid].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure);
			return true;
		}
	}
	return false;
}

bool OxidiseHotThirdTransition(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = ThirdTransitionPropertiesFor(sourceType);
	if (parts[i].temp < properties.oxygenThreshold ||
		(sourceType == PT_TA && parts[i].life > 0))
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2 || !ConsumeEvent(sim))
			{
				continue;
			}
			auto oxygen = ID(packed);
			auto temperature = std::min(
				parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_MSCR);
			ResetReactionProduct(parts[i], PT_MSCR);
			parts[i].ctype = sourceType;
			parts[i].temp = temperature;
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[oxygen].temp = temperature;
			parts[oxygen].life = 20;
			parts[oxygen].dcolour = properties.flameColour;
			return true;
		}
	}
	return false;
}

bool VaporiseThirdTransition(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = ThirdTransitionPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.flameColour;
	return true;
}

bool UpdateThirdTransition(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsThirdTransitionExtension(sourceType))
	{
		return false;
	}
	if (CaptureHafniumNeutron(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (PassivateTantalum(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (AlloyRheniumSuperalloy(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (OxidiseOsmiumToToxicVapour(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (CatalysePeroxideWithIridium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseThirdTransition(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactThirdTransitionWithAcid(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return OxidiseHotThirdTransition(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool ReleaseLanthanumHydrogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_LA || parts[i].tmp <= 0 || parts[i].temp < 900.0f)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry) ||
				pmap[y + ry][x + rx])
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}
			int hydrogen = sim->create_part(-1, x + rx, y + ry, PT_H2);
			if (hydrogen < 0)
			{
				return false;
			}
			parts[hydrogen].temp = parts[i].temp;
			--parts[i].tmp;
			AddBoundedPressure(sim, x, y, 0.15f);
			return true;
		}
	}
	return false;
}

bool AbsorbLanthanumHydrogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_LA || parts[i].tmp >= 4 || parts[i].temp < 300.0f ||
		parts[i].temp >= 700.0f)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_H2 || !ConsumeEvent(sim))
			{
				continue;
			}
			auto hydrogen = ID(packed);
			parts[i].temp = std::min(
				std::max(parts[i].temp, parts[hydrogen].temp) + 20.0f, MAX_TEMP);
			parts[i].tmp = std::min(parts[i].tmp + 1, 4);
			sim->kill_part(hydrogen);
			return true;
		}
	}
	return false;
}

bool ReleaseCeriumOxygen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_CE || parts[i].tmp <= 0 || parts[i].temp < 1050.0f)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry) ||
				pmap[y + ry][x + rx])
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}
			int oxygen = sim->create_part(-1, x + rx, y + ry, PT_O2);
			if (oxygen < 0)
			{
				return false;
			}
			parts[oxygen].temp = parts[i].temp;
			--parts[i].tmp;
			return true;
		}
	}
	return false;
}

bool AbsorbCeriumOxygen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_CE || parts[i].tmp >= 4 || parts[i].temp < 400.0f ||
		parts[i].temp >= 750.0f)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2 || !ConsumeEvent(sim))
			{
				continue;
			}
			auto oxygen = ID(packed);
			parts[i].temp = std::min(
				std::max(parts[i].temp, parts[oxygen].temp) + 30.0f, MAX_TEMP);
			parts[i].tmp = std::min(parts[i].tmp + 1, 4);
			sim->kill_part(oxygen);
			return true;
		}
	}
	return false;
}

bool DecayPromethium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_PM || parts[i].life > 0)
	{
		return false;
	}
	if (!ConsumeEvent(sim))
	{
		parts[i].life = 1;
		return false;
	}
	auto temperature = std::min(parts[i].temp + 180.0f, MAX_TEMP);
	if (parts[i].type == PT_LAVA)
	{
		parts[i].ctype = PT_SM;
		parts[i].tmp = 0;
		parts[i].tmp2 = 0;
		parts[i].life = 0;
	}
	else
	{
		sim->part_change_type(i, x, y, PT_SM);
		ResetReactionProduct(parts[i], PT_SM);
	}
	parts[i].temp = temperature;
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x0003FFF0;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool CaptureLanthanideNeutron(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_SM && sourceType != PT_GD && sourceType != PT_LU)
	{
		return false;
	}
	float captureHeat = sourceType == PT_GD ? 180.0f :
		(sourceType == PT_SM ? 90.0f : 60.0f);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			for (auto packed : { pmap[y + ry][x + rx], sim->photons[y + ry][x + rx] })
			{
				if (!packed || TYP(packed) != PT_NEUT)
				{
					continue;
				}
				if (!ConsumeEvent(sim))
				{
					return false;
				}
				auto neutron = ID(packed);
				parts[i].temp = std::min(
					std::max(parts[i].temp, parts[neutron].temp) + captureHeat,
					MAX_TEMP);
				parts[i].tmp = std::min(parts[i].tmp + 1, 255);
				parts[i].life = std::max(parts[i].life, 30);
				sim->kill_part(neutron);
				return true;
			}
		}
	}
	return false;
}

bool MagnetiseLanthanide(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_PR && sourceType != PT_ND && sourceType != PT_SM &&
		sourceType != PT_GD && sourceType != PT_DY && sourceType != PT_HO)
	{
		return false;
	}
	if (parts[i].life > 0 || !HasLocalDischarge(x, y, pmap, sim) ||
		!ConsumeEvent(sim))
	{
		return false;
	}
	int intensity = sourceType == PT_DY ? 80 :
		(sourceType == PT_HO ? 75 : (sourceType == PT_ND ? 70 : 55));
	parts[i].life = intensity;
	parts[i].tmp2 = std::min(parts[i].tmp2 + 1, 255);
	parts[i].temp = std::min(parts[i].temp + float(intensity) * 0.25f, MAX_TEMP);
	return true;
}

bool FluoresceLanthanide(UPDATE_FUNC_ARGS, int sourceType)
{
	if ((sourceType != PT_EU && sourceType != PT_TB) || parts[i].life > 0)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = sim->photons[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_PHOT || !ConsumeEvent(sim))
			{
				continue;
			}
			auto photon = ID(packed);
			parts[photon].ctype = sourceType == PT_EU ? 0x000FF000 : 0x00003FF0;
			parts[photon].life = std::max(parts[photon].life, 18);
			parts[i].life = 60;
			parts[i].temp = std::min(parts[i].temp + 10.0f, MAX_TEMP);
			return true;
		}
	}
	return false;
}

bool AmplifyLanthanidePhoton(UPDATE_FUNC_ARGS, int sourceType)
{
	if ((sourceType != PT_ER && sourceType != PT_TM) || parts[i].life > 0 ||
		!HasLocalPhoton(x, y, sim))
	{
		return false;
	}
	if (!ConsumeEvent(sim))
	{
		return false;
	}
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon < 0)
	{
		return false;
	}
	parts[photon].ctype = sourceType == PT_ER ? 0x00003FF0 : 0x03F00000;
	parts[photon].temp = std::min(parts[i].temp + 20.0f, MAX_TEMP);
	parts[photon].life = 18;
	parts[i].life = 45;
	parts[i].temp = std::min(parts[i].temp + 12.0f, MAX_TEMP);
	return true;
}

bool ReactYtterbiumWithWater(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_YB || parts[i].temp < 330.0f)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || !IsWaterLike(TYP(packed)) || !ConsumeEvent(sim))
			{
				continue;
			}
			auto water = ID(packed);
			auto temperature = std::min(
				std::max(parts[i].temp, parts[water].temp) + 155.0f, MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(water, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[water], PT_H2);
			parts[i].temp = temperature;
			parts[water].temp = temperature;
			AddBoundedPressure(sim, x, y, 0.6f);
			return true;
		}
	}
	return false;
}

bool ReactLanthanideWithAcid(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = LanthanidePropertiesFor(sourceType);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_ACID)
			{
				continue;
			}
			auto acid = ID(packed);
			if (std::max(parts[i].temp, parts[acid].temp) < properties.acidThreshold ||
				!ConsumeEvent(sim))
			{
				continue;
			}
			auto temperature = std::min(
				std::max(parts[i].temp, parts[acid].temp) +
					float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(acid, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[acid], PT_H2);
			parts[i].temp = temperature;
			parts[acid].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure);
			return true;
		}
	}
	return false;
}

bool OxidiseHotLanthanide(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = LanthanidePropertiesFor(sourceType);
	if (parts[i].temp < properties.oxygenThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2 || !ConsumeEvent(sim))
			{
				continue;
			}
			auto oxygen = ID(packed);
			auto temperature = std::min(
				parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_MSCR);
			ResetReactionProduct(parts[i], PT_MSCR);
			parts[i].ctype = sourceType;
			parts[i].temp = temperature;
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[oxygen].temp = temperature;
			parts[oxygen].life = 20;
			parts[oxygen].dcolour = properties.emissionColour;
			return true;
		}
	}
	return false;
}

bool VaporiseLanthanide(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = LanthanidePropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.emissionColour;
	return true;
}

bool UpdateLanthanide(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsLanthanide(sourceType))
	{
		return false;
	}
	if (DecayPromethium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReleaseLanthanumHydrogen(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		AbsorbLanthanumHydrogen(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		ReleaseCeriumOxygen(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		AbsorbCeriumOxygen(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		CaptureLanthanideNeutron(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		FluoresceLanthanide(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		AmplifyLanthanidePhoton(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		MagnetiseLanthanide(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		ReactYtterbiumWithWater(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		VaporiseLanthanide(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		ReactLanthanideWithAcid(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return OxidiseHotLanthanide(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

void InitialiseActinideState(Simulation *sim, Particle &particle, int type)
{
	auto properties = ActinidePropertiesFor(type);
	particle.tmp = 0;
	particle.tmp2 = 0;
	particle.life = properties.lifeMin > 0 ?
		sim->rng.between(properties.lifeMin, properties.lifeMax) : 0;
}

void InitialiseSuperheavyState(Simulation *sim, Particle &particle, int type)
{
	auto properties = SuperheavyPropertiesFor(type);
	particle.tmp = 0;
	particle.tmp2 = 0;
	particle.life = properties.lifeMin > 0 ?
		sim->rng.between(properties.lifeMin, properties.lifeMax) : 0;
}

void ChangeActinideProduct(UPDATE_FUNC_ARGS, int product)
{
	if (parts[i].type == PT_LAVA &&
		(IsActinideExtension(product) || IsSuperheavyTransition(product)))
	{
		parts[i].ctype = product;
		if (IsActinideExtension(product))
		{
			InitialiseActinideState(sim, parts[i], product);
		}
		else
		{
			InitialiseSuperheavyState(sim, parts[i], product);
		}
		return;
	}
	sim->part_change_type(i, x, y, product);
	ResetReactionProduct(parts[i], product);
	if (IsActinideExtension(product))
	{
		InitialiseActinideState(sim, parts[i], product);
	}
	else if (IsSuperheavyTransition(product))
	{
		InitialiseSuperheavyState(sim, parts[i], product);
	}
	else if (product == PT_FR)
	{
		parts[i].tmp = sim->rng.between(180, 360);
	}
	else if (product == PT_RA)
	{
		parts[i].tmp = sim->rng.between(900, 1800);
	}
}

bool DecayActinide(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsActinideExtension(sourceType) || parts[i].life > 0)
	{
		return false;
	}
	auto properties = ActinidePropertiesFor(sourceType);
	if (!ConsumeEvent(sim))
	{
		parts[i].life = 1;
		return false;
	}
	auto temperature = std::min(parts[i].temp + properties.decayHeat, MAX_TEMP);
	ChangeActinideProduct(UPDATE_FUNC_SUBCALL_ARGS, properties.decayProduct);
	parts[i].temp = temperature;
	int radiationType = sourceType == PT_CF ? PT_NEUT : PT_PHOT;
	int radiation = sim->create_part(-3, x, y, radiationType);
	if (radiation >= 0)
	{
		parts[radiation].temp = temperature;
		parts[radiation].life = radiationType == PT_NEUT ? 24 : 18;
		if (radiationType == PT_PHOT)
		{
			parts[radiation].ctype = 0x0003FFF0;
		}
	}
	AddBoundedPressure(sim, x, y, properties.pressure);
	return true;
}

bool CaptureActinideNeutron(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = ActinidePropertiesFor(sourceType);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			for (auto packed : { pmap[y + ry][x + rx], sim->photons[y + ry][x + rx] })
			{
				if (!packed || TYP(packed) != PT_NEUT)
				{
					continue;
				}
				auto neutron = ID(packed);
				auto neutronTemperature = parts[neutron].temp;
				if (sourceType == PT_LR &&
					std::max(parts[i].temp, neutronTemperature) < 900.0f)
				{
					continue;
				}
				bool fission = sourceType == PT_CF &&
					std::max(parts[i].temp, neutronTemperature) >= 900.0f;
				if (!ConsumeEvent(sim))
				{
					return false;
				}
				int captureCount = std::min(parts[i].tmp + 1, 255);
				auto temperature = std::min(
					std::max(parts[i].temp, neutronTemperature) +
						properties.captureHeat + (fission ? 280.0f : 0.0f),
					MAX_TEMP);
				sim->kill_part(neutron);
				int product = fission ? PT_CM : properties.captureProduct;
				if (product != NT)
				{
					ChangeActinideProduct(UPDATE_FUNC_SUBCALL_ARGS, product);
				}
				else
				{
					parts[i].life = std::max(parts[i].life, 60);
				}
				parts[i].tmp = captureCount;
				parts[i].temp = temperature;
				if (fission)
				{
					int outgoing = sim->create_part(-3, x, y, PT_NEUT);
					if (outgoing >= 0)
					{
						parts[outgoing].temp = temperature;
						parts[outgoing].life = 24;
					}
					AddBoundedPressure(sim, x, y, 0.8f);
				}
				return true;
			}
		}
	}
	return false;
}

bool ReactActinideWithAcid(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = ActinidePropertiesFor(sourceType);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_ACID)
			{
				continue;
			}
			auto acid = ID(packed);
			if (std::max(parts[i].temp, parts[acid].temp) < properties.acidThreshold ||
				!ConsumeEvent(sim))
			{
				continue;
			}
			auto temperature = std::min(
				std::max(parts[i].temp, parts[acid].temp) +
					float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_MSCR);
			ResetReactionProduct(parts[i], PT_MSCR);
			parts[i].ctype = sourceType;
			sim->part_change_type(acid, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[acid], PT_H2);
			parts[i].temp = temperature;
			parts[acid].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure);
			return true;
		}
	}
	return false;
}

bool OxidiseHotActinide(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = ActinidePropertiesFor(sourceType);
	if (parts[i].temp < properties.oxygenThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2 || !ConsumeEvent(sim))
			{
				continue;
			}
			auto oxygen = ID(packed);
			auto temperature = std::min(
				parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_MSCR);
			ResetReactionProduct(parts[i], PT_MSCR);
			parts[i].ctype = sourceType;
			parts[i].temp = temperature;
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[oxygen].temp = temperature;
			parts[oxygen].life = 20;
			parts[oxygen].dcolour = properties.emissionColour;
			return true;
		}
	}
	return false;
}

bool VaporiseActinide(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = ActinidePropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 45;
	parts[i].dcolour = properties.emissionColour;
	return true;
}

bool UpdateActinide(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsActinideExtension(sourceType))
	{
		return false;
	}
	if (DecayActinide(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		CaptureActinideNeutron(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		VaporiseActinide(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		ReactActinideWithAcid(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return OxidiseHotActinide(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

void ChangeSuperheavyProduct(UPDATE_FUNC_ARGS, int product)
{
	if (parts[i].type == PT_LAVA && IsSuperheavyTransition(product))
	{
		parts[i].ctype = product;
		InitialiseSuperheavyState(sim, parts[i], product);
		return;
	}
	sim->part_change_type(i, x, y, product);
	ResetReactionProduct(parts[i], product);
	if (IsSuperheavyTransition(product))
	{
		InitialiseSuperheavyState(sim, parts[i], product);
	}
	else if (product == PT_NH)
	{
		parts[i].tmp = sim->rng.between(90, 180);
	}
}

bool DecaySuperheavy(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsSuperheavyTransition(sourceType) || parts[i].life > 0)
	{
		return false;
	}
	auto properties = SuperheavyPropertiesFor(sourceType);
	if (!ConsumeEvent(sim))
	{
		parts[i].life = 1;
		return false;
	}
	auto temperature = std::min(parts[i].temp + properties.decayHeat, MAX_TEMP);
	ChangeSuperheavyProduct(UPDATE_FUNC_SUBCALL_ARGS, properties.decayProduct);
	parts[i].temp = temperature;
	int radiation = sim->create_part(-3, x, y, properties.radiationType);
	if (radiation >= 0)
	{
		parts[radiation].temp = temperature;
		parts[radiation].life = properties.radiationType == PT_NEUT ? 24 : 18;
		if (properties.radiationType == PT_PHOT)
		{
			parts[radiation].ctype = 0x03F03F00;
		}
	}
	AddBoundedPressure(sim, x, y, properties.pressure);
	return true;
}

bool SynthesizeSuperheavy(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = SuperheavyPropertiesFor(sourceType);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			for (auto packed : { pmap[y + ry][x + rx], sim->photons[y + ry][x + rx] })
			{
				if (!packed || TYP(packed) != PT_NEUT)
				{
					continue;
				}
				auto neutron = ID(packed);
				auto collisionTemperature =
					std::max(parts[i].temp, parts[neutron].temp);
				if (collisionTemperature < properties.synthesisThreshold)
				{
					continue;
				}
				if (!ConsumeEvent(sim))
				{
					return false;
				}
				int captureCount = std::min(parts[i].tmp2 + 1, 255);
				auto temperature = std::min(
					collisionTemperature + properties.captureHeat, MAX_TEMP);
				sim->kill_part(neutron);
				ChangeSuperheavyProduct(
					UPDATE_FUNC_SUBCALL_ARGS, properties.captureProduct);
				parts[i].tmp2 = captureCount;
				parts[i].temp = temperature;
				AddBoundedPressure(sim, x, y, properties.pressure * 0.5f);
				return true;
			}
		}
	}
	return false;
}

bool ReactSuperheavyWithAcid(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = SuperheavyPropertiesFor(sourceType);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_ACID)
			{
				continue;
			}
			auto acid = ID(packed);
			if (std::max(parts[i].temp, parts[acid].temp) < properties.acidThreshold ||
				!ConsumeEvent(sim))
			{
				continue;
			}
			auto temperature = std::min(
				std::max(parts[i].temp, parts[acid].temp) +
					float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_MSCR);
			ResetReactionProduct(parts[i], PT_MSCR);
			parts[i].ctype = sourceType;
			sim->part_change_type(acid, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[acid], PT_H2);
			parts[i].temp = temperature;
			parts[acid].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure);
			return true;
		}
	}
	return false;
}

bool OxidiseHotSuperheavy(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = SuperheavyPropertiesFor(sourceType);
	if (parts[i].temp < properties.oxygenThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2 || !ConsumeEvent(sim))
			{
				continue;
			}
			auto oxygen = ID(packed);
			auto temperature = std::min(
				parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_MSCR);
			ResetReactionProduct(parts[i], PT_MSCR);
			parts[i].ctype = sourceType;
			parts[i].temp = temperature;
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[oxygen].temp = temperature;
			parts[oxygen].life = 20;
			parts[oxygen].dcolour = properties.emissionColour;
			return true;
		}
	}
	return false;
}

bool VaporiseSuperheavy(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = SuperheavyPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 36;
	parts[i].dcolour = properties.emissionColour;
	return true;
}

bool UpdateSuperheavy(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsSuperheavyTransition(sourceType))
	{
		return false;
	}
	if (DecaySuperheavy(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		SynthesizeSuperheavy(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		VaporiseSuperheavy(UPDATE_FUNC_SUBCALL_ARGS, sourceType) ||
		ReactSuperheavyWithAcid(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return OxidiseHotSuperheavy(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}
}

int OmniNobleGasUpdate(UPDATE_FUNC_ARGS)
{
	if (DecayRadioactiveGas(UPDATE_FUNC_SUBCALL_ARGS))
	{
		return 1;
	}
	ExchangeCryogenicHeat(UPDATE_FUNC_SUBCALL_ARGS);
	EmitDischarge(UPDATE_FUNC_SUBCALL_ARGS);
	return 0;
}

int OmniNobleGasGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->life > 0)
	{
		int intensity = std::min(cpart->life * 14, 150);
		*colr = std::min(*colr + intensity / 2, 255);
		*colg = std::min(*colg + intensity / 2, 255);
		*colb = std::min(*colb + intensity / 2, 255);
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*firea = intensity;
		*pixel_mode |= PMODE_GLOW | PMODE_ADD | FIRE_ADD;
	}
	else if (cpart->type == PT_RN || cpart->type == PT_OG)
	{
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void OmniNobleGasCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_RN)
	{
		sim->parts[i].tmp = sim->rng.between(1200, 2400);
	}
	else if (t == PT_OG)
	{
		sim->parts[i].tmp = sim->rng.between(45, 120);
	}
}

int OmniAlkaliMetalUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateAlkali(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenAlkaliUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateAlkali(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

void OmniAlkaliMetalCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_FR)
	{
		sim->parts[i].tmp = sim->rng.between(180, 360);
	}
}

int OmniAlkalineEarthMetalUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateAlkalineEarth(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenAlkalineEarthUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateAlkalineEarth(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

void OmniAlkalineEarthMetalCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_RA)
	{
		sim->parts[i].tmp = sim->rng.between(900, 1800);
	}
}

int OmniBoronGroupUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateBoronGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenBoronGroupUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateBoronGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

void OmniBoronGroupCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_NH)
	{
		sim->parts[i].tmp = sim->rng.between(90, 180);
	}
}

int OmniCarbonGroupUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateCarbonGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenCarbonGroupUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateCarbonGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

int OmniCarbonGroupGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->type == PT_GE && cpart->life > 0)
	{
		int intensity = std::min(cpart->life * 12, 120);
		*firea = intensity;
		*firer = std::min(*colr + 40, 255);
		*fireg = std::min(*colg + 55, 255);
		*fireb = 255;
		*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	}
	else if (cpart->type == PT_FL)
	{
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void OmniCarbonGroupCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_FL)
	{
		sim->parts[i].tmp = sim->rng.between(60, 130);
	}
}

int OmniNitrogenGroupUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateNitrogenGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenNitrogenGroupUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateNitrogenGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

int OmniNitrogenGroupGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->type == PT_N && cpart->life > 0)
	{
		int intensity = std::min(cpart->life * 10, 120);
		*firea = intensity;
		*firer = std::min(*colr + 60, 255);
		*fireg = std::min(*colg + 20, 255);
		*fireb = 255;
		*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	}
	else if (cpart->type == PT_MC)
	{
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void OmniNitrogenGroupCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_MC)
	{
		sim->parts[i].tmp = sim->rng.between(50, 110);
	}
}

int OmniOxygenGroupUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateOxygenGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenOxygenGroupUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateOxygenGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

int OmniOxygenGroupGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->type == PT_SE && cpart->life > 0)
	{
		int intensity = std::min(cpart->life * 10, 120);
		*firea = intensity;
		*firer = std::min(*colr + 40, 255);
		*fireg = std::min(*colg + 70, 255);
		*fireb = 255;
		*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	}
	else if (cpart->type == PT_LV)
	{
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void OmniOxygenGroupCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_LV)
	{
		sim->parts[i].tmp = sim->rng.between(40, 90);
	}
}

int OmniHalogenUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateHalogen(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenHalogenUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateHalogen(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

int OmniHalogenGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->type == PT_AT || cpart->type == PT_TS)
	{
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void OmniHalogenCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_AT)
	{
		sim->parts[i].tmp = sim->rng.between(240, 480);
	}
	else if (t == PT_TS)
	{
		sim->parts[i].tmp = sim->rng.between(35, 75);
	}
}

int OmniFirstTransitionUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateFirstTransition(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenFirstTransitionUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateFirstTransition(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

int OmniFirstTransitionGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->type == PT_SC && cpart->life > 0)
	{
		int intensity = std::min(cpart->life * 10, 120);
		*firea = intensity;
		*firer = std::min(*colr + 35, 255);
		*fireg = std::min(*colg + 55, 255);
		*fireb = 255;
		*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	}
	return 0;
}

int OmniSecondTransitionUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateSecondTransition(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenSecondTransitionUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateSecondTransition(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

int OmniSecondTransitionGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->type == PT_Y && cpart->life > 0)
	{
		int intensity = std::min(cpart->life * 10, 120);
		*firea = intensity;
		*firer = std::min(*colr + 25, 255);
		*fireg = 255;
		*fireb = std::min(*colb + 40, 255);
		*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	}
	else if (cpart->type == PT_TC)
	{
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void OmniSecondTransitionCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_TC)
	{
		sim->parts[i].tmp = sim->rng.between(600, 1200);
	}
}

int OmniThirdTransitionUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateThirdTransition(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenThirdTransitionUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateThirdTransition(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

int OmniThirdTransitionGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->type == PT_TA && cpart->life > 0)
	{
		int intensity = std::min(cpart->life, 120);
		*firea = intensity;
		*firer = std::min(*colr + 35, 255);
		*fireg = std::min(*colg + 20, 255);
		*fireb = 255;
		*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	}
	return 0;
}

void OmniThirdTransitionCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_HF)
	{
		sim->parts[i].tmp = 0;
	}
}

int OmniLanthanideUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateLanthanide(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenLanthanideUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateLanthanide(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

int OmniLanthanideGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->life > 0)
	{
		auto colour = LanthanidePropertiesFor(cpart->type).emissionColour;
		int intensity = std::min(cpart->life * 3, 150);
		*firea = intensity;
		*firer = int((colour >> 16) & 0xFF);
		*fireg = int((colour >> 8) & 0xFF);
		*fireb = int(colour & 0xFF);
		*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	}
	return 0;
}

void OmniLanthanideCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp = 0;
	sim->parts[i].tmp2 = 0;
	if (t == PT_PM)
	{
		sim->parts[i].life = sim->rng.between(180, 360);
	}
	else
	{
		sim->parts[i].life = 0;
	}
}

int OmniActinideUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateActinide(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenActinideUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateActinide(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

int OmniActinideGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->life > 0)
	{
		auto properties = ActinidePropertiesFor(cpart->type);
		int intensity = std::min(
			std::max(24, 150 - std::min(cpart->life / 8, 120)), 150);
		*firea = intensity;
		*firer = int((properties.emissionColour >> 16) & 0xFF);
		*fireg = int((properties.emissionColour >> 8) & 0xFF);
		*fireb = int(properties.emissionColour & 0xFF);
		*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	}
	return 0;
}

void OmniActinideCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	InitialiseActinideState(sim, sim->parts[i], t);
}

int OmniSuperheavyUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateSuperheavy(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenSuperheavyUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateSuperheavy(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

int OmniSuperheavyGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->life > 0)
	{
		auto properties = SuperheavyPropertiesFor(cpart->type);
		int intensity = std::min(
			std::max(36, 170 - std::min(cpart->life / 5, 130)), 170);
		*firea = intensity;
		*firer = int((properties.emissionColour >> 16) & 0xFF);
		*fireg = int((properties.emissionColour >> 8) & 0xFF);
		*fireb = int(properties.emissionColour & 0xFF);
		*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	}
	return 0;
}

void OmniSuperheavyCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	InitialiseSuperheavyState(sim, sim->parts[i], t);
}
