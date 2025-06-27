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
