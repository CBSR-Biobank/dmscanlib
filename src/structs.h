#ifndef STRUCTS_H_
#define STRUCTS_H_

/*
 * structs.h
 *
 *  Created on: Jun 30, 2009
 *      Author: nelson
 */


typedef struct sScPixelLoc {
	int x;
	int y;
} ScPixelLoc;

typedef struct sScPixelFrame {
	int x0; // top
	int y0; // left
	int x1; // bottom
	int y1; // right
} ScPixelFrame;

/**
 * Specifies the region to scan.
 */
typedef struct sScFrame {
	int frameId;
	double x0; // top
	double y0; // left
	double x1; // bottom
	double y1; // right
} ScFrame;

#endif /* STRUCTS_H_ */
