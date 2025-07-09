#pragma once

#include <common.h>

typedef struct FlashbangContext FlashbangContext;

FlashbangContext* flashbang_new();
void flashbang_init(FlashbangContext* context);
int flashbang_poll();
void flashbang_set_window_background(FlashbangContext* context, u8 r, u8 g, u8 b);
void flashbang_draw(FlashbangContext* context);
void flashbang_free(FlashbangContext* context);