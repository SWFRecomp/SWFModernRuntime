#include "constants.h"

const float stage_to_ndc[16] =
{
	1.0f/(FRAME_WIDTH_TWIPS/2.0f),
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	-1.0f/(FRAME_HEIGHT_TWIPS/2.0f),
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	1.0f,
	0.0f,
	-1.0f,
	1.0f,
	0.0f,
	1.0f,
};