/*
  this file is part of the source code for fpg, the filament winding
  path generator. Produces G-code sequence to drive a filament winder
  to make composite tubes.

  Copyright (C) 2025  Bruno Schwander

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published
  by the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _GENERATE_H_
#define _GENERATE_H_


typedef enum {
  INVALID = 0,
  INFORMATION,
  SIM_GNUPLOT,
  GCODE
} run_mode_t ;

typedef struct {
  float tow_width;
  float diameter;
  float length;
  float angle;
  int tow_count;
  int pattern;
  int skip;
  int range;
  int segment_count;
} wind_def_t;

void generate_information(wind_def_t* params);
int generate_path(run_mode_t run_mode, wind_def_t* params);

void run_test(int test_num);

#endif
