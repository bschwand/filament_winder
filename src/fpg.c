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

#include <getopt.h>
#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <string.h>

#include "debug_log.h"
#include "generate.h"


int main(int argc, char *argv[]) {
  int c;
  int digit_optind = 0;

  int test_num = 0;

  run_mode_t run_mode = INVALID;

  wind_def_t params = {0, 0, 0, 0, 0, 0, 0, 5};

  while (1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
      // one of 'i', 's', 'g' (Information, Sim Gnuplot, G-code)
      {"mode",      required_argument, 0, 'm' },
      // width of one single tow strand
      {"tow_width", required_argument, 0, 'w' }, // in mm
      // mandrel diameter
      {"diameter",  required_argument, 0, 'd' }, // in mm
      // tube length
      {"length",    required_argument, 0, 'L' }, // in mm
      // winding angle
      {"angle",     required_argument, 0, 'a' }, // in degrees
      // how many tow in one circumference
      {"tow_count", required_argument, 0, 'N' }, // integer
      // pattern number
      {"pattern",   required_argument, 0, 'p' }, // integer
      // skip as defined
      {"skip",      required_argument, 0, 's' }, // integer
      // integer how many extra tow_count values to output when running in information mode
      {"range",     required_argument, 0, 'r' },
      // number of line segments in one dwell arc
      {"segment_count",     required_argument, 0, 'c' },
      // for test and debugging, run the test number requested
      {"test",      required_argument, 0, 't' }, // integer
      {"verbose",   no_argument, 0, 'v' },
      {0,           0,                 0,  0  }
    };

    c = getopt_long(argc, argv, "m:w:d:L:a:N:p:s:vr:t:c:",
                    long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 0:
      printf("option %s", long_options[option_index].name);
      if (optarg)
        printf(" with arg %s", optarg);
      printf("\n");
      break;

    case 'm':
      DEBUG_LOG("option m with value '%s'\n", optarg);
      if(!strcmp("i", optarg)){
        run_mode = INFORMATION;
      }
      if(!strcmp("s", optarg)){
        run_mode = SIM_GNUPLOT;
      }
      if(!strcmp("g", optarg)){
        run_mode = GCODE;
      }
      if(run_mode == INVALID){
        printf("Invalid runmode\n");
      }
      DEBUG_LOG("runmode: %d\n", run_mode);
      break;

    case 'w':
      if(!(params.tow_width = atof(optarg))){
        printf("tow width is invalid!\n");
        exit(1);
      }
      DEBUG_LOG("option w with value '%s'\n", optarg);
      DEBUG_LOG("tow_width : %f\n", params.tow_width);
      break;

    case 'd':
      if(!(params.diameter = atof(optarg))){
        printf("diameter is invalid!\n");
        exit(1);
      }
      DEBUG_LOG("option d with value '%s'\n", optarg);
      DEBUG_LOG("diameter : %f\n", params.diameter);
      break;

    case 'L':
      if(!(params.length = atof(optarg))){
        printf("length is invalid!\n");
        exit(1);
      }
      DEBUG_LOG("option L with value '%s'\n", optarg);
      DEBUG_LOG("length : %f\n", params.length);
      break;

    case 'a':
      if(!(params.angle = atof(optarg))){
        printf("angle is invalid!\n");
        exit(1);
      }
      DEBUG_LOG("option a with value '%s'\n", optarg);
      DEBUG_LOG("angle : %f\n", params.angle);
      break;

    case 'N':
      if(!(params.tow_count = atof(optarg))){
        printf("tow_count is invalid!\n");
        exit(1);
      }
      DEBUG_LOG("option N with value '%s'\n", optarg);
      DEBUG_LOG("tow_count : %f\n", params.tow_count);
      break;

    case 'p':
      if(!(params.pattern = atof(optarg))){
        printf("pattern is invalid!\n");
        exit(1);
      }
      DEBUG_LOG("option p with value '%s'\n", optarg);
      DEBUG_LOG("pattern : %f\n", params.pattern);
      break;

    case 's':
      if(!(params.skip = atof(optarg))){
        printf("skip is invalid!\n");
        exit(1);
      }
      DEBUG_LOG("option s with value '%s'\n", optarg);
      DEBUG_LOG("skip : %f\n", params.skip);
      break;

    case 'r':
      if(!(params.range = atof(optarg))){
        printf("range is invalid!\n");
        exit(1);
      }
      DEBUG_LOG("option range with value '%s'\n", optarg);
      DEBUG_LOG("range : %f\n", params.range);
      break;

    case 'c':
      if(!(params.segment_count = atof(optarg))){
        printf("segment_count is invalid!\n");
        exit(1);
      }
      DEBUG_LOG("option segment_count with value '%s'\n", optarg);
      DEBUG_LOG("segment_count : %d\n", params.segment_count);
      break;

    case 't':
      if(!(test_num = atoi(optarg))){
        printf("test number is invalid!\n");
        exit(1);
      }
      DEBUG_LOG("test number with value '%s'\n", optarg);
      DEBUG_LOG("test : %d\n", test_num);
      run_test(test_num);
      exit(1);
      break;

    case 'v':
      debug_log = 1;
      break;

    case '?':
      printf("Invalid option %c\n", optopt);
      exit(1);
      break;

    default:
      printf("?? getopt returned character code 0%o ??\n", c);
    }
  }

  if (optind < argc) {
    printf("non-option ARGV-elements: ");
    while (optind < argc)
      printf("%s ", argv[optind++]);
    printf("\n");
  }


  // check options are consistent and run
  switch(run_mode){
  case INFORMATION:
    if((params.tow_width > 0) &&
       (params.diameter > 0) &&
       (params.length > 0) &&
       (params.angle > 25) && (params.angle < 75)){
      generate_information(&params);
      break;
    }else{
      printf("Need to specify -w -d -L -a parameters for INFORMATION mode\n -a > 25 and < 75\n");
      exit(1);
    }

  case SIM_GNUPLOT:
  case GCODE:
    if((params.tow_count > 0) &&
       (params.diameter > 0) &&
       (params.length > 0) &&
       (params.angle > 25) && (params.angle < 75) &&
       ((params.pattern > 1) &&
        (params.skip <= (params.pattern - 2 )) ) &&
       (params.segment_count >= 2)
       ){
      generate_path(run_mode, &params);
      break;
    }else{
      printf("Need to specify all parameters except tow_width for this mode\n");
      printf("winding angle must be < 75 and > 25\n");
      printf("segment count must be >= 6\n");
      exit(1);
    }
  }

  exit(EXIT_SUCCESS);
}
