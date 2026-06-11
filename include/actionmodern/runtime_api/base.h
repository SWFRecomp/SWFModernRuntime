#pragma once

typedef enum
{
	NATIVE_ARRAY = 0,
	NATIVE_BITMAP_DATA,
	NATIVE_COLOR_TRANSFORM,
	NATIVE_FUNCTION,
	NATIVE_MOVIECLIP,
	NATIVE_NUMBER,
	NATIVE_SOUND,
	NATIVE_STRING,
} NativeType;

typedef struct
{
	NativeType type;
} BaseExtData;