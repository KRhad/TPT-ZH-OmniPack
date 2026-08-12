#pragma once
#include "Particle.h"
#include "Stickman.h"
#include "WallType.h"
#include "Sign.h"
#include "ElementDefs.h"
#include "BuiltinGOL.h"
#include "MenuSection.h"
#include "AccessProperty.h"
#include "CoordStack.h"
#include "common/tpt-rand.h"
#include "gravity/Gravity.h"
#include "graphics/RendererFrame.h"
#include "Element.h"
#include "SimulationConfig.h"
#include "SimulationSettings.h"
#include <cstring>
#include <cstddef>
#include <atomic>
#include <vector>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>

constexpr int CHANNELS = int(MAX_TEMP - 73) / 100 + 2;

class FrameTime;
class Snapshot;
class Brush;
struct SimulationSample;
struct matrix2d;
struct vector2d;

class Simulation;
class Renderer;
class Air;
class OmniAtmosphere;
class GameSave;

class Parts
{
	int pfree;

public:
	std::array<Particle, NPART> data;
	// initialized in clear_sim
	int active;

	operator const Particle *() const
	{
		return data.data();
	}

	operator Particle *()
	{
		return data.data();
	}

	Parts()
	{
		Reset();
	}

	Parts(const Parts &other) = default;

	Parts &operator =(const Parts &other)
	{
		std::copy(other.data.begin(), other.data.begin() + other.active, data.begin());
		active = other.active;
		pfree = other.pfree;
		return *this;
	}

	Parts(const Parts &&other) = delete;
	Parts &operator =(const Parts &&other) = delete;

	void Reset();
	void Free(int i);
	int Alloc();
	void Flatten();

	bool MaxPartsReached() const
	{
		return pfree == -1 && active >= NPART;
	}
};

struct RenderableSimulation
{
	GravityInput gravIn;
	GravityOutput gravOut; // invariant: when grav is empty, this is in its default-constructed state
	bool gravForceRecalc = true;
	std::vector<sign> signs;

	int currentTick = 0;
	int emp_decor = 0;

	playerst player;
	playerst player2;
	playerst fighters[MAX_FIGHTERS]; //Defined in Stickman.h

	float vx[YCELLS][XCELLS];
	float vy[YCELLS][XCELLS];
	float pv[YCELLS][XCELLS];
	float hv[YCELLS][XCELLS];

	unsigned char bmap[YCELLS][XCELLS];
	unsigned char emap[YCELLS][XCELLS];

	Parts parts;
	int pmap[YRES][XRES];
	int photons[YRES][XRES];

	int aheat_enable = 0;

	bool useLuaCallbacks = false;
};

class Simulation : public RenderableSimulation
{
public:
	enum class OmniAtmospherePersistenceStatus : uint8_t
	{
		ClassicNotApplicable,
		FreshPreset,
		LoadedV2,
		MigratedLegacyProjection,
		RegionStateOmitted,
	};

	GravityPtr grav;
	std::unique_ptr<Air> air;
	std::unique_ptr<OmniAtmosphere> omniAtmosphere;
	int omniSimulationMode = OMNI_CLASSIC;
	OmniAtmospherePersistenceStatus omniAtmospherePersistenceStatus =
		OmniAtmospherePersistenceStatus::ClassicNotApplicable;

	RNG rng;

	int replaceModeSelected = 0;
	int replaceModeFlags = 0;
	int debug_nextToUpdate = 0;
	int debug_mostRecentlyUpdated = -1; // -1 when between full update loops
	int elementCount[PT_NUM];
	int ISWIRE = 0;
	bool force_stacking_check = false;
	int emp_trigger_count = 0;
	bool etrd_count_valid = false;
	int etrd_life0_count = 0;
	int lightningRecreate = 0;
	bool gravWallChanged = false;

	Particle portalp[CHANNELS][8][80];
	int wireless[CHANNELS][2];

	int CGOL = 0;
	int GSPEED = 1;
	unsigned int gol[YRES][XRES][5];

	float fvx[YCELLS][XCELLS];
	float fvy[YCELLS][XCELLS];
	float omniLegacyPressureShadow[YCELLS][XCELLS]{};
	float omniLegacyVelocityXShadow[YCELLS][XCELLS]{};
	float omniLegacyVelocityYShadow[YCELLS][XCELLS]{};
	float omniLegacyTemperatureShadow[YCELLS][XCELLS]{};
	bool omniLegacyProjectionShadowValid = false;
	int Element_LOLZ_lolz[XRES/9][YRES/9];
	int Element_LOVE_love[XRES/9][YRES/9];
	int Element_PSTN_tempParts[std::max(XRES, YRES)];
	int Element_PPIP_ppip_changed;

	unsigned int pmap_count[YRES][XRES];

	int edgeMode = EDGE_VOID;
	int gravityMode = GRAV_VERTICAL;
	float customGravityX = 0;
	float customGravityY = 0;
	int legacy_enable = 0;
	int water_equal_test = 0;
	int pretty_powder = 0;
	int sandcolour_frame = 0;
	int deco_space = DECOSPACE_SRGB;

	// initialized in clear_sim
	bool elementRecount;
	bool elementRecountAfterSim;
	unsigned char fighcount; //Contains the number of fighters
	uint64_t frameCount;
	bool ensureDeterminism;

	struct OmniEventMetrics
	{
		uint64_t total;
		uint64_t currentFrame;
		uint64_t peakPerFrame;
	};

	// Audit-only record accounting. These counters deliberately have no mass,
	// amount-of-substance, momentum or energy units.
	struct OmniLifecycleLedgerMetrics
	{
		bool enabled;
		bool activeTick;
		bool recordUnitsOnly;
		bool lastFrameReconciled;
		uint64_t ticksStarted;
		uint64_t ticksCompleted;
		uint64_t reconciliationFailures;
		uint64_t creates;
		uint64_t kills;
		uint64_t typeTransitions;
		uint64_t replacements;
		uint64_t directTypeTransitions;
		uint64_t sparkFastPathTransitions;
		uint64_t brmtTungPreparationTransitions;
		uint64_t loadFallbackTransitions;
		uint64_t outsideTickEvents;
		uint64_t outsideTickDirectTransitions;
		int64_t lastBeginRecords;
		int64_t lastEndRecords;
		uint64_t lastUnattributedRecordDeltaAbs;
		uint64_t totalUnattributedRecordDeltaAbs;
		int lastFirstMismatchedType;
		int64_t lastInvalidTypeRecords;
	};

	// This is a Legacy-field diagnostic. A correction event reports a specific
	// executed cap branch; it is not a physical source/sink or conservation term.
	enum class OmniCorrectionKind : uint8_t
	{
		AirAmbientHeatTemperatureCapHigh,
		AirAmbientHeatTemperatureCapLow,
		AirAmbientHeatVelocityXCapHigh,
		AirAmbientHeatVelocityXCapLow,
		AirAmbientHeatVelocityYCapHigh,
		AirAmbientHeatVelocityYCapLow,
		AirDynamicsPressureCapHigh,
		AirDynamicsPressureCapLow,
		AirDynamicsVelocityXCapHigh,
		AirDynamicsVelocityXCapLow,
		AirDynamicsVelocityYCapHigh,
		AirDynamicsVelocityYCapLow,
		Count,
	};
	static constexpr size_t OmniCorrectionKindCount = static_cast<size_t>(OmniCorrectionKind::Count);
	static constexpr size_t OmniCorrectionLedgerEventCapacity = 256;

	struct OmniCorrectionEvent
	{
		uint64_t sequence{};
		int cellX{};
		int cellY{};
		float before{};
		float after{};
		OmniCorrectionKind kind{};
	};

	struct OmniCorrectionLedgerMetrics
	{
		bool enabled;
		bool legacyFieldUnitsOnly;
		bool auditedAirCapsOnly;
		uint64_t totalEvents;
		uint64_t retainedEvents;
		uint64_t droppedEvents;
		std::array<uint64_t, OmniCorrectionKindCount> kindCounts;
		std::array<OmniCorrectionEvent, OmniCorrectionLedgerEventCapacity> events;
	};

	struct OmniWaterCouplingMetrics
	{
		bool activeTick = false;
		double initialWaterMassKg = 0.0;
		double finalWaterMassKg = 0.0;
		// This is a deliberately narrow transaction total: atmosphere energy plus
		// the enthalpy owned by condensed/water-vapour parcels.  It makes a
		// transfer verifiable without pretending that unrelated Legacy field
		// writers are physical sources.
		double initialCoupledEnergyJ = 0.0;
		double finalCoupledEnergyJ = 0.0;
		double transferredToAtmosphereKg = 0.0;
		double sensibleEnergyToParticlesJ = 0.0;
		double particleEnergyRemovedJ = 0.0;
		double atmosphereEnergyAddedJ = 0.0;
		// Legacy tools, walls, off-screen culling and non-water type changes are
		// explicit external lifecycle events, never implicit evaporation.
		double externalWaterMassSourceKg = 0.0;
		double externalWaterMassSinkKg = 0.0;
		double externalWaterEnergySourceJ = 0.0;
		double externalWaterEnergySinkJ = 0.0;
		double waterMassResidualKg = 0.0;
		double coupledEnergyResidualJ = 0.0;
		uint64_t requests = 0;
		uint64_t vaporParcelsInjected = 0;
		uint64_t evaporationTransfers = 0;
		uint64_t phaseTypeChanges = 0;
	};

	// Enhanced-mode carbon combustion ledger. Authoritative carbon mass lives
	// in a dedicated sidecar; Particle ABI and existing tmp4 semantics remain
	// untouched.
	struct OmniChemistryMetrics
	{
		bool activeTick = false;
		double initialCarbonMassKg = 0.0;
		double finalCarbonMassKg = 0.0;
		double initialOxygenMassKg = 0.0;
		double finalOxygenMassKg = 0.0;
		double initialCarbonDioxideMassKg = 0.0;
		double finalCarbonDioxideMassKg = 0.0;
		double carbonConsumedKg = 0.0;
		double oxygenConsumedKg = 0.0;
		double carbonDioxideProducedKg = 0.0;
		double chemicalEnergyReleasedJ = 0.0;
		double transactionEnergyDeltaJ = 0.0;
		double massResidualKg = 0.0;
		double carbonAtomResidualMol = 0.0;
		double oxygenAtomResidualMol = 0.0;
		double energyResidualJ = 0.0;
		// Legacy type changes/deletions are external ownership events, not
		// carbon-oxidation products. Keep their mass separate from the closed
		// reaction conservation residual.
		double externalCarbonMassSinkKg = 0.0;
		double totalMassBalanceResidualKg = 0.0;
		double totalCarbonAtomBalanceResidualMol = 0.0;
		uint64_t candidateParticles = 0;
		uint64_t committedTransactions = 0;
		uint64_t oxygenLimitedTransactions = 0;
		uint64_t carbonLimitedTransactions = 0;
		uint64_t rejectedTransactions = 0;
	};

	// Enhanced/Scientific NaCl solution ledger. Solvent ownership remains the
	// existing Omni water parcel sidecar; this sidecar owns only the dissolved
	// or crystallised NaCl mass and mirrors the solvent for validation.
	struct OmniSolutionMetrics
	{
		bool activeTick = false;
		double initialSolventMassKg = 0.0;
		double finalSolventMassKg = 0.0;
		double initialSoluteMassKg = 0.0;
		double finalSoluteMassKg = 0.0;
		double dissolvedMassKg = 0.0;
		double crystallisedMassKg = 0.0;
		double neutralisedAcidMassKg = 0.0;
		double neutralisedBaseMassKg = 0.0;
		double neutralSaltProducedKg = 0.0;
		double neutralisationWaterProducedKg = 0.0;
		double neutralisationEnergyReleasedJ = 0.0;
		double transferredSolventFromWaterKg = 0.0;
		double transferredSolventToAtmosphereKg = 0.0;
		double externalSolventSourceKg = 0.0;
		double externalSolventSinkKg = 0.0;
		double externalSoluteSourceKg = 0.0;
		double externalSoluteSinkKg = 0.0;
		double solventMassResidualKg = 0.0;
		double soluteMassResidualKg = 0.0;
		double totalSolutionMassResidualKg = 0.0;
		uint64_t dissolutionTransactions = 0;
		uint64_t crystallisationTransactions = 0;
		uint64_t neutralisationTransactions = 0;
		uint64_t saturationLimitedTransactions = 0;
		uint64_t rateLimitedTransactions = 0;
	};

	// initialized very late >_>
	int NUM_PARTS;
	int sandcolour;
	int sandcolour_interface;

	void Load(const GameSave *save, bool includePressure, Vec2<int> blockP); // block coordinates
	std::unique_ptr<GameSave> Save(bool includePressure, Rect<int> partR); // particle coordinates
	void SaveSimOptions(GameSave &gameSave);
	SimulationSample GetSample(int x, int y);

	std::unique_ptr<Snapshot> CreateSnapshot() const;
	void Restore(const Snapshot &snap);

	int is_blocking(int t, int x, int y) const;
	int is_boundary(int pt, int x, int y) const;
	int find_next_boundary(int pt, int *x, int *y, int dm, int *em, bool reverse) const;
	void photoelectric_effect(int nx, int ny);
	int do_move(int i, int x, int y, float nxf, float nyf);
	bool move(int i, int x, int y, float nxf, float nyf);
	int try_move(int i, int x, int y, int nx, int ny);
	int eval_move(int pt, int nx, int ny, unsigned *rr) const;

	struct PlanMoveResult
	{
		int fin_x, fin_y, clear_x, clear_y;
		float fin_xf, fin_yf, clear_xf, clear_yf;
		float vx, vy;
	};
	template<bool UpdateEmap, class Sim>
	static PlanMoveResult PlanMove(Sim &sim, int i, int x, int y);

	bool IsWallBlocking(int x, int y, int type) const;
	void create_cherenkov_photon(int pp);
	void create_gain_photon(int pp);
	void kill_part(int i);
	bool FloodFillPmapCheck(int x, int y, int type) const;
	int flood_prop(int x, int y, const AccessProperty &changeProperty);
	bool flood_water(int x, int y, int i);
	int FloodINST(int x, int y);
	void detach(int i);
	bool part_change_type(int i, int x, int y, int t);
	//int InCurrentBrush(int i, int j, int rx, int ry);
	//int get_brush_flags();
	int create_part(int p, int x, int y, int t, int v = -1);
	int createPartTempVel(int i, int x, int y, int t);
	void delete_part(int x, int y);
	void get_sign_pos(int i, int *x0, int *y0, int *w, int *h);
	int is_wire(int x, int y);
	int is_wire_off(int x, int y);
	void set_emap(int x, int y);
	int parts_avg(int ci, int ni, int t);
	virtual void UpdateParticles(int start, int end) = 0; // Dispatches an update to the range [start, end).
	void SimulateGoL();
	void RecalcFreeParticles(bool do_life_dec);
	void CheckStacking();
	void BeforeSim(bool willUpdate);
	void AfterSim();
	void clear_area(int area_x, int area_y, int area_w, int area_h);
	void ResetOmniEventMetrics();
	void RecordOmniEvent();
	OmniEventMetrics GetOmniEventMetrics() const;
	void SetOmniLifecycleLedgerEnabled(bool enabled);
	void ResetOmniLifecycleLedger();
	OmniLifecycleLedgerMetrics GetOmniLifecycleLedgerMetrics() const;
	void SetOmniCorrectionLedgerEnabled(bool enabled);
	void ResetOmniCorrectionLedger();
	OmniCorrectionLedgerMetrics GetOmniCorrectionLedgerMetrics() const;
	OmniWaterCouplingMetrics GetOmniWaterCouplingMetrics() const { return omniWaterCouplingMetrics; }
	OmniChemistryMetrics GetOmniChemistryMetrics() const { return omniChemistryMetrics; }
	OmniSolutionMetrics GetOmniSolutionMetrics() const { return omniSolutionMetrics; }
	double GetOmniWaterParcelMassKg(int particleId) const;
	double GetOmniWaterParcelSpecificEnthalpyJPerKg(int particleId) const;
	double TotalOmniParticleWaterMassKg() const;
	double GetOmniCarbonParcelMassKg(int particleId) const;
	double TotalOmniParticleCarbonMassKg() const;
	// Called by COAL/BCOL only in Enhanced/Scientific. Classic never enters
	// this route and retains upstream combustion semantics.
	bool UpdateOmniCarbonCombustion(int particleId, int x, int y);
	// Enhanced/Scientific solution ownership boundary. Classic returns false and
	// continues through the official Legacy element update path.
	bool UpdateOmniSolutionParticle(int particleId, int x, int y);
	double GetOmniSolutionSolventMassKg(int particleId) const;
	double GetOmniSolutionSoluteMassKg(int particleId) const;
	double GetOmniSolutionNeutralSaltMassKg(int particleId) const;

	void SetEdgeMode(int newEdgeMode);
	void SetDecoSpace(int newDecoSpace);
	void SetOmniSimulationMode(int newMode);
	int GetOmniSimulationMode() const { return omniSimulationMode; }
	bool IsOmniAtmosphereActive() const { return omniSimulationMode != OMNI_CLASSIC; }
	const char *GetOmniAtmospherePersistenceStatus() const;

	//Drawing Deco
	void ApplyDecoration(int x, int y, int colR, int colG, int colB, int colA, int mode);
	void ApplyDecorationPoint(int x, int y, int colR, int colG, int colB, int colA, int mode, Brush const &cBrush);
	void ApplyDecorationLine(int x1, int y1, int x2, int y2, int colR, int colG, int colB, int colA, int mode, Brush const &cBrush);
	void ApplyDecorationBox(int x1, int y1, int x2, int y2, int colR, int colG, int colB, int colA, int mode);
	bool ColorCompare(const RendererFrame &frame, int x, int y, int replaceR, int replaceG, int replaceB);
	void ApplyDecorationFill(const RendererFrame &frame, int x, int y, int colR, int colG, int colB, int colA, int replaceR, int replaceG, int replaceB);

	//Drawing Walls
	int CreateWalls(int x, int y, int rx, int ry, int wall, Brush const *cBrush);
	void CreateWallLine(int x1, int y1, int x2, int y2, int rx, int ry, int wall, Brush const *cBrush);
	void CreateWallBox(int x1, int y1, int x2, int y2, int wall);
	int FloodWalls(int x, int y, int wall, int bm);

	//Drawing Particles
	int CreateParts(int p, int positionX, int positionY, int c, Brush const &cBrush, int flags);
	int CreateParts(int p, int x, int y, int rx, int ry, int c, int flags);
	int CreatePartFlags(int p, int x, int y, int c, int flags);
	void CreateLine(int x1, int y1, int x2, int y2, int c, Brush const &cBrush, int flags);
	void CreateLine(int x1, int y1, int x2, int y2, int c);
	void CreateBox(int p, int x1, int y1, int x2, int y2, int c, int flags);
	int FloodParts(int x, int y, int c, int cm, int flags);

	void GetGravityField(int x, int y, float particleGrav, float newtonGrav, float & pGravX, float & pGravY) const;

	int get_wavelength_bin(int *wm);
	struct GetNormalResult
	{
		bool success;
		float nx, ny;
		int lx, ly, rx, ry;
	};
	GetNormalResult get_normal(int pt, int x, int y, float dx, float dy) const;
	template<bool PhotoelectricEffect, class Sim>
	static GetNormalResult get_normal_interp(Sim &sim, int pt, float x0, float y0, float dx, float dy);
	void clear_sim();
	Simulation();
	virtual ~Simulation();

	void EnableNewtonianGravity(bool enable);

	FrameTime *frameTime = nullptr;

	static std::unique_ptr<Simulation> Factory();

protected:
	struct OmniWaterTransferRequest
	{
		int particleId = -1;
		int cellX = 0;
		int cellY = 0;
		double sensibleEnergyToParticleJ = 0.0;
		double requestedEvaporationMassKg = 0.0;
		bool injectVaporParcel = false;
	};

	std::array<double, NPART> omniWaterParcelMassKg{};
	// The particle temperature is a renderer/Legacy projection. This sidecar is
	// authoritative for water parcels so energy inside a latent-heat plateau is
	// not discarded between fixed simulation ticks.
	std::array<double, NPART> omniWaterParcelSpecificEnthalpyJPerKg{};
	std::array<double, NPART> omniCarbonParcelMassKg{};
	std::array<double, NPART> omniSolutionSolventMassKg{};
	std::array<double, NPART> omniSolutionSoluteMassKg{};
	std::array<double, NPART> omniSolutionNeutralSaltMassKg{};
	std::vector<OmniWaterTransferRequest> omniWaterTransferRequests;
	OmniWaterCouplingMetrics omniWaterCouplingMetrics{};
	OmniChemistryMetrics omniChemistryMetrics{};
	OmniSolutionMetrics omniSolutionMetrics{};
	bool omniSolutionInternalMutation = false;

	bool QueueOmniWaterParticleCoupling(int particleId, int x, int y);
	void BeginOmniWaterCouplingTick();
	void CommitOmniWaterCouplingTick();
	void FinishOmniWaterCouplingTick();
	void TransferOmniWaterParticleToAtmosphere(int particleId, int cellX, int cellY,
		double requestedMassKg = -1.0);
	void InitializeOmniWaterParcelMass(int particleId, int type);
	void ClearOmniWaterParcelMass(int particleId, bool recordExternalSink = true);
	double OmniWaterParcelSpecificEnthalpy(int particleId) const;
	double TotalOmniWaterCoupledEnergyJ() const;
	void InitializeOmniCarbonParcelMass(int particleId, int type);
	void ClearOmniCarbonParcelMass(int particleId, bool recordExternalSink = true);
	void BeginOmniChemistryTick();
	void FinishOmniChemistryTick();
	void BeginOmniSolutionTick();
	void FinishOmniSolutionTick();
	void InitializeOmniSolutionState(int particleId, int type, bool directCreate);
	void ClearOmniSolutionState(int particleId, bool recordExternalSink = true);
	void SetOmniSolutionMassesKg(int particleId, double solventMassKg, double soluteMassKg,
		bool recordLedgerAdjustment = true);
	void SetOmniSolutionNeutralSaltMassKg(int particleId, double massKg,
		bool recordLedgerAdjustment = true);
	double TotalOmniSolutionSolventMassKg() const;
	double TotalOmniSolutionSoluteMassKg() const;

	enum class OmniLifecycleMutationKind : uint8_t
	{
		Create,
		Kill,
		TypeChange,
		Replacement,
		SparkFastPath,
		BrmtTungPreparation,
		LoadFallback,
	};
	void RecordOmniLifecycleMutation(int oldType, int newType, OmniLifecycleMutationKind kind);

private:
	friend class Air;

	std::atomic<uint64_t> omniEventCountTotal{ 0 };
	std::atomic<uint64_t> omniEventCountCurrentFrame{ 0 };
	std::atomic<uint64_t> omniEventCountPeakPerFrame{ 0 };

	// The current SimulationImpl particle dispatcher is serial. A future
	// parallel dispatcher must add a deterministic per-worker reduction before
	// recording into these per-type arrays.
	bool omniLifecycleLedgerEnabled = false;
	bool omniLifecycleLedgerActiveTick = false;
	bool omniLifecycleLedgerLastFrameReconciled = false;
	uint64_t omniLifecycleLedgerTicksStarted = 0;
	uint64_t omniLifecycleLedgerTicksCompleted = 0;
	uint64_t omniLifecycleLedgerReconciliationFailures = 0;
	uint64_t omniLifecycleLedgerCreates = 0;
	uint64_t omniLifecycleLedgerKills = 0;
	uint64_t omniLifecycleLedgerTypeTransitions = 0;
	uint64_t omniLifecycleLedgerReplacements = 0;
	uint64_t omniLifecycleLedgerDirectTypeTransitions = 0;
	uint64_t omniLifecycleLedgerSparkFastPathTransitions = 0;
	uint64_t omniLifecycleLedgerBrmtTungPreparationTransitions = 0;
	uint64_t omniLifecycleLedgerLoadFallbackTransitions = 0;
	uint64_t omniLifecycleLedgerOutsideTickEvents = 0;
	uint64_t omniLifecycleLedgerOutsideTickDirectTransitions = 0;
	int64_t omniLifecycleLedgerLastBeginRecords = 0;
	int64_t omniLifecycleLedgerLastEndRecords = 0;
	uint64_t omniLifecycleLedgerLastUnattributedRecordDeltaAbs = 0;
	uint64_t omniLifecycleLedgerTotalUnattributedRecordDeltaAbs = 0;
	int omniLifecycleLedgerLastFirstMismatchedType = -1;
	int64_t omniLifecycleLedgerLastInvalidTypeRecords = 0;
	std::array<int64_t, PT_NUM> omniLifecycleLedgerBeginHistogram{};
	std::array<int64_t, PT_NUM> omniLifecycleLedgerEventDelta{};

	bool omniCorrectionLedgerEnabled = false;
	uint64_t omniCorrectionLedgerTotalEvents = 0;
	uint64_t omniCorrectionLedgerDroppedEvents = 0;
	size_t omniCorrectionLedgerNextEvent = 0;
	size_t omniCorrectionLedgerRetainedEvents = 0;
	std::array<uint64_t, OmniCorrectionKindCount> omniCorrectionLedgerKindCounts{};
	std::array<OmniCorrectionEvent, OmniCorrectionLedgerEventCapacity> omniCorrectionLedgerEvents{};

	void BeginOmniLifecycleLedgerTick();
	void EndOmniLifecycleLedgerTick();
	// The current Air solver is serial. A future parallel backend must record
	// per-worker events and merge them deterministically before exposing them.
	void RecordOmniCorrection(OmniCorrectionKind kind, int cellX, int cellY, float before, float after);

	CoordStack& getCoordStackSingleton();

	void ResetNewtonianGravity(GravityInput newGravIn, GravityOutput newGravOut);
	void DispatchNewtonianGravity();
	void UpdateGravityMask();
};
