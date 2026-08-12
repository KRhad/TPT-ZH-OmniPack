#include "Simulation.h"
#include "Air.h"
#include "OmniAtmosphere.h"
#include "OmniThermal.h"
#include "OmniReactionRuntime.h"
#include "OmniSolution.h"
#include "ElementClasses.h"
#include "TransitionConstants.h"
#include "gravity/Gravity.h"
#include "ToolClasses.h"
#include "SimulationData.h"
#include "OmniMetallurgy.h"
#include "OmniMaterials.h"
#include "OmniIsotopes.h"
#include "client/GameSave.h"
#include "common/tpt-rand.h"
#include "common/Defer.h"
#include "FrameTime.h"
#include "gui/game/Brush.h"
#include "elements/EMP.h"
#include "elements/LOLZ.h"
#include "elements/STKM.h"
#include "elements/PIPE.h"
#include "elements/FILT.h"
#include "elements/PRTI.h"
#include "elements/PLNT.h"
#include <bit>
#include <iostream>
#include <numbers>
#include <set>

namespace
{
	bool IsFiniteDoubleBits(double value)
	{
		return (std::bit_cast<uint64_t>(value) & UINT64_C(0x7FF0000000000000)) !=
			UINT64_C(0x7FF0000000000000);
	}

	bool IsFiniteFloatBits(float value)
	{
		return (std::bit_cast<uint32_t>(value) & UINT32_C(0x7F800000)) !=
			UINT32_C(0x7F800000);
	}

	bool IsOmniManagedWaterType(int type)
	{
		// Only pure-water parcels whose complete phase/coupling path is owned by
		// 1.0.7 belong here. Solutions, aerosols and Legacy water variants retain
		// their upstream semantics until composition ownership exists in 1.0.9.
		return type == PT_WATR || type == PT_ICEI || type == PT_WTRV;
	}

	bool IsOmniManagedCarbonType(int type)
	{
		return type == PT_COAL || type == PT_BCOL;
	}

	bool IsOmniManagedSolutionType(int type)
	{
		return type == PT_SALT || type == PT_SLTW || type == PT_ACID || type == PT_BASE;
	}

	constexpr double CarbonMolarMassKgPerMol = 0.0120107;
	constexpr double OxygenMolarMassKgPerMol = 0.0319988;
	constexpr double CarbonDioxideMolarMassKgPerMol = CarbonMolarMassKgPerMol + OxygenMolarMassKgPerMol;

	const OmniReactionRuntime &CarbonOxidationRuntime()
	{
		static const OmniReactionRuntime runtime = OmniReactionRuntime::CarbonOxidationV1();
		return runtime;
	}

	bool IsRefractingGlass(int type)
	{
		return IsGlassMaterialType(type);
	}

	bool OmniAtmosphereWallBlocked(const Simulation &sim, int x, int y)
	{
		const int wall = sim.bmap[y][x];
		return wall == WL_WALL || wall == WL_WALLELEC || wall == WL_BLOCKAIR ||
			(wall == WL_EWALL && !sim.emap[y][x]);
	}

	void SyncOmniAtmosphereBoundary(Simulation &sim)
	{
		if (!sim.omniAtmosphere)
			return;
		sim.omniAtmosphere->SetReferenceState(
			OmniPhysicalScale::ReferenceDensityKgM3,
			sim.air ? sim.air->ambientAirTemp : OmniPhysicalScale::ReferenceTemperatureK);
		double gravityY = 0.0;
		if (sim.gravityMode == GRAV_VERTICAL)
			gravityY = 9.80665;
		else if (sim.gravityMode == GRAV_CUSTOM)
			gravityY = double(sim.customGravityY) * OmniPhysicalScale::PixelLengthM /
				(OmniPhysicalScale::TimestepS * OmniPhysicalScale::TimestepS);
		// A scalar Y source cannot represent radial gravity; that mode remains an
		// explicit zero-atmosphere-gravity fallback until a 2D source field exists.
		sim.omniAtmosphere->SetGravityY(gravityY);
		for (int y = 0; y < YCELLS; ++y)
		{
			for (int x = 0; x < XCELLS; ++x)
				sim.omniAtmosphere->SetBlocked(x, y, OmniAtmosphereWallBlocked(sim, x, y));
		}
		switch (sim.edgeMode)
		{
		case EDGE_LOOP:
			sim.omniAtmosphere->SetBoundaryMode(OmniAtmosphereBoundary::Periodic);
			break;
		case EDGE_SOLID:
			sim.omniAtmosphere->SetBoundaryMode(OmniAtmosphereBoundary::Sealed);
			break;
		default:
			sim.omniAtmosphere->SetBoundaryMode(OmniAtmosphereBoundary::Open);
			break;
		}
	}

	void ExportOmniAtmosphereToLegacyFields(Simulation &sim)
	{
		if (!sim.omniAtmosphere)
			return;
		for (int y = 0; y < YCELLS; ++y)
		{
			for (int x = 0; x < XCELLS; ++x)
			{
				sim.pv[y][x] = sim.omniLegacyPressureShadow[y][x] =
					sim.omniAtmosphere->LegacyPressure(x, y);
				sim.vx[y][x] = sim.omniLegacyVelocityXShadow[y][x] =
					sim.omniAtmosphere->LegacyVelocityX(x, y);
				sim.vy[y][x] = sim.omniLegacyVelocityYShadow[y][x] =
					sim.omniAtmosphere->LegacyVelocityY(x, y);
				sim.hv[y][x] = sim.omniLegacyTemperatureShadow[y][x] =
					sim.omniAtmosphere->LegacyTemperature(x, y);
			}
		}
		sim.omniLegacyProjectionShadowValid = true;
	}

	void ImportLegacyAtmosphereSources(Simulation &sim)
	{
		if (!sim.omniAtmosphere)
			return;
		const auto &config = sim.omniAtmosphere->Config();
		for (int y = 0; y < YCELLS; ++y)
		{
			for (int x = 0; x < XCELLS; ++x)
			{
				if (sim.omniAtmosphere->IsBlocked(x, y))
					continue;
				if (sim.omniLegacyProjectionShadowValid &&
					sim.pv[y][x] == sim.omniLegacyPressureShadow[y][x] &&
					sim.vx[y][x] == sim.omniLegacyVelocityXShadow[y][x] &&
					sim.vy[y][x] == sim.omniLegacyVelocityYShadow[y][x] &&
					sim.hv[y][x] == sim.omniLegacyTemperatureShadow[y][x])
				{
					continue;
				}
				const double velocityX = IsFiniteFloatBits(sim.vx[y][x]) ? sim.vx[y][x] : 0.0;
				const double velocityY = IsFiniteFloatBits(sim.vy[y][x]) ? sim.vy[y][x] : 0.0;
				const double legacyPressure = IsFiniteFloatBits(sim.pv[y][x]) ? sim.pv[y][x] : 0.0;
				const double pressure = std::max(
					config.referencePressure + legacyPressure * config.legacyPressureScalePa,
					config.pressureFloor);
				const double legacyTemperature = IsFiniteFloatBits(sim.hv[y][x])
					? sim.hv[y][x]
					: config.referenceTemperature;
				const double temperature = std::clamp(legacyTemperature, 1.0, double(MAX_TEMP));
				const auto current = sim.omniAtmosphere->Primitive(x, y);
				const double mixtureGasConstant = current.finite && current.mixtureGasConstant > 0.0
					? current.mixtureGasConstant
					: config.gasConstant;
				const double density = pressure / (mixtureGasConstant * temperature);
				const double kinetic = 0.5 * density * (velocityX * velocityX + velocityY * velocityY);
				sim.omniAtmosphere->ImportLegacyProjection(x, y, {
					density,
					density * velocityX,
					density * velocityY,
					// ImportLegacyProjection decodes this compatibility value back
					// into the requested temperature before rebuilding mixture energy.
					density * config.gasConstant * temperature / (config.gamma - 1.0) + kinetic,
				});
			}
		}
	}

	struct SimulationImpl : public Simulation
	{
		struct Neighbourhood
		{
			std::array<int, 8> surround;
			int surround_space = 0;
			int nt = 0; //if nt is greater than 1 after this, then there is a particle around the current particle, that is NOT the current particle's type, for water movement.
			float pGravX = 0;
			float pGravY = 0;
		};
		void MovementPhase(int i, Neighbourhood neighbourhood);
		Neighbourhood GetNeighbourhood(int i) const;
		bool TransitionPhase(int i, const Neighbourhood &neighbourhood);

		void UpdateParticles(int start, int end) final override;
	};
}

static float remainder_p(float x, float y)
{
	return std::fmod(x, y) + (x>=0 ? 0 : y);
}

void Simulation::Load(const GameSave *save, bool includePressure, Vec2<int> blockP) // block coordinates
{
	auto partP = blockP * CELL;

	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

	RecalcFreeParticles(false);

	struct ExistingParticle
	{
		int id;
		Vec2<int> pos;
	};
	std::vector<ExistingParticle> existingParticles;
	auto pasteArea = RES.OriginRect() & RectSized(partP, save->blockSize * CELL);
	for (int i = 0; i < parts.active; i++)
	{
		if (parts[i].type)
		{
			auto p = Vec2<int>{ int(parts[i].x + 0.5f), int(parts[i].y + 0.5f) };
			if (pasteArea.Contains(p))
			{
				existingParticles.push_back({ i, p });
			}
		}
	}
	std::sort(existingParticles.begin(), existingParticles.end(), [](const auto &lhs, const auto &rhs) {
		return std::tie(lhs.pos.Y, lhs.pos.X) < std::tie(rhs.pos.Y, rhs.pos.X);
	});
	PlaneAdapter<std::vector<size_t>> existingParticleIndices(pasteArea.size, existingParticles.size());
	{
		auto lastPos = Vec2<int>{ -1, -1 }; // not a valid pos in existingParticles
		for (auto it = existingParticles.begin(); it != existingParticles.end(); ++it)
		{
			if (lastPos != it->pos)
			{
				existingParticleIndices[it->pos - pasteArea.pos] = it - existingParticles.begin();
				lastPos = it->pos;
			}
		}
	}
	auto removeExistingParticles = [this, pasteArea, &existingParticles, &existingParticleIndices](Vec2<int> p) {
		auto rp = p - pasteArea.pos;
		if (existingParticleIndices.Size().OriginRect().Contains(rp))
		{
			auto index = existingParticleIndices[rp];
			for (auto it = existingParticles.begin() + index; it != existingParticles.end() && it->pos == p; ++it)
			{
				kill_part(it->id);
			}
			existingParticleIndices[rp] = existingParticles.size();
		}
	};

	auto oldPrettyPowders = pretty_powder;
	pretty_powder = false;
	Defer restorePrettyPowders([this, oldPrettyPowders]() {
		pretty_powder = oldPrettyPowders;
	});
	std::map<unsigned int, unsigned int> soapList;
	for (int n = 0; n < NPART && n < save->particlesCount; n++)
	{
		Particle tempPart = save->particles[n];
		if (tempPart.type <= 0 || tempPart.type >= PT_NUM)
		{
			continue;
		}

		tempPart.x += (float)partP.X;
		tempPart.y += (float)partP.Y;
		int x = int(std::floor(tempPart.x + 0.5f));
		int y = int(std::floor(tempPart.y + 0.5f));

		// Check various scenarios where we are unable to spawn the element, and set type to 0 to block spawning later
		if (!InBounds(x, y))
		{
			continue;
		}

		// Ensure we can spawn this element
		if ((player.spwn == 1 && tempPart.type==PT_STKM) || (player2.spwn == 1 && tempPart.type==PT_STKM2))
		{
			continue;
		}
		if ((tempPart.type == PT_SPAWN || tempPart.type == PT_SPAWN2) && elementCount[tempPart.type])
		{
			continue;
		}
		if (tempPart.type == PT_FIGH && !Element_FIGH_CanAlloc(this))
		{
			continue;
		}
		if (!elements[tempPart.type].Enabled)
		{
			continue;
		}

		if (elements[tempPart.type].CreateAllowed)
		{
			if (!(*(elements[tempPart.type].CreateAllowed))(this, -3, int(tempPart.x + 0.5f), int(tempPart.y + 0.5f), tempPart.type))
			{
				continue;
			}
		}

		removeExistingParticles({ x, y });

		// Allocate particle (this location is guaranteed to be empty due to "full scan" logic above)
		auto i = create_part(-3, x, y, tempPart.type);
		if (i == -1)
		{
			continue;
		}
		parts[i] = tempPart;
		if (save->hasOmniWaterParcelState)
		{
			const bool currentWaterState =
				save->omniWaterParcelStateVersion == GameSave::OmniWaterParcelStateVersion;
			if ((!currentWaterState && save->omniWaterParcelStateVersion !=
					GameSave::OmniWaterParcelLegacyStateVersion) ||
				save->omniWaterParcelMassKg.size() != static_cast<size_t>(save->particlesCount) ||
				(currentWaterState && save->omniWaterParcelSpecificEnthalpyJPerKg.size() !=
					static_cast<size_t>(save->particlesCount)))
			{
				throw std::runtime_error("malformed Omni water parcel save payload");
			}
			omniWaterParcelMassKg[i] = save->omniWaterParcelMassKg[n];
			omniWaterParcelSpecificEnthalpyJPerKg[i] = currentWaterState
				? save->omniWaterParcelSpecificEnthalpyJPerKg[n]
				: OmniThermal::WaterSpecificEnthalpyJPerKg(tempPart.temp);
			if (!IsFiniteDoubleBits(omniWaterParcelMassKg[i]) || omniWaterParcelMassKg[i] < 0.0 ||
				!IsFiniteDoubleBits(omniWaterParcelSpecificEnthalpyJPerKg[i]) ||
				omniWaterParcelSpecificEnthalpyJPerKg[i] < 0.0)
			{
				throw std::runtime_error("invalid Omni water parcel save payload");
			}
		}
		else
		{
			InitializeOmniWaterParcelMass(i, tempPart.type);
		}
		if (save->hasOmniCarbonParcelState)
		{
			if (save->omniCarbonParcelStateVersion != GameSave::OmniCarbonParcelStateVersion ||
				save->omniCarbonParcelMassKg.size() != static_cast<size_t>(save->particlesCount))
				throw std::runtime_error("malformed Omni carbon parcel save payload");
			omniCarbonParcelMassKg[i] = save->omniCarbonParcelMassKg[n];
			if (!IsFiniteDoubleBits(omniCarbonParcelMassKg[i]) || omniCarbonParcelMassKg[i] < 0.0 ||
				(omniCarbonParcelMassKg[i] > 0.0 && !IsOmniManagedCarbonType(tempPart.type)))
			{
				throw std::runtime_error("invalid Omni carbon parcel save payload");
			}
		}
		else
		{
			InitializeOmniCarbonParcelMass(i, tempPart.type);
		}
		if (save->hasOmniSolutionState)
		{
			const bool currentSolutionState =
				save->omniSolutionStateVersion == GameSave::OmniSolutionStateVersion;
			if ((!currentSolutionState && save->omniSolutionStateVersion !=
					GameSave::OmniSolutionLegacyStateVersion) ||
				save->omniSolutionSolventMassKg.size() != static_cast<size_t>(save->particlesCount) ||
				save->omniSolutionSoluteMassKg.size() != static_cast<size_t>(save->particlesCount) ||
				save->omniSolutionNeutralSaltMassKg.size() != static_cast<size_t>(save->particlesCount))
				throw std::runtime_error("malformed Omni solution save payload");
			omniSolutionSolventMassKg[i] = save->omniSolutionSolventMassKg[n];
			omniSolutionSoluteMassKg[i] = save->omniSolutionSoluteMassKg[n];
			omniSolutionNeutralSaltMassKg[i] = save->omniSolutionNeutralSaltMassKg[n];
			if (!IsFiniteDoubleBits(omniSolutionSolventMassKg[i]) ||
				!IsFiniteDoubleBits(omniSolutionSoluteMassKg[i]) ||
				!IsFiniteDoubleBits(omniSolutionNeutralSaltMassKg[i]) ||
				omniSolutionSolventMassKg[i] < 0.0 || omniSolutionSoluteMassKg[i] < 0.0 ||
				omniSolutionNeutralSaltMassKg[i] < 0.0 ||
				((omniSolutionSolventMassKg[i] > 0.0 || omniSolutionSoluteMassKg[i] > 0.0 ||
					omniSolutionNeutralSaltMassKg[i] > 0.0) &&
					tempPart.type != PT_SALT && tempPart.type != PT_SLTW &&
					tempPart.type != PT_ACID && tempPart.type != PT_BASE))
				throw std::runtime_error("invalid Omni solution save payload");
		}
		else
		{
			omniSolutionSolventMassKg[i] = 0.0;
			omniSolutionSoluteMassKg[i] = 0.0;
			omniSolutionNeutralSaltMassKg[i] = 0.0;
			InitializeOmniSolutionState(i, tempPart.type, false);
		}


		switch (parts[i].type)
		{
		case PT_STKM:
			Element_STKM_init_legs(this, &player, i);
			player.spwn = 1;
			player.elem = PT_DUST;

			if ((save->version < Version(93, 0) && parts[i].ctype == SPC_AIR) ||
			        (save->version < Version(88, 0) && parts[i].ctype == OLD_SPC_AIR))
			{
				player.fan = true;
			}
			if (save->stkm.rocketBoots1)
				player.rocketBoots = true;
			if (save->stkm.fan1)
				player.fan = true;
			break;
		case PT_STKM2:
			Element_STKM_init_legs(this, &player2, i);
			player2.spwn = 1;
			player2.elem = PT_DUST;
			if ((save->version < Version(93, 0) && parts[i].ctype == SPC_AIR) ||
			        (save->version < Version(88, 0) && parts[i].ctype == OLD_SPC_AIR))
			{
				player2.fan = true;
			}
			if (save->stkm.rocketBoots2)
				player2.rocketBoots = true;
			if (save->stkm.fan2)
				player2.fan = true;
			break;
		case PT_SPAWN:
			player.spawnID = i;
			break;
		case PT_SPAWN2:
			player2.spawnID = i;
			break;
		case PT_FIGH:
		{
			unsigned int oldTmp = parts[i].tmp;
			parts[i].tmp = Element_FIGH_Alloc(this);
			if (parts[i].tmp >= 0)
			{
				bool fan = false;
				if ((save->version < Version(93, 0) && parts[i].ctype == SPC_AIR)
						|| (save->version < Version(88, 0) && parts[i].ctype == OLD_SPC_AIR))
				{
					fan = true;
					parts[i].ctype = 0;
				}
				fighters[parts[i].tmp].elem = PT_DUST;
				Element_FIGH_NewFighter(this, parts[i].tmp, i, parts[i].ctype);
				if (fan)
					fighters[parts[i].tmp].fan = true;
				for (unsigned int fighNum : save->stkm.rocketBootsFigh)
				{
					if (fighNum == oldTmp)
						fighters[parts[i].tmp].rocketBoots = true;
				}
				for (unsigned int fighNum : save->stkm.fanFigh)
				{
					if (fighNum == oldTmp)
						fighters[parts[i].tmp].fan = true;
				}
			}
			else
			{
				// Should not be possible because we verify with CanAlloc above this
				auto oldType = parts[i].type;
				parts[i].type = 0;
				RecordOmniLifecycleMutation(oldType, PT_NONE, OmniLifecycleMutationKind::LoadFallback);
			}
			break;
		}
		case PT_SOAP:
			soapList.insert(std::pair<unsigned int, unsigned int>(n, i));
			break;
		}
		if (GameSave::PressureInTmp3(parts[i].type) && !includePressure)
		{
			parts[i].tmp3 = 0;
		}
	}
	parts.active = NPART;
	force_stacking_check = true;
	Element_PPIP_ppip_changed = 1;

	// Sort out pmap, just to be on the safe side.
	RecalcFreeParticles(false);

	// fix SOAP links using soapList, a map of old particle ID -> new particle ID
	// loop through every old particle (loaded from save), and convert .tmp / .tmp2
	for (std::map<unsigned int, unsigned int>::iterator iter = soapList.begin(), end = soapList.end(); iter != end; ++iter)
	{
		int i = (*iter).second;
		if ((parts[i].ctype & 0x2) == 2)
		{
			std::map<unsigned int, unsigned int>::iterator n = soapList.find(parts[i].tmp);
			if (n != end)
				parts[i].tmp = n->second;
			// sometimes the proper SOAP isn't found. It should remove the link, but seems to break some saves
			// so just ignore it
		}
		if ((parts[i].ctype & 0x4) == 4)
		{
			std::map<unsigned int, unsigned int>::iterator n = soapList.find(parts[i].tmp2);
			if (n != end)
				parts[i].tmp2 = n->second;
			// sometimes the proper SOAP isn't found. It should remove the link, but seems to break some saves
			// so just ignore it
		}
	}

	for (size_t i = 0; i < save->signs.size() && signs.size() < MAXSIGNS; i++)
	{
		if (save->signs[i].text.length())
		{
			sign tempSign = save->signs[i];
			tempSign.x += partP.X;
			tempSign.y += partP.Y;
			if (!InBounds(tempSign.x, tempSign.y))
			{
				continue;
			}
			signs.push_back(tempSign);
		}
	}
	auto useGravityMaps = save->hasGravityMaps && grav;
	auto targetBlocks = RectSized(blockP + save->blockContent.TopLeft(), save->blockContent.size) & CELLS.OriginRect();
	for (auto bpos : targetBlocks)
	{
		auto spos = bpos - blockP;
		if (save->blockMap[spos])
		{
			bmap[bpos.Y][bpos.X] = save->blockMap[spos];
			fvx[bpos.Y][bpos.X] = save->fanVelX[spos];
			fvy[bpos.Y][bpos.X] = save->fanVelY[spos];
		}
		if (includePressure)
		{
			if (save->hasPressure)
			{
				pv[bpos.Y][bpos.X] = save->pressure[spos];
				vx[bpos.Y][bpos.X] = save->velocityX[spos];
				vy[bpos.Y][bpos.X] = save->velocityY[spos];
			}
			if (save->hasAmbientHeat)
			{
				hv[bpos.Y][bpos.X] = save->ambientHeat[spos];
			}
			if (save->hasBlockAirMaps)
			{
				air->bmap_blockair [bpos.Y][bpos.X] = save->blockAir [spos];
				air->bmap_blockairh[bpos.Y][bpos.X] = save->blockAirh[spos];
			}
		}
		if (useGravityMaps)
		{
			gravIn.mass   [bpos] = save->gravMass  [spos];
			gravIn.mask   [bpos] = save->gravMask  [spos];
			gravOut.forceX[bpos] = save->gravForceX[spos];
			gravOut.forceY[bpos] = save->gravForceY[spos];
			gravForceRecalc = true; // gravOut changed outside DispatchNewtonianGravity
		}
	}
	if (useGravityMaps)
	{
		ResetNewtonianGravity(gravIn, gravOut);
	}

	gravWallChanged = true;
	if (!save->hasBlockAirMaps)
	{
		air->ApproximateBlockAirMaps(targetBlocks);
	}
	if (IsOmniAtmosphereActive() && omniAtmosphere)
	{
		SyncOmniAtmosphereBoundary(*this);
		if (!includePressure)
		{
			omniAtmospherePersistenceStatus = OmniAtmospherePersistenceStatus::RegionStateOmitted;
		}
		else if (save->hasOmniAtmosphereState)
		{
			if (save->omniAtmosphereStateVersion != GameSave::OmniAtmosphereStateVersion ||
				save->omniAtmosphereSpecies.size() != omniAtmosphere->SpeciesCount())
			{
				throw std::runtime_error("unsupported OmniAtmosphere save payload");
			}
			const size_t saveCellCount = size_t(save->blockSize.X) * size_t(save->blockSize.Y);
			if (save->omniAtmosphereSpeciesMassDensity.size() !=
					saveCellCount * save->omniAtmosphereSpecies.size() ||
				save->omniAtmosphereMomentumX.size() != saveCellCount ||
				save->omniAtmosphereMomentumY.size() != saveCellCount ||
				save->omniAtmosphereTotalEnergy.size() != saveCellCount ||
				save->omniAtmosphereCondensedWaterDensity.size() != saveCellCount ||
				save->omniAtmosphereCellValid.size() != saveCellCount)
			{
				throw std::runtime_error("malformed OmniAtmosphere save payload");
			}
			for (size_t species = 0; species < omniAtmosphere->SpeciesCount(); ++species)
			{
				if (save->omniAtmosphereSpecies[species] != omniAtmosphere->SpeciesDefinition(species).id)
					throw std::runtime_error("unsupported OmniAtmosphere save species registry");
			}
			for (auto bpos : targetBlocks)
			{
				const auto spos = bpos - blockP;
				const size_t cell = size_t(spos.Y) * size_t(save->blockSize.X) + size_t(spos.X);
				if (cell >= save->omniAtmosphereCellValid.size() || !save->omniAtmosphereCellValid[cell])
					continue;
				std::vector<double> speciesMassDensity(omniAtmosphere->SpeciesCount(), 0.0);
				for (size_t species = 0; species < omniAtmosphere->SpeciesCount(); ++species)
				{
					speciesMassDensity[species] = save->omniAtmosphereSpeciesMassDensity[
						cell * omniAtmosphere->SpeciesCount() + species];
				}
				if (!omniAtmosphere->RestoreSerializedCell(
					bpos.X,
					bpos.Y,
					speciesMassDensity,
					save->omniAtmosphereMomentumX[cell],
					save->omniAtmosphereMomentumY[cell],
					save->omniAtmosphereTotalEnergy[cell],
					save->omniAtmosphereCondensedWaterDensity[cell]))
				{
					throw std::runtime_error("invalid OmniAtmosphere save cell");
				}
			}
			omniAtmospherePersistenceStatus = OmniAtmospherePersistenceStatus::LoadedV2;
		}
		else
		{
			ImportLegacyAtmosphereSources(*this);
			omniAtmospherePersistenceStatus = OmniAtmospherePersistenceStatus::MigratedLegacyProjection;
		}
		ExportOmniAtmosphereToLegacyFields(*this);
	}
}

std::unique_ptr<GameSave> Simulation::Save(bool includePressure, Rect<int> partR) // particle coordinates
{
	auto blockR = RectBetween(partR.TopLeft() / CELL, partR.BottomRight() / CELL);
	auto blockP = blockR.pos;

	auto newSave = std::make_unique<GameSave>(blockR.size);
	newSave->frameCount = frameCount;
	newSave->rngState = rng.state();
	newSave->hasOmniWaterParcelState = IsOmniAtmosphereActive();
	newSave->omniWaterParcelStateVersion = newSave->hasOmniWaterParcelState
		? GameSave::OmniWaterParcelStateVersion
		: 0;
	if (newSave->hasOmniWaterParcelState)
	{
		newSave->omniWaterParcelMassKg.reserve(NUM_PARTS);
		newSave->omniWaterParcelSpecificEnthalpyJPerKg.reserve(NUM_PARTS);
	}
	newSave->hasOmniCarbonParcelState = IsOmniAtmosphereActive();
	newSave->omniCarbonParcelStateVersion = newSave->hasOmniCarbonParcelState
		? GameSave::OmniCarbonParcelStateVersion : 0;
	if (newSave->hasOmniCarbonParcelState)
		newSave->omniCarbonParcelMassKg.reserve(NUM_PARTS);
	newSave->hasOmniSolutionState = IsOmniAtmosphereActive();
	newSave->omniSolutionStateVersion = newSave->hasOmniSolutionState
		? GameSave::OmniSolutionStateVersion : 0;
	if (newSave->hasOmniSolutionState)
	{
		newSave->omniSolutionSolventMassKg.reserve(NUM_PARTS);
		newSave->omniSolutionSoluteMassKg.reserve(NUM_PARTS);
		newSave->omniSolutionNeutralSaltMassKg.reserve(NUM_PARTS);
	}

	int storedParts = 0;
	int elementCount[PT_NUM];
	std::fill(elementCount, elementCount+PT_NUM, 0);
	// Map of soap particles loaded into this save, old ID -> new ID
	// Now stores all particles, not just SOAP (but still only used for soap)
	std::map<unsigned int, unsigned int> particleMap;

	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	for (int i = 0; i < NPART; i++)
	{
		int x, y;
		x = int(parts[i].x + 0.5f);
		y = int(parts[i].y + 0.5f);
		if (parts[i].type && partR.Contains({ x, y }))
		{
			Particle tempPart = parts[i];
			tempPart.x -= blockP.X * CELL;
			tempPart.y -= blockP.Y * CELL;
			if (elements[tempPart.type].Enabled)
			{
				particleMap.insert(std::pair<unsigned int, unsigned int>(i, storedParts));
				*newSave << tempPart;
				if (newSave->hasOmniWaterParcelState)
				{
					newSave->omniWaterParcelMassKg.push_back(omniWaterParcelMassKg[i]);
					newSave->omniWaterParcelSpecificEnthalpyJPerKg.push_back(
						omniWaterParcelSpecificEnthalpyJPerKg[i]);
				}
				if (newSave->hasOmniCarbonParcelState)
					newSave->omniCarbonParcelMassKg.push_back(omniCarbonParcelMassKg[i]);
				if (newSave->hasOmniSolutionState)
				{
					newSave->omniSolutionSolventMassKg.push_back(omniSolutionSolventMassKg[i]);
					newSave->omniSolutionSoluteMassKg.push_back(omniSolutionSoluteMassKg[i]);
					newSave->omniSolutionNeutralSaltMassKg.push_back(
						omniSolutionNeutralSaltMassKg[i]);
				}
				storedParts++;
				elementCount[tempPart.type]++;

			}
		}
	}

	if (storedParts && elementCount[PT_SOAP])
	{
		// fix SOAP links using particleMap, a map of old particle ID -> new particle ID
		// loop through every new particle (saved into the save), and convert .tmp / .tmp2
		for (std::map<unsigned int, unsigned int>::iterator iter = particleMap.begin(), end = particleMap.end(); iter != end; ++iter)
		{
			int i = (*iter).second;
			if (newSave->particles[i].type != PT_SOAP)
				continue;
			if ((newSave->particles[i].ctype & 0x2) == 2)
			{
				std::map<unsigned int, unsigned int>::iterator n = particleMap.find(newSave->particles[i].tmp);
				if (n != end)
					newSave->particles[i].tmp = n->second;
				else
				{
					newSave->particles[i].tmp = 0;
					newSave->particles[i].ctype ^= 2;
				}
			}
			if ((newSave->particles[i].ctype & 0x4) == 4)
			{
				std::map<unsigned int, unsigned int>::iterator n = particleMap.find(newSave->particles[i].tmp2);
				if (n != end)
					newSave->particles[i].tmp2 = n->second;
				else
				{
					newSave->particles[i].tmp2 = 0;
					newSave->particles[i].ctype ^= 4;
				}
			}
		}
	}

	for (size_t i = 0; i < MAXSIGNS && i < signs.size(); i++)
	{
		if (signs[i].text.length() && partR.Contains({ signs[i].x, signs[i].y }))
		{
			sign tempSign = signs[i];
			tempSign.x -= blockP.X * CELL;
			tempSign.y -= blockP.Y * CELL;
			*newSave << tempSign;
		}
	}
	if (includePressure && IsOmniAtmosphereActive() && omniAtmosphere)
	{
		const size_t cellCount = size_t(newSave->blockSize.X) * size_t(newSave->blockSize.Y);
		newSave->hasOmniAtmosphereState = true;
		newSave->omniAtmosphereStateVersion = GameSave::OmniAtmosphereStateVersion;
		newSave->omniAtmosphereSpecies.reserve(omniAtmosphere->SpeciesCount());
		for (size_t species = 0; species < omniAtmosphere->SpeciesCount(); ++species)
			newSave->omniAtmosphereSpecies.emplace_back(omniAtmosphere->SpeciesDefinition(species).id);
		newSave->omniAtmosphereSpeciesMassDensity.resize(cellCount * omniAtmosphere->SpeciesCount());
		newSave->omniAtmosphereMomentumX.resize(cellCount);
		newSave->omniAtmosphereMomentumY.resize(cellCount);
		newSave->omniAtmosphereTotalEnergy.resize(cellCount);
		newSave->omniAtmosphereCondensedWaterDensity.resize(cellCount);
		newSave->omniAtmosphereCellValid.assign(cellCount, 1);
	}

	for (auto bpos : newSave->blockSize.OriginRect())
	{
		if(bmap[bpos.Y + blockP.Y][bpos.X + blockP.X])
		{
			newSave->blockMap[bpos] = bmap[bpos.Y + blockP.Y][bpos.X + blockP.X];
			newSave->fanVelX[bpos] = fvx[bpos.Y + blockP.Y][bpos.X + blockP.X];
			newSave->fanVelY[bpos] = fvy[bpos.Y + blockP.Y][bpos.X + blockP.X];
		}
		if (includePressure)
		{
			const int sourceX = bpos.X + blockP.X;
			const int sourceY = bpos.Y + blockP.Y;
			newSave->pressure[bpos] = pv[sourceY][sourceX];
			newSave->velocityX[bpos] = vx[sourceY][sourceX];
			newSave->velocityY[bpos] = vy[sourceY][sourceX];
			newSave->ambientHeat[bpos] = hv[sourceY][sourceX];
			newSave->blockAir[bpos] = air->bmap_blockair[sourceY][sourceX];
			newSave->blockAirh[bpos] = air->bmap_blockairh[sourceY][sourceX];
			if (newSave->hasOmniAtmosphereState)
			{
				const size_t cell = size_t(bpos.Y) * size_t(newSave->blockSize.X) + size_t(bpos.X);
				for (size_t species = 0; species < omniAtmosphere->SpeciesCount(); ++species)
				{
					newSave->omniAtmosphereSpeciesMassDensity[cell * omniAtmosphere->SpeciesCount() + species] =
						omniAtmosphere->SpeciesMassDensity(sourceX, sourceY, species);
				}
				const auto &state = omniAtmosphere->State(sourceX, sourceY);
				newSave->omniAtmosphereMomentumX[cell] = state.momentumX;
				newSave->omniAtmosphereMomentumY[cell] = state.momentumY;
				newSave->omniAtmosphereTotalEnergy[cell] = state.totalEnergy;
				newSave->omniAtmosphereCondensedWaterDensity[cell] =
					omniAtmosphere->Primitive(sourceX, sourceY).condensedWaterDensity;
			}
		}
		if (grav)
		{
			newSave->gravMass  [bpos] = gravIn.mass   [bpos + blockP];
			newSave->gravMask  [bpos] = gravIn.mask   [bpos + blockP];
			newSave->gravForceX[bpos] = gravOut.forceX[bpos + blockP];
			newSave->gravForceY[bpos] = gravOut.forceY[bpos + blockP];
		}
	}
	if (includePressure)
	{
		newSave->hasBlockAirMaps = true;
	}
	if (grav)
	{
		newSave->hasGravityMaps = true;
	}
	if (includePressure || ensureDeterminism)
	{
		newSave->hasPressure = true;
		newSave->hasAmbientHeat = true;
	}
	newSave->ensureDeterminism = ensureDeterminism;

	newSave->stkm.rocketBoots1 = player.rocketBoots;
	newSave->stkm.rocketBoots2 = player2.rocketBoots;
	newSave->stkm.fan1 = player.fan;
	newSave->stkm.fan2 = player2.fan;
	for (unsigned char i = 0; i < MAX_FIGHTERS; i++)
	{
		if (fighters[i].rocketBoots)
			newSave->stkm.rocketBootsFigh.push_back(i);
		if (fighters[i].fan)
			newSave->stkm.fanFigh.push_back(i);
	}

	SaveSimOptions(*newSave);
	newSave->pmapbits = PMAPBITS;
	return newSave;
}

void Simulation::SaveSimOptions(GameSave &gameSave)
{
	gameSave.gravityMode = gravityMode;
	gameSave.customGravityX = customGravityX;
	gameSave.customGravityY = customGravityY;
	gameSave.airMode = air->airMode;
	gameSave.ambientAirTemp = air->ambientAirTemp;
	gameSave.edgePressure = air->edgePressure;
	gameSave.edgeVelocityX = air->edgeVelocityX;
	gameSave.edgeVelocityY = air->edgeVelocityY;
	gameSave.vorticityCoeff = air->vorticityCoeff;
	gameSave.convectionMode = air->convectionMode;
	gameSave.edgeMode = edgeMode;
	gameSave.legacyEnable = legacy_enable;
	gameSave.waterEEnabled = water_equal_test;
	gameSave.gravityEnable = bool(grav);
	gameSave.aheatEnable = aheat_enable;
	gameSave.omniSimulationMode = omniSimulationMode;
}

bool Simulation::FloodFillPmapCheck(int x, int y, int type) const
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	if (type == 0)
		return !pmap[y][x] && !photons[y][x];
	if (elements[type].Properties&TYPE_ENERGY)
		return TYP(photons[y][x]) == type;
	else
		return TYP(pmap[y][x]) == type;
}

CoordStack& Simulation::getCoordStackSingleton()
{
	// Future-proofing in case Simulation is later multithreaded
	thread_local CoordStack cs;
	return cs;
}

int Simulation::flood_prop(int x, int y, const AccessProperty &changeProperty)
{
	int i, x1, x2, dy = 1;
	int did_something = 0;
	int r = pmap[y][x];
	if (!r)
		r = photons[y][x];
	if (!r)
		return 0;
	int parttype = TYP(r);
	char * bitmap = (char*)malloc(XRES*YRES); //Bitmap for checking
	if (!bitmap) return -1;
	memset(bitmap, 0, XRES*YRES);
	try
	{
		CoordStack& cs = getCoordStackSingleton();
		cs.clear();

		cs.push(x, y);
		do
		{
			cs.pop(x, y);
			x1 = x2 = x;
			while (x1>=CELL)
			{
				if (!FloodFillPmapCheck(x1-1, y, parttype) || bitmap[(y*XRES)+x1-1])
					break;
				x1--;
			}
			while (x2<XRES-CELL)
			{
				if (!FloodFillPmapCheck(x2+1, y, parttype) || bitmap[(y*XRES)+x2+1])
					break;
				x2++;
			}
			for (x=x1; x<=x2; x++)
			{
				i = pmap[y][x];
				if (!i)
					i = photons[y][x];
				if (!i)
					continue;
				changeProperty.Set(this, ID(i));
				bitmap[(y*XRES)+x] = 1;
				did_something = 1;
			}
			if (y>=CELL+dy)
				for (x=x1; x<=x2; x++)
					if (FloodFillPmapCheck(x, y-dy, parttype) && !bitmap[((y-dy)*XRES)+x])
						cs.push(x, y-dy);
			if (y<YRES-CELL-dy)
				for (x=x1; x<=x2; x++)
					if (FloodFillPmapCheck(x, y+dy, parttype) && !bitmap[((y+dy)*XRES)+x])
						cs.push(x, y+dy);
		} while (cs.getSize()>0);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		free(bitmap);
		return -1;
	}
	free(bitmap);
	return did_something;
}

int Simulation::FloodINST(int x, int y)
{
	int x1, x2;
	int created_something = 0;

	const auto isSparkableInst = [this](int x, int y) -> bool {
		return TYP(pmap[y][x])==PT_INST && parts[ID(pmap[y][x])].life==0;
	};

	const auto isInst = [this](int x, int y) -> bool {
		return TYP(pmap[y][x])==PT_INST || (TYP(pmap[y][x])==PT_SPRK  && parts[ID(pmap[y][x])].ctype==PT_INST);
	};

	if (!isSparkableInst(x,y))
		return 1;

	CoordStack& cs = getCoordStackSingleton();
	cs.clear();

	cs.push(x, y);

	try
	{
		do
		{
			cs.pop(x, y);
			x1 = x2 = x;
			// go left as far as possible
			while (x1>=CELL && isSparkableInst(x1-1, y))
			{
				x1--;
			}
			// go right as far as possible
			while (x2<XRES-CELL && isSparkableInst(x2+1, y))
			{
				x2++;
			}
			// fill span
			for (x=x1; x<=x2; x++)
			{
				if (create_part(-1, x, y, PT_SPRK)>=0)
					created_something = 1;
			}

			// add vertically adjacent pixels to stack
			// (wire crossing for INST)
			if (y>=CELL+1 && x1==x2 &&
				isInst(x1-1, y-1) && isInst(x1, y-1) && isInst(x1+1, y-1) &&
				!isInst(x1-1, y-2) && isInst(x1, y-2) && !isInst(x1+1, y-2))
			{
				// travelling vertically up, skipping a horizontal line
				if (isSparkableInst(x1, y-2))
				{
						cs.push(x1, y-2);
				}
			}
			else if (y>=CELL+1)
			{
				for (x=x1; x<=x2; x++)
				{
					if (isSparkableInst(x, y-1))
					{
						if (x==x1 || x==x2 || y>=YRES-CELL-1 || !isInst(x, y+1) || isInst(x+1, y+1) || isInst(x-1, y+1))
						{
							// if at the end of a horizontal section, or if it's a T junction or not a 1px wire crossing
							cs.push(x, y-1);
						}
					}
				}
			}

			if (y<YRES-CELL-1 && x1==x2 &&
				isInst(x1-1, y+1) && isInst(x1, y+1) && isInst(x1+1, y+1) &&
				!isInst(x1-1, y+2) && isInst(x1, y+2) && !isInst(x1+1, y+2))
			{
				// travelling vertically down, skipping a horizontal line
				if (isSparkableInst(x1, y+2))
				{
					cs.push(x1, y+2);
				}
			}
			else if (y<YRES-CELL-1)
			{
				for (x=x1; x<=x2; x++)
				{
					if (isSparkableInst(x, y+1))
					{
						if (x==x1 || x==x2 || y<0 || !isInst(x, y-1) || isInst(x+1, y-1) || isInst(x-1, y-1))
						{
							// if at the end of a horizontal section, or if it's a T junction or not a 1px wire crossing
							cs.push(x, y+1);
						}

					}
				}
			}
		} while (cs.getSize()>0);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return created_something;
}

bool Simulation::flood_water(int x, int y, int i)
{
	int x1, x2, originalX = x, originalY = y;
	int r = pmap[y][x];
	if (!r)
		return false;

	// Bitmap for checking where we've already looked
	auto bitmapPtr = std::unique_ptr<char[]>(new char[XRES * YRES]);
	char *bitmap = bitmapPtr.get();
	std::fill(&bitmap[0], &bitmap[0] + XRES * YRES, 0);

	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	try
	{
		CoordStack& cs = getCoordStackSingleton();
		cs.clear();

		cs.push(x, y);
		do
		{
			cs.pop(x, y);
			x1 = x2 = x;
			while (x1 >= CELL)
			{
				if (elements[TYP(pmap[y][x1 - 1])].Falldown != 2 || bitmap[(y * XRES) + x1 - 1])
					break;
				x1--;
			}
			while (x2 < XRES-CELL)
			{
				if (elements[TYP(pmap[y][x2 + 1])].Falldown != 2 || bitmap[(y * XRES) + x2 + 1])
					break;
				x2++;
			}
			for (int x = x1; x <= x2; x++)
			{
				if ((y - 1) > originalY && !pmap[y - 1][x])
				{
					// Try to move the water to a random position on this line, because there's probably a free location somewhere
					int randPos = rng.between(x, x2);
					if (!pmap[y - 1][randPos] && eval_move(parts[i].type, randPos, y - 1, nullptr))
						x = randPos;
					// Couldn't move to random position, so try the original position on the left
					else if (!eval_move(parts[i].type, x, y - 1, nullptr))
						continue;

					move(i, originalX, originalY, float(x), float(y - 1));
					return true;
				}

				bitmap[(y * XRES) + x] = 1;
			}
			if (y >= CELL + 1)
				for (int x = x1; x <= x2; x++)
					if (elements[TYP(pmap[y - 1][x])].Falldown == 2 && !bitmap[((y - 1) * XRES) + x])
						cs.push(x, y - 1);
			if (y < YRES - CELL - 1)
				for (int x = x1; x <= x2; x++)
					if (elements[TYP(pmap[y + 1][x])].Falldown == 2 && !bitmap[((y + 1) * XRES) + x])
						cs.push(x, y + 1);
		} while (cs.getSize() > 0);
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return false;
	}
	return false;
}

void Simulation::SetEdgeMode(int newEdgeMode)
{
	edgeMode = newEdgeMode;
	switch(edgeMode)
	{
	case EDGE_VOID:
	case EDGE_LOOP:
		for(int i = 0; i<XCELLS; i++)
		{
			bmap[0][i] = 0;
			bmap[YCELLS-1][i] = 0;
		}
		for(int i = 1; i<(YCELLS-1); i++)
		{
			bmap[i][0] = 0;
			bmap[i][XCELLS-1] = 0;
		}
		break;
	case EDGE_SOLID:
		int i;
		for(i=0; i<XCELLS; i++)
		{
			bmap[0][i] = WL_WALL;
			bmap[YCELLS-1][i] = WL_WALL;
		}
		for(i=1; i<(YCELLS-1); i++)
		{
			bmap[i][0] = WL_WALL;
			bmap[i][XCELLS-1] = WL_WALL;
		}
		break;
	default:
		SetEdgeMode(EDGE_VOID);
	}
	if (IsOmniAtmosphereActive() && omniAtmosphere)
	{
		SyncOmniAtmosphereBoundary(*this);
		ExportOmniAtmosphereToLegacyFields(*this);
	}
}

void Simulation::SetOmniSimulationMode(int newMode)
{
	if (newMode < OMNI_CLASSIC || newMode >= NUM_OMNI_SIMULATION_MODES)
		newMode = OMNI_CLASSIC;
	if (omniSimulationMode == newMode)
		return;
	omniSimulationMode = newMode;
	if (newMode == OMNI_CLASSIC)
	{
		std::fill(omniSolutionSolventMassKg.begin(), omniSolutionSolventMassKg.end(), 0.0);
		std::fill(omniSolutionSoluteMassKg.begin(), omniSolutionSoluteMassKg.end(), 0.0);
		std::fill(omniSolutionNeutralSaltMassKg.begin(), omniSolutionNeutralSaltMassKg.end(), 0.0);
		omniSolutionMetrics = {};
	}
	else
	{
		for (int particleId = 0; particleId < parts.active; ++particleId)
			if (parts[particleId].type && IsOmniManagedSolutionType(parts[particleId].type))
				InitializeOmniSolutionState(particleId, parts[particleId].type, false);
	}
	if (!omniAtmosphere)
		return;
	if (newMode == OMNI_CLASSIC)
	{
		omniLegacyProjectionShadowValid = false;
		omniAtmospherePersistenceStatus = OmniAtmospherePersistenceStatus::ClassicNotApplicable;
		air->Clear();
		air->ClearAirH();
		return;
	}
	omniAtmospherePersistenceStatus = OmniAtmospherePersistenceStatus::FreshPreset;
	omniAtmosphere->ResetUniform(OmniPhysicalScale::ReferenceDensityKgM3, air->ambientAirTemp);
	omniAtmosphere->SetExecutionMode(OmniAtmosphereExecution::RuntimeLowMach);
	SyncOmniAtmosphereBoundary(*this);
	ExportOmniAtmosphereToLegacyFields(*this);
}

const char *Simulation::GetOmniAtmospherePersistenceStatus() const
{
	switch (omniAtmospherePersistenceStatus)
	{
	case OmniAtmospherePersistenceStatus::ClassicNotApplicable:
		return "classic_not_applicable";
	case OmniAtmospherePersistenceStatus::FreshPreset:
		return "fresh_preset";
	case OmniAtmospherePersistenceStatus::LoadedV2:
		return "loaded_v2";
	case OmniAtmospherePersistenceStatus::MigratedLegacyProjection:
		return "migrated_1_0_6_legacy_projection";
	case OmniAtmospherePersistenceStatus::RegionStateOmitted:
		return "region_state_omitted";
	}
	return "unknown";
}

// Now simply creates a 0 pixel radius line without all the complicated flags / other checks
// Would make sense to move to Editing.cpp but SPRK needs it.
void Simulation::CreateLine(int x1, int y1, int x2, int y2, int c)
{
	bool reverseXY = abs(y2-y1) > abs(x2-x1);
	int x, y, dx, dy, sy;
	float e, de;
	int v = ID(c);
	c = TYP(c);
	if (reverseXY)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	e = 0.0f;
	de = dx ? dy/(float)dx : 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x=x1; x<=x2; x++)
	{
		if (reverseXY)
			create_part(-1, y, x, c, v);
		else
			create_part(-1, x, y, c, v);
		e += de;
		if (e >= 0.5f)
		{
			y += sy;
			if ((y1<y2) ? (y<=y2) : (y>=y2))
			{
				if (reverseXY)
					create_part(-1, y, x, c, v);
				else
					create_part(-1, x, y, c, v);
			}
			e -= 1.0f;
		}
	}
}

inline int Simulation::is_wire(int x, int y)
{
	return bmap[y][x]==WL_DETECT || bmap[y][x]==WL_EWALL || bmap[y][x]==WL_ALLOWLIQUID || bmap[y][x]==WL_WALLELEC || bmap[y][x]==WL_ALLOWALLELEC || bmap[y][x]==WL_EHOLE || bmap[y][x]==WL_STASIS;
}

inline int Simulation::is_wire_off(int x, int y)
{
	return (bmap[y][x]==WL_DETECT || bmap[y][x]==WL_EWALL || bmap[y][x]==WL_ALLOWLIQUID || bmap[y][x]==WL_WALLELEC || bmap[y][x]==WL_ALLOWALLELEC || bmap[y][x]==WL_EHOLE || bmap[y][x]==WL_STASIS) && emap[y][x]<8;
}

// implement __builtin_ctz and __builtin_clz on msvc
#ifdef _MSC_VER
unsigned msvc_ctz(unsigned a)
{
	unsigned long i;
	_BitScanForward(&i, a);
	return i;
}

unsigned msvc_clz(unsigned a)
{
	unsigned long i;
	_BitScanReverse(&i, a);
	return 31 - i;
}

#define __builtin_ctz msvc_ctz
#define __builtin_clz msvc_clz
#endif

int Simulation::get_wavelength_bin(int *wm)
{
	int i, w0, wM, r;

	if (!(*wm & 0x3FFFFFFF))
		return -1;

#if defined(__GNUC__) || defined(_MSVC_VER)
	w0 = __builtin_ctz(*wm | 0xC0000000);
	wM = 31 - __builtin_clz(*wm & 0x3FFFFFFF);
#else
	w0 = 30;
	wM = 0;
	for (i = 0; i < 30; i++)
		if (*wm & (1<<i))
		{
			if (i < w0)
				w0 = i;
			if (i > wM)
				wM = i;
		}
#endif

	if (wM - w0 < 5)
		return wM + w0;

	r = rng.gen();
	i = (r >> 1) % (wM-w0-4);
	i += w0;

	if (r & 1)
	{
		*wm &= 0x1F << i;
		return (i + 2) * 2;
	}
	else
	{
		*wm &= 0xF << i;
		return (i + 2) * 2 - 1;
	}
}

void Simulation::set_emap(int x, int y)
{
	int x1, x2;

	if (!is_wire_off(x, y))
		return;

	// go left as far as possible
	x1 = x2 = x;
	while (x1>0)
	{
		if (!is_wire_off(x1-1, y))
			break;
		x1--;
	}
	while (x2<XCELLS-1)
	{
		if (!is_wire_off(x2+1, y))
			break;
		x2++;
	}

	// fill span
	for (x=x1; x<=x2; x++)
		emap[y][x] = 16;

	// fill children

	if (y>1 && x1==x2 &&
	        is_wire(x1-1, y-1) && is_wire(x1, y-1) && is_wire(x1+1, y-1) &&
	        !is_wire(x1-1, y-2) && is_wire(x1, y-2) && !is_wire(x1+1, y-2))
		set_emap(x1, y-2);
	else if (y>0)
		for (x=x1; x<=x2; x++)
			if (is_wire_off(x, y-1))
			{
				if (x==x1 || x==x2 || y>=YCELLS-1 ||
				        is_wire(x-1, y-1) || is_wire(x+1, y-1) ||
				        is_wire(x-1, y+1) || !is_wire(x, y+1) || is_wire(x+1, y+1))
					set_emap(x, y-1);
			}

	if (y<YCELLS-2 && x1==x2 &&
	        is_wire(x1-1, y+1) && is_wire(x1, y+1) && is_wire(x1+1, y+1) &&
	        !is_wire(x1-1, y+2) && is_wire(x1, y+2) && !is_wire(x1+1, y+2))
		set_emap(x1, y+2);
	else if (y<YCELLS-1)
		for (x=x1; x<=x2; x++)
			if (is_wire_off(x, y+1))
			{
				if (x==x1 || x==x2 || y<0 ||
				        is_wire(x-1, y+1) || is_wire(x+1, y+1) ||
				        is_wire(x-1, y-1) || !is_wire(x, y-1) || is_wire(x+1, y-1))
					set_emap(x, y+1);
			}
}

int Simulation::parts_avg(int ci, int ni,int t)
{
	if (t==PT_INSL)//to keep electronics working
	{
		int pmr = pmap[((int)(parts[ci].y+0.5f) + (int)(parts[ni].y+0.5f))/2][((int)(parts[ci].x+0.5f) + (int)(parts[ni].x+0.5f))/2];
		if (pmr)
			return parts[ID(pmr)].type;
		else
			return PT_NONE;
	}
	else
	{
		int pmr2 = pmap[(int)((parts[ci].y + parts[ni].y)/2+0.5f)][(int)((parts[ci].x + parts[ni].x)/2+0.5f)];//seems to be more accurate.
		if (pmr2)
		{
			if (parts[ID(pmr2)].type==t)
				return t;
		}
		else
			return PT_NONE;
	}
	return PT_NONE;
}

void Parts::Reset()
{
	memset(data.data(), 0, sizeof(Particle)*NPART);
	active = 0;
	pfree = -1;
}

void Simulation::clear_sim(void)
{
	for (auto i = 0; i < parts.active; i++)
	{
		if (parts[i].type)
		{
			kill_part(i);
		}
	}
	ensureDeterminism = false;
	frameCount = 0;
	debug_nextToUpdate = 0;
	debug_mostRecentlyUpdated = -1;
	emp_decor = 0;
	emp_trigger_count = 0;
	signs.clear();
	memset(bmap, 0, sizeof(bmap));
	memset(emap, 0, sizeof(emap));
	parts.Reset();
	std::fill(omniWaterParcelMassKg.begin(), omniWaterParcelMassKg.end(), 0.0);
	std::fill(omniWaterParcelSpecificEnthalpyJPerKg.begin(),
		omniWaterParcelSpecificEnthalpyJPerKg.end(), 0.0);
	std::fill(omniCarbonParcelMassKg.begin(), omniCarbonParcelMassKg.end(), 0.0);
	std::fill(omniSolutionSolventMassKg.begin(), omniSolutionSolventMassKg.end(), 0.0);
	std::fill(omniSolutionSoluteMassKg.begin(), omniSolutionSoluteMassKg.end(), 0.0);
	std::fill(omniSolutionNeutralSaltMassKg.begin(), omniSolutionNeutralSaltMassKg.end(), 0.0);
	omniWaterTransferRequests.clear();
	omniWaterCouplingMetrics = {};
	omniChemistryMetrics = {};
	omniSolutionMetrics = {};
	NUM_PARTS = 0;
	memset(pmap, 0, sizeof(pmap));
	memset(fvx, 0, sizeof(fvx));
	memset(fvy, 0, sizeof(fvy));
	omniLegacyProjectionShadowValid = false;
	memset(photons, 0, sizeof(photons));
	memset(wireless, 0, sizeof(wireless));
	memset(gol, 0, sizeof(gol));
	memset(portalp, 0, sizeof(portalp));
	memset(fighters, 0, sizeof(fighters));
	memset(&player, 0, sizeof(player));
	memset(&player2, 0, sizeof(player2));
	memset(&Element_LOLZ_lolz, 0, sizeof(Element_LOLZ_lolz));
	memset(&Element_LOVE_love, 0, sizeof(Element_LOVE_love));
	memset(&Element_PSTN_tempParts, 0, sizeof(Element_PSTN_tempParts));
	Element_PPIP_ppip_changed = 0;
	std::fill(elementCount, elementCount+PT_NUM, 0);
	elementRecount = true;
	elementRecountAfterSim = false;
	fighcount = 0;
	player.spwn = 0;
	player.spawnID = -1;
	player.rocketBoots = false;
	player.fan = false;
	player2.spwn = 0;
	player2.spawnID = -1;
	player2.rocketBoots = false;
	player2.fan = false;
	//memset(pers_bg, 0, WINDOWW*YRES*PIXELSIZE);
	//memset(fire_r, 0, sizeof(fire_r));
	//memset(fire_g, 0, sizeof(fire_g));
	//memset(fire_b, 0, sizeof(fire_b));
	//if(gravmask)
		//memset(gravmask, 0xFFFFFFFF, NCELL*sizeof(unsigned));
	ResetNewtonianGravity({}, {});
	if(air)
	{
		air->Clear();
		air->ClearAirH();
	}
	if (omniAtmosphere)
	{
		omniAtmosphere->ResetUniform(OmniPhysicalScale::ReferenceDensityKgM3, air ? air->ambientAirTemp : OmniPhysicalScale::ReferenceTemperatureK);
		omniAtmosphere->SetExecutionMode(OmniAtmosphereExecution::RuntimeLowMach);
	}
	omniAtmospherePersistenceStatus = IsOmniAtmosphereActive()
		? OmniAtmospherePersistenceStatus::FreshPreset
		: OmniAtmospherePersistenceStatus::ClassicNotApplicable;
	SetEdgeMode(edgeMode);
}

bool Simulation::IsWallBlocking(int x, int y, int type) const
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	if (bmap[y/CELL][x/CELL])
	{
		int wall = bmap[y/CELL][x/CELL];
		if (wall == WL_ALLOWGAS && !(elements[type].Properties&TYPE_GAS))
			return true;
		else if (wall == WL_ALLOWENERGY && !(elements[type].Properties&TYPE_ENERGY))
			return true;
		else if (wall == WL_ALLOWLIQUID && !(elements[type].Properties&TYPE_LIQUID))
			return true;
		else if (wall == WL_ALLOWPOWDER && !(elements[type].Properties&TYPE_PART))
			return true;
		else if (wall == WL_ALLOWAIR || wall == WL_WALL || wall == WL_WALLELEC)
			return true;
		else if (wall == WL_EWALL && !emap[y/CELL][x/CELL])
			return true;
		else if (wall == WL_DETECT && (elements[type].Properties&TYPE_SOLID))
			return true;
	}
	return false;
}

/*
   RETURN-value explanation
1 = Swap
0 = No move/Bounce
2 = Both particles occupy the same space.
 */
int Simulation::eval_move(int pt, int nx, int ny, unsigned *rr) const
{
	unsigned r;
	int result;

	if (nx<0 || ny<0 || nx>=XRES || ny>=YRES)
		return 0;

	r = pmap[ny][nx];
	if (r)
		r = (r&~PMAPMASK) | parts[ID(r)].type;
	if (rr)
		*rr = r;
	if (pt>=PT_NUM || TYP(r)>=PT_NUM)
		return 0;
	auto &sd = SimulationData::CRef();
	auto &can_move = sd.can_move;
	auto &elements = sd.elements;
	result = can_move[pt][TYP(r)];
	if (result == 3)
	{
		switch (TYP(r))
		{
		case PT_LCRY:
			if (pt==PT_PHOT)
				result = (parts[ID(r)].life > 5)? 2 : 0;
			break;
		case PT_GPMP:
			if (pt == PT_PHOT)
				result = (parts[ID(r)].life < 10) ? 2 : 0;
			break;
		case PT_INVIS:
		{
			float pressureResistance = 0.0f;
			if (parts[ID(r)].tmp > 0)
				pressureResistance = (float)parts[ID(r)].tmp;
			else
				pressureResistance = 4.0f;

			if (pv[ny/CELL][nx/CELL] < -pressureResistance || pv[ny/CELL][nx/CELL] > pressureResistance)
				result = 2;
			else
				result = 0;
			break;
		}
		case PT_PVOD:
			if (parts[ID(r)].life == 10)
			{
				if (!parts[ID(r)].ctype || (parts[ID(r)].ctype==pt)!=(parts[ID(r)].tmp&1))
					result = 1;
				else
					result = 0;
			}
			else result = 0;
			break;
		case PT_VOID:
			if (!parts[ID(r)].ctype || (parts[ID(r)].ctype==pt)!=(parts[ID(r)].tmp&1))
				result = 1;
			else
				result = 0;
			break;
		case PT_SWCH:
			if (pt == PT_TRON)
			{
				if (parts[ID(r)].life >= 10)
					return 2;
				else
					return 0;
			}
			break;
		default:
			// This should never happen
			// If it were to happen, try_move would interpret a 3 as a 1
			result =  1;
		}
	}
	if (bmap[ny/CELL][nx/CELL])
	{
		if (IsWallBlocking(nx, ny, pt))
			return 0;
		if (bmap[ny/CELL][nx/CELL]==WL_EHOLE && !emap[ny/CELL][nx/CELL] && !(elements[pt].Properties&TYPE_SOLID) && !(elements[TYP(r)].Properties&TYPE_SOLID))
			return 2;
	}
	return result;
}

int Simulation::try_move(int i, int x, int y, int nx, int ny)
{
	unsigned r = 0, e;

	if (x==nx && y==ny)
		return 1;
	if (nx<0 || ny<0 || nx>=XRES || ny>=YRES)
		return 1;

	e = eval_move(parts[i].type, nx, ny, &r);

	/* half-silvered mirror */
	if (!e && parts[i].type==PT_PHOT && ((TYP(r)==PT_BMTL && rng.chance(1, 2)) || TYP(pmap[y][x])==PT_BMTL))
		e = 2;

	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	if (!e) //if no movement
	{
		int rt = TYP(r);
		if (rt == PT_WOOD)
		{
			//@ WOOD -> SAWD
			float vel = std::sqrt(std::pow(parts[i].vx, 2) + std::pow(parts[i].vy, 2));
			if (vel > 5)
				part_change_type(ID(r), nx, ny, PT_SAWD);
		}
		if (!(elements[parts[i].type].Properties & TYPE_ENERGY))
			return 0;
		if (!legacy_enable && parts[i].type==PT_PHOT && r)//PHOT heat conduction
		{
			if (rt == PT_COAL || rt == PT_BCOL)
				parts[ID(r)].temp = parts[i].temp;

			if (rt < PT_NUM && !sd.IsHeatInsulator(parts[ID(r)]) && rt != PT_FILT)
				parts[i].temp = parts[ID(r)].temp = restrict_flt((parts[ID(r)].temp+parts[i].temp)/2, MIN_TEMP, MAX_TEMP);
		}
		else if ((parts[i].type==PT_NEUT || parts[i].type==PT_ELEC) && (rt==PT_CLNE || rt==PT_PCLN || rt==PT_BCLN || rt==PT_PBCN))
		{
			if (!parts[ID(r)].ctype)
				parts[ID(r)].ctype = parts[i].type;
		}
		if (rt==PT_PRTI && (elements[parts[i].type].Properties & TYPE_ENERGY))
		{
			int nnx, count;
			for (count=0; count<8; count++)
			{
				if (isign(x-nx)==isign(portal_rx[count]) && isign(y-ny)==isign(portal_ry[count]))
					break;
			}
			count = count%8;
			parts[ID(r)].tmp = (int)((parts[ID(r)].temp-73.15f)/100+1);
			if (parts[ID(r)].tmp>=CHANNELS) parts[ID(r)].tmp = CHANNELS-1;
			else if (parts[ID(r)].tmp<0) parts[ID(r)].tmp = 0;
			for ( nnx=0; nnx<80; nnx++)
				if (!portalp[parts[ID(r)].tmp][count][nnx].type)
				{
					portalp[parts[ID(r)].tmp][count][nnx] = parts[i];
					kill_part(i);
					break;
				}
		}
		return 0;
	}

	if (e == 2) //if occupy same space
	{
		switch (parts[i].type)
		{
		case PT_PHOT:
		{
			switch (TYP(r))
			{
			case PT_GLOW:
				if (!parts[ID(r)].life && rng.chance(1, 30))
				{
					parts[ID(r)].life = 120;
					create_gain_photon(i);
				}
				break;
			case PT_FILT:
				parts[i].ctype = Element_FILT_interactWavelengths(this, &parts[ID(r)], parts[i].ctype);
				break;
			case PT_C5:
				if (parts[ID(r)].life > 0 && (parts[ID(r)].ctype & parts[i].ctype & 0xFFFFFFC0))
				{
					float vx = ((parts[ID(r)].tmp << 16) >> 16) / 255.0f;
					float vy = (parts[ID(r)].tmp >> 16) / 255.0f;
					float vn = parts[i].vx * parts[i].vx + parts[i].vy * parts[i].vy;
					// if the resulting velocity would be 0, that would cause division by 0 inside the else
					// shoot the photon off at a 90 degree angle instead (probably particle order dependent)
					if (parts[i].vx + vx == 0 && parts[i].vy + vy == 0)
					{
						parts[i].vx = vy;
						parts[i].vy = -vx;
					}
					else
					{
						parts[i].ctype = (parts[ID(r)].ctype & parts[i].ctype) >> 6;
						// add momentum of photons to each other
						parts[i].vx += vx;
						parts[i].vy += vy;
						// normalize velocity to original value
						vn /= parts[i].vx * parts[i].vx + parts[i].vy * parts[i].vy;
						vn = sqrtf(vn);
						parts[i].vx *= vn;
						parts[i].vy *= vn;
					}
					parts[ID(r)].life = 0;
					parts[ID(r)].ctype = 0;
				}
				else if(!parts[ID(r)].ctype && parts[i].ctype & 0xFFFFFFC0)
				{
					parts[ID(r)].life = 1;
					parts[ID(r)].ctype = parts[i].ctype;
					parts[ID(r)].tmp = (0xFFFF & (int)(parts[i].vx * 255.0f)) | (0xFFFF0000 & (int)(parts[i].vy * 16711680.0f));
					parts[ID(r)].tmp2 = (0xFFFF & (int)((parts[i].x - x) * 255.0f)) | (0xFFFF0000 & (int)((parts[i].y - y) * 16711680.0f));
					kill_part(i);
				}
				break;
			case PT_INVIS:
			{
				float pressureResistance = 0.0f;
				pressureResistance = (parts[ID(r)].tmp > 0) ? (float)parts[ID(r)].tmp : 4.0f;

				if (pv[ny/CELL][nx/CELL] >= -pressureResistance && pv[ny/CELL][nx/CELL] <= pressureResistance)
				{
					//@ PHOT + INVIS -> NEUT + INVIS
					part_change_type(i,x,y,PT_NEUT);
					parts[i].ctype = 0;
				}
				break;
			}
			case PT_BIZR:
			case PT_BIZRG:
			case PT_BIZRS:
				//@ PHOT + BIZR/BIZRG/BIZRS -> ELEC + BIZR/BIZRG/BIZRS
				part_change_type(i, x, y, PT_ELEC);
				parts[i].ctype = 0;
				break;
			case PT_H2:
				if (!(parts[i].tmp&0x1))
				{
					//@ PHOT + H2 -> PROT + ELEC
					part_change_type(i, x, y, PT_PROT);
					parts[i].ctype = 0;
					parts[i].tmp2 = 0x1;

					create_part(ID(r), x, y, PT_ELEC);
					return 1;
				}
				break;
			case PT_GPMP:
				if (parts[ID(r)].life == 0)
				{
					//@ PHOT + GPMP -> GRVT + GPMP
					part_change_type(i, x, y, PT_GRVT);
					parts[i].tmp = int(parts[ID(r)].temp - 273.15f);
				}
				break;
			}
			break;
		}
		case PT_NEUT:
			//@ NEUT + GLAS/BGLA -> NEUT + GLAS/BGLA + PHOT
			if (IsRefractingGlass(TYP(r)))
				if (rng.chance(1, 10))
					create_cherenkov_photon(i);
			break;
		case PT_ELEC:
			if (TYP(r) == PT_GLOW)
			{
				//@ ELEC + GLOW -> PHOT + GLOW
				part_change_type(i, x, y, PT_PHOT);
				parts[i].ctype = 0x3FFFFFFF;
			}
			break;
		case PT_PROT:
			//@ PROT + INVIS -> NEUT + INVIS
			if (TYP(r) == PT_INVIS)
				part_change_type(i, x, y, PT_NEUT);
			break;
		case PT_BIZR:
		case PT_BIZRG:
			if (TYP(r) == PT_FILT)
				parts[i].ctype = Element_FILT_interactWavelengths(this, &parts[ID(r)], parts[i].ctype);
			break;
		}
		return 1;
	}
	//else e=1 , we are trying to swap the particles, return 0 no swap/move, 1 is still overlap/move, because the swap takes place later

	switch (TYP(r))
	{
	case PT_VOID:
	case PT_PVOD:
		// this is where void eats particles
		// void ctype already checked in eval_move
		kill_part(i);
		return 0;
	case PT_BHOL:
	case PT_NBHL:
		// this is where blackhole eats particles
		if (!legacy_enable)
		{
			parts[ID(r)].temp = restrict_flt(parts[ID(r)].temp+parts[i].temp/2, MIN_TEMP, MAX_TEMP);//3.0f;
		}
		kill_part(i);
		return 0;
	case PT_WHOL:
	case PT_NWHL:
		// whitehole eats anar
		if (parts[i].type == PT_ANAR)
		{
			if (!legacy_enable)
			{
				parts[ID(r)].temp = restrict_flt(parts[ID(r)].temp - (MAX_TEMP-parts[i].temp)/2, MIN_TEMP, MAX_TEMP);
			}
			kill_part(i);
			return 0;
		}
		break;
	case PT_DEUT:
		if (parts[i].type == PT_ELEC)
		{
			if(parts[ID(r)].life < 6000)
				parts[ID(r)].life += 1;
			parts[ID(r)].temp = 0;
			kill_part(i);
			return 0;
		}
		break;
	case PT_VIBR:
	case PT_BVBR:
		if ((elements[parts[i].type].Properties & TYPE_ENERGY))
		{
			parts[ID(r)].tmp += 20;
			kill_part(i);
			return 0;
		}
		break;
	}

	switch (parts[i].type)
	{
	case PT_NEUT:
		if (elements[TYP(r)].Properties & PROP_NEUTABSORB)
		{
			kill_part(i);
			return 0;
		}
		break;
	case PT_CNCT:
		{
			float cnctGravX, cnctGravY; // Calculate offset from gravity
			GetGravityField(x, y, elements[PT_CNCT].Gravity, elements[PT_CNCT].Gravity, cnctGravX, cnctGravY);
			int offsetX = 0, offsetY = 0;
			if (cnctGravX > 0.0f) offsetX++;
			else if (cnctGravX < 0.0f) offsetX--;
			if (cnctGravY > 0.0f) offsetY++;
			else if (cnctGravY < 0.0f) offsetY--;
			if ((offsetX != 0) != (offsetY != 0) && // Is this a different position (avoid diagonals, doesn't work well)
				((nx - x) * offsetX > 0 || (ny - y) * offsetY > 0) && // Is the destination particle below the moving particle
				(TYP(pmap[y+offsetY][x+offsetX]) == PT_CNCT || TYP(pmap[y+offsetY][x+offsetX]) == PT_ROCK)) //check below CNCT for another CNCT or ROCK
				return 0;
		}
		break;
	case PT_GBMB:
		if (parts[i].life > 0)
			return 0;
		break;
	}

	if ((bmap[y/CELL][x/CELL]==WL_EHOLE && !emap[y/CELL][x/CELL]) && !(bmap[ny/CELL][nx/CELL]==WL_EHOLE && !emap[ny/CELL][nx/CELL]))
		return 0;

	int ri = ID(r); //ri is the particle number at r (pmap[ny][nx])
	if (r)//the swap part, if we make it this far, swap
	{
		if (parts[i].type==PT_NEUT) {
			// target material is NEUTPENETRATE, meaning it gets moved around when neutron passes
			unsigned s = pmap[y][x];
			if (s && !(elements[TYP(s)].Properties&PROP_NEUTPENETRATE))
				return 1; // if the element currently underneath neutron isn't NEUTPENETRATE, don't move anything except the neutron
			// if nothing is currently underneath neutron, only move target particle
			if(bmap[y/CELL][x/CELL] == WL_ALLOWENERGY)
				return 1; // do not drag target particle into an energy only wall
			if (s)
			{
				pmap[ny][nx] = (s&~PMAPMASK)|parts[ID(s)].type;
				parts[ID(s)].x = float(nx);
				parts[ID(s)].y = float(ny);
			}
			else
				pmap[ny][nx] = 0;
			parts[ri].x = float(x);
			parts[ri].y = float(y);
			pmap[y][x] = PMAP(ri, parts[ri].type);
			return 1;
		}

		if (pmap[ny][nx] && ID(pmap[ny][nx]) == ri)
			pmap[ny][nx] = 0;
		parts[ri].x = parts[i].x;
		parts[ri].y = parts[i].y;
		int rx = int(parts[ri].x + 0.5f);
		int ry = int(parts[ri].y + 0.5f);
		pmap[ry][rx] = PMAP(ri, parts[ri].type);
	}
	return 1;
}

// try to move particle, and if successful update pmap and parts[i].x,y
int Simulation::do_move(int i, int x, int y, float nxf, float nyf)
{
	int nx = (int)(nxf+0.5f), ny = (int)(nyf+0.5f), result;
	if (edgeMode == EDGE_LOOP)
	{
		bool x_ok = (nx >= CELL && nx < XRES-CELL);
		bool y_ok = (ny >= CELL && ny < YRES-CELL);
		if (!x_ok)
			nxf = remainder_p(nxf-CELL+.5f, XRES-CELL*2.0f)+CELL-.5f;
		if (!y_ok)
			nyf = remainder_p(nyf-CELL+.5f, YRES-CELL*2.0f)+CELL-.5f;
		nx = (int)(nxf+0.5f);
		ny = (int)(nyf+0.5f);

		/*if (!x_ok || !y_ok)
		{
			//make sure there isn't something blocking it on the other side
			//only needed if this if statement is moved after the try_move (like my mod)
			//if (!eval_move(t, nx, ny, NULL) || (t == PT_PHOT && pmap[ny][nx]))
			//	return -1;
		}*/
	}
	if (parts[i].type == PT_NONE)
		return 0;
	result = try_move(i, x, y, nx, ny);
	if (result)
	{
		if (!move(i, x, y, nxf, nyf))
			return -1;
	}
	return result;
}

bool Simulation::move(int i, int x, int y, float nxf, float nyf)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	int nx = (int)(nxf+0.5f), ny = (int)(nyf+0.5f);
	int t = parts[i].type;
	parts[i].x = nxf;
	parts[i].y = nyf;
	if (ny != y || nx != x)
	{
		if (pmap[y][x] && ID(pmap[y][x]) == i)
			pmap[y][x] = 0;
		if (photons[y][x] && ID(photons[y][x]) == i)
			photons[y][x] = 0;
		// kill_part if particle is out of bounds
		if (nx < CELL || nx >= XRES - CELL || ny < CELL || ny >= YRES - CELL)
		{
			kill_part(i);
			return false;
		}
		if (elements[t].Properties & TYPE_ENERGY)
			photons[ny][nx] = PMAP(i, t);
		else if (t)
			pmap[ny][nx] = PMAP(i, t);
	}

	return true;
}

void Simulation::photoelectric_effect(int nx, int ny)//create sparks from PHOT when hitting PSCN and NSCN
{
	unsigned r = pmap[ny][nx];

	if (TYP(r) == PT_PSCN && !parts[ID(r)].life)
	{
		if (TYP(pmap[ny][nx-1]) == PT_NSCN || TYP(pmap[ny][nx+1]) == PT_NSCN ||
		        TYP(pmap[ny-1][nx]) == PT_NSCN ||  TYP(pmap[ny+1][nx]) == PT_NSCN)
		{
			parts[ID(r)].ctype = PT_PSCN;
			part_change_type(ID(r), nx, ny, PT_SPRK);
			parts[ID(r)].life = 4;
		}
	}
}

unsigned static direction_to_map(float dx, float dy, int t)
{
	// TODO:
	// Adding extra directions causes some inaccuracies.
	// Not adding them causes problems with some diagonal surfaces (photons absorbed instead of reflected).
	// For now, don't add them.
	// Solution may involve more intelligent setting of initial i0 value in find_next_boundary?
	// or rewriting normal/boundary finding code

	return (dx >= 0) |
		   (((dx + dy) >= 0) << 1) |     /*  567  */
		   ((dy >= 0) << 2) |            /*  4+0  */
		   (((dy - dx) >= 0) << 3) |     /*  321  */
		   ((dx <= 0) << 4) |
		   (((dx + dy) <= 0) << 5) |
		   ((dy <= 0) << 6) |
		   (((dy - dx) <= 0) << 7);
	/*
	return (dx >= -0.001) |
		   (((dx + dy) >= -0.001) << 1) |     //  567
		   ((dy >= -0.001) << 2) |            //  4+0
		   (((dy - dx) >= -0.001) << 3) |     //  321
		   ((dx <= 0.001) << 4) |
		   (((dx + dy) <= 0.001) << 5) |
		   ((dy <= 0.001) << 6) |
		   (((dy - dx) <= 0.001) << 7);
	}*/
}

int Simulation::is_blocking(int t, int x, int y) const
{
	if (t & REFRACT) {
		if (x<0 || y<0 || x>=XRES || y>=YRES)
			return 0;
		if (IsRefractingGlass(TYP(pmap[y][x])))
			return 1;
		return 0;
	}

	return !eval_move(t, x, y, nullptr);
}

int Simulation::is_boundary(int pt, int x, int y) const
{
	if (!is_blocking(pt,x,y))
		return 0;
	if (is_blocking(pt,x,y-1) && is_blocking(pt,x,y+1) && is_blocking(pt,x-1,y) && is_blocking(pt,x+1,y))
		return 0;
	return 1;
}

int Simulation::find_next_boundary(int pt, int *x, int *y, int dm, int *em, bool reverse) const
{
	static int dx[8] = {1,1,0,-1,-1,-1,0,1};
	static int dy[8] = {0,1,1,1,0,-1,-1,-1};
	static int de[8] = {0x83,0x07,0x0E,0x1C,0x38,0x70,0xE0,0xC1};

	if (*x <= 0 || *x >= XRES-1 || *y <= 0 || *y >= YRES-1)
	{
		return 0;
	}

	if (*em != -1)
	{
		dm &= de[*em];
	}

	unsigned int mask = 0;
	for (int i = 0; i < 8; ++i)
	{
		if ((dm & (1U << i)) && is_blocking(pt, *x + dx[i], *y + dy[i]))
		{
			mask |= (1U << i);
		}
	}
	for (int i = 0; i < 8; ++i)
	{
		int n = (i + (reverse ? 1 : -1)) & 7;
		if (((mask & (1U << i))) && !(mask & (1U << n)))
		{
			*x += dx[i];
			*y += dy[i];
			*em = i;
			return 1;
		}
	}

	return 0;
}

Simulation::GetNormalResult Simulation::get_normal(int pt, int x, int y, float dx, float dy) const
{
	int ldm, rdm, lm, rm;
	int lx, ly, lv, rx, ry, rv;
	int i, j;
	float r, ex, ey;

	if (!dx && !dy)
		return { false };

	if (!is_boundary(pt, x, y))
		return { false };

	ldm = (pt & REFRACT) ? 0xFF : direction_to_map(-dy, dx, pt);
	rdm = (pt & REFRACT) ? 0xFF : direction_to_map(dy, -dx, pt);
	lx = rx = x;
	ly = ry = y;
	lv = rv = 1;
	lm = rm = -1;

	j = 0;
	for (i=0; i<SURF_RANGE; i++) {
		if (lv)
			lv = find_next_boundary(pt, &lx, &ly, ldm, &lm, true);
		if (rv)
			rv = find_next_boundary(pt, &rx, &ry, rdm, &rm, false);
		j += lv + rv;
		if (!lv && !rv)
			break;
	}

	if (j < NORMAL_MIN_EST)
		return { false };

	if ((lx == rx) && (ly == ry))
		return { false };
	ex = float(rx - lx);
	ey = float(ry - ly);
	r = 1.0f/hypot(ex, ey);
	auto nx =  ey * r;
	auto ny = -ex * r;

	return { true, nx, ny, lx, ly, rx, ry };
}

template<bool PhotoelectricEffect, class Sim>
Simulation::GetNormalResult Simulation::get_normal_interp(Sim &sim, int pt, float x0, float y0, float dx, float dy)
{
	int x, y, i;

	dx /= NORMAL_FRAC;
	dy /= NORMAL_FRAC;

	for (i=0; i<NORMAL_INTERP; i++) {
		x = (int)(x0 + 0.5f);
		y = (int)(y0 + 0.5f);
		if (x < 0 || y < 0 || x >= XRES || y >= YRES)
		{
			return { false };
		}
		if (sim.is_boundary(pt, x, y))
			break;
		x0 += dx;
		y0 += dy;
	}
	if (i >= NORMAL_INTERP)
		return { false };

	if constexpr (PhotoelectricEffect)
	{
		if (pt == PT_PHOT)
			sim.photoelectric_effect(x, y);
	}

	return sim.get_normal(pt, x, y, dx, dy);
}

template
Simulation::GetNormalResult Simulation::get_normal_interp<false, const Simulation>(const Simulation &sim, int pt, float x0, float y0, float dx, float dy);

void Simulation::kill_part(int i)//kills particle number i
{
	if (i < 0 || i >= NPART)
		return;
	
	int x = (int)(parts[i].x + 0.5f);
	int y = (int)(parts[i].y + 0.5f);

	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	int t = parts[i].type;
	if (t && elements[t].ChangeType)
	{
		(*(elements[t].ChangeType))(this, i, x, y, t, PT_NONE);
	}

	if (x >= 0 && y >= 0 && x < XRES && y < YRES)
	{
		if (pmap[y][x] && ID(pmap[y][x]) == i)
			pmap[y][x] = 0;
		else if (photons[y][x] && ID(photons[y][x]) == i)
			photons[y][x] = 0;
	}

	// This shouldn't happen but ... you never know?
	if (t == PT_NONE)
		return;

	elementCount[t]--;
	RecordOmniLifecycleMutation(t, PT_NONE, OmniLifecycleMutationKind::Kill);
	ClearOmniWaterParcelMass(i);
	ClearOmniCarbonParcelMass(i);
	ClearOmniSolutionState(i, !omniSolutionInternalMutation);

	parts.Free(i);
	NUM_PARTS -= 1;
}

void Parts::Free(int i)
{
	data[i].type = PT_NONE;
	data[i].life = pfree;
	pfree = i;
}

// Changes the type of particle number i, to t.  This also changes pmap at the same time
// Returns true if the particle was killed
bool Simulation::part_change_type(int i, int x, int y, int t)
{
	if (x<0 || y<0 || x>=XRES || y>=YRES || i>=NPART || t<0 || t>=PT_NUM || !parts[i].type)
		return false;

	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	if (!elements[t].Enabled || t == PT_NONE)
	{
		kill_part(i);
		return true;
	}
	if (elements[t].CreateAllowed)
	{
		if (!(*(elements[t].CreateAllowed))(this, i, x, y, t))
			return false;
	}

	if (elements[parts[i].type].ChangeType)
		(*(elements[parts[i].type].ChangeType))(this, i, x, y, parts[i].type, t);
	if (elements[t].ChangeType)
		(*(elements[t].ChangeType))(this, i, x, y, parts[i].type, t);

	if (parts[i].type > 0 && parts[i].type < PT_NUM && elementCount[parts[i].type])
		elementCount[parts[i].type]--;
	elementCount[t]++;

	auto oldType = parts[i].type;
	parts[i].type = t;
	if (IsOmniManagedWaterType(t))
	{
		if (!IsOmniManagedWaterType(oldType) || omniWaterParcelMassKg[i] <= 0.0)
			InitializeOmniWaterParcelMass(i, t);
	}
	else if (IsOmniManagedWaterType(oldType))
	{
		// A Legacy mutation is not evidence of evaporation.  Keep the event in
		// the conservation ledger as an explicit external sink; only the
		// thermal coupling path is allowed to create atmospheric H2O mass.
		ClearOmniWaterParcelMass(i);
	}
	if (IsOmniManagedCarbonType(t))
	{
		if (!IsOmniManagedCarbonType(oldType))
			InitializeOmniCarbonParcelMass(i, t);
	}
	else if (IsOmniManagedCarbonType(oldType))
	{
		ClearOmniCarbonParcelMass(i);
	}
	if (IsOmniManagedSolutionType(t))
	{
		if (!IsOmniManagedSolutionType(oldType))
			InitializeOmniSolutionState(i, t, !omniSolutionInternalMutation);
	}
	else if (IsOmniManagedSolutionType(oldType))
	{
		ClearOmniSolutionState(i, !omniSolutionInternalMutation);
	}
	RecordOmniLifecycleMutation(oldType, t, OmniLifecycleMutationKind::TypeChange);
	if (elements[t].Properties & TYPE_ENERGY)
	{
		photons[y][x] = PMAP(i, t);
		if (pmap[y][x] && ID(pmap[y][x]) == i)
			pmap[y][x] = 0;
	}
	else
	{
		pmap[y][x] = PMAP(i, t);
		if (photons[y][x] && ID(photons[y][x]) == i)
			photons[y][x] = 0;
	}
	return false;
}

//the function for creating a particle, use p=-1 for creating a new particle, -2 is from a brush, or a particle number to replace a particle.
//tv = Type (PMAPBITS bits) + Var (32-PMAPBITS bits), var is usually 0
int Simulation::create_part(int p, int x, int y, int t, int v)
{
	int i, oldType = PT_NONE;
	double previousOmniCarbonMassKg = 0.0;
	double previousOmniWaterMassKg = 0.0;
	double previousOmniWaterSpecificEnthalpyJPerKg = 0.0;
	double previousOmniSolutionSolventMassKg = 0.0;
	double previousOmniSolutionSoluteMassKg = 0.0;
	double previousOmniSolutionNeutralSaltMassKg = 0.0;

	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	if (x<0 || y<0 || x>=XRES || y>=YRES || t<=0 || t>=PT_NUM || !elements[t].Enabled)
		return -1;

	if (t == PT_SPRK && p != -3 && !(p == -2 && elements[TYP(pmap[y][x])].CtypeDraw))
	{
		int type = TYP(pmap[y][x]);
		int index = ID(pmap[y][x]);
		if(type == PT_WIRE)
		{
			parts[index].ctype = PT_DUST;
			return index;
		}
		if (!(type == PT_INST || (elements[type].Properties&PROP_CONDUCTS)) || parts[index].life!=0)
			return -1;
		if (p == -2 && type == PT_INST)
		{
			FloodINST(x, y);
			return index;
		}
		parts[index].type = PT_SPRK;
		RecordOmniLifecycleMutation(type, PT_SPRK, OmniLifecycleMutationKind::SparkFastPath);
		parts[index].life = 4;
		parts[index].ctype = type;
		pmap[y][x] = (pmap[y][x]&~PMAPMASK) | PT_SPRK;
		if (parts[index].temp+10.0f < 673.0f && !legacy_enable && (type==PT_METL || type == PT_BMTL || type == PT_BRMT || type == PT_PSCN || type == PT_NSCN || type == PT_ETRD || type == PT_NBLE || type == PT_IRON))
			parts[index].temp = parts[index].temp+10.0f;
		return index;
	}

	if (p == -2)
	{
		if (pmap[y][x])
		{
			int drawOn = TYP(pmap[y][x]);
			if (elements[drawOn].CtypeDraw)
				elements[drawOn].CtypeDraw(this, ID(pmap[y][x]), t, v);
			return -1;
		}
		else if (IsWallBlocking(x, y, t))
			return -1;
		else if (photons[y][x] && (elements[t].Properties & TYPE_ENERGY))
			return -1;
	}

	if (elements[t].CreateAllowed)
	{
		if (!(*(elements[t].CreateAllowed))(this, p, x, y, t))
			return -1;
	}

	if (p == -1 || //creating from anything but brush
	    p == -2 || //creating from brush
	    p == -3) //skip pmap checks, e.g. for sing explosion
	{
		if (p == -1)
		{
			// If there is a particle, only allow creation if the new particle can occupy the same space as the existing particle
			// If there isn't a particle but there is a wall, check whether the new particle is allowed to be in it
			//   (not "!=2" for wall check because eval_move returns 1 for moving into empty space)
			// If there's no particle and no wall, assume creation is allowed
			if (pmap[y][x] ? (eval_move(t, x, y, nullptr) != 2) : (bmap[y/CELL][x/CELL] && eval_move(t, x, y, nullptr) == 0))
			{
				return -1;
			}
		}
		i = parts.Alloc();
		if (i == -1)
		{
			return -1;
		}
		NUM_PARTS += 1;
	}
	else
	{
		int oldX = (int)(parts[p].x + 0.5f);
		int oldY = (int)(parts[p].y + 0.5f);

		if (InBounds(oldX, oldY))
		{
			if (pmap[oldY][oldX] && ID(pmap[oldY][oldX]) == p)
				pmap[oldY][oldX] = 0;
			if (photons[oldY][oldX] && ID(photons[oldY][oldX]) == p)
				photons[oldY][oldX] = 0;
		}

		oldType = parts[p].type;
		if (IsOmniManagedWaterType(oldType) && omniWaterParcelMassKg[p] > 0.0)
		{
			previousOmniWaterMassKg = omniWaterParcelMassKg[p];
			previousOmniWaterSpecificEnthalpyJPerKg =
				omniWaterParcelSpecificEnthalpyJPerKg[p];
			if (!IsOmniManagedWaterType(t))
			{
				// Replacement is an external lifecycle operation, not a phase
				// transition.  Its mass/energy is accounted by Clear below.
				ClearOmniWaterParcelMass(p);
			}
		}
		if (IsOmniManagedCarbonType(oldType) && omniCarbonParcelMassKg[p] > 0.0)
		{
			previousOmniCarbonMassKg = omniCarbonParcelMassKg[p];
			if (!IsOmniManagedCarbonType(t))
				ClearOmniCarbonParcelMass(p);
		}
		if (IsOmniManagedSolutionType(oldType))
		{
			previousOmniSolutionSolventMassKg = omniSolutionSolventMassKg[p];
			previousOmniSolutionSoluteMassKg = omniSolutionSoluteMassKg[p];
			previousOmniSolutionNeutralSaltMassKg = omniSolutionNeutralSaltMassKg[p];
			if (!IsOmniManagedSolutionType(t))
				ClearOmniSolutionState(p, !omniSolutionInternalMutation);
		}

		if (elements[oldType].ChangeType)
			(*(elements[oldType].ChangeType))(this, p, oldX, oldY, oldType, t);
		if (oldType)
			elementCount[oldType]--;

		i = p;
	}

	parts[i] = elements[t].DefaultProperties;
	parts[i].type = t;
	if (IsOmniManagedWaterType(oldType) && IsOmniManagedWaterType(t) &&
		previousOmniWaterMassKg > 0.0)
	{
		omniWaterParcelMassKg[i] = previousOmniWaterMassKg;
		omniWaterParcelSpecificEnthalpyJPerKg[i] =
			previousOmniWaterSpecificEnthalpyJPerKg;
	}
	else
	{
		InitializeOmniWaterParcelMass(i, t);
	}
	if (IsOmniManagedCarbonType(oldType) && IsOmniManagedCarbonType(t) &&
		previousOmniCarbonMassKg > 0.0)
	{
		omniCarbonParcelMassKg[i] = previousOmniCarbonMassKg;
	}
	else
	{
		InitializeOmniCarbonParcelMass(i, t);
	}
	if (IsOmniManagedSolutionType(oldType) && IsOmniManagedSolutionType(t) &&
		(previousOmniSolutionSolventMassKg > 0.0 || previousOmniSolutionSoluteMassKg > 0.0 ||
			previousOmniSolutionNeutralSaltMassKg > 0.0))
	{
		omniSolutionSolventMassKg[i] = previousOmniSolutionSolventMassKg;
		omniSolutionSoluteMassKg[i] = previousOmniSolutionSoluteMassKg;
		omniSolutionNeutralSaltMassKg[i] = previousOmniSolutionNeutralSaltMassKg;
	}
	else
	{
		InitializeOmniSolutionState(i, t,
			!IsOmniManagedSolutionType(oldType) && !omniSolutionInternalMutation);
	}
	if (oldType == PT_NONE)
		RecordOmniLifecycleMutation(PT_NONE, t, OmniLifecycleMutationKind::Create);
	else
		RecordOmniLifecycleMutation(oldType, t, OmniLifecycleMutationKind::Replacement);
	parts[i].x = (float)x;
	parts[i].y = (float)y;

	//and finally set the pmap/photon maps to the newly created particle
	if (elements[t].Properties & TYPE_ENERGY)
		photons[y][x] = PMAP(i, t);
	else if (t!=PT_STKM && t!=PT_STKM2 && t!=PT_FIGH)
		pmap[y][x] = PMAP(i, t);

	//Fancy dust effects for powder types
	if((elements[t].Properties & TYPE_PART) && pretty_powder)
	{
		int colr, colg, colb;
		int sandcolourToUse = p == -2 ? sandcolour_interface : sandcolour;
		RGB colour = elements[t].Colour;
		colr = colour.Red   + int(sandcolourToUse * 1.3) + rng.between(-20, 20) + rng.between(-15, 15);
		colg = colour.Green + int(sandcolourToUse * 1.3) + rng.between(-20, 20) + rng.between(-15, 15);
		colb = colour.Blue  + int(sandcolourToUse * 1.3) + rng.between(-20, 20) + rng.between(-15, 15);
		colr = std::clamp(colr, 0, 255);
		colg = std::clamp(colg, 0, 255);
		colb = std::clamp(colb, 0, 255);
		parts[i].dcolour = (rng.between(0, 149)<<24) | (colr<<16) | (colg<<8) | colb;
	}

	// Set non-static properties (such as randomly generated ones)
	if (elements[t].Create)
		(*(elements[t].Create))(this, i, x, y, t, v);

	if (elements[t].ChangeType)
		(*(elements[t].ChangeType))(this, i, x, y, oldType, t);

	elementCount[t]++;
	return i;
}

// Change part type but preserve temperature and velocity.
// May fail if i isn't an existing particle id.
int Simulation::createPartTempVel(int i, int x, int y, int t)
{
	auto temp = parts[i].temp;
	auto vx = parts[i].vx;
	auto vy = parts[i].vy;

	auto np = create_part(i, x, y, t);
	if (np >= 0)
	{
		parts[np].temp = temp;
		parts[np].vx = vx;
		parts[np].vy = vy;
	}

	return np;
}

int Parts::Alloc()
{
	if (pfree != -1)
	{
		auto i = pfree;
		pfree = data[i].life;
		return i;
	}
	if (active < NPART)
	{
		auto i = active;
		active += 1;
		return i;
	}
	return -1;
}

void Simulation::create_gain_photon(int pp)//photons from PHOT going through GLOW
{
	if (parts.MaxPartsReached())
	{
		return;
	}
	auto lr = 2 * rng.between(0, 1) - 1; // -1 or 1
	auto xx = parts[pp].x - lr * 0.3f * parts[pp].vy;
	auto yy = parts[pp].y + lr * 0.3f * parts[pp].vx;
	auto nx = int(xx + 0.5f);
	auto ny = int(yy + 0.5f);
	if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES)
	{
		return;
	}
	auto g = pmap[ny][nx];
	if (TYP(g) != PT_GLOW)
	{
		return;
	}
	auto oldTemp = parts[ID(g)].temp;
	auto i = create_part(-3, nx, ny, PT_PHOT);
	if (i == -1)
	{
		return;
	}
	parts[i].x = xx;
	parts[i].y = yy;
	parts[i].vx = parts[pp].vx;
	parts[i].vy = parts[pp].vy;
	parts[i].temp = oldTemp;
	auto temp_bin = int((parts[i].temp - 273.0f) * 0.25f);
	if (temp_bin < 0) temp_bin = 0;
	if (temp_bin > 25) temp_bin = 25;
	parts[i].ctype = 0x1F << temp_bin;
}

void Simulation::create_cherenkov_photon(int pp)//photons from NEUT going through GLAS
{
	if (parts.MaxPartsReached())
	{
		return;
	}
	auto nx = int(parts[pp].x + 0.5f);
	auto ny = int(parts[pp].y + 0.5f);
	auto g = pmap[ny][nx];
	if (!IsRefractingGlass(TYP(g)))
	{
		return;
	}
	if (std::hypot(parts[pp].vx, parts[pp].vy) < 1.44f)
	{
		return;
	}
	auto oldTemp = parts[ID(g)].temp;
	auto i = create_part(-3, nx, ny, PT_PHOT);
	if (i == -1)
	{
		return;
	}
	auto lr = 2 * rng.between(0, 1) - 1; // -1 or 1
	parts[i].ctype = 0x00000F80;
	parts[i].x = parts[pp].x;
	parts[i].y = parts[pp].y;
	parts[i].temp = oldTemp;
	parts[i].vx = parts[pp].vx - lr * 2.5f * parts[pp].vy;
	parts[i].vy = parts[pp].vy + lr * 2.5f * parts[pp].vx;
	/* photons have speed of light. no discussion. */
	auto r = 1.269f / std::hypot(parts[i].vx, parts[i].vy);
	parts[i].vx *= r;
	parts[i].vy *= r;
}

void Simulation::GetGravityField(int x, int y, float particleGrav, float newtonGrav, float & pGravX, float & pGravY) const
{
	switch (gravityMode)
	{
	default:
	case GRAV_VERTICAL: //normal, vertical gravity
		pGravX = 0;
		pGravY = particleGrav;
		break;
	case GRAV_OFF: //no gravity
		pGravX = 0;
		pGravY = 0;
		break;
	case GRAV_RADIAL: //radial gravity
		{
			pGravX = 0;
			pGravY = 0;
			auto dx = float(x - XCNTR);
			auto dy = float(y - YCNTR);
			if (dx || dy)
			{
				auto pGravD = 0.01f - hypotf(dx, dy);
				pGravX = particleGrav * (dx / pGravD);
				pGravY = particleGrav * (dy / pGravD);
			}
		}
		break;
	case GRAV_CUSTOM: //custom gravity
		pGravX = particleGrav * customGravityX;
		pGravY = particleGrav * customGravityY;
		break;
	}
	if (newtonGrav)
	{
		pGravX += newtonGrav * gravOut.forceX[Vec2{ x, y } / CELL];
		pGravY += newtonGrav * gravOut.forceY[Vec2{ x, y } / CELL];
	}
}

void Simulation::delete_part(int x, int y)//calls kill_part with the particle located at x,y
{
	unsigned i;

	if (x<0 || y<0 || x>=XRES || y>=YRES)
		return;

	i = photons[y][x] ? photons[y][x] : pmap[y][x];

	if (!i)
		return;
	kill_part(ID(i));
}

template<bool UpdateEmap, class Sim>
Simulation::PlanMoveResult Simulation::PlanMove(Sim &sim, int i, int x, int y)
{
	auto &parts = sim.parts;
	auto &bmap = sim.bmap;
	auto &emap = sim.emap;
	auto &pmap = sim.pmap;
	auto edgeMode = sim.edgeMode;
	auto &sd = SimulationData::CRef();
	auto &can_move = sd.can_move;
	auto t = parts[i].type;
	int fin_x, fin_y, clear_x, clear_y;
	float fin_xf, fin_yf, clear_xf, clear_yf;
	auto vx = parts[i].vx;
	auto vy = parts[i].vy;
	auto mv = fmaxf(fabsf(vx), fabsf(vy));
	if (mv < ISTP || std::isnan(mv))
	{
		clear_x = x;
		clear_y = y;
		clear_xf = parts[i].x;
		clear_yf = parts[i].y;
		fin_xf = clear_xf + vx;
		fin_yf = clear_yf + vy;
		fin_x = (int)(fin_xf+0.5f);
		fin_y = (int)(fin_yf+0.5f);
	}
	else
	{
		if (mv > MAX_VELOCITY)
		{
			vx *= MAX_VELOCITY/mv;
			vy *= MAX_VELOCITY/mv;
			mv = MAX_VELOCITY;
		}
		// interpolate to see if there is anything in the way
		auto dx = vx*ISTP/mv;
		auto dy = vy*ISTP/mv;
		fin_xf = parts[i].x;
		fin_yf = parts[i].y;
		fin_x = (int)(fin_xf+0.5f);
		fin_y = (int)(fin_yf+0.5f);
		bool closedEholeStart = InBounds(fin_x, fin_y) && (bmap[fin_y/CELL][fin_x/CELL] == WL_EHOLE && !emap[fin_y/CELL][fin_x/CELL]);
		while (1)
		{
			mv -= ISTP;
			fin_xf += dx;
			fin_yf += dy;
			fin_x = (int)(fin_xf+0.5f);
			fin_y = (int)(fin_yf+0.5f);
			if (edgeMode == EDGE_LOOP)
			{
				bool x_ok = (fin_xf >= CELL-.5f && fin_xf < XRES-CELL-.5f);
				bool y_ok = (fin_yf >= CELL-.5f && fin_yf < YRES-CELL-.5f);
				if (!x_ok)
					fin_xf = remainder_p(fin_xf-CELL+.5f, XRES-CELL*2.0f)+CELL-.5f;
				if (!y_ok)
					fin_yf = remainder_p(fin_yf-CELL+.5f, YRES-CELL*2.0f)+CELL-.5f;
				fin_x = (int)(fin_xf+0.5f);
				fin_y = (int)(fin_yf+0.5f);
			}
			if (mv <= 0.0f)
			{
				// nothing found
				fin_xf = parts[i].x + vx;
				fin_yf = parts[i].y + vy;
				if (edgeMode == EDGE_LOOP)
				{
					bool x_ok = (fin_xf >= CELL-.5f && fin_xf < XRES-CELL-.5f);
					bool y_ok = (fin_yf >= CELL-.5f && fin_yf < YRES-CELL-.5f);
					if (!x_ok)
						fin_xf = remainder_p(fin_xf-CELL+.5f, XRES-CELL*2.0f)+CELL-.5f;
					if (!y_ok)
						fin_yf = remainder_p(fin_yf-CELL+.5f, YRES-CELL*2.0f)+CELL-.5f;
				}
				fin_x = (int)(fin_xf+0.5f);
				fin_y = (int)(fin_yf+0.5f);
				clear_xf = fin_xf-dx;
				clear_yf = fin_yf-dy;
				clear_x = (int)(clear_xf+0.5f);
				clear_y = (int)(clear_yf+0.5f);
				break;
			}
			//block if particle can't move (0), or some special cases where it returns 1 (can_move = 3 but returns 1 meaning particle will be eaten)
			//also photons are still blocked (slowed down) by any particle (even ones it can move through), and absorb wall also blocks particles
			int eval = sim.eval_move(t, fin_x, fin_y, nullptr);
			if (!eval || (can_move[t][TYP(pmap[fin_y][fin_x])] == 3 && eval == 1) || (t == PT_PHOT && pmap[fin_y][fin_x]) || bmap[fin_y/CELL][fin_x/CELL]==WL_DESTROYALL || closedEholeStart!=(bmap[fin_y/CELL][fin_x/CELL] == WL_EHOLE && !emap[fin_y/CELL][fin_x/CELL]))
			{
				// found an obstacle
				clear_xf = fin_xf-dx;
				clear_yf = fin_yf-dy;
				clear_x = (int)(clear_xf+0.5f);
				clear_y = (int)(clear_yf+0.5f);
				break;
			}
			if constexpr (UpdateEmap)
			{
				if (sim.bmap[fin_y/CELL][fin_x/CELL]==WL_DETECT && sim.emap[fin_y/CELL][fin_x/CELL]<8)
					sim.set_emap(fin_x/CELL, fin_y/CELL);
			}
		}
	}
	return {
		fin_x,
		fin_y,
		clear_x,
		clear_y,
		fin_xf,
		fin_yf,
		clear_xf,
		clear_yf,
		vx,
		vy,
	};
}

template
Simulation::PlanMoveResult Simulation::PlanMove<false, const Simulation>(const Simulation &sim, int i, int x, int y);

std::unique_ptr<Simulation> Simulation::Factory()
{
	return std::make_unique<SimulationImpl>();
}

SimulationImpl::Neighbourhood SimulationImpl::GetNeighbourhood(int i) const
{
	auto t = parts[i].type;
	auto x = int(parts[i].x + 0.5f);
	auto y = int(parts[i].y + 0.5f);
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	Neighbourhood n;
	auto j = 0;
	for (auto nx=-1; nx<2; nx++)
	{
		for (auto ny=-1; ny<2; ny++)
		{
			if (nx||ny)
			{
				auto r = pmap[y+ny][x+nx];
				n.surround[j] = r;
				j++;
				n.surround_space += (!TYP(r)); // count empty space
				n.nt += (TYP(r)!=t); // count empty space and particles of different type
			}
		}
	}
	if (!(elements[t].Properties & TYPE_SOLID) && (elements[t].Gravity || elements[t].NewtonianGravity))
	{
		GetGravityField(x, y, elements[t].Gravity, elements[t].NewtonianGravity, n.pGravX, n.pGravY);
	}
	return n;
}

void SimulationImpl::UpdateParticles(int start, int end)
{
	//the main particle loop function, goes over all particles.
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	for (auto i = start; i < end && i < parts.active; i++)
	{
		auto t = parts[i].type;
		if (!t)
		{
			continue;
		}
		debug_mostRecentlyUpdated = i;

		auto x = int(parts[i].x+0.5f);
		auto y = int(parts[i].y+0.5f);

		// Kill a particle off screen
		if (x<CELL || y<CELL || x>=XRES-CELL || y>=YRES-CELL)
		{
			kill_part(i);
			continue;
		}

		// Kill a particle in a wall where it isn't supposed to go
		if (bmap[y/CELL][x/CELL] &&
		   (bmap[y/CELL][x/CELL]==WL_WALL ||
		    bmap[y/CELL][x/CELL]==WL_WALLELEC ||
		    bmap[y/CELL][x/CELL]==WL_ALLOWAIR ||
		    (bmap[y/CELL][x/CELL]==WL_DESTROYALL) ||
		    (bmap[y/CELL][x/CELL]==WL_ALLOWLIQUID && !(elements[t].Properties&TYPE_LIQUID)) ||
		    (bmap[y/CELL][x/CELL]==WL_ALLOWPOWDER && !(elements[t].Properties&TYPE_PART)) ||
		    (bmap[y/CELL][x/CELL]==WL_ALLOWGAS && !(elements[t].Properties&TYPE_GAS)) || //&& elements[t].Falldown!=0 && parts[i].type!=PT_FIRE && parts[i].type!=PT_SMKE && parts[i].type!=PT_CFLM) ||
		            (bmap[y/CELL][x/CELL]==WL_ALLOWENERGY && !(elements[t].Properties&TYPE_ENERGY)) ||
		    (bmap[y/CELL][x/CELL]==WL_EWALL && !emap[y/CELL][x/CELL])) && (t!=PT_STKM) && (t!=PT_STKM2) && (t!=PT_FIGH))
		{
			kill_part(i);
			continue;
		}

		// Make sure that STASIS'd particles don't tick.
		if (bmap[y/CELL][x/CELL] == WL_STASIS && emap[y/CELL][x/CELL]<8) {
			continue;
		}

		if (bmap[y/CELL][x/CELL]==WL_DETECT && emap[y/CELL][x/CELL]<8)
			set_emap(x/CELL, y/CELL);

		// Managed Enhanced water uses an explicit deferred momentum/thermal
		// coupling boundary. Do not also apply Legacy Air source writes.
		if (!(IsOmniAtmosphereActive() && IsOmniManagedWaterType(t)))
		{
			vx[y/CELL][x/CELL] = vx[y/CELL][x/CELL]*elements[t].AirLoss + elements[t].AirDrag*parts[i].vx;
			vy[y/CELL][x/CELL] = vy[y/CELL][x/CELL]*elements[t].AirLoss + elements[t].AirDrag*parts[i].vy;
		}
		if (elements[t].HotAir && !(IsOmniAtmosphereActive() && IsOmniManagedWaterType(t)))
		{
			if (t==PT_GAS||t==PT_NBLE)
			{
				if (pv[y/CELL][x/CELL]<3.5f)
					pv[y/CELL][x/CELL] += 4.0f*elements[t].HotAir*(3.5f-pv[y/CELL][x/CELL]);
			}
			else//add the hotair variable to the pressure map, like black hole, or white hole.
			{
				pv[y/CELL][x/CELL] += 4.0f*elements[t].HotAir;
			}
		}

		auto neighbourhood = GetNeighbourhood(i);

		//velocity updates for the particle
		if (t != PT_SPNG || !(parts[i].flags&FLAG_MOVABLE))
		{
			parts[i].vx *= elements[t].Loss;
			parts[i].vy *= elements[t].Loss;
		}
		//particle gets velocity from the vx and vy maps
		parts[i].vx += elements[t].Advection*vx[y/CELL][x/CELL] + neighbourhood.pGravX;
		parts[i].vy += elements[t].Advection*vy[y/CELL][x/CELL] + neighbourhood.pGravY;


		if (elements[t].Diffusion)//the random diffusion that gasses have
		{
			parts[i].vx += elements[t].Diffusion*(2.0f*rng.uniform01()-1.0f);
			parts[i].vy += elements[t].Diffusion*(2.0f*rng.uniform01()-1.0f);
		}

		auto transitionOccurred = TransitionPhase(i, neighbourhood);
		if (!parts[i].type)
		{
			continue;
		}
		if (transitionOccurred)
		{
			t = parts[i].type;
		}

		const bool omniSolutionHandled = UpdateOmniSolutionParticle(i, x, y);
		if (!parts[i].type)
			continue;
		if (omniSolutionHandled)
		{
			// Enhanced solution state owns SALT/SLTW chemistry. Preserve the
			// ordinary movement phase below, but do not run Legacy random
			// dissolution/crystallisation a second time.
			x = int(parts[i].x + 0.5f);
			y = int(parts[i].y + 0.5f);
		}

		// Call the element update even when solution composition was handled.
		// WATR/DSTW/SLTW guard only their salt-specific Legacy branches, while
		// extinguishing, erosion, plant and special-element gameplay remains.
		if (!(omniSolutionHandled && (t == PT_ACID || t == PT_BASE)) && elements[t].Update)
		{
			if ((*(elements[t].Update))(this, i, x, y, neighbourhood.surround_space, neighbourhood.nt, parts, pmap))
				continue;
			x = int(parts[i].x+0.5f);
			y = int(parts[i].y+0.5f);
		}

		if (!omniSolutionHandled && legacy_enable)//if heat sim is off
			Element::legacyUpdate(this, i,x,y,neighbourhood.surround_space,neighbourhood.nt, parts, pmap);

		if (parts[i].type == PT_NONE)//if its dead, skip to next particle
			continue;

		if (transitionOccurred)
			continue;

		if (!parts[i].vx&&!parts[i].vy)//if its not moving, skip to next particle, movement code it next
			continue;

		MovementPhase(i, neighbourhood);
	}
}

bool SimulationImpl::TransitionPhase(int i, const Neighbourhood &neighbourhood)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

	auto t = parts[i].type;
	auto x = int(parts[i].x + 0.5f);
	auto y = int(parts[i].y + 0.5f);
	bool transitionOccurred = false;
	if (IsOmniAtmosphereActive() && IsOmniManagedSolutionType(t))
	{
		// Composition, evaporation and crystallisation for SALT/SLTW are owned
		// by UpdateOmniSolutionParticle in Enhanced/Scientific. This prevents
		// the Legacy threshold transition from destroying authoritative masses.
		return false;
	}
	if (QueueOmniWaterParticleCoupling(i, x, y))
	{
		// Managed water reconstructs enthalpy from the public temperature
		// projection and commits all gas/particle exchanges after iteration.
		// Classic and all unmanaged materials retain the original path below.
		return false;
	}
	if (!legacy_enable)
	{
		float gel_scale = 1.0f;
		if (t==PT_GEL)
			gel_scale = parts[i].tmp*2.55f;

		if ((elements[t].Properties&TYPE_LIQUID) && (t!=PT_GEL || gel_scale > (1 + rng.between(0, 254))))
		{
			float convGravX, convGravY;
			GetGravityField(x, y, -2.0f, -2.0f, convGravX, convGravY);
			auto offsetX = std::clamp(int(std::round(convGravX + x)), x-1, x+1);
			auto offsetY = std::clamp(int(std::round(convGravY + y)), y-1, y+1);
			// Some heat convection for liquids
			if (offsetX != x || offsetY != y)
			{
				auto r = pmap[offsetY][offsetX];
				if (r && parts[i].type == TYP(r))
				{
					if (parts[i].temp>parts[ID(r)].temp)
					{
						auto swappage = parts[i].temp;
						parts[i].temp = parts[ID(r)].temp;
						parts[ID(r)].temp = swappage;
					}
				}
			}
		}

		// Heat transfer code
		if (t && !sd.IsHeatInsulator(parts[i]) && rng.chance(int(elements[t].HeatConduct*gel_scale), 250))
		{
			// Heat transfer with air
			if (aheat_enable && !(elements[t].Properties&PROP_NOAMBHEAT))
			{
				auto dtemp = hv[y/CELL][x/CELL] - parts[i].temp; // Temperature difference
				auto hc = sd.HeatCapacityOf(parts[i]);
				auto alpha = std::min(0.04f, 0.4f * hc); // alpha / heat_capacity must be < 1

				// Here we completely ignore that there are CELL^2 "air pixels" in a cell, and the heat capacity of air
				parts[i].temp = restrict_flt(parts[i].temp + alpha*dtemp / hc, MIN_TEMP, MAX_TEMP);
				hv[y/CELL][x/CELL] = restrict_flt(hv[y/CELL][x/CELL] - alpha*dtemp, MIN_TEMP, MAX_TEMP);
			}

			// Heat transfer with other elements
			auto hc_total = 0.0f; // Total heat capacity of elements involved
			auto c_heat = 0.0f; // Total heat distributed between elements
			int surround_hconduct[8]; // IDs of elements which exchange heat

			for (auto j=0; j<8; j++)
			{
				surround_hconduct[j] = i;
				auto r = neighbourhood.surround[j];

				if (!r)
					continue;

				auto rt = TYP(r);

				// Check if we can conduct heat
				if (!rt || sd.IsHeatInsulator(parts[ID(r)])
				        || (t == PT_FILT && (rt == PT_BRAY || rt == PT_BIZR || rt == PT_BIZRG))
				        || (rt == PT_FILT && (t == PT_BRAY || t == PT_PHOT || t == PT_BIZR || t == PT_BIZRG))
				        || (t == PT_ELEC && rt == PT_DEUT)
				        || (t == PT_DEUT && rt == PT_ELEC)
				        || (t == PT_HSWC && rt == PT_FILT && parts[i].tmp == 1)
				        || (t == PT_FILT && rt == PT_HSWC && parts[ID(r)].tmp == 1))
					continue;

				surround_hconduct[j] = ID(r);
				auto hc = sd.HeatCapacityOf(parts[ID(r)]);
				c_heat += parts[ID(r)].temp*hc;
				hc_total += hc;
			}

			// Add the current particle
			auto hc = sd.HeatCapacityOf(parts[i]);
			c_heat += parts[i].temp*hc;
			hc_total += hc;

			// Equilibrium temperature
			float pt = restrict_flt(c_heat / hc_total, MIN_TEMP, MAX_TEMP);

			parts[i].temp = pt;
			for (auto j=0; j<8; j++)
			{
				parts[surround_hconduct[j]].temp = pt;
			}

			auto ctemph = pt;
			auto ctempl = pt;
			// change boiling point with pressure
			if (((elements[t].Properties&TYPE_LIQUID) && sd.IsElementOrNone(elements[t].HighTemperatureTransition) && (elements[elements[t].HighTemperatureTransition].Properties&TYPE_GAS))
			        || t==PT_LNTG || t==PT_SLTW)
				ctemph -= 2.0f*pv[y/CELL][x/CELL];
			else if (((elements[t].Properties&TYPE_GAS) && sd.IsElementOrNone(elements[t].LowTemperatureTransition) && (elements[elements[t].LowTemperatureTransition].Properties&TYPE_LIQUID))
			         || t==PT_WTRV)
				ctempl -= 2.0f*pv[y/CELL][x/CELL];
			auto s = 1;

			//A fix for ice with ctype = 0
			if ((t==PT_ICEI || t==PT_SNOW) && (!sd.IsElement(parts[i].ctype) || parts[i].ctype==PT_ICEI || parts[i].ctype==PT_SNOW))
				parts[i].ctype = PT_WATR;

			if (elements[t].HighTemperatureTransition != NT && ctemph>=elements[t].HighTemperature)
			{
				// particle type change due to high temperature
				if (elements[t].HighTemperatureTransition != ST)
				{
					if (t == PT_FOG)
						parts[i].ctype = 0; // clear unnecessary ctype
					t = elements[t].HighTemperatureTransition;
				}
				else if (t == PT_ICEI || t == PT_SNOW)
				{
					if (parts[i].ctype > 0 && parts[i].ctype < PT_NUM && parts[i].ctype != t)
					{
						if (elements[parts[i].ctype].LowTemperatureTransition==PT_ICEI || elements[parts[i].ctype].LowTemperatureTransition==PT_SNOW)
						{
							if (pt<elements[parts[i].ctype].LowTemperature)
								s = 0;
						}
						else if (pt<273.15f)
							s = 0;

						if (s)
						{
							t = parts[i].ctype;
							parts[i].ctype = PT_NONE;
							parts[i].life = 0;
						}
					}
					else
						s = 0;
				}
				else if (t == PT_SLTW)
				{
					//@ SLTW -> SALT/WTRV
					t = rng.chance(1, 4) ? PT_SALT : PT_WTRV;
				}
				else if (t == PT_BRMT)
				{
					if (IsOmniRecoverableScrap(parts[i]))
					{
						// The metallurgy update uses the recorded source metal's
						// melting point. Do not let generic BRMT's 1273 K fallback
						// erase the carried type first.
						s = 0;
					}
					//@ BRMT(TUNG) -> LAVA(TUNG)
					else if (parts[i].ctype == PT_TUNG)
					{
						if (ctemph < elements[parts[i].ctype].HighTemperature)
							s = 0;
					else
					{
						t = PT_LAVA;
						auto oldType = parts[i].type;
						parts[i].type = PT_TUNG;
						RecordOmniLifecycleMutation(oldType, PT_TUNG, OmniLifecycleMutationKind::BrmtTungPreparation);
					}
					}
					else if (ctemph >= elements[t].HighTemperature)
						t = PT_LAVA;
					else
						s = 0;
				}
				else if (t == PT_CRMC)
				{
					float pres = std::max((pv[y/CELL][x/CELL]+pv[(y-2)/CELL][x/CELL]+pv[(y+2)/CELL][x/CELL]+pv[y/CELL][(x-2)/CELL]+pv[y/CELL][(x+2)/CELL])*2.0f, 0.0f);
					if (ctemph < pres+elements[PT_CRMC].HighTemperature)
						s = 0;
					else
						t = PT_LAVA;
				}
				else if (t == PT_RIME)
				{
					if (parts[i].tmp > 5)
					{
						//@ RIME -> ACID
						t = PT_ACID;
						parts[i].life = 25 + 5 * parts[i].tmp;
						parts[i].tmp = 0;
					}
					else
					{
						//@ RIME -> WATR
						t = parts[i].ctype == PT_DSTW ? PT_DSTW : PT_WATR;
						parts[i].ctype = 0;
					}
				}
				else
					s = 0;
			}
			else if (elements[t].LowTemperatureTransition != NT && ctempl<elements[t].LowTemperature)
			{
				// particle type change due to low temperature
				if (elements[t].LowTemperatureTransition != ST)
				{
					t = elements[t].LowTemperatureTransition;
				}
				else if (t == PT_WTRV)
				{
					//@ WTRV -> RIME/DSTW
					t = (pt < 273.0f) ? PT_RIME : PT_DSTW;
				}
				else if (t == PT_LAVA)
				{
					if (OmniIsotopeOwnsMoltenTransition(parts[i]))
						s = 0;
					else if (parts[i].ctype > 0 && parts[i].ctype < PT_NUM && parts[i].ctype != PT_LAVA && elements[parts[i].ctype].Enabled)
					{
						if (parts[i].ctype == PT_THRM && pt >= elements[PT_BMTL].HighTemperature)
							s = 0;
						else if ((parts[i].ctype == PT_VIBR || parts[i].ctype == PT_BVBR) && pt >= 273.15f)
							s = 0;
						else if (parts[i].ctype == PT_TUNG)
						{
							// TUNG does its own melting in its update function, so HighTemperatureTransition is not LAVA so it won't be handled by the code for HighTemperatureTransition==PT_LAVA below
							// However, the threshold is stored in HighTemperature to allow it to be changed from Lua
							if (pt >= elements[parts[i].ctype].HighTemperature)
								s = 0;
						}
						else if (parts[i].ctype == PT_CRMC)
						{
							float pres = std::max((pv[y/CELL][x/CELL]+pv[(y-2)/CELL][x/CELL]+pv[(y+2)/CELL][x/CELL]+pv[y/CELL][(x-2)/CELL]+pv[y/CELL][(x+2)/CELL])*2.0f, 0.0f);
							if (ctemph >= pres+elements[PT_CRMC].HighTemperature)
								s = 0;
						}
						else if (elements[parts[i].ctype].HighTemperatureTransition == PT_LAVA || parts[i].ctype == PT_HEAC)
						{
							if (pt >= elements[parts[i].ctype].HighTemperature)
								s = 0;
						}
						else if (pt>=973.0f)
							s = 0; // freezing point for lava with any other (not listed in ptransitions as turning into lava) ctype
						if (s)
						{
							t = parts[i].ctype;
							parts[i].ctype = PT_NONE;
							if (t == PT_THRM)
							{
								//@ LAVA(THRM) -> BMTL
								parts[i].tmp = 0;
								t = PT_BMTL;
							}
							if (t == PT_PLUT)
							{
								//@ LAVA(PLUT) -> LAVA
								parts[i].tmp = 0;
								t = PT_LAVA;
							}
						}
					}
					else if (pt<973.0f)
						t = PT_STNE; //@ LAVA -> STNE
					else
						s = 0;
				}
				else
					s = 0;
			}
			else
				s = 0;

			if (s) // particle type change occurred
			{
				if (t==PT_ICEI || t==PT_LAVA || t==PT_SNOW)
					parts[i].ctype = parts[i].type;
				if (t == PT_RIME)
					parts[i].ctype = PT_DSTW;
				if (!(t==PT_ICEI && parts[i].ctype==PT_FRZW) && t!=PT_ACID)
					parts[i].life = 0;
				if (t == PT_FIRE)
				{
					//hackish, if tmp isn't 0 the FIRE might turn into DSTW later
					//idealy transitions should use create_part(i) but some elements rely on properties staying constant
					//and I don't feel like checking each one right now
					parts[i].tmp = 0;

					if (parts[i].type == PT_SEED)
					{
						parts[i].ctype = 0;
						parts[i].tmp2 = 0;
						parts[i].tmp3 = 0;
						parts[i].tmp4 = 0;
					}
				}
				if ((elements[t].Properties&TYPE_GAS) && !(elements[parts[i].type].Properties&TYPE_GAS))
					pv[y/CELL][x/CELL] += 0.50f;

				if (t == PT_NONE)
				{
					kill_part(i);
					return true;
				}
				// part_change_type could refuse to change the type and kill the particle
				// for example, changing type to STKM but one already exists
				// we need to account for that to not cause simulation corruption issues
				if (part_change_type(i,x,y,t))
					return true;

				if (t==PT_FIRE || t==PT_PLSM || t==PT_CFLM)
					parts[i].life = rng.between(120, 169);
				if (t == PT_LAVA)
				{
					if (parts[i].ctype == PT_BRMT) parts[i].ctype = PT_BMTL;
					else if (parts[i].ctype == PT_SAND) parts[i].ctype = PT_GLAS;
					else if (parts[i].ctype == PT_BGLA) parts[i].ctype = PT_GLAS;
					else if (parts[i].ctype == PT_PQRT) parts[i].ctype = PT_QRTZ;
					else if (parts[i].ctype == PT_LITH && parts[i].tmp2 > 3) parts[i].ctype = PT_GLAS;
					parts[i].life = rng.between(240, 359);
				}
				transitionOccurred = true;
			}

			pt = parts[i].temp = restrict_flt(parts[i].temp, MIN_TEMP, MAX_TEMP);
			if (t == PT_LAVA)
			{
				parts[i].life = int(restrict_flt((parts[i].temp-700)/7, 0, 400));
				if (parts[i].ctype==PT_THRM&&parts[i].tmp>0)
				{
					parts[i].tmp--;
					parts[i].temp = 3500;
				}
				if (parts[i].ctype==PT_PLUT&&parts[i].tmp>0)
				{
					parts[i].tmp--;
					parts[i].temp = MAX_TEMP;
				}
			}
		}
		else
		{
			if (!(air->bmap_blockairh[y/CELL][x/CELL]&0x8))
				air->bmap_blockairh[y/CELL][x/CELL]++;
			parts[i].temp = restrict_flt(parts[i].temp, MIN_TEMP, MAX_TEMP);
		}
	}

	if (t==PT_LIFE)
	{
		parts[i].temp = restrict_flt(parts[i].temp-50.0f, MIN_TEMP, MAX_TEMP);
	}
	//spark updates from walls
	if ((elements[t].Properties&PROP_CONDUCTS) || t==PT_SPRK)
	{
		auto nx = x % CELL;
		if (nx == 0)
			nx = x/CELL - 1;
		else if (nx == CELL-1)
			nx = x/CELL + 1;
		else
			nx = x/CELL;
		auto ny = y % CELL;
		if (ny == 0)
			ny = y/CELL - 1;
		else if (ny == CELL-1)
			ny = y/CELL + 1;
		else
			ny = y/CELL;
		if (nx>=0 && ny>=0 && nx<XCELLS && ny<YCELLS)
		{
			if (t!=PT_SPRK)
			{
				if (emap[ny][nx]==12 && !parts[i].life && bmap[ny][nx] != WL_STASIS)
				{
					part_change_type(i,x,y,PT_SPRK);
					parts[i].life = 4;
					parts[i].ctype = t;
					t = PT_SPRK;
				}
			}
			else if (bmap[ny][nx]==WL_DETECT || bmap[ny][nx]==WL_EWALL || bmap[ny][nx]==WL_ALLOWLIQUID || bmap[ny][nx]==WL_WALLELEC || bmap[ny][nx]==WL_ALLOWALLELEC || bmap[ny][nx]==WL_EHOLE)
				set_emap(nx, ny);
		}
	}

	//the basic explosion, from the .explosive variable
	if ((elements[t].Explosive&2) && pv[y/CELL][x/CELL]>2.5f)
	{
		parts[i].life = rng.between(180, 259);
		parts[i].temp = restrict_flt(elements[PT_FIRE].DefaultProperties.temp + (elements[t].Flammable/2), MIN_TEMP, MAX_TEMP);
		t = PT_FIRE;
		part_change_type(i,x,y,t);
		pv[y/CELL][x/CELL] += 0.25f * CFDS;
	}

	{
		auto s = 1;
		auto gravtot = std::abs(gravOut.forceX[Vec2{ x, y } / CELL]) +
		               std::abs(gravOut.forceY[Vec2{ x, y } / CELL]);
		if (elements[t].HighPressureTransition != NT && pv[y/CELL][x/CELL]>elements[t].HighPressure) {
			// particle type change due to high pressure
			if (elements[t].HighPressureTransition != ST)
				t = elements[t].HighPressureTransition;
			else if (t==PT_BMTL) {
				if (pv[y/CELL][x/CELL]>2.5f)
					t = PT_BRMT;
				else if (pv[y/CELL][x/CELL]>1.0f && parts[i].tmp==1)
					t = PT_BRMT;
				else s = 0;
			}
			else s = 0;
		} else if (elements[t].LowPressureTransition != NT && pv[y/CELL][x/CELL]<elements[t].LowPressure && gravtot<=(elements[t].LowPressure/4.0f)) {
			// particle type change due to low pressure
			if (elements[t].LowPressureTransition != ST)
				t = elements[t].LowPressureTransition;
			else s = 0;
		} else if (elements[t].HighPressureTransition != NT && gravtot>(elements[t].HighPressure/4.0f)) {
			// particle type change due to high gravity
			if (elements[t].HighPressureTransition != ST)
				t = elements[t].HighPressureTransition;
			else if (t==PT_BMTL) {
				if (gravtot>0.625f)
					t = PT_BRMT;
				else if (gravtot>0.25f && parts[i].tmp==1)
					t = PT_BRMT;
				else s = 0;
			}
			else s = 0;
		} else s = 0;

		// particle type change occurred
		if (s)
		{
			if (t == PT_NONE)
			{
				kill_part(i);
				return true;
			}
			parts[i].life = 0;

			// To prevent PIPE -> BRMT setting BRMT's ctype
			if (t == PT_BRMT)
			{
				parts[i].ctype = 0;
				parts[i].tmp = 0;
				parts[i].tmp2 = 0;
				parts[i].tmp3 = 0;
				parts[i].tmp4 = 0;
			}

			// part_change_type could refuse to change the type and kill the particle
			// for example, changing type to STKM but one already exists
			// we need to account for that to not cause simulation corruption issues
			if (part_change_type(i,x,y,t))
				return true;
			if (t == PT_FIRE)
				parts[i].life = rng.between(120, 169);
			transitionOccurred = true;
		}
	}
	return transitionOccurred;
}

void SimulationImpl::MovementPhase(int i, Neighbourhood neighbourhood)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

	auto t = parts[i].type;
	auto x = int(parts[i].x+0.5f);
	auto y = int(parts[i].y+0.5f);
	int fin_x, fin_y, clear_x, clear_y;
	float fin_xf, fin_yf, clear_xf, clear_yf;
	{
		auto mr = PlanMove<true>(*this, i, x, y);
		fin_x    = mr.fin_x;
		fin_y    = mr.fin_y;
		clear_x  = mr.clear_x;
		clear_y  = mr.clear_y;
		fin_xf   = mr.fin_xf;
		fin_yf   = mr.fin_yf;
		clear_xf = mr.clear_xf;
		clear_yf = mr.clear_yf;
		parts[i].vx = mr.vx;
		parts[i].vy = mr.vy;
	}

	auto stagnant = parts[i].flags & FLAG_STAGNANT;
	parts[i].flags &= ~FLAG_STAGNANT;

	if (t==PT_STKM || t==PT_STKM2 || t==PT_FIGH)
	{
		//head movement, let head pass through anything
		parts[i].x += parts[i].vx;
		parts[i].y += parts[i].vy;
		int nx = (int)((float)parts[i].x+0.5f);
		int ny = (int)((float)parts[i].y+0.5f);
		if (edgeMode == EDGE_LOOP)
		{
			bool x_ok = (nx >= CELL && nx < XRES-CELL);
			bool y_ok = (ny >= CELL && ny < YRES-CELL);
			int oldnx = nx, oldny = ny;
			if (!x_ok)
			{
				parts[i].x = remainder_p(parts[i].x-CELL+.5f, XRES-CELL*2.0f)+CELL-.5f;
				nx = (int)((float)parts[i].x+0.5f);
			}
			if (!y_ok)
			{
				parts[i].y = remainder_p(parts[i].y-CELL+.5f, YRES-CELL*2.0f)+CELL-.5f;
				ny = (int)((float)parts[i].y+0.5f);
			}

			if (!x_ok || !y_ok) //when moving from left to right stickmen might be able to fall through solid things, fix with "eval_move(t, nx+diffx, ny+diffy, NULL)" but then they die instead
			{
				//adjust stickmen legs
				playerst* stickman = nullptr;
				int t = parts[i].type;
				if (t == PT_STKM)
					stickman = &player;
				else if (t == PT_STKM2)
					stickman = &player2;
				else if (t == PT_FIGH && parts[i].tmp >= 0 && parts[i].tmp < MAX_FIGHTERS)
					stickman = &fighters[parts[i].tmp];

				if (stickman)
					for (int i = 0; i < 16; i+=2)
					{
						stickman->legs[i] += (nx-oldnx);
						stickman->legs[i+1] += (ny-oldny);
						stickman->accs[i/2] *= .95f;
					}
				parts[i].vy *= .95f;
				parts[i].vx *= .95f;
			}
		}
		if (ny!=y || nx!=x)
		{
			if (pmap[y][x] && ID(pmap[y][x]) == i)
				pmap[y][x] = 0;
			else if (photons[y][x] && ID(photons[y][x]) == i)
				photons[y][x] = 0;
			if (nx<CELL || nx>=XRES-CELL || ny<CELL || ny>=YRES-CELL)
			{
				kill_part(i);
				return;
			}
			if (elements[t].Properties & TYPE_ENERGY)
				photons[ny][nx] = PMAP(i, t);
			else if (t)
				pmap[ny][nx] = PMAP(i, t);
		}
	}
	else if (elements[t].Properties & TYPE_ENERGY)
	{
		if (t == PT_PHOT)
		{
			if (parts[i].flags&FLAG_SKIPMOVE)
			{
				parts[i].flags &= ~FLAG_SKIPMOVE;
				return;
			}

			if (eval_move(PT_PHOT, fin_x, fin_y, nullptr))
			{
				int rt = TYP(pmap[fin_y][fin_x]);
				int lt = TYP(pmap[y][x]);
				int rt_glas = IsRefractingGlass(rt);
				int lt_glas = IsRefractingGlass(lt);
				if ((rt_glas && !lt_glas) || (lt_glas && !rt_glas))
				{
					auto gn = get_normal_interp<true>(*this, REFRACT|t, parts[i].x, parts[i].y, parts[i].vx, parts[i].vy);
					if (!gn.success) {
						kill_part(i);
						return;
					}
					auto nrx = gn.nx;
					auto nry = gn.ny;
					auto r = get_wavelength_bin(&parts[i].ctype);
					if (r == -1 || !(parts[i].ctype&0x3FFFFFFF))
					{
						kill_part(i);
						return;
					}
					auto nn = GLASS_IOR - GLASS_DISP*(r-30)/30.0f;
					nn *= nn;

					auto enter = rt_glas && !lt_glas;
					nrx = enter ? -nrx : nrx;
					nry = enter ? -nry : nry;
					nn = enter ? 1.0f/nn : nn;
					auto ct1 = parts[i].vx*nrx + parts[i].vy*nry;
					auto ct2 = 1.0f - (nn*nn)*(1.0f-(ct1*ct1));
					if (ct2 < 0.0f) {
						// total internal reflection
						parts[i].vx -= 2.0f*ct1*nrx;
						parts[i].vy -= 2.0f*ct1*nry;
						fin_xf = parts[i].x;
						fin_yf = parts[i].y;
						fin_x = x;
						fin_y = y;
					} else {
						// refraction
						ct2 = sqrtf(ct2);
						ct2 = ct2 - nn*ct1;
						parts[i].vx = nn*parts[i].vx + ct2*nrx;
						parts[i].vy = nn*parts[i].vy + ct2*nry;
					}
				}
			}
		}
		if (stagnant)//FLAG_STAGNANT set, was reflected on previous frame
		{
			// cast coords as int then back to float for compatibility with existing saves
			if (!do_move(i, x, y, (float)fin_x, (float)fin_y) && parts[i].type) {
				kill_part(i);
				return;
			}
		}
		else if (!do_move(i, x, y, fin_xf, fin_yf))
		{
			if (parts[i].type == PT_NONE)
				return;
			// reflection
			parts[i].flags |= FLAG_STAGNANT;
			if (t==PT_NEUT && rng.chance(1, 10))
			{
				kill_part(i);
				return;
			}
			auto r = pmap[fin_y][fin_x];

			if ((TYP(r)==PT_PIPE || TYP(r) == PT_PPIP) && !TYP(parts[ID(r)].ctype))
			{
				Element_PIPE_transfer_part_to_pipe(parts+i, parts+(ID(r)));
				return;
			}

			if (t == PT_PHOT)
			{
				auto mask = elements[TYP(r)].PhotonReflectWavelengths;
				if (TYP(r) == PT_LITH)
				{
					int wl_bin = parts[ID(r)].ctype / 4;
					if (wl_bin < 0) wl_bin = 0;
					if (wl_bin > 25) wl_bin = 25;
					mask = (0x1F << wl_bin);
				}
				else if (TYP(r) == PT_SEED)
				{
					// Reflect different wavelengths based on SEED's color genes
					int colour = (parts[ID(r)].ctype >> PLNT_COLOUR) & 0x3f;

					mask |= ((colour & 0b110000) != 0) ? 0 : (1 << 25); // Red
					mask |= ((colour & 0b001100) != 0) ? 0 : (1 << 15); // Green
					mask |= ((colour & 0b000011) != 0) ? 0 : (1 << 5); // Blue

				}
				parts[i].ctype &= mask;
			}

			auto gn = get_normal_interp<true>(*this, t, parts[i].x, parts[i].y, parts[i].vx, parts[i].vy);
			if (gn.success)
			{
				auto nrx = gn.nx;
				auto nry = gn.ny;
				if (TYP(r) == PT_CRMC)
				{
					float r = rng.between(-50, 50) * 0.01f, rx, ry, anrx, anry;
					r = r * r * r;
					rx = cosf(r); ry = sinf(r);
					anrx = rx * nrx + ry * nry;
					anry = rx * nry - ry * nrx;
					auto dp = anrx*parts[i].vx + anry*parts[i].vy;
					parts[i].vx -= 2.0f*dp*anrx;
					parts[i].vy -= 2.0f*dp*anry;
				}
				else
				{
					auto dp = nrx*parts[i].vx + nry*parts[i].vy;
					parts[i].vx -= 2.0f*dp*nrx;
					parts[i].vy -= 2.0f*dp*nry;
				}
				// leave the actual movement until next frame so that reflection of fast particles and refraction happen correctly
			}
			else
			{
				if (t!=PT_NEUT)
					kill_part(i);
				return;
			}
			if (!(parts[i].ctype&0x3FFFFFFF) && t == PT_PHOT)
			{
				kill_part(i);
				return;
			}
		}
	}
	else if (elements[t].Falldown==0)
	{
		// gasses and solids (but not powders)
		if (!do_move(i, x, y, fin_xf, fin_yf))
		{
			if (parts[i].type == PT_NONE)
				return;
			// can't move there, so bounce off
			if (fin_x>x+ISTP) fin_x=x+ISTP;
			if (fin_x<x-ISTP) fin_x=x-ISTP;
			if (fin_y>y+ISTP) fin_y=y+ISTP;
			if (fin_y<y-ISTP) fin_y=y-ISTP;
			if (do_move(i, x, y, float(2*x-fin_x), float(fin_y)))
			{
				parts[i].vx *= elements[t].Collision;
			}
			else if (do_move(i, x, y, float(fin_x), float(2*y-fin_y)))
			{
				parts[i].vy *= elements[t].Collision;
			}
			else
			{
				parts[i].vx *= elements[t].Collision;
				parts[i].vy *= elements[t].Collision;
			}
		}
	}
	else
	{
		// Checking stagnant is cool, but then it doesn't update when you change it later.
		if (water_equal_test && elements[t].Falldown == 2 && rng.chance(1, 200))
		{
			if (flood_water(x, y, i))
				return;
		}
		// liquids and powders
		if (!do_move(i, x, y, fin_xf, fin_yf))
		{
			if (parts[i].type == PT_NONE)
				return;
			if (fin_x!=x && do_move(i, x, y, fin_xf, clear_yf))
			{
				parts[i].vx *= elements[t].Collision;
				parts[i].vy *= elements[t].Collision;
			}
			else if (fin_y!=y && do_move(i, x, y, clear_xf, fin_yf))
			{
				parts[i].vx *= elements[t].Collision;
				parts[i].vy *= elements[t].Collision;
			}
			else
			{
				auto pGravX = neighbourhood.pGravX;
				auto pGravY = neighbourhood.pGravY;
				auto r = rng.between(0, 1) * 2 - 1;// position search direction (left/right first)
				if ((clear_x!=x || clear_y!=y || neighbourhood.nt || neighbourhood.surround_space) &&
					(fabsf(parts[i].vx)>0.01f || fabsf(parts[i].vy)>0.01f))
				{
					// allow diagonal movement if target position is blocked
					// but no point trying this if particle is stuck in a block of identical particles
					auto dx = parts[i].vx - parts[i].vy*r;
					auto dy = parts[i].vy + parts[i].vx*r;

					auto mv = std::max(fabsf(dx), fabsf(dy));
					dx /= mv;
					dy /= mv;
					if (do_move(i, x, y, clear_xf+dx, clear_yf+dy))
					{
						parts[i].vx *= elements[t].Collision;
						parts[i].vy *= elements[t].Collision;
						return;
					}
					{
						auto swappage = dx;
						dx = dy*r;
						dy = -swappage*r;
					}
					if (do_move(i, x, y, clear_xf+dx, clear_yf+dy))
					{
						parts[i].vx *= elements[t].Collision;
						parts[i].vy *= elements[t].Collision;
						return;
					}
				}
				if (elements[t].Falldown>1 && !grav && gravityMode==GRAV_VERTICAL && parts[i].vy>fabsf(parts[i].vx))
				{
					auto s = 0;
					// stagnant is true if FLAG_STAGNANT was set for this particle in previous frame
					int rt;
					if (!stagnant || neighbourhood.nt) //nt is if there is an something else besides the current particle type, around the particle
						rt = 30;//slight less water lag, although it changes how it moves a lot
					else
						rt = 10;

					if (t==PT_GEL)
						rt = int(parts[i].tmp*0.20f+5.0f);

					auto nx = -1, ny = -1;
					for (auto j=clear_x+r; j>=0 && j>=clear_x-rt && j<clear_x+rt && j<XRES; j+=r)
					{
						if ((TYP(pmap[fin_y][j])!=t || bmap[fin_y/CELL][j/CELL])
							&& (s=do_move(i, x, y, (float)j, fin_yf)))
						{
							nx = (int)(parts[i].x+0.5f);
							ny = (int)(parts[i].y+0.5f);
							break;
						}
						if (fin_y!=clear_y && (TYP(pmap[clear_y][j])!=t || bmap[clear_y/CELL][j/CELL])
							&& (s=do_move(i, x, y, (float)j, clear_yf)))
						{
							nx = (int)(parts[i].x+0.5f);
							ny = (int)(parts[i].y+0.5f);
							break;
						}
						if (TYP(pmap[clear_y][j])!=t || (bmap[clear_y/CELL][j/CELL] && bmap[clear_y/CELL][j/CELL]!=WL_STREAM))
							break;
					}

					r = (parts[i].vy>0) ? 1 : -1;

					if (s==1)
						for (auto j=ny+r; j>=0 && j<YRES && j>=ny-rt && j<ny+rt; j+=r)
						{
							if ((TYP(pmap[j][nx])!=t || bmap[j/CELL][nx/CELL]) && do_move(i, nx, ny, (float)nx, (float)j))
								break;
							if (TYP(pmap[j][nx])!=t || (bmap[j/CELL][nx/CELL] && bmap[j/CELL][nx/CELL]!=WL_STREAM))
								break;
						}
					else if (s==-1) {} // particle is out of bounds
					else if ((clear_x!=x||clear_y!=y) && do_move(i, x, y, clear_xf, clear_yf)) {}
					else parts[i].flags |= FLAG_STAGNANT;
					parts[i].vx *= elements[t].Collision;
					parts[i].vy *= elements[t].Collision;
				}
				else if (elements[t].Falldown>1 && fabsf(pGravX*parts[i].vx+pGravY*parts[i].vy)>fabsf(pGravY*parts[i].vx-pGravX*parts[i].vy))
				{
					float nxf, nyf, prev_pGravX, prev_pGravY, ptGrav = elements[t].Gravity;
					auto s = 0;
					// stagnant is true if FLAG_STAGNANT was set for this particle in previous frame
					// nt is if there is something else besides the current particle type around the particle
					// 30 gives slightly less water lag, although it changes how it moves a lot
					auto rt = (!stagnant || neighbourhood.nt) ? 30 : 10;

					// clear_xf, clear_yf is the last known position that the particle should almost certainly be able to move to
					nxf = clear_xf;
					nyf = clear_yf;
					auto nx = clear_x;
					auto ny = clear_y;
					// Look for spaces to move horizontally (perpendicular to gravity direction), keep going until a space is found or the number of positions examined = rt
					for (auto j=0;j<rt;j++)
					{
						// Calculate overall gravity direction
						GetGravityField(nx, ny, ptGrav, 1.0f, pGravX, pGravY);
						// Scale gravity vector so that the largest component is 1 pixel
						auto mv = std::max(fabsf(pGravX), fabsf(pGravY));
						if (mv<0.0001f) break;
						pGravX /= mv;
						pGravY /= mv;
						// Move 1 pixel perpendicularly to gravity
						// r is +1/-1, to try moving left or right at random
						if (j)
						{
							// Not quite the gravity direction
							// Gravity direction + last change in gravity direction
							// This makes liquid movement a bit less frothy, particularly for balls of liquid in radial gravity. With radial gravity, instead of just moving along a tangent, the attempted movement will follow the curvature a bit better.
							nxf += r*(pGravY*2.0f-prev_pGravY);
							nyf += -r*(pGravX*2.0f-prev_pGravX);
						}
						else
						{
							nxf += r*pGravY;
							nyf += -r*pGravX;
						}
						prev_pGravX = pGravX;
						prev_pGravY = pGravY;
						// Check whether movement is allowed
						nx = (int)(nxf+0.5f);
						ny = (int)(nyf+0.5f);
						if (nx<0 || ny<0 || nx>=XRES || ny >=YRES)
							break;
						if (TYP(pmap[ny][nx])!=t || bmap[ny/CELL][nx/CELL])
						{
							s = do_move(i, x, y, nxf, nyf);
							if (s)
							{
								// Movement was successful
								nx = (int)(parts[i].x+0.5f);
								ny = (int)(parts[i].y+0.5f);
								break;
							}
							// A particle of a different type, or a wall, was found. Stop trying to move any further horizontally unless the wall should be completely invisible to particles.
							if (TYP(pmap[ny][nx])!=t || bmap[ny/CELL][nx/CELL]!=WL_STREAM)
								break;
						}
					}
					if (s==1)
					{
						// The particle managed to move horizontally, now try to move vertically (parallel to gravity direction)
						// Keep going until the particle is blocked (by something that isn't the same element) or the number of positions examined = rt
						clear_x = nx;
						clear_y = ny;
						for (auto j=0;j<rt;j++)
						{
							// Calculate overall gravity direction
							GetGravityField(nx, ny, ptGrav, 1.0f, pGravX, pGravY);
							// Scale gravity vector so that the largest component is 1 pixel
							auto mv = std::max(fabsf(pGravX), fabsf(pGravY));
							if (mv<0.0001f) break;
							pGravX /= mv;
							pGravY /= mv;
							// Move 1 pixel in the direction of gravity
							nxf += pGravX;
							nyf += pGravY;
							nx = (int)(nxf+0.5f);
							ny = (int)(nyf+0.5f);
							if (nx<0 || ny<0 || nx>=XRES || ny>=YRES)
								break;
							// If the space is anything except the same element (a wall, empty space, or occupied by a particle of a different element), try to move into it
							if (TYP(pmap[ny][nx])!=t || bmap[ny/CELL][nx/CELL])
							{
								s = do_move(i, clear_x, clear_y, nxf, nyf);
								if (s || TYP(pmap[ny][nx])!=t || bmap[ny/CELL][nx/CELL]!=WL_STREAM)
									break; // found the edge of the liquid and movement into it succeeded, so stop moving down
							}
						}
					}
					else if (s==-1) {} // particle is out of bounds
					else if ((clear_x!=x||clear_y!=y) && do_move(i, x, y, clear_xf, clear_yf)) {} // try moving to the last clear position
					else parts[i].flags |= FLAG_STAGNANT;
					parts[i].vx *= elements[t].Collision;
					parts[i].vy *= elements[t].Collision;
				}
				else
				{
					// if interpolation was done, try moving to last clear position
					if ((clear_x!=x||clear_y!=y) && do_move(i, x, y, clear_xf, clear_yf)) {}
					else parts[i].flags |= FLAG_STAGNANT;
					parts[i].vx *= elements[t].Collision;
					parts[i].vy *= elements[t].Collision;
				}
			}
		}
	}
}

void Simulation::RecalcFreeParticles(bool do_life_dec)
{
	FrameTime::Span span(frameTime, "Simulation::RecalcFreeParticles");
	memset(pmap, 0, sizeof(pmap));
	memset(pmap_count, 0, sizeof(pmap_count));
	memset(photons, 0, sizeof(photons));

	NUM_PARTS = 0;
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	//the particle loop that resets the pmap/photon maps every frame, to update them.
	for (int i = 0; i < parts.active; i++)
	{
		if (!parts[i].type)
		{
			continue;
		}
		auto t = parts[i].type;
		auto x = int(parts[i].x+0.5f);
		auto y = int(parts[i].y+0.5f);
		bool inBounds = false;
		if (x>=0 && y>=0 && x<XRES && y<YRES)
		{
			if (elements[t].Properties & TYPE_ENERGY)
				photons[y][x] = PMAP(i, t);
			else
			{
				// Particles are sometimes allowed to go inside INVS and FILT
				// To make particles collide correctly when inside these elements, these elements must not overwrite an existing pmap entry from particles inside them
				if (!pmap[y][x] || (t!=PT_INVIS && t!= PT_FILT))
					pmap[y][x] = PMAP(i, t);
				// (there are a few exceptions, including energy particles - currently no limit on stacking those)
				if (t!=PT_THDR && t!=PT_EMBR && t!=PT_FIGH && t!=PT_PLSM)
					pmap_count[y][x]++;
			}
			inBounds = true;
		}
		NUM_PARTS ++;

		if (elementRecount && t >= 0 && t < PT_NUM && elements[t].Enabled)
			elementCount[t]++;

		//decrease particle life
		if (do_life_dec)
		{
			if (t<0 || t>=PT_NUM || !elements[t].Enabled)
			{
				kill_part(i);
				continue;
			}

			unsigned int elem_properties = elements[t].Properties;
			if (parts[i].life>0 && (elem_properties&PROP_LIFE_DEC) && !(inBounds && bmap[y/CELL][x/CELL] == WL_STASIS && emap[y/CELL][x/CELL]<8))
			{
				// automatically decrease life
				parts[i].life--;
				if (parts[i].life<=0 && (elem_properties&(PROP_LIFE_KILL_DEC|PROP_LIFE_KILL)))
				{
					// kill on change to no life
					kill_part(i);
					continue;
				}
			}
			else if (parts[i].life<=0 && (elem_properties&PROP_LIFE_KILL) && !(inBounds && bmap[y/CELL][x/CELL] == WL_STASIS && emap[y/CELL][x/CELL]<8))
			{
				// kill if no life
				kill_part(i);
				continue;
			}
		}
	}
	parts.Flatten();
	if (elementRecount)
		elementRecount = false;
}

void Parts::Flatten()
{
	int newActive = 0;
	auto *ppfree = &pfree;
	for (int i = 0; i < active; i++)
	{
		if (data[i].type)
		{
			for (auto j = newActive; j < i; ++j)
			{
				*ppfree = j;
				ppfree = &data[j].life;
			}
			newActive = i + 1;
		}
	}
	*ppfree = -1;
	active = newActive;
}

void Simulation::SimulateGoL()
{
	auto &builtinGol = SimulationData::builtinGol;
	CGOL = 0;
	for (int i = 0; i < parts.active; ++i)
	{
		auto &part = parts[i];
		if (part.type != PT_LIFE)
		{
			continue;
		}
		auto x = int(part.x + 0.5f);
		auto y = int(part.y + 0.5f);
		if (x < CELL || y < CELL || x >= XRES - CELL || y >= YRES - CELL)
		{
			continue;
		}
		unsigned int golnum = part.ctype;
		unsigned int ruleset = golnum;
		if (golnum < NGOL)
		{
			ruleset = builtinGol[golnum].ruleset;
			golnum += 1;
		}
		if (part.tmp2 == int((ruleset >> 17) & 0xF) + 1)
		{
			for (int yy = -1; yy <= 1; ++yy)
			{
				for (int xx = -1; xx <= 1; ++xx)
				{
					if (xx || yy)
					{
						// * Calculate address of the neighbourList, taking wraparound
						//   into account. The fact that the GOL space is 2 CELL's worth
						//   narrower in both dimensions than the simulation area makes
						//   this a bit awkward.
						int ax = ((x + xx + XRES - 3 * CELL) % (XRES - 2 * CELL)) + CELL;
						int ay = ((y + yy + YRES - 3 * CELL) % (YRES - 2 * CELL)) + CELL;
						if (pmap[ay][ax] && TYP(pmap[ay][ax]) != PT_LIFE)
						{
							continue;
						}
						unsigned int (&neighbourList)[5] = gol[ay][ax];
						// * Bump overall neighbour counter (bits 30..28) for the entire list.
						neighbourList[0] += 1U << 28;
						for (int l = 0; l < 5; ++l)
						{
							auto neighbourRuleset = neighbourList[l] & 0x001FFFFFU;
							if (neighbourRuleset == golnum)
							{
								// * Bump population counter (bits 23..21) of the
								//   same kind of cell.
								neighbourList[l] += 1U << 21;
								break;
							}
							if (neighbourRuleset == 0)
							{
								// * Add the new kind of cell to the population. Both counters
								//   have a bias of -1, so they're intentionally initialised
								//   to 0 instead of 1 here. This is all so they can both
								//   fit in 3 bits.
								neighbourList[l] = ((yy & 3) << 26) | ((xx & 3) << 24) | golnum;
								break;
							}
							// * If after 5 iterations the cell still hasn't contributed
							//   to a list entry, it's surely a 6th kind of cell, meaning
							//   there could be at most 3 of it in the neighbourhood,
							//   as there are already 5 other kinds of cells present in
							//   the list. This in turn means that it couldn't possibly
							//   win the population ratio-based contest later on.
						}
					}
				}
			}
		}
		else
		{
			if (!(bmap[y / CELL][x / CELL] == WL_STASIS && emap[y / CELL][x / CELL] < 8))
			{
				part.tmp2 -= 1;
			}
		}
	}
	for (int y = CELL; y < YRES - CELL; ++y)
	{
		for (int x = CELL; x < XRES - CELL; ++x)
		{
			int r = pmap[y][x];
			if (r && TYP(r) != PT_LIFE)
			{
				continue;
			}
			unsigned int (&neighbourList)[5] = gol[y][x];
			auto nl0 = neighbourList[0];
			if (r || nl0)
			{
				// * Get overall neighbour count (bits 30..28).
				unsigned int neighbours = nl0 ? ((nl0 >> 28) & 7) + 1 : 0;
				if (!(bmap[y / CELL][x / CELL] == WL_STASIS && emap[y / CELL][x / CELL] < 8))
				{
					if (r)
					{
						auto &part = parts[ID(r)];
						unsigned int ruleset = part.ctype;
						if (ruleset < NGOL)
						{
							ruleset = builtinGol[ruleset].ruleset;
						}
						if (!((ruleset >> neighbours) & 1) && part.tmp2 == int(ruleset >> 17) + 1)
						{
							// * Start death sequence.
							part.tmp2 -= 1;
						}
					}
					else
					{
						unsigned int golnumToCreate = 0xFFFFFFFFU;
						unsigned int createFromEntry = 0U;
						unsigned int majority = neighbours / 2 + neighbours % 2;
						for (int l = 0; l < 5; ++l)
						{
							auto golnum = neighbourList[l] & 0x001FFFFFU;
							if (!golnum)
							{
								break;
							}
							auto ruleset = golnum;
							if (golnum - 1 < NGOL)
							{
								ruleset = builtinGol[golnum - 1].ruleset;
								golnum -= 1;
							}
							if ((ruleset >> (neighbours + 8)) & 1 && ((neighbourList[l] >> 21) & 7) + 1 >= majority && golnum < golnumToCreate)
							{
								golnumToCreate = golnum;
								createFromEntry = neighbourList[l];
							}
						}
						if (golnumToCreate != 0xFFFFFFFFU)
						{
							// * 0x200000: No need to look for colours, they'll be set later anyway.
							int i = create_part(-1, x, y, PT_LIFE, golnumToCreate | 0x200000);
							if (i >= 0)
							{
								int xx = (createFromEntry >> 24) & 3;
								int yy = (createFromEntry >> 26) & 3;
								if (xx == 3) xx = -1;
								if (yy == 3) yy = -1;
								int ax = ((x - xx + XRES - 3 * CELL) % (XRES - 2 * CELL)) + CELL;
								int ay = ((y - yy + YRES - 3 * CELL) % (YRES - 2 * CELL)) + CELL;
								auto &sample = parts[ID(pmap[ay][ax])];
								parts[i].dcolour = sample.dcolour;
								parts[i].tmp = sample.tmp;
							}
						}
					}
				}
				for (int l = 0; l < 5 && neighbourList[l]; ++l)
				{
					neighbourList[l] = 0;
				}
			}
		}
	}
	for (int y = CELL; y < YRES - CELL; ++y)
	{
		for (int x = CELL; x < XRES - CELL; ++x)
		{
			int r = pmap[y][x];
			if (r && TYP(r) == PT_LIFE && parts[ID(r)].tmp2 <= 0)
			{
				kill_part(ID(r));
			}
		}
	}
}

void Simulation::CheckStacking()
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	bool excessive_stacking_found = false;
	force_stacking_check = false;
	for (int y = 0; y < YRES; y++)
	{
		for (int x = 0; x < XRES; x++)
		{
			// Use a threshold, since some particle stacking can be normal (e.g. BIZR + FILT)
			// Setting pmap_count[y][x] > NPART means BHOL will form in that spot
			if (pmap_count[y][x]>5)
			{
				if (bmap[y/CELL][x/CELL]==WL_EHOLE)
				{
					// Allow more stacking in E-hole
					if (pmap_count[y][x]>1500)
					{
						pmap_count[y][x] = pmap_count[y][x] + NPART;
						excessive_stacking_found = 1;
					}
				}
				else if (pmap_count[y][x]>1500 || (unsigned int)rng.between(0, 1599) <= (pmap_count[y][x]+100))
				{
					pmap_count[y][x] = pmap_count[y][x] + NPART;
					excessive_stacking_found = true;
				}
			}
		}
	}
	if (excessive_stacking_found)
	{
		for (int i = 0; i < parts.active; i++)
		{
			if (parts[i].type)
			{
				int t = parts[i].type;
				int x = (int)(parts[i].x+0.5f);
				int y = (int)(parts[i].y+0.5f);
				if (x>=0 && y>=0 && x<XRES && y<YRES && !(elements[t].Properties&TYPE_ENERGY))
				{
					if (pmap_count[y][x]>=NPART)
					{
						if (pmap_count[y][x]>NPART)
						{
							//@ stacking -> NBHL
							create_part(i, x, y, PT_NBHL);
							parts[i].temp = MAX_TEMP;
							parts[i].tmp = pmap_count[y][x]-NPART;//strength of grav field
							if (parts[i].tmp>51200) parts[i].tmp = 51200;
							pmap_count[y][x] = NPART;
						}
						else
						{
							kill_part(i);
						}
					}
				}
			}
		}
	}
}

void Simulation::UpdateGravityMask()
{
	for (auto p : CELLS.OriginRect())
	{
		gravIn.mask[p] = 0;
	}
	std::stack<Vec2<int>> toCheck;
	auto check = [this, &toCheck](Vec2<int> p) {
		if (!(bmap[p.Y][p.X] == WL_GRAV || gravIn.mask[p]))
		{
			gravIn.mask[p] = UINT32_C(0xFFFFFFFF);
			for (auto o : RectSized<int>({ -1, -1 }, { 3, 3 }))
			{
				if ((o.X + o.Y) & 1) // i.e. immediate neighbours but not diagonal ones
				{
					auto q = p + o;
					if (CELLS.OriginRect().Contains(q))
					{
						toCheck.push(q);
					}
				}
			}
		}
	};
	for (auto x = 0; x < CELLS.X; ++x)
	{
		check({ x, 0           });
		check({ x, CELLS.Y - 1 });
	}
	for (auto y = 1; y < CELLS.Y - 1; ++y) // corners already checked in the previous loop
	{
		check({ 0          , y });
		check({ CELLS.X - 1, y });
	}
	while (!toCheck.empty())
	{
		auto p = toCheck.top();
		toCheck.pop();
		check(p);
	}
}

void Simulation::ResetOmniEventMetrics()
{
	omniEventCountTotal.store(0, std::memory_order_relaxed);
	omniEventCountCurrentFrame.store(0, std::memory_order_relaxed);
	omniEventCountPeakPerFrame.store(0, std::memory_order_relaxed);
}

void Simulation::RecordOmniEvent()
{
	const auto current = omniEventCountCurrentFrame.fetch_add(1, std::memory_order_relaxed) + 1;
	omniEventCountTotal.fetch_add(1, std::memory_order_relaxed);
	auto peak = omniEventCountPeakPerFrame.load(std::memory_order_relaxed);
	while (current > peak
		&& !omniEventCountPeakPerFrame.compare_exchange_weak(
			peak, current, std::memory_order_relaxed, std::memory_order_relaxed))
	{
	}
}

Simulation::OmniEventMetrics Simulation::GetOmniEventMetrics() const
{
	return {
		omniEventCountTotal.load(std::memory_order_relaxed),
		omniEventCountCurrentFrame.load(std::memory_order_relaxed),
		omniEventCountPeakPerFrame.load(std::memory_order_relaxed),
	};
}

void Simulation::SetOmniLifecycleLedgerEnabled(bool enabled)
{
	omniLifecycleLedgerEnabled = enabled;
	ResetOmniLifecycleLedger();
}

void Simulation::ResetOmniLifecycleLedger()
{
	omniLifecycleLedgerActiveTick = false;
	omniLifecycleLedgerLastFrameReconciled = false;
	omniLifecycleLedgerTicksStarted = 0;
	omniLifecycleLedgerTicksCompleted = 0;
	omniLifecycleLedgerReconciliationFailures = 0;
	omniLifecycleLedgerCreates = 0;
	omniLifecycleLedgerKills = 0;
	omniLifecycleLedgerTypeTransitions = 0;
	omniLifecycleLedgerReplacements = 0;
	omniLifecycleLedgerDirectTypeTransitions = 0;
	omniLifecycleLedgerSparkFastPathTransitions = 0;
	omniLifecycleLedgerBrmtTungPreparationTransitions = 0;
	omniLifecycleLedgerLoadFallbackTransitions = 0;
	omniLifecycleLedgerOutsideTickEvents = 0;
	omniLifecycleLedgerOutsideTickDirectTransitions = 0;
	omniLifecycleLedgerLastBeginRecords = 0;
	omniLifecycleLedgerLastEndRecords = 0;
	omniLifecycleLedgerLastUnattributedRecordDeltaAbs = 0;
	omniLifecycleLedgerTotalUnattributedRecordDeltaAbs = 0;
	omniLifecycleLedgerLastFirstMismatchedType = -1;
	omniLifecycleLedgerLastInvalidTypeRecords = 0;
	std::fill(omniLifecycleLedgerBeginHistogram.begin(), omniLifecycleLedgerBeginHistogram.end(), 0);
	std::fill(omniLifecycleLedgerEventDelta.begin(), omniLifecycleLedgerEventDelta.end(), 0);
}

Simulation::OmniLifecycleLedgerMetrics Simulation::GetOmniLifecycleLedgerMetrics() const
{
	return {
		omniLifecycleLedgerEnabled,
		omniLifecycleLedgerActiveTick,
		true,
		omniLifecycleLedgerLastFrameReconciled,
		omniLifecycleLedgerTicksStarted,
		omniLifecycleLedgerTicksCompleted,
		omniLifecycleLedgerReconciliationFailures,
		omniLifecycleLedgerCreates,
		omniLifecycleLedgerKills,
		omniLifecycleLedgerTypeTransitions,
		omniLifecycleLedgerReplacements,
		omniLifecycleLedgerDirectTypeTransitions,
		omniLifecycleLedgerSparkFastPathTransitions,
		omniLifecycleLedgerBrmtTungPreparationTransitions,
		omniLifecycleLedgerLoadFallbackTransitions,
		omniLifecycleLedgerOutsideTickEvents,
		omniLifecycleLedgerOutsideTickDirectTransitions,
		omniLifecycleLedgerLastBeginRecords,
		omniLifecycleLedgerLastEndRecords,
		omniLifecycleLedgerLastUnattributedRecordDeltaAbs,
		omniLifecycleLedgerTotalUnattributedRecordDeltaAbs,
		omniLifecycleLedgerLastFirstMismatchedType,
		omniLifecycleLedgerLastInvalidTypeRecords,
	};
}

void Simulation::SetOmniCorrectionLedgerEnabled(bool enabled)
{
	omniCorrectionLedgerEnabled = enabled;
	ResetOmniCorrectionLedger();
}

void Simulation::ResetOmniCorrectionLedger()
{
	omniCorrectionLedgerTotalEvents = 0;
	omniCorrectionLedgerDroppedEvents = 0;
	omniCorrectionLedgerNextEvent = 0;
	omniCorrectionLedgerRetainedEvents = 0;
	std::fill(omniCorrectionLedgerKindCounts.begin(), omniCorrectionLedgerKindCounts.end(), 0);
	std::fill(omniCorrectionLedgerEvents.begin(), omniCorrectionLedgerEvents.end(), OmniCorrectionEvent{});
}

Simulation::OmniCorrectionLedgerMetrics Simulation::GetOmniCorrectionLedgerMetrics() const
{
	OmniCorrectionLedgerMetrics metrics{};
	metrics.enabled = omniCorrectionLedgerEnabled;
	metrics.legacyFieldUnitsOnly = true;
	metrics.auditedAirCapsOnly = true;
	metrics.totalEvents = omniCorrectionLedgerTotalEvents;
	metrics.retainedEvents = omniCorrectionLedgerRetainedEvents;
	metrics.droppedEvents = omniCorrectionLedgerDroppedEvents;
	metrics.kindCounts = omniCorrectionLedgerKindCounts;
	if (omniCorrectionLedgerRetainedEvents)
	{
		auto first = (omniCorrectionLedgerNextEvent + OmniCorrectionLedgerEventCapacity
			- omniCorrectionLedgerRetainedEvents) % OmniCorrectionLedgerEventCapacity;
		for (size_t offset = 0; offset < omniCorrectionLedgerRetainedEvents; offset++)
		{
			metrics.events[offset] = omniCorrectionLedgerEvents[
				(first + offset) % OmniCorrectionLedgerEventCapacity
			];
		}
	}
	return metrics;
}

double Simulation::GetOmniWaterParcelMassKg(int particleId) const
{
	if (particleId < 0 || particleId >= NPART)
		return 0.0;
	return omniWaterParcelMassKg[particleId];
}

double Simulation::GetOmniWaterParcelSpecificEnthalpyJPerKg(int particleId) const
{
	return OmniWaterParcelSpecificEnthalpy(particleId);
}

void Simulation::InitializeOmniWaterParcelMass(int particleId, int type)
{
	if (particleId < 0 || particleId >= NPART)
		return;
	if (IsOmniAtmosphereActive() && IsOmniManagedWaterType(type))
	{
		const bool wasUnowned = !(omniWaterParcelMassKg[particleId] > 0.0);
		omniWaterParcelMassKg[particleId] = OmniPhysicalScale::DefaultWaterParcelMassKg;
		omniWaterParcelSpecificEnthalpyJPerKg[particleId] =
			OmniThermal::WaterSpecificEnthalpyJPerKg(parts[particleId].temp);
		if (wasUnowned && omniWaterCouplingMetrics.activeTick)
		{
			omniWaterCouplingMetrics.externalWaterMassSourceKg += omniWaterParcelMassKg[particleId];
			omniWaterCouplingMetrics.externalWaterEnergySourceJ +=
				omniWaterParcelMassKg[particleId] * omniWaterParcelSpecificEnthalpyJPerKg[particleId];
		}
	}
	else
	{
		omniWaterParcelMassKg[particleId] = 0.0;
		omniWaterParcelSpecificEnthalpyJPerKg[particleId] = 0.0;
	}
}

void Simulation::ClearOmniWaterParcelMass(int particleId, bool recordExternalSink)
{
	if (particleId >= 0 && particleId < NPART)
	{
		const double massKg = IsFiniteDoubleBits(omniWaterParcelMassKg[particleId]) &&
			omniWaterParcelMassKg[particleId] > 0.0 ? omniWaterParcelMassKg[particleId] : 0.0;
		const double specificEnthalpy = IsFiniteDoubleBits(omniWaterParcelSpecificEnthalpyJPerKg[particleId]) &&
			omniWaterParcelSpecificEnthalpyJPerKg[particleId] >= 0.0
			? omniWaterParcelSpecificEnthalpyJPerKg[particleId]
			: (IsOmniManagedWaterType(parts[particleId].type)
				? OmniThermal::WaterSpecificEnthalpyJPerKg(parts[particleId].temp) : 0.0);
		if (recordExternalSink && omniWaterCouplingMetrics.activeTick && massKg > 0.0)
		{
			omniWaterCouplingMetrics.externalWaterMassSinkKg += massKg;
			omniWaterCouplingMetrics.externalWaterEnergySinkJ += massKg * specificEnthalpy;
		}
		omniWaterParcelMassKg[particleId] = 0.0;
		omniWaterParcelSpecificEnthalpyJPerKg[particleId] = 0.0;
	}
}

double Simulation::OmniWaterParcelSpecificEnthalpy(int particleId) const
{
	if (particleId < 0 || particleId >= NPART ||
		!IsOmniManagedWaterType(parts[particleId].type))
	{
		return 0.0;
	}
	const double value = omniWaterParcelSpecificEnthalpyJPerKg[particleId];
	return IsFiniteDoubleBits(value) && value >= 0.0
		? value
		: OmniThermal::WaterSpecificEnthalpyJPerKg(parts[particleId].temp);
}

double Simulation::TotalOmniParticleWaterMassKg() const
{
	double total = 0.0;
	for (int particleId = 0; particleId < parts.active; ++particleId)
	{
		if (parts[particleId].type && IsOmniManagedWaterType(parts[particleId].type))
			total += omniWaterParcelMassKg[particleId];
		else if (parts[particleId].type == PT_SLTW)
			total += GetOmniSolutionSolventMassKg(particleId);
		else if (parts[particleId].type == PT_DSTW && IsOmniAtmosphereActive())
			total += OmniPhysicalScale::DefaultWaterParcelMassKg;
	}
	return total;
}

double Simulation::TotalOmniWaterCoupledEnergyJ() const
{
	double total = omniAtmosphere ? omniAtmosphere->TotalEnergyJ() : 0.0;
	for (int particleId = 0; particleId < parts.active; ++particleId)
	{
		if (!parts[particleId].type || !IsOmniManagedWaterType(parts[particleId].type))
			continue;
		const double massKg = omniWaterParcelMassKg[particleId];
		const double specificEnthalpy = OmniWaterParcelSpecificEnthalpy(particleId);
		if (massKg > 0.0 && IsFiniteDoubleBits(specificEnthalpy))
			total += massKg * specificEnthalpy;
	}
	for (int particleId = 0; particleId < parts.active; ++particleId)
	{
		if (parts[particleId].type != PT_SLTW)
			continue;
		const double massKg = GetOmniSolutionSolventMassKg(particleId);
		if (massKg > 0.0)
			total += massKg * OmniThermal::WaterSpecificEnthalpyJPerKg(parts[particleId].temp);
	}
	return total;
}

void Simulation::BeginOmniWaterCouplingTick()
{
	omniWaterTransferRequests.clear();
	omniWaterCouplingMetrics = {};
	if (!IsOmniAtmosphereActive() || !omniAtmosphere)
		return;
	omniWaterCouplingMetrics.activeTick = true;
	omniWaterCouplingMetrics.initialWaterMassKg = TotalOmniParticleWaterMassKg() +
		omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_H2O) +
		omniAtmosphere->TotalCondensedWaterMassKg();
	omniWaterCouplingMetrics.initialCoupledEnergyJ = TotalOmniWaterCoupledEnergyJ();
}

bool Simulation::QueueOmniWaterParticleCoupling(int particleId, int x, int y)
{
	if (!IsOmniAtmosphereActive() || !omniAtmosphere || particleId < 0 || particleId >= NPART ||
		!IsOmniManagedWaterType(parts[particleId].type) || x < 0 || y < 0 || x >= XRES || y >= YRES)
	{
		return false;
	}
	if (!(omniWaterParcelMassKg[particleId] > 0.0))
	{
		omniWaterParcelMassKg[particleId] = OmniPhysicalScale::DefaultWaterParcelMassKg;
		omniWaterParcelSpecificEnthalpyJPerKg[particleId] =
			OmniThermal::WaterSpecificEnthalpyJPerKg(parts[particleId].temp);
	}
	const auto authoritativeState = OmniThermal::WaterFromSpecificEnthalpy(
		omniWaterParcelSpecificEnthalpyJPerKg[particleId]);
	if (IsFiniteFloatBits(parts[particleId].temp) &&
		IsFiniteDoubleBits(authoritativeState.temperatureK) &&
		std::abs(double(parts[particleId].temp) - authoritativeState.temperatureK) > 1.0e-3)
	{
		// Lua, property tools and tests write the public temperature field. Import
		// that explicit source once at the fixed-step coupling boundary; later
		// latent-heat evolution remains authoritative in the enthalpy sidecar.
		omniWaterParcelSpecificEnthalpyJPerKg[particleId] =
			OmniThermal::WaterSpecificEnthalpyJPerKg(parts[particleId].temp);
	}
	const int cellX = x / CELL;
	const int cellY = y / CELL;
	const auto primitive = omniAtmosphere->Primitive(cellX, cellY);
	if (!primitive.finite)
		return true;
	OmniWaterTransferRequest request{};
	request.particleId = particleId;
	request.cellX = cellX;
	request.cellY = cellY;
	const double massKg = omniWaterParcelMassKg[particleId];
	const double particleEnthalpy = OmniWaterParcelSpecificEnthalpy(particleId);
	const double airEquivalentEnthalpy = OmniThermal::WaterSpecificEnthalpyJPerKg(primitive.temperature);
	if (IsFiniteDoubleBits(particleEnthalpy) && IsFiniteDoubleBits(airEquivalentEnthalpy))
	{
		// Relax five percent toward local thermal equilibrium per fixed tick. The
		// equal-and-opposite atmosphere update is deferred until all proposals are known.
		request.sensibleEnergyToParticleJ = OmniThermal::WaterSensibleExchangeEnergyJ(
			massKg, particleEnthalpy, airEquivalentEnthalpy, 0.05);
	}
	if (parts[particleId].type == PT_WTRV)
	{
		request.injectVaporParcel = true;
		request.requestedEvaporationMassKg = massKg;
	}
	else
	{
		request.requestedEvaporationMassKg = OmniThermal::WaterEvaporationRequestMassKg(
			massKg, parts[particleId].temp, primitive.relativeHumidity, primitive.pressure);
	}
	omniWaterTransferRequests.push_back(request);
	omniWaterCouplingMetrics.requests++;
	return true;
}

void Simulation::TransferOmniWaterParticleToAtmosphere(
	int particleId, int cellX, int cellY, double requestedMassKg)
{
	if (!omniAtmosphere || particleId < 0 || particleId >= NPART ||
		cellX < 0 || cellY < 0 || cellX >= static_cast<int>(omniAtmosphere->Width()) ||
		cellY >= static_cast<int>(omniAtmosphere->Height()))
	{
		return;
	}
	const double ownedMassKg = omniWaterParcelMassKg[particleId];
	const double massKg = requestedMassKg > 0.0
		? std::min(requestedMassKg, ownedMassKg)
		: ownedMassKg;
	if (!(massKg > 0.0))
		return;
	const double cellVolumeM3 = omniAtmosphere->Config().scale.cellVolumeM3();
	const double particleEnergyJ = massKg * OmniWaterParcelSpecificEnthalpy(particleId);
	const double atmosphereBeforeJ = omniAtmosphere->TotalEnergyJ();
	omniAtmosphere->AddSpeciesMassDensity(cellX, cellY, OMNI_SPECIES_H2O, massKg / cellVolumeM3);
	const double automaticEnergyJ = omniAtmosphere->TotalEnergyJ() - atmosphereBeforeJ;
	omniAtmosphere->AddEnergyDensity(cellX, cellY, (particleEnergyJ - automaticEnergyJ) / cellVolumeM3);
	omniWaterCouplingMetrics.transferredToAtmosphereKg += massKg;
	omniWaterCouplingMetrics.particleEnergyRemovedJ += particleEnergyJ;
	omniWaterCouplingMetrics.atmosphereEnergyAddedJ += particleEnergyJ;
	omniWaterCouplingMetrics.vaporParcelsInjected++;
	omniWaterParcelMassKg[particleId] = std::max(ownedMassKg - massKg, 0.0);
}

void Simulation::CommitOmniWaterCouplingTick()
{
	if (!omniWaterCouplingMetrics.activeTick || !omniAtmosphere)
		return;
	const double cellVolumeM3 = omniAtmosphere->Config().scale.cellVolumeM3();
	for (const auto &request : omniWaterTransferRequests)
	{
		const int particleId = request.particleId;
		if (particleId < 0 || particleId >= NPART || !parts[particleId].type ||
			!IsOmniManagedWaterType(parts[particleId].type) ||
			request.cellX < 0 || request.cellY < 0 ||
			request.cellX >= static_cast<int>(omniAtmosphere->Width()) ||
			request.cellY >= static_cast<int>(omniAtmosphere->Height()))
		{
			continue;
		}
		double massKg = omniWaterParcelMassKg[particleId];
		if (!(massKg > 0.0))
			continue;

		const double beforeEnthalpy = OmniWaterParcelSpecificEnthalpy(particleId);
		if (IsFiniteDoubleBits(beforeEnthalpy) && IsFiniteDoubleBits(request.sensibleEnergyToParticleJ))
		{
			const auto next = OmniThermal::WaterFromSpecificEnthalpy(
				beforeEnthalpy + request.sensibleEnergyToParticleJ / massKg);
			const double nextTemperature = std::clamp(next.temperatureK, double(MIN_TEMP), double(MAX_TEMP));
			const double afterEnthalpy = nextTemperature == next.temperatureK
				? next.specificEnthalpyJPerKg
				: OmniThermal::WaterSpecificEnthalpyJPerKg(nextTemperature);
			const double actualEnergyToParticle = massKg * (afterEnthalpy - beforeEnthalpy);
			if (IsFiniteDoubleBits(actualEnergyToParticle))
			{
				parts[particleId].temp = static_cast<float>(nextTemperature);
				omniWaterParcelSpecificEnthalpyJPerKg[particleId] = afterEnthalpy;
				omniAtmosphere->AddEnergyDensity(
					request.cellX, request.cellY, -actualEnergyToParticle / cellVolumeM3);
				omniWaterCouplingMetrics.sensibleEnergyToParticlesJ += actualEnergyToParticle;
				omniWaterCouplingMetrics.atmosphereEnergyAddedJ -= actualEnergyToParticle;
			}
		}

		if (request.injectVaporParcel && parts[particleId].type == PT_WTRV)
		{
			const double referenceCellGasMassKg = omniAtmosphere->Config().referenceDensity * cellVolumeM3;
			TransferOmniWaterParticleToAtmosphere(
				particleId, request.cellX, request.cellY,
				std::min(request.requestedEvaporationMassKg, referenceCellGasMassKg));
			if (omniWaterParcelMassKg[particleId] <= 1.0e-15)
				kill_part(particleId);
			continue;
		}

		const double requestedMassKg = std::min(request.requestedEvaporationMassKg, massKg);
		if (requestedMassKg > 0.0 &&
			(parts[particleId].type == PT_WATR || parts[particleId].type == PT_ICEI))
		{
			const auto primitive = omniAtmosphere->Primitive(request.cellX, request.cellY);
			const auto &waterSpecies = omniAtmosphere->SpeciesDefinition(OMNI_SPECIES_H2O);
			const double vaporSpecificEnergy = OmniThermal::WaterVaporSpecificEnergyJPerKg(
				primitive.temperature, waterSpecies.specificHeatCpJKgK, waterSpecies.molarMassKgPerMol);
			const double liquidSpecificEnergy = OmniWaterParcelSpecificEnthalpy(particleId);
			const double minimumSpecificEnergy = OmniThermal::WaterSpecificEnthalpyJPerKg(
				std::max(double(MIN_TEMP), 1.0));
			const double referenceCellGasMassKg = omniAtmosphere->Config().referenceDensity * cellVolumeM3;
			const double transferMassKg = OmniThermal::WaterEvaporationTransferMassKg(
				requestedMassKg, massKg, liquidSpecificEnergy, minimumSpecificEnergy,
				vaporSpecificEnergy, referenceCellGasMassKg);
			if (transferMassKg > 0.0)
			{
				const double atmosphereBeforeJ = omniAtmosphere->TotalEnergyJ();
				omniAtmosphere->AddSpeciesMassDensity(
					request.cellX, request.cellY, OMNI_SPECIES_H2O, transferMassKg / cellVolumeM3);
				const double atmosphereEnergyAddedJ = omniAtmosphere->TotalEnergyJ() - atmosphereBeforeJ;
				const double remainingMassKg = massKg - transferMassKg;
				if (remainingMassKg > 1.0e-15)
				{
					const auto remaining = OmniThermal::WaterRemainingState(
						massKg, liquidSpecificEnergy, transferMassKg,
						atmosphereEnergyAddedJ, minimumSpecificEnergy);
					parts[particleId].temp = static_cast<float>(std::clamp(
						remaining.temperatureK, double(MIN_TEMP), double(MAX_TEMP)));
					omniWaterParcelSpecificEnthalpyJPerKg[particleId] =
						remaining.specificEnthalpyJPerKg;
					omniWaterParcelMassKg[particleId] = remainingMassKg;
				}
				else
				{
					omniWaterParcelMassKg[particleId] = 0.0;
					kill_part(particleId);
				}
				omniWaterCouplingMetrics.transferredToAtmosphereKg += transferMassKg;
				omniWaterCouplingMetrics.particleEnergyRemovedJ += atmosphereEnergyAddedJ;
				omniWaterCouplingMetrics.atmosphereEnergyAddedJ += atmosphereEnergyAddedJ;
				omniWaterCouplingMetrics.evaporationTransfers++;
			}
		}

		if (!parts[particleId].type)
			continue;
		const auto thermalState = OmniThermal::WaterFromSpecificEnthalpy(
			OmniWaterParcelSpecificEnthalpy(particleId));
		if (parts[particleId].type == PT_WATR && thermalState.phase == OmniThermalWaterPhase::Ice)
		{
			if (!part_change_type(particleId, int(parts[particleId].x + 0.5f),
				int(parts[particleId].y + 0.5f), PT_ICEI))
			{
				parts[particleId].ctype = PT_WATR;
				omniWaterCouplingMetrics.phaseTypeChanges++;
			}
		}
		else if (parts[particleId].type == PT_ICEI && thermalState.phase == OmniThermalWaterPhase::Liquid)
		{
			if (!part_change_type(particleId, int(parts[particleId].x + 0.5f),
				int(parts[particleId].y + 0.5f), PT_WATR))
			{
				parts[particleId].ctype = PT_NONE;
				omniWaterCouplingMetrics.phaseTypeChanges++;
			}
		}
	}
	omniWaterTransferRequests.clear();
}

void Simulation::FinishOmniWaterCouplingTick()
{
	if (!omniWaterCouplingMetrics.activeTick || !omniAtmosphere)
		return;
	omniWaterCouplingMetrics.finalWaterMassKg = TotalOmniParticleWaterMassKg() +
		omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_H2O) +
		omniAtmosphere->TotalCondensedWaterMassKg();
	omniWaterCouplingMetrics.finalCoupledEnergyJ = TotalOmniWaterCoupledEnergyJ();
	omniWaterCouplingMetrics.waterMassResidualKg = omniWaterCouplingMetrics.finalWaterMassKg -
		omniWaterCouplingMetrics.initialWaterMassKg +
		omniWaterCouplingMetrics.externalWaterMassSinkKg -
		omniWaterCouplingMetrics.externalWaterMassSourceKg;
	omniWaterCouplingMetrics.coupledEnergyResidualJ = omniWaterCouplingMetrics.finalCoupledEnergyJ -
		omniWaterCouplingMetrics.initialCoupledEnergyJ +
		omniWaterCouplingMetrics.externalWaterEnergySinkJ -
		omniWaterCouplingMetrics.externalWaterEnergySourceJ;
	omniWaterCouplingMetrics.activeTick = false;
}

double Simulation::GetOmniSolutionSolventMassKg(int particleId) const
{
	if (particleId < 0 || particleId >= NPART || !IsOmniManagedSolutionType(parts[particleId].type))
		return 0.0;
	return IsFiniteDoubleBits(omniSolutionSolventMassKg[particleId]) &&
		omniSolutionSolventMassKg[particleId] > 0.0 ? omniSolutionSolventMassKg[particleId] : 0.0;
}

double Simulation::GetOmniSolutionSoluteMassKg(int particleId) const
{
	if (particleId < 0 || particleId >= NPART || !IsOmniManagedSolutionType(parts[particleId].type))
		return 0.0;
	return IsFiniteDoubleBits(omniSolutionSoluteMassKg[particleId]) &&
		omniSolutionSoluteMassKg[particleId] > 0.0 ? omniSolutionSoluteMassKg[particleId] : 0.0;
}

double Simulation::GetOmniSolutionNeutralSaltMassKg(int particleId) const
{
	if (particleId < 0 || particleId >= NPART || !IsOmniManagedSolutionType(parts[particleId].type))
		return 0.0;
	return IsFiniteDoubleBits(omniSolutionNeutralSaltMassKg[particleId]) &&
		omniSolutionNeutralSaltMassKg[particleId] > 0.0
		? omniSolutionNeutralSaltMassKg[particleId] : 0.0;
}

double Simulation::TotalOmniSolutionSolventMassKg() const
{
	double total = omniAtmosphere ?
		omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_H2O) +
		omniAtmosphere->TotalCondensedWaterMassKg() : 0.0;
	for (int particleId = 0; particleId < parts.active; ++particleId)
	{
		if (!parts[particleId].type)
			continue;
		if (IsOmniManagedSolutionType(parts[particleId].type))
			total += GetOmniSolutionSolventMassKg(particleId);
		else if (IsOmniManagedWaterType(parts[particleId].type))
			total += GetOmniWaterParcelMassKg(particleId);
	}
	return total;
}

double Simulation::TotalOmniSolutionSoluteMassKg() const
{
	double total = 0.0;
	for (int particleId = 0; particleId < parts.active; ++particleId)
		if (parts[particleId].type && IsOmniManagedSolutionType(parts[particleId].type))
			total += GetOmniSolutionSoluteMassKg(particleId) +
				GetOmniSolutionNeutralSaltMassKg(particleId);
	return total;
}

void Simulation::InitializeOmniSolutionState(int particleId, int type, bool directCreate)
{
	if (particleId < 0 || particleId >= NPART || !IsOmniAtmosphereActive() ||
		!IsOmniManagedSolutionType(type))
	{
		if (particleId >= 0 && particleId < NPART)
		{
			omniSolutionSolventMassKg[particleId] = 0.0;
			omniSolutionSoluteMassKg[particleId] = 0.0;
			omniSolutionNeutralSaltMassKg[particleId] = 0.0;
		}
		return;
	}
	if (omniSolutionSolventMassKg[particleId] > 0.0 || omniSolutionSoluteMassKg[particleId] > 0.0 ||
		omniSolutionNeutralSaltMassKg[particleId] > 0.0)
		return;
	if (type == PT_SALT)
	{
		omniSolutionSolventMassKg[particleId] = 0.0;
		omniSolutionSoluteMassKg[particleId] = OmniSolution::DefaultSolidSoluteMassKg;
		omniSolutionNeutralSaltMassKg[particleId] = 0.0;
	}
	else if (type == PT_SLTW)
	{
		omniSolutionSolventMassKg[particleId] = OmniSolution::DefaultSolutionSolventMassKg;
		omniSolutionSoluteMassKg[particleId] = OmniSolution::MaximumDissolvedSoluteKg(
			omniSolutionSolventMassKg[particleId], parts[particleId].temp);
		omniSolutionNeutralSaltMassKg[particleId] = 0.0;
	}
	else
	{
		omniSolutionSolventMassKg[particleId] = OmniSolution::DefaultSolutionSolventMassKg;
		omniSolutionSoluteMassKg[particleId] = OmniSolution::DefaultReactiveSoluteMassKg;
		omniSolutionNeutralSaltMassKg[particleId] = 0.0;
	}
	if (directCreate && omniSolutionMetrics.activeTick)
	{
		omniSolutionMetrics.externalSolventSourceKg += omniSolutionSolventMassKg[particleId];
		omniSolutionMetrics.externalSoluteSourceKg += omniSolutionSoluteMassKg[particleId] +
			omniSolutionNeutralSaltMassKg[particleId];
	}
}

void Simulation::ClearOmniSolutionState(int particleId, bool recordExternalSink)
{
	if (particleId < 0 || particleId >= NPART)
		return;
	const double solvent = GetOmniSolutionSolventMassKg(particleId);
	const double solute = GetOmniSolutionSoluteMassKg(particleId);
	const double neutralSalt = GetOmniSolutionNeutralSaltMassKg(particleId);
	if (recordExternalSink && omniSolutionMetrics.activeTick)
	{
		omniSolutionMetrics.externalSolventSinkKg += solvent;
		omniSolutionMetrics.externalSoluteSinkKg += solute + neutralSalt;
	}
	omniSolutionSolventMassKg[particleId] = 0.0;
	omniSolutionSoluteMassKg[particleId] = 0.0;
	omniSolutionNeutralSaltMassKg[particleId] = 0.0;
}

void Simulation::SetOmniSolutionNeutralSaltMassKg(int particleId, double massKg,
	bool recordLedgerAdjustment)
{
	if (particleId < 0 || particleId >= NPART || !IsOmniManagedSolutionType(parts[particleId].type) ||
		!IsFiniteDoubleBits(massKg) || massKg < 0.0)
		return;
	const double oldMassKg = GetOmniSolutionNeutralSaltMassKg(particleId);
	omniSolutionNeutralSaltMassKg[particleId] = massKg;
	if (recordLedgerAdjustment && omniSolutionMetrics.activeTick)
	{
		if (massKg > oldMassKg)
			omniSolutionMetrics.externalSoluteSourceKg += massKg - oldMassKg;
		else
			omniSolutionMetrics.externalSoluteSinkKg += oldMassKg - massKg;
	}
}

void Simulation::SetOmniSolutionMassesKg(int particleId, double solventMassKg, double soluteMassKg,
	bool recordLedgerAdjustment)
{
	if (particleId < 0 || particleId >= NPART || !IsOmniManagedSolutionType(parts[particleId].type) ||
		!IsFiniteDoubleBits(solventMassKg) || !IsFiniteDoubleBits(soluteMassKg) ||
		solventMassKg < 0.0 || soluteMassKg < 0.0)
		return;
	const double oldSolvent = GetOmniSolutionSolventMassKg(particleId);
	const double oldSolute = GetOmniSolutionSoluteMassKg(particleId);
	omniSolutionSolventMassKg[particleId] = solventMassKg;
	omniSolutionSoluteMassKg[particleId] = soluteMassKg;
	if (recordLedgerAdjustment && omniSolutionMetrics.activeTick)
	{
		if (solventMassKg > oldSolvent)
			omniSolutionMetrics.externalSolventSourceKg += solventMassKg - oldSolvent;
		else
			omniSolutionMetrics.externalSolventSinkKg += oldSolvent - solventMassKg;
		if (soluteMassKg > oldSolute)
			omniSolutionMetrics.externalSoluteSourceKg += soluteMassKg - oldSolute;
		else
			omniSolutionMetrics.externalSoluteSinkKg += oldSolute - soluteMassKg;
	}
}

void Simulation::BeginOmniSolutionTick()
{
	omniSolutionMetrics = {};
	if (!IsOmniAtmosphereActive())
		return;
	omniSolutionMetrics.activeTick = true;
	omniSolutionMetrics.initialSolventMassKg = TotalOmniSolutionSolventMassKg();
	omniSolutionMetrics.initialSoluteMassKg = TotalOmniSolutionSoluteMassKg();
}

void Simulation::FinishOmniSolutionTick()
{
	if (!omniSolutionMetrics.activeTick)
		return;
	omniSolutionMetrics.finalSolventMassKg = TotalOmniSolutionSolventMassKg();
	omniSolutionMetrics.finalSoluteMassKg = TotalOmniSolutionSoluteMassKg();
	omniSolutionMetrics.solventMassResidualKg = omniSolutionMetrics.finalSolventMassKg -
		omniSolutionMetrics.initialSolventMassKg + omniSolutionMetrics.externalSolventSinkKg -
		omniSolutionMetrics.externalSolventSourceKg -
		omniSolutionMetrics.neutralisationWaterProducedKg;
	omniSolutionMetrics.soluteMassResidualKg = omniSolutionMetrics.finalSoluteMassKg -
		omniSolutionMetrics.initialSoluteMassKg + omniSolutionMetrics.externalSoluteSinkKg -
		omniSolutionMetrics.externalSoluteSourceKg +
		omniSolutionMetrics.neutralisationWaterProducedKg;
	omniSolutionMetrics.totalSolutionMassResidualKg =
		omniSolutionMetrics.solventMassResidualKg + omniSolutionMetrics.soluteMassResidualKg;
	omniSolutionMetrics.activeTick = false;
}

bool Simulation::UpdateOmniSolutionParticle(int particleId, int x, int y)
{
	if (!IsOmniAtmosphereActive() || particleId < 0 || particleId >= NPART ||
		!parts[particleId].type || !IsOmniManagedSolutionType(parts[particleId].type))
		return false;
	const int type = parts[particleId].type;
	const double temperatureK = std::clamp(double(parts[particleId].temp), double(MIN_TEMP), double(MAX_TEMP));
	auto neighbour = [&](int wantedType, int skip) -> int {
		for (int rx = -1; rx <= 1; ++rx)
			for (int ry = -1; ry <= 1; ++ry)
			{
				if ((!rx && !ry) || x + rx < 0 || y + ry < 0 || x + rx >= XRES || y + ry >= YRES)
					continue;
				const int packed = pmap[y + ry][x + rx];
				if (packed && ID(packed) != skip && TYP(packed) == wantedType)
					return ID(packed);
			}
		return -1;
	};

	if (type == PT_SALT)
	{
			const int water = neighbour(PT_WATR, particleId);
			const int distilledWater = water < 0 ? neighbour(PT_DSTW, particleId) : -1;
			const int solution = water < 0 && distilledWater < 0
				? neighbour(PT_SLTW, particleId) : -1;
			const int target = water >= 0 ? water : (distilledWater >= 0 ? distilledWater : solution);
		if (target < 0)
			return false;
		const double available = GetOmniSolutionSoluteMassKg(particleId);
		const bool pureWaterTarget = water >= 0 || distilledWater >= 0;
		const double solvent = pureWaterTarget ?
			(water >= 0 ? GetOmniWaterParcelMassKg(target) : OmniPhysicalScale::DefaultWaterParcelMassKg) :
			GetOmniSolutionSolventMassKg(target);
		const double dissolved = GetOmniSolutionSoluteMassKg(target);
		const double neutralSalt = GetOmniSolutionNeutralSaltMassKg(target);
		const double capacity = std::max(OmniSolution::MaximumDissolvedSoluteKg(
			solvent, parts[target].temp) - dissolved - neutralSalt, 0.0);
		const double transfer = std::min({ available, capacity, OmniSolution::MaximumDissolutionMassPerTransactionKg });
		if (!(transfer > 0.0))
		{
			if (omniSolutionMetrics.activeTick)
				omniSolutionMetrics.saturationLimitedTransactions++;
			return true;
		}
		if (pureWaterTarget)
		{
			const double waterMass = solvent;
			// Transfer solvent ownership, rather than recording a Legacy water
			// deletion followed by an unrelated solution source.
			if (water >= 0)
				ClearOmniWaterParcelMass(target, false);
			omniSolutionInternalMutation = true;
			part_change_type(target, int(parts[target].x + 0.5f), int(parts[target].y + 0.5f), PT_SLTW);
			omniSolutionInternalMutation = false;
			SetOmniSolutionMassesKg(target, waterMass, transfer, false);
		}
		else
			SetOmniSolutionMassesKg(target, solvent, dissolved + transfer, false);
		SetOmniSolutionMassesKg(particleId, 0.0, available - transfer, false);
		if (omniSolutionMetrics.activeTick)
		{
			omniSolutionMetrics.dissolvedMassKg += transfer;
			omniSolutionMetrics.dissolutionTransactions++;
		}
		if (GetOmniSolutionSoluteMassKg(particleId) <= 1.0e-15)
		{
			omniSolutionInternalMutation = true;
			kill_part(particleId);
			omniSolutionInternalMutation = false;
		}
		return true;
	}
	if (type == PT_ACID)
	{
		const int base = neighbour(PT_BASE, particleId);
		if (base < 0)
			return false;
		const double acidKg = GetOmniSolutionSoluteMassKg(particleId);
		const double baseKg = GetOmniSolutionSoluteMassKg(base);
		const double moles = std::min({
			acidKg / OmniSolution::HydrogenChlorideMolarMassKgPerMol,
			baseKg / OmniSolution::SodiumHydroxideMolarMassKgPerMol,
			OmniSolution::MaximumNeutralisationMolesPerTransaction });
		if (!(moles > 0.0))
			return true;
		const double acidNeutralSaltBefore = GetOmniSolutionNeutralSaltMassKg(particleId);
		const double baseNeutralSaltBefore = GetOmniSolutionNeutralSaltMassKg(base);
		const double acidConsumedKg = moles * OmniSolution::HydrogenChlorideMolarMassKgPerMol;
		const double baseConsumedKg = moles * OmniSolution::SodiumHydroxideMolarMassKgPerMol;
		const double saltProducedKg = moles * OmniSolution::SodiumChlorideMolarMassKgPerMol;
		const double waterProducedKg = moles * OmniSolution::WaterMolarMassKgPerMol;
		const double energyJ = moles * OmniSolution::StrongAcidBaseNeutralisationEnergyJPerMol;
		const double acidSolvent = GetOmniSolutionSolventMassKg(particleId);
		const double baseSolvent = GetOmniSolutionSolventMassKg(base);
		SetOmniSolutionMassesKg(particleId, acidSolvent + waterProducedKg * 0.5,
			std::max(acidKg - acidConsumedKg, 0.0), false);
		SetOmniSolutionMassesKg(base, baseSolvent + waterProducedKg * 0.5,
			std::max(baseKg - baseConsumedKg, 0.0), false);
		SetOmniSolutionNeutralSaltMassKg(particleId,
			acidNeutralSaltBefore + saltProducedKg * 0.5, false);
		SetOmniSolutionNeutralSaltMassKg(base,
			baseNeutralSaltBefore + saltProducedKg * 0.5, false);
		const double totalAqueousMassKg =
			GetOmniSolutionSolventMassKg(particleId) + GetOmniSolutionSolventMassKg(base) +
			GetOmniSolutionSoluteMassKg(particleId) + GetOmniSolutionSoluteMassKg(base) +
			GetOmniSolutionNeutralSaltMassKg(particleId) + GetOmniSolutionNeutralSaltMassKg(base);
		if (totalAqueousMassKg > 0.0 && energyJ > 0.0)
		{
			const double deltaK = energyJ / (totalAqueousMassKg * 4184.0);
			parts[particleId].temp = static_cast<float>(std::clamp(
				double(parts[particleId].temp) + deltaK, double(MIN_TEMP), double(MAX_TEMP)));
			parts[base].temp = static_cast<float>(std::clamp(
				double(parts[base].temp) + deltaK, double(MIN_TEMP), double(MAX_TEMP)));
		}
		if (omniSolutionMetrics.activeTick)
		{
			omniSolutionMetrics.neutralisedAcidMassKg += acidConsumedKg;
			omniSolutionMetrics.neutralisedBaseMassKg += baseConsumedKg;
			omniSolutionMetrics.neutralSaltProducedKg += saltProducedKg;
			omniSolutionMetrics.neutralisationWaterProducedKg += waterProducedKg;
			omniSolutionMetrics.neutralisationEnergyReleasedJ += energyJ;
			omniSolutionMetrics.neutralisationTransactions++;
		}
		if (GetOmniSolutionSoluteMassKg(particleId) <= 1.0e-15)
		{
			omniSolutionInternalMutation = true;
			part_change_type(particleId, x, y, PT_SLTW);
			omniSolutionInternalMutation = false;
			SetOmniSolutionMassesKg(particleId, acidSolvent + waterProducedKg * 0.5, 0.0, false);
			SetOmniSolutionNeutralSaltMassKg(
				particleId, acidNeutralSaltBefore + saltProducedKg * 0.5, false);
		}
		if (GetOmniSolutionSoluteMassKg(base) <= 1.0e-15)
		{
			const int bx = int(parts[base].x + 0.5f), by = int(parts[base].y + 0.5f);
			const double baseNeutralSaltKg = GetOmniSolutionNeutralSaltMassKg(base);
			omniSolutionInternalMutation = true;
			part_change_type(base, bx, by, PT_SLTW);
			omniSolutionInternalMutation = false;
			SetOmniSolutionMassesKg(base, baseSolvent + waterProducedKg * 0.5, 0.0, false);
			SetOmniSolutionNeutralSaltMassKg(base, baseNeutralSaltKg, false);
		}
		return true;
	}
	if (type == PT_BASE)
		return false;
	if (type == PT_SLTW)
	{
		double solvent = GetOmniSolutionSolventMassKg(particleId);
		double solute = GetOmniSolutionSoluteMassKg(particleId);
		double neutralSalt = GetOmniSolutionNeutralSaltMassKg(particleId);
		if (omniAtmosphere && solvent > 0.0)
		{
			const int cellX = x / CELL, cellY = y / CELL;
			const auto primitive = omniAtmosphere->Primitive(cellX, cellY);
			if (primitive.finite)
			{
				const double requestedMassKg = OmniThermal::WaterEvaporationRequestMassKg(
					solvent, temperatureK, primitive.relativeHumidity, primitive.pressure);
				const double cellVolumeM3 = omniAtmosphere->Config().scale.cellVolumeM3();
				const double referenceCellGasMassKg = omniAtmosphere->Config().referenceDensity * cellVolumeM3;
				const double transferMassKg = std::min({ requestedMassKg, solvent,
					referenceCellGasMassKg * 0.05 });
				if (transferMassKg > 0.0)
				{
					const double liquidSpecificEnergy =
						OmniThermal::WaterSpecificEnthalpyJPerKg(temperatureK);
					const double particleEnergyJ = transferMassKg * liquidSpecificEnergy;
					const double atmosphereBeforeJ = omniAtmosphere->TotalEnergyJ();
					omniAtmosphere->AddSpeciesMassDensity(
						cellX, cellY, OMNI_SPECIES_H2O, transferMassKg / cellVolumeM3);
					const double automaticEnergyJ = omniAtmosphere->TotalEnergyJ() - atmosphereBeforeJ;
					omniAtmosphere->AddEnergyDensity(
						cellX, cellY, (particleEnergyJ - automaticEnergyJ) / cellVolumeM3);
					solvent -= transferMassKg;
					SetOmniSolutionMassesKg(particleId, solvent, solute, false);
					omniSolutionMetrics.transferredSolventToAtmosphereKg += transferMassKg;
					omniWaterCouplingMetrics.transferredToAtmosphereKg += transferMassKg;
					omniWaterCouplingMetrics.particleEnergyRemovedJ += particleEnergyJ;
					omniWaterCouplingMetrics.atmosphereEnergyAddedJ += particleEnergyJ;
				}
			}
		}
		const double capacity = OmniSolution::MaximumDissolvedSoluteKg(solvent, temperatureK);
		if (solute + neutralSalt > capacity + 1.0e-18)
		{
			const double transfer = std::min(solute + neutralSalt - capacity,
				OmniSolution::MaximumCrystallisationMassPerTickKg);
			int emptyX = -1, emptyY = -1;
			for (int rx = -1; rx <= 1 && emptyX < 0; ++rx)
				for (int ry = -1; ry <= 1; ++ry)
				{
					const int ex = x + rx, ey = y + ry;
					if ((!rx && !ry) || ex < CELL || ey < CELL || ex >= XRES - CELL || ey >= YRES - CELL)
						continue;
					if (!pmap[ey][ex] && !photons[ey][ex] && !IsWallBlocking(ex, ey, PT_SALT))
					{
						emptyX = ex;
						emptyY = ey;
						break;
					}
				}
			if (emptyX >= 0)
			{
				omniSolutionInternalMutation = true;
				const int salt = create_part(-1, emptyX, emptyY, PT_SALT);
				omniSolutionInternalMutation = false;
				if (salt >= 0)
				{
					SetOmniSolutionMassesKg(salt, 0.0, transfer, false);
					const double fromNeutral = std::min(neutralSalt, transfer);
					neutralSalt -= fromNeutral;
					solute -= transfer - fromNeutral;
					SetOmniSolutionMassesKg(particleId, solvent, solute, false);
					SetOmniSolutionNeutralSaltMassKg(particleId, neutralSalt, false);
					if (omniSolutionMetrics.activeTick)
					{
						omniSolutionMetrics.crystallisedMassKg += transfer;
						omniSolutionMetrics.crystallisationTransactions++;
					}
				}
			}
			else if (omniSolutionMetrics.activeTick)
				omniSolutionMetrics.rateLimitedTransactions++;
		}
		if (solvent <= 1.0e-15 && solute + neutralSalt > 0.0 && parts[particleId].type == PT_SLTW)
		{
			omniSolutionInternalMutation = true;
			part_change_type(particleId, x, y, PT_SALT);
			omniSolutionInternalMutation = false;
			SetOmniSolutionMassesKg(particleId, 0.0, solute + neutralSalt, false);
			SetOmniSolutionNeutralSaltMassKg(particleId, 0.0, false);
		}
		return true;
	}
	return false;
}

double Simulation::GetOmniCarbonParcelMassKg(int particleId) const
{
	if (particleId < 0 || particleId >= NPART || !IsOmniManagedCarbonType(parts[particleId].type) ||
		!(omniCarbonParcelMassKg[particleId] > 0.0))
	{
		return 0.0;
	}
	return omniCarbonParcelMassKg[particleId];
}

void Simulation::InitializeOmniCarbonParcelMass(int particleId, int type)
{
	if (particleId < 0 || particleId >= NPART || !IsOmniAtmosphereActive() ||
		!IsOmniManagedCarbonType(type) || omniCarbonParcelMassKg[particleId] > 0.0)
	{
		return;
	}
	omniCarbonParcelMassKg[particleId] = OmniPhysicalScale::DefaultCarbonParcelMassKg;
}

void Simulation::ClearOmniCarbonParcelMass(int particleId, bool recordExternalSink)
{
	if (particleId < 0 || particleId >= NPART)
		return;
	const double massKg = IsFiniteDoubleBits(omniCarbonParcelMassKg[particleId]) &&
		omniCarbonParcelMassKg[particleId] > 0.0 ? omniCarbonParcelMassKg[particleId] : 0.0;
	if (recordExternalSink && omniChemistryMetrics.activeTick && massKg > 0.0)
		omniChemistryMetrics.externalCarbonMassSinkKg += massKg;
	omniCarbonParcelMassKg[particleId] = 0.0;
}

double Simulation::TotalOmniParticleCarbonMassKg() const
{
	double total = 0.0;
	for (int particleId = 0; particleId < parts.active; ++particleId)
		total += GetOmniCarbonParcelMassKg(particleId);
	return total;
}

void Simulation::BeginOmniChemistryTick()
{
	omniChemistryMetrics = {};
	if (!IsOmniAtmosphereActive() || !omniAtmosphere)
		return;
	omniChemistryMetrics.activeTick = true;
	omniChemistryMetrics.initialCarbonMassKg = TotalOmniParticleCarbonMassKg();
	omniChemistryMetrics.initialOxygenMassKg = omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_O2);
	omniChemistryMetrics.initialCarbonDioxideMassKg = omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_CO2);
}

bool Simulation::UpdateOmniCarbonCombustion(int particleId, int x, int y)
{
	FrameTime::SubsystemSpan profilerSpan(frameTime, FrameTime::Subsystem::Chemistry);
	if (!IsOmniAtmosphereActive() || !omniAtmosphere || particleId < 0 || particleId >= NPART ||
		!IsOmniManagedCarbonType(parts[particleId].type) || x < 0 || y < 0 || x >= XRES || y >= YRES)
	{
		return false;
	}
	InitializeOmniCarbonParcelMass(particleId, parts[particleId].type);
	omniChemistryMetrics.candidateParticles++;
	const int cellX = x / CELL;
	const int cellY = y / CELL;
	const auto primitive = omniAtmosphere->Primitive(cellX, cellY);
	const double carbonMassKg = GetOmniCarbonParcelMassKg(particleId);
	if (!primitive.finite || !(carbonMassKg > 0.0) ||
		!IsFiniteFloatBits(parts[particleId].temp) || parts[particleId].temp < 450.0f)
	{
		return false;
	}
	const double oxygenMassKg = omniAtmosphere->SpeciesMassDensity(cellX, cellY, OMNI_SPECIES_O2) *
		omniAtmosphere->Config().scale.cellVolumeM3();
	const auto plan = CarbonOxidationRuntime().PlanCarbonOxidation({
		carbonMassKg, oxygenMassKg, parts[particleId].temp,
		OmniPhysicalScale::TimestepS, 1.0,
	});
	if (!plan.valid)
	{
		omniChemistryMetrics.rejectedTransactions++;
		return false;
	}
	if (!(plan.carbonConsumedKg > 0.0))
		return false;
	const double actualCarbonKg = std::min(plan.carbonConsumedKg, carbonMassKg);
	if (!(actualCarbonKg > 0.0))
		return false;
	const double scale = actualCarbonKg / plan.carbonConsumedKg;
	const double actualOxygenKg = plan.oxygenConsumedKg * scale;
	const double actualCarbonDioxideKg = plan.carbonDioxideProducedKg * scale;
	const double actualHeatJ = plan.heatReleasedJ * scale;
	const double parcelVelocityX = double(parts[particleId].vx) *
		OmniPhysicalScale::PixelLengthM / OmniPhysicalScale::TimestepS;
	const double parcelVelocityY = double(parts[particleId].vy) *
		OmniPhysicalScale::PixelLengthM / OmniPhysicalScale::TimestepS;
	std::vector<double> delta(omniAtmosphere->SpeciesCount(), 0.0);
	delta[OMNI_SPECIES_O2] = -actualOxygenKg;
	delta[OMNI_SPECIES_CO2] = actualCarbonDioxideKg;
	OmniAtmosphereReactionTransfer transfer{};
	if (!omniAtmosphere->ApplyReactionSpeciesTransfer(cellX, cellY, delta, actualHeatJ,
		parcelVelocityX, parcelVelocityY, &transfer))
	{
		omniChemistryMetrics.rejectedTransactions++;
		return false;
	}
	omniCarbonParcelMassKg[particleId] = std::max(carbonMassKg - actualCarbonKg, 0.0);
	omniChemistryMetrics.carbonConsumedKg += actualCarbonKg;
	omniChemistryMetrics.oxygenConsumedKg += actualOxygenKg;
	omniChemistryMetrics.carbonDioxideProducedKg += actualCarbonDioxideKg;
	omniChemistryMetrics.chemicalEnergyReleasedJ += actualHeatJ;
	omniChemistryMetrics.transactionEnergyDeltaJ += transfer.totalEnergyDeltaJ;
	omniChemistryMetrics.energyResidualJ += transfer.totalEnergyDeltaJ -
		(transfer.sensibleEnergyDeltaJ + transfer.sourceKineticEnergyJ + actualHeatJ);
	omniChemistryMetrics.committedTransactions++;
	omniChemistryMetrics.oxygenLimitedTransactions += plan.limitedByOxygen;
	omniChemistryMetrics.carbonLimitedTransactions += plan.limitedByCarbon;
	if (omniCarbonParcelMassKg[particleId] <= 1.0e-15)
	{
		// Zero first, so kill_part records no unaccounted external carbon sink.
		omniCarbonParcelMassKg[particleId] = 0.0;
		kill_part(particleId);
		return true;
	}
	return false;
}

void Simulation::FinishOmniChemistryTick()
{
	if (!omniChemistryMetrics.activeTick || !omniAtmosphere)
		return;
	omniChemistryMetrics.finalCarbonMassKg = TotalOmniParticleCarbonMassKg();
	omniChemistryMetrics.finalOxygenMassKg = omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_O2);
	omniChemistryMetrics.finalCarbonDioxideMassKg = omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_CO2);
	const double initialTotal = omniChemistryMetrics.initialCarbonMassKg +
		omniChemistryMetrics.initialOxygenMassKg + omniChemistryMetrics.initialCarbonDioxideMassKg;
	const double finalTotal = omniChemistryMetrics.finalCarbonMassKg +
		omniChemistryMetrics.finalOxygenMassKg + omniChemistryMetrics.finalCarbonDioxideMassKg;
	omniChemistryMetrics.massResidualKg = finalTotal - initialTotal;
	omniChemistryMetrics.totalMassBalanceResidualKg = omniChemistryMetrics.massResidualKg +
		omniChemistryMetrics.externalCarbonMassSinkKg;
	omniChemistryMetrics.carbonAtomResidualMol =
		(omniChemistryMetrics.finalCarbonMassKg / CarbonMolarMassKgPerMol +
			omniChemistryMetrics.finalCarbonDioxideMassKg / CarbonDioxideMolarMassKgPerMol) -
		(omniChemistryMetrics.initialCarbonMassKg / CarbonMolarMassKgPerMol +
			omniChemistryMetrics.initialCarbonDioxideMassKg / CarbonDioxideMolarMassKgPerMol);
	omniChemistryMetrics.totalCarbonAtomBalanceResidualMol =
		omniChemistryMetrics.carbonAtomResidualMol +
		omniChemistryMetrics.externalCarbonMassSinkKg / CarbonMolarMassKgPerMol;
	omniChemistryMetrics.oxygenAtomResidualMol =
		2.0 * (omniChemistryMetrics.finalOxygenMassKg / OxygenMolarMassKgPerMol +
			omniChemistryMetrics.finalCarbonDioxideMassKg / CarbonDioxideMolarMassKgPerMol) -
		2.0 * (omniChemistryMetrics.initialOxygenMassKg / OxygenMolarMassKgPerMol +
			omniChemistryMetrics.initialCarbonDioxideMassKg / CarbonDioxideMolarMassKgPerMol);
	omniChemistryMetrics.activeTick = false;
}

void Simulation::RecordOmniCorrection(
	OmniCorrectionKind kind,
	int cellX,
	int cellY,
	float before,
	float after
)
{
	if (!omniCorrectionLedgerEnabled)
		return;

	const auto kindIndex = static_cast<size_t>(kind);
	if (kindIndex >= OmniCorrectionKindCount)
		return;
	omniCorrectionLedgerKindCounts[kindIndex]++;
	omniCorrectionLedgerTotalEvents++;
	omniCorrectionLedgerEvents[omniCorrectionLedgerNextEvent] = {
		omniCorrectionLedgerTotalEvents,
		cellX,
		cellY,
		before,
		after,
		kind,
	};
	if (omniCorrectionLedgerRetainedEvents < OmniCorrectionLedgerEventCapacity)
		omniCorrectionLedgerRetainedEvents++;
	else
		omniCorrectionLedgerDroppedEvents++;
	omniCorrectionLedgerNextEvent = (omniCorrectionLedgerNextEvent + 1)
		% OmniCorrectionLedgerEventCapacity;
}

void Simulation::BeginOmniLifecycleLedgerTick()
{
	if (!omniLifecycleLedgerEnabled)
		return;

	if (omniLifecycleLedgerActiveTick)
	{
		// A nested boundary would invalidate the prior frame's reconciliation.
		// There is no such path in the current dispatcher, but fail closed if one
		// is introduced later.
		omniLifecycleLedgerReconciliationFailures++;
		omniLifecycleLedgerLastFrameReconciled = false;
	}

	std::fill(omniLifecycleLedgerBeginHistogram.begin(), omniLifecycleLedgerBeginHistogram.end(), 0);
	std::fill(omniLifecycleLedgerEventDelta.begin(), omniLifecycleLedgerEventDelta.end(), 0);
	int64_t records = 0;
	int64_t invalidTypeRecords = 0;
	for (int i = 0; i < parts.active; i++)
	{
		auto type = parts[i].type;
		if (type != PT_NONE)
		{
			records++;
			if (type > PT_NONE && type < PT_NUM)
				omniLifecycleLedgerBeginHistogram[type]++;
			else
				invalidTypeRecords++;
		}
	}

	omniLifecycleLedgerLastBeginRecords = records;
	omniLifecycleLedgerLastInvalidTypeRecords = invalidTypeRecords;
	omniLifecycleLedgerActiveTick = true;
	omniLifecycleLedgerTicksStarted++;
}

void Simulation::EndOmniLifecycleLedgerTick()
{
	if (!omniLifecycleLedgerEnabled || !omniLifecycleLedgerActiveTick)
		return;

	std::array<int64_t, PT_NUM> endHistogram{};
	int64_t records = 0;
	int64_t invalidTypeRecords = 0;
	for (int i = 0; i < parts.active; i++)
	{
		auto type = parts[i].type;
		if (type != PT_NONE)
		{
			records++;
			if (type > PT_NONE && type < PT_NUM)
				endHistogram[type]++;
			else
				invalidTypeRecords++;
		}
	}

	auto absoluteRecordDelta = [](int64_t value) -> uint64_t {
		if (value >= 0)
			return static_cast<uint64_t>(value);
		return static_cast<uint64_t>(-(value + 1)) + 1;
	};

	uint64_t unattributedRecordDeltaAbs = 0;
	int firstMismatchedType = -1;
	for (int type = PT_NONE + 1; type < PT_NUM; type++)
	{
		auto expected = omniLifecycleLedgerBeginHistogram[type] + omniLifecycleLedgerEventDelta[type];
		auto delta = endHistogram[type] - expected;
		if (delta)
		{
			unattributedRecordDeltaAbs += absoluteRecordDelta(delta);
			if (firstMismatchedType == -1)
				firstMismatchedType = type;
		}
	}
	if (invalidTypeRecords != omniLifecycleLedgerLastInvalidTypeRecords)
	{
		unattributedRecordDeltaAbs += absoluteRecordDelta(
			invalidTypeRecords - omniLifecycleLedgerLastInvalidTypeRecords
		);
		if (firstMismatchedType == -1)
			firstMismatchedType = PT_NUM; // invalid-type sentinel, never a live Element ID
	}

	omniLifecycleLedgerLastEndRecords = records;
	omniLifecycleLedgerLastInvalidTypeRecords = invalidTypeRecords;
	omniLifecycleLedgerLastUnattributedRecordDeltaAbs = unattributedRecordDeltaAbs;
	omniLifecycleLedgerTotalUnattributedRecordDeltaAbs += unattributedRecordDeltaAbs;
	omniLifecycleLedgerLastFirstMismatchedType = firstMismatchedType;
	omniLifecycleLedgerLastFrameReconciled = unattributedRecordDeltaAbs == 0;
	if (!omniLifecycleLedgerLastFrameReconciled)
		omniLifecycleLedgerReconciliationFailures++;
	omniLifecycleLedgerActiveTick = false;
	omniLifecycleLedgerTicksCompleted++;
}

void Simulation::RecordOmniLifecycleMutation(int oldType, int newType, OmniLifecycleMutationKind kind)
{
	if (!omniLifecycleLedgerEnabled)
		return;

	bool direct = false;
	switch (kind)
	{
	case OmniLifecycleMutationKind::Create:
		omniLifecycleLedgerCreates++;
		break;
	case OmniLifecycleMutationKind::Kill:
		omniLifecycleLedgerKills++;
		break;
	case OmniLifecycleMutationKind::TypeChange:
		omniLifecycleLedgerTypeTransitions++;
		break;
	case OmniLifecycleMutationKind::Replacement:
		omniLifecycleLedgerTypeTransitions++;
		omniLifecycleLedgerReplacements++;
		break;
	case OmniLifecycleMutationKind::SparkFastPath:
		omniLifecycleLedgerTypeTransitions++;
		omniLifecycleLedgerSparkFastPathTransitions++;
		direct = true;
		break;
	case OmniLifecycleMutationKind::BrmtTungPreparation:
		omniLifecycleLedgerTypeTransitions++;
		omniLifecycleLedgerBrmtTungPreparationTransitions++;
		direct = true;
		break;
	case OmniLifecycleMutationKind::LoadFallback:
		omniLifecycleLedgerTypeTransitions++;
		omniLifecycleLedgerLoadFallbackTransitions++;
		direct = true;
		break;
	}
	if (direct)
		omniLifecycleLedgerDirectTypeTransitions++;

	if (!omniLifecycleLedgerActiveTick)
	{
		omniLifecycleLedgerOutsideTickEvents++;
		if (direct)
			omniLifecycleLedgerOutsideTickDirectTransitions++;
		return;
	}

	if (oldType > PT_NONE && oldType < PT_NUM)
		omniLifecycleLedgerEventDelta[oldType]--;
	if (newType > PT_NONE && newType < PT_NUM)
		omniLifecycleLedgerEventDelta[newType]++;
}

//updates pmap, gol, and some other simulation stuff (but not particles)
void Simulation::BeforeSim(bool willUpdate)
{
	BeginOmniLifecycleLedgerTick();
	if (willUpdate)
	{
		if (IsOmniAtmosphereActive())
		{
			FrameTime::Span span(frameTime, "OmniAtmosphere::Step");
			FrameTime::SubsystemSpan profilerSpan(frameTime, FrameTime::Subsystem::Air);
			SyncOmniAtmosphereBoundary(*this);
			ImportLegacyAtmosphereSources(*this);
			BeginOmniWaterCouplingTick();
			omniAtmosphere->Step();
			BeginOmniSolutionTick();
			BeginOmniChemistryTick();
			ExportOmniAtmosphereToLegacyFields(*this);
		}
		else
		{
			{
				FrameTime::Span span(frameTime, "Air::update_air");
				FrameTime::SubsystemSpan profilerSpan(frameTime, FrameTime::Subsystem::Air);
				air->update_air();
			}

			if(aheat_enable)
			{
				FrameTime::SubsystemSpan profilerSpan(frameTime, FrameTime::Subsystem::AmbientHeat);
				air->update_airh();
			}
		}

		{
			FrameTime::SubsystemSpan profilerSpan(frameTime, FrameTime::Subsystem::Gravity);
			DispatchNewtonianGravity();
		}
		// gravIn::mass is now potentially garbage, which is ok, we were going to clear it for the frame anyway
		for (auto p : gravIn.mass.Size().OriginRect())
		{
			gravIn.mass[p] = 0.f;
		}

		if(emp_decor>0)
			emp_decor -= emp_decor/25+2;
		if(emp_decor < 0)
			emp_decor = 0;
		etrd_count_valid = false;
		etrd_life0_count = 0;

		currentTick++;
		omniEventCountCurrentFrame.store(0, std::memory_order_relaxed);

		elementRecount |= !(currentTick%180);
		elementRecountAfterSim = elementRecount;
		if (elementRecount)
			std::fill(elementCount, elementCount+PT_NUM, 0);
	}
	sandcolour_interface = int(20.0f*sin(float(sandcolour_frame)*std::numbers::pi_v<float>/180.0f));
	sandcolour_frame = (sandcolour_frame+1)%360;
	sandcolour = int(20.0f*sin(float(frameCount)*std::numbers::pi_v<float>/180.0f));

	if (gravWallChanged)
	{
		UpdateGravityMask();
		gravWallChanged = false;
	}

	if (debug_nextToUpdate == 0)
		RecalcFreeParticles(willUpdate);

	if (willUpdate)
	{
		// decrease wall conduction, make walls block air and ambient heat
		for (int y = 0; y < YCELLS; y++)
		{
			for (int x = 0; x < XCELLS; x++)
			{
				if (emap[y][x])
					emap[y][x] --;
				air->bmap_blockair[y][x] = (bmap[y][x]==WL_WALL || bmap[y][x]==WL_WALLELEC || bmap[y][x]==WL_BLOCKAIR || (bmap[y][x]==WL_EWALL && !emap[y][x]));
				air->bmap_blockairh[y][x] = (air->bmap_blockair[y][x] || bmap[y][x]==WL_GRAV) ? 0x8 : 0;
			}
		}

		// check for stacking and create BHOL if found
		if (force_stacking_check || rng.chance(1, 10))
		{
			CheckStacking();
		}

		// LOVE and LOLZ element handling
		if (elementCount[PT_LOVE] > 0 || elementCount[PT_LOLZ] > 0)
		{
			int nx, nnx, ny, nny, r, rt;
			for (ny=0; ny<YRES-4; ny++)
			{
				for (nx=0; nx<XRES-4; nx++)
				{
					r=pmap[ny][nx];
					if (!r)
					{
						continue;
					}
					else if ((ny<9||nx<9||ny>YRES-7||nx>XRES-10)&&(parts[ID(r)].type==PT_LOVE||parts[ID(r)].type==PT_LOLZ))
						kill_part(ID(r));
					else if (parts[ID(r)].type==PT_LOVE)
					{
						Element_LOVE_love[nx/9][ny/9] = 1;
					}
					else if (parts[ID(r)].type==PT_LOLZ)
					{
						Element_LOLZ_lolz[nx/9][ny/9] = 1;
					}
				}
			}
			for (nx=9; nx<=XRES-18; nx++)
			{
				for (ny=9; ny<=YRES-7; ny++)
				{
					if (Element_LOVE_love[nx/9][ny/9]==1)
					{
						for ( nnx=0; nnx<9; nnx++)
							for ( nny=0; nny<9; nny++)
							{
								if (ny+nny>0&&ny+nny<YRES&&nx+nnx>=0&&nx+nnx<XRES)
								{
									rt=pmap[ny+nny][nx+nnx];
									if (!rt&&Element_LOVE_RuleTable[nnx][nny]==1)
										create_part(-1,nx+nnx,ny+nny,PT_LOVE);
									else if (!rt)
										continue;
									else if (parts[ID(rt)].type==PT_LOVE&&Element_LOVE_RuleTable[nnx][nny]==0)
										kill_part(ID(rt));
								}
							}
					}
					Element_LOVE_love[nx/9][ny/9]=0;
					if (Element_LOLZ_lolz[nx/9][ny/9]==1)
					{
						for ( nnx=0; nnx<9; nnx++)
							for ( nny=0; nny<9; nny++)
							{
								if (ny+nny>0&&ny+nny<YRES&&nx+nnx>=0&&nx+nnx<XRES)
								{
									rt=pmap[ny+nny][nx+nnx];
									if (!rt&&Element_LOLZ_RuleTable[nny][nnx]==1)
										create_part(-1,nx+nnx,ny+nny,PT_LOLZ);
									else if (!rt)
										continue;
									else if (parts[ID(rt)].type==PT_LOLZ&&Element_LOLZ_RuleTable[nny][nnx]==0)
										kill_part(ID(rt));

								}
							}
					}
					Element_LOLZ_lolz[nx/9][ny/9]=0;
				}
			}
		}

		// make WIRE work
		if(elementCount[PT_WIRE] > 0)
		{
			for (int nx = 0; nx < XRES; nx++)
			{
				for (int ny = 0; ny < YRES; ny++)
				{
					int r = pmap[ny][nx];
					if (!r)
						continue;
					if(parts[ID(r)].type == PT_WIRE)
						parts[ID(r)].tmp = parts[ID(r)].ctype;
				}
			}
		}

		// update PPIP tmp?
		if (Element_PPIP_ppip_changed)
		{
			for (int i = 0; i < parts.active; i++)
			{
				if (parts[i].type==PT_PPIP)
				{
					parts[i].tmp |= (parts[i].tmp&0xE0000000)>>3;
					parts[i].tmp &= ~0xE0000000;
				}
			}
			Element_PPIP_ppip_changed = 0;
		}

		// Simulate GoL
		// GSPEED is frames per generation
		if (elementCount[PT_LIFE]>0 && ++CGOL>=GSPEED)
		{
			SimulateGoL();
		}

		// wifi channel reseting
		if (ISWIRE > 0)
		{
			for (int q = 0; q < (int)(MAX_TEMP-73.15f)/100+2; q++)
			{
				wireless[q][0] = wireless[q][1];
				wireless[q][1] = 0;
			}
			ISWIRE--;
		}

		// spawn STKM and STK2
		if (!player.spwn && player.spawnID >= 0)
			create_part(-1, (int)parts[player.spawnID].x, (int)parts[player.spawnID].y, PT_STKM);
		if (!player2.spwn && player2.spawnID >= 0)
			create_part(-1, (int)parts[player2.spawnID].x, (int)parts[player2.spawnID].y, PT_STKM2);

		// particle update happens right after this function (called separately)
	}
}

void Simulation::AfterSim()
{
	if (IsOmniAtmosphereActive())
	{
		// The second Legacy projection reconciliation exists to capture particle
		// update writes and to commit queued condensed/gas water transfers.  In an
		// empty quiet world neither source exists, so repeating two full-grid
		// projection scans only burns the Atmosphere frame budget.  External Lua
		// or tool writes made after this callback are still imported by the next
		// BeforeSim reconciliation.
		const bool reconcileParticleSources = parts.active > 0 || !omniWaterTransferRequests.empty();
		if (reconcileParticleSources)
		{
			ImportLegacyAtmosphereSources(*this);
			if (omniWaterCouplingMetrics.activeTick && omniAtmosphere)
			{
				// Legacy field writers are external sources. Rebase the dedicated
				// particle/gas transaction ledger after importing them so coupling
				// residuals do not misattribute those sources.
				omniWaterCouplingMetrics.initialWaterMassKg = TotalOmniParticleWaterMassKg() +
					omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_H2O) +
					omniAtmosphere->TotalCondensedWaterMassKg();
				omniWaterCouplingMetrics.initialCoupledEnergyJ = TotalOmniWaterCoupledEnergyJ();
			}
			CommitOmniWaterCouplingTick();
			ExportOmniAtmosphereToLegacyFields(*this);
		}
		FinishOmniWaterCouplingTick();
		FinishOmniSolutionTick();
		FinishOmniChemistryTick();
	}
	debug_mostRecentlyUpdated = -1;
	bool repairElementCounts = elementRecountAfterSim;
	if (!repairElementCounts)
	{
		int64_t countedParticles = 0;
		for (int type = PT_NONE + 1; type < PT_NUM; type++)
		{
			if (elementCount[type] < 0)
			{
				repairElementCounts = true;
				break;
			}
			countedParticles += elementCount[type];
		}
		if (!repairElementCounts && countedParticles != NUM_PARTS)
			repairElementCounts = true;
	}
	if (repairElementCounts)
	{
		// The update loop can delete or change a later particle before that
		// particle has contributed to the in-progress recount. Rebuild from the
		// final live set so public element counts cannot remain negative or stale
		// whenever the cheap per-frame invariant check detects mismatched totals,
		// while retaining the periodic repair.
		auto &elements = SimulationData::CRef().elements;
		std::fill(elementCount, elementCount + PT_NUM, 0);
		for (int i = 0; i < parts.active; i++)
		{
			auto type = parts[i].type;
			if (type > PT_NONE && type < PT_NUM && elements[type].Enabled)
				elementCount[type]++;
		}
		elementRecountAfterSim = false;
	}

	if (emp_trigger_count)
	{
		// pitiful attempt at trying to keep code relating to a given element in the same file
		Element_EMP_Trigger(this, emp_trigger_count);
		emp_trigger_count = 0;
	}

	EndOmniLifecycleLedgerTick();
	frameCount += 1;
}

Simulation::~Simulation() = default;

Simulation::Simulation()
{
	std::fill(elementCount, elementCount+PT_NUM, 0);
	elementRecount = true;

	//Create and attach air simulation
	air = std::make_unique<Air>(*this);
	omniAtmosphere = std::make_unique<OmniAtmosphere>(OmniAtmosphereConfig{
		.width = XCELLS,
		.height = YCELLS,
	});

	player.comm = 0;
	player2.comm = 0;

	clear_sim();

	UpdateGravityMask();
}

void Simulation::DispatchNewtonianGravity()
{
	if (grav)
	{
		grav->Exchange(gravOut, gravIn, gravForceRecalc);
		gravForceRecalc = false;
	}
}

void Simulation::ResetNewtonianGravity(GravityInput newGravIn, GravityOutput newGravOut)
{
	gravIn = newGravIn;
	DispatchNewtonianGravity();
	// gravIn is now potentially garbage, set it again
	gravIn = newGravIn;
	if (grav)
	{
		gravOut = newGravOut;
		gravForceRecalc = true; // gravOut changed outside DispatchNewtonianGravity
		gravWallChanged = true;
	}
}

void Simulation::EnableNewtonianGravity(bool enable)
{
	if (grav && !enable)
	{
		grav.reset();
		gravOut = {}; // reset as per the invariant
		gravForceRecalc = true; // gravOut changed outside DispatchNewtonianGravity
	}
	if (!grav && enable)
	{
		grav = Gravity::Create();
		auto oldGravIn = gravIn;
		DispatchNewtonianGravity();
		// gravIn is now potentially garbage, set it again
		gravIn = std::move(oldGravIn);
	}
}

// we want XRES * YRES <= (1 << (31 - PMAPBITS)), but we do a division because multiplication could silently overflow
static_assert(uint32_t(XRES) <= (UINT32_C(1) << (31 - PMAPBITS)) / uint32_t(YRES), "not enough space in pmap");
