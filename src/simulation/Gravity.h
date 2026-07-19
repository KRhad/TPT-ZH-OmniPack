/**
 * Powder Toy - Newtonian gravity (header)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef GRAVITY_H
#define GRAVITY_H

#ifdef GRAVFFT
#include <fftw3.h>
#endif
#include <thread>
#include <mutex>
#include <condition_variable>
#include "defines.h"

class Gravity
{
private:
	bool enabled = false;

	// Maps to be processed by the gravity thread
	float *th_ogravmap = nullptr;
	float *th_gravmap = nullptr;
	float *th_gravx = nullptr;
	float *th_gravy = nullptr;
	float *th_gravp = nullptr;

	int th_gravchanged = 0;

	std::thread gravthread;
	std::mutex gravmutex;
	std::condition_variable gravcv;
	int grav_ready = 0;
	int gravthread_done = 0;
	bool ignoreNextResult = false;

#ifdef GRAVFFT
	bool grav_fft_status = false;
	float *th_ptgravx = nullptr;
	float *th_ptgravy = nullptr;
	float *th_gravmapbig = nullptr;
	float *th_gravxbig = nullptr;
	float *th_gravybig = nullptr;

	fftwf_complex *th_ptgravxt, *th_ptgravyt, *th_gravmapbigt, *th_gravxbigt, *th_gravybigt;
	fftwf_plan plan_gravmap, plan_gravx_inverse, plan_gravy_inverse;
#endif

	struct mask_el {
		char *shape;
		char shapeout;
		mask_el *next;
	};
	using mask_el = struct mask_el;

	bool grav_mask_r(int x, int y, char checkmap[YRES/CELL][XRES/CELL], char shape[YRES/CELL][XRES/CELL]);
	void mask_free(mask_el *c_mask_el);

	void update_grav();
	void update_grav_async();

#ifdef GRAVFFT
	void grav_fft_init();
	void grav_fft_cleanup();
#endif

public:
	// Maps to be used by the main thread
	float *gravmap = nullptr;
	float *gravp = nullptr;
	float *gravy = nullptr;
	float *gravx = nullptr;
	unsigned *gravmask = nullptr;

	bool gravWallChanged = false;

	Gravity();
	~Gravity();

	bool IsEnabled() { return enabled; }

	void Clear();

	void UpdateAsync();
	void CalculateMask();

	void StartAsync();
	void StopAsync();
};

#endif
