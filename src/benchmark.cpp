#include <cstdio>
#include <cmath>

#include "EventLoopSDL.h"
#include "powder.h"
#include "graphics.h"
#include "powdergraphics.h"
#include "benchmark.h"
#include "save_legacy.h"

#include "common/Point.h"
#include "game/Save.h"
#include "game/Sign.h"
#include "graphics/Pixel.h"
#include "json/json.h"
#include "simulation/Simulation.h"

char *benchmark_file = NULL;
double benchmark_loops_multiply = 1.0; // Increase for more accurate results (particularly on fast computers)
int benchmark_repeat_count = 5; // this too, but try benchmark_loops_multiply first

double benchmark_get_time()
{
	return GetTicks()/1000.0;
}

// repeat_count - how many times to run the test, iterations_count = number of loops to execute each time

#define BENCHMARK_INIT(repeat_count, iterations_count) \
{\
	int bench_i, bench_iterations = (int)((iterations_count)*benchmark_loops_multiply);\
	int bench_run_i, bench_runs=(repeat_count);\
	double bench_mean=0.0, bench_prevmean, bench_variance=0.0;\
	double bench_start, bench_end;\
	for (bench_run_i=1; bench_run_i<=bench_runs; bench_run_i++)\
	{

#define BENCHMARK_RUN() \
		bench_start = benchmark_get_time();\
		for (bench_i=0;bench_i<bench_iterations;bench_i++)\
		{


#define BENCHMARK_START(repeat_count, iterations_count) \
	BENCHMARK_INIT(repeat_count, iterations_count) \
	BENCHMARK_RUN()

#define BENCHMARK_END() \
		}\
		bench_end = benchmark_get_time();\
		bench_prevmean = bench_mean;\
		bench_mean += ((double)(bench_end-bench_start) - bench_mean) / bench_run_i;\
		bench_variance += (bench_end-bench_start - bench_prevmean) * (bench_end-bench_start - bench_mean);\
	}\
	if (bench_runs>1) \
		printf("mean time per iteration %g ms, std dev %g ms (%g%%)\n", bench_mean/bench_iterations * 1000.0, sqrt(bench_variance/(bench_runs-1))/bench_iterations * 1000.0, sqrt(bench_variance/(bench_runs-1)) / bench_mean * 100.0);\
	else \
		printf("mean time per iteration %g ms\n", bench_mean/bench_iterations * 1000.0);\
}

bool benchmark_load_save(Simulation *sim, Save *save)
{
	try
	{
		sim->LoadSave(0, 0, save, 1);
	}
	catch (ParseException & e)
	{
		printf("Error loading save: %s", e.what());
		return true;
	}
	return false;
}

void benchmark_run()
{
	pixel *vid_buf = (pixel*)calloc((XRES+BARSIZE)*(YRES+MENUSIZE), PIXELSIZE);
	Simulation *sim = globalSim;
	aheat_enable = true;
	if (benchmark_file)
	{
		int size;
		char *file_data = (char*)file_load(benchmark_file, &size);
		Save *save = new Save(file_data, size);
		if (file_data)
		{
			if (!benchmark_load_save(sim, save))
			{
				printf("Save speed test:\n");

				printf("Update particles+air: ");
				BENCHMARK_INIT(benchmark_repeat_count, 200)
				{
					benchmark_load_save(sim, save);
					sys_pause = false;
					framerender = 0;
					BENCHMARK_RUN()
					{
						sim->air->UpdateAir();
						sim->air->UpdateAirHeat(sim);
						sim->Tick();
					}
				}
				BENCHMARK_END()

				printf("Load save: ");
				BENCHMARK_INIT(benchmark_repeat_count, 100)
				{
					BENCHMARK_RUN()
					{
						benchmark_load_save(sim, save);
					}
				}
				BENCHMARK_END()

				printf("Update particles - paused: ");
				BENCHMARK_INIT(benchmark_repeat_count, 1000)
				{
					benchmark_load_save(sim, save);
					sys_pause = true;
					framerender = 0;
					BENCHMARK_RUN()
					{
						sim->Tick();
					}
				}
				BENCHMARK_END()

				printf("Update particles - unpaused: ");
				BENCHMARK_INIT(benchmark_repeat_count, 200)
				{
					benchmark_load_save(sim, save);
					sys_pause = false;
					framerender = 0;
					BENCHMARK_RUN()
					{
						sim->Tick();
					}
				}
				BENCHMARK_END()

				printf("Render particles: ");
				BENCHMARK_INIT(benchmark_repeat_count, 1500)
				{
					benchmark_load_save(sim, save);
					sys_pause = false;
					framerender = 0;
					decorations_enable = true;
					sim->Tick();
					BENCHMARK_RUN()
					{
						render_parts(vid_buf, sim, Point(0,0));
					}
				}
				BENCHMARK_END()

				printf("Render particles+fire: ");
				BENCHMARK_INIT(benchmark_repeat_count, 1200)
				{
					benchmark_load_save(sim, save);
					sys_pause = false;
					framerender = 0;
					decorations_enable = true;
					sim->Tick();
					BENCHMARK_RUN()
					{
						render_parts(vid_buf, sim, Point(0, 0));
						render_fire(vid_buf);
					}
				}
				BENCHMARK_END()


			}
			free(file_data);
		}
	}
	else
	{
		printf("General speed test:\n");
		clear_sim();

		// Commenting out because rarely necessary to run benchmark, and this uses private methods
/*#ifdef GRAVFFT
		sim->grav->grav_fft_init();
#endif
		sim->grav->update_grav();
		printf("Gravity - no gravmap changes: ");
		BENCHMARK_START(benchmark_repeat_count, 100000)
		{
			sim->grav->update_grav();
		}
		BENCHMARK_END()

		printf("Gravity - 1 gravmap cell changed: ");
		BENCHMARK_START(benchmark_repeat_count, 1000)
		{
			sim->grav->th_gravmap[(YRES/CELL-1)*(XRES/CELL) + XRES/CELL - 1] = (bench_i%5)+1.0f; //bench_i defined in BENCHMARK_START macro
			sim->grav->update_grav();
		}
		BENCHMARK_END()

		printf("Gravity - membwand: ");
		BENCHMARK_START(benchmark_repeat_count, 10000)
		{
			membwand(sim->grav->gravy, sim->grav->gravmask, (XRES/CELL)*(YRES/CELL)*sizeof(float), (XRES/CELL)*(YRES/CELL)*sizeof(unsigned));
			membwand(sim->grav->gravx, sim->grav->gravmask, (XRES/CELL)*(YRES/CELL)*sizeof(float), (XRES/CELL)*(YRES/CELL)*sizeof(unsigned));
		}
		BENCHMARK_END()*/

		printf("Air - no walls, no changes: ");
		BENCHMARK_START(benchmark_repeat_count, 3000)
		{
			sim->air->UpdateAir();
		}
		BENCHMARK_END()

		printf("Air + aheat - no walls, no changes: ");
		BENCHMARK_START(benchmark_repeat_count, 1600)
		{
			sim->air->UpdateAir();
			sim->air->UpdateAirHeat(sim);
		}
		BENCHMARK_END()
	}
	free(vid_buf);
}


