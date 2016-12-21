/**
 * Parallel VLSI Wire Routing via OpenMP
 * Amolak Nagi (amolakn)
 */

#ifndef __WIREOPT_H__
#define __WIREOPT_H__

#include <omp.h>

enum Direction { horizontal, vertical };


typedef struct
{
	Direction pathDirection;
	int bendIndex;
	bool straightWire;
} wire_path_t;

typedef struct
{
  /* Define the data structure for wire here */
	// These are the start and end coordinates for the wire
	// These should be set upon initialization, but never after
	int x1;
	int y1;
	int x2;
	int y2;

	// This path will be changed over time
	wire_path_t wirePath;

} wire_t;

typedef int cost_t;

const char *get_option_string(const char *option_name, const char *default_value);
int get_option_int(const char *option_name, int default_value);
float get_option_float(const char *option_name, float default_value);

#endif
