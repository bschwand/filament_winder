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

#include "debug_log.h"
#include "generate.h"
#include <math.h>
#include <stdbool.h>

int primes[]= {  2,  3,  5,  7, 11, 13, 17, 19, 23, 29,
                31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
                73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
                127, 131, 137, 139, 149, 151, 157, 163, 167,
                173, 179, 181, 191, 193, 197, 199 };

double to_degrees(double radians) {
  return radians * (180.0 / M_PI);
}

double to_radians(double degrees) {
  return degrees * (M_PI / 180);
}

float complement_to_90(float angle){
  if(angle < 0)
    return -(90.0 + angle);
  else
    return (90.0 - angle);
}

void print_line(run_mode_t run_mode,
                float start_x, float start_angle,
                float end_x, float end_angle, // angle here is actually Y coordinate in degrees
                float head_angle){ // head_angle here is actually Z coord in degrees, zero is up

  float cut_height = 360;
  DEBUG_LOG("%f %f %f %f\n", start_x, start_angle, end_x, end_angle);

  double start_f, start_i;
  start_f = modf(start_angle, &start_i);
  long start_mod = (long)start_i;
  start_mod = start_mod % 360;
  start_i = (float)start_mod;

  double end_f, end_i;
  end_f = modf(end_angle, &end_i);
  long end_mod = (long)end_i;
  end_mod = end_mod % 360;
  end_i = (float)end_mod;

  if(run_mode == SIM_GNUPLOT){

    if((end_angle - start_angle + start_i + start_f) >= 360.0){
      // already write first piece that is inside
      float slope = (end_angle - start_angle)/(end_x - start_x);
      float end_x_step = (360.0 - (start_i + start_f)) / slope;

      printf("%f %f\n%f %f\n\n", start_x, start_i + start_f, start_x + end_x_step, 360.0);

      // draw the other chunks
      start_x += end_x_step;
      start_angle += (360.0 - (start_i + start_f));

      print_line(run_mode, start_x, start_angle, end_x, end_angle, head_angle);

    }else{
      // simple case, start and end are inside the 360 band, no wraparound
      printf("%f %f\n%f %f\n\n", start_x, start_i + start_f, end_x, end_i + end_f);
    }
  }else{
    // turn the head first, then do the line. Otherwise it would interpolate head rotation
    // but we want the head pointed in the correct angle for the whole move, so do it first
    printf("G1 Z%f\nG1 X%f Y%f\n", head_angle, end_x, end_angle);
  }

}

void print_dwell(run_mode_t run_mode,
                 float start_x,
                 float start_angle, float end_angle, float winding_angle,
                 int segments, float diameter,
                 bool going_left){ // if true the dwell is a half circle on the left

  float theta,   // half angle of the dwell arc on the plane of the developed cylinder
    Cx, Ca,  // coordinates of dwell center
    dr,      // dwell radius
    curr_theta, // theta to iterate over
    Segx, Segy, // dwell segment end coordinates
    prevSegx, prevSegy; // previous segment end coordinates, i.e. start coords
  float head_angle;

  theta = 90.0 - winding_angle;
  Ca = (end_angle + start_angle)/2;

  // we have to use the arc length from start_angle to Ca, not the angle difference
  // thus the (M_PI * diameter / 360.0) factor to convert from angle to arc length
  float CaSa_arc =  (M_PI * diameter / 360.0) * (Ca - start_angle);
  float tan_theta = tan(to_radians(theta));

  if(going_left)
    Cx = (CaSa_arc + start_x * tan_theta) / tan_theta;
  else
    Cx = (-CaSa_arc + start_x * tan_theta) / tan_theta;

  DEBUG_LOG("Cx: %f Ca: %f\n", Cx, Ca);

  dr = sqrt(pow(Cx - start_x, 2) + pow(CaSa_arc, 2));
  DEBUG_LOG("dr: %f\n", dr);

  DEBUG_LOG("start_x: %f start_angle: %f\n", start_x, start_angle);
  prevSegx = start_x;
  prevSegy = start_angle;

  for(int i = 1; i < segments; i++){
    curr_theta = -theta + i * 2 * theta / segments;
    DEBUG_LOG("curr_theta: %f\n", curr_theta);
    if(going_left)
      Segx = Cx - (dr * cos(to_radians(curr_theta)));
    else
      Segx = Cx + (dr * cos(to_radians(curr_theta)));

    // Ca is an angle, Segy too. We need to convert the arc length into an angle
    // thus multiply by (360 / (M_PI * diameter))
    Segy = Ca + dr * sin(to_radians(curr_theta)) * (360 / (M_PI * diameter));

    DEBUG_LOG("Segx: %f Segy:%f\n", Segx, Segy);

    print_line(run_mode, prevSegx, prevSegy, Segx, Segy,
               (going_left) ? curr_theta : -curr_theta);
    prevSegx = Segx;
    prevSegy = Segy;
  }
  // last segment to dwell end_angle
  DEBUG_LOG("start_x: %f end_angle: %f\n", start_x, end_angle);
  // complement to 90 because winding angle is given in reference to horizontal,
  // but our z axis is setup with zero point to top, + CW and - CCW
  print_line(run_mode, prevSegx, prevSegy, start_x, end_angle,
             (going_left) ? complement_to_90(winding_angle)
             : -complement_to_90(winding_angle));
}

// prints possible patterns for tow_count, i.e. divisors (for now just prime divisors)
void print_patterns(int tow_count){
  for(int i=0; i<sizeof(primes)/sizeof(int); i++){
    if(tow_count % primes[i] == 0)
      printf("%d ", primes[i]);
  }
  printf("\n");
}

void generate_information(wind_def_t* params){
  float circumference = M_PI * params->diameter;
  float projected_tow_width = params->tow_width / cos(to_radians(params->angle));
  float N = ceil(circumference / projected_tow_width);

  DEBUG_LOG("N: %f\n", N);

  int tows;
  printf("Tow count\toverlap\t patterns\n");
  for(tows = N; tows < (N + params->range); tows++){
    printf("\t%4d\t%5.2f %%\t ", tows,
           (((float)(tows * projected_tow_width)) / circumference - 1.0) * 100.0
           );

    print_patterns(tows);
  }

}

int generate_path(run_mode_t run_mode, wind_def_t* params){

  // linear distance covered over L (i.e. length perpendicular to mandrel if one to wound
  // around the mandrel to cover same angle around, with a winding angle over the length L)
  float D,
    // angle covered while winding that length of tow D
    gamma,
    // total angle covered over 2 * L
    beta,
    // final angle on the circumference after winding 2 * L
    theta;
  double theta_i, theta_f;
  long theta_mod;

  float c, // pattern angle with skip
    dw;    // dwell angle

  double dw_i, dw_f;
  long dw_mod;

  D = params->length * tan(to_radians(params->angle));
  gamma = (D * 360.0) / (M_PI * params->diameter);
  beta = 2 * gamma;
  theta_f = modf(beta, &theta_i);
  theta_mod = (long)theta_i % 360;
  theta = (float)theta_mod + (float)theta_f;

  c = (360.0 / params->pattern) * ( params->skip + 1 );

  // we can choose an integer k so dw is not too small
  // dw = (k*360 + ((c - theta) mod 360)) /2;

  dw = c - theta;
  dw_f = modf(dw, &dw_i);
  dw_mod = (long)dw_i % 360;
  dw =  ((float)dw_mod + dw_f) / 2.0;

  // choose a k so dw is not negative or too small, i.e. add k*360/2
  if(dw < 0)
    dw = dw + 180.0;

  DEBUG_LOG("dw: %f\n", dw);

  int current_circuit, current_cycle;
  int cycle_count = params->tow_count / params->pattern;

  float current_angle = 0;
  float dw_end_angle;
  float shift = 360.0 / params->tow_count;

  for(current_cycle = 0; current_cycle < cycle_count ; current_cycle++){
    for(current_circuit = 0; current_circuit < params->pattern; current_circuit++){
      print_line(run_mode, 0, current_angle, params->length, current_angle + gamma,
                 complement_to_90(params->angle));
      current_angle += gamma;
      print_dwell(run_mode,
                  params->length, current_angle, current_angle + dw,
                  params->angle, params->segment_count, params->diameter, false);
      current_angle += dw;
      print_line(run_mode, params->length, current_angle, 0, current_angle + gamma,
                 -complement_to_90(params->angle));
      current_angle += gamma;

      dw_end_angle = current_angle + dw;
      // on last dwell, add the shift
      if((current_circuit + 1) == params->pattern)
        dw_end_angle += shift;

      print_dwell(run_mode,
                  0, current_angle, dw_end_angle,
                  params->angle, params->segment_count, params->diameter, true);

      current_angle = dw_end_angle;
    }
  }

}


void run_test(int test_num){
  switch(test_num){
  case 1:
    print_line(SIM_GNUPLOT, 100, 0, 0, 45, 45);
    break;
  case 2:
    print_line(SIM_GNUPLOT, 0, 0, 100, 45, 45);
    break;
  case 3:
    print_line(SIM_GNUPLOT, 0, 0, 100, 360, 45);
    break;
  case 4:
    print_line(SIM_GNUPLOT, 0, 0, 100, 365, 45);
    break;
  case 5:
    print_line(SIM_GNUPLOT, 0, 180, 100, 375, 45);
    break;
  case 6:
    print_line(SIM_GNUPLOT, 0, 460, 100, 620, 45);
    break;
  case 7:
    print_line(SIM_GNUPLOT, 0, 180, 100, 685, 45);
    break;
  case 8:
    print_line(SIM_GNUPLOT, 100, 180, 0, 685, 45);
    break;
  case 9:
    print_line(SIM_GNUPLOT, 40, 180, 100, 685, 45);
    break;
  case 10:
    print_line(SIM_GNUPLOT, 0, 0, 1000, 965, 45);
    break;
  case 11:
    print_line(SIM_GNUPLOT, 600, 180, 100, 1865, 45);
    break;
  case 12:
    print_dwell( SIM_GNUPLOT, 0, 100, 250, 45, 12, 100, true);
    break;
  case 13:
    print_dwell( SIM_GNUPLOT, 100, 100, 250, 45, 12, 100, false);
    break;
  case 14:
    print_dwell( SIM_GNUPLOT, 0, 250, 500, 45, 12, 100, true);
    break;
  case 15:
    print_dwell( SIM_GNUPLOT, 100, 250, 500, 45, 12, 100, false);
    break;
  case 16:
    print_dwell( SIM_GNUPLOT, 0, 250, 1500, 45, 24, 100, true);
    break;
  case 17:
    print_dwell( SIM_GNUPLOT, 100, 250, 1500, 45, 24, 100, false);
    break;
  case 18:
    print_dwell( SIM_GNUPLOT, 0, 100, 250, 45, 12, 150, true);
    break;
  }
}
