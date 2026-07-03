#pragma once

#include <stdbool.h>

#include <tesselator.h>

#include <context.h>

typedef TESStesselator recomp_triangulate_t;

void triInit(SWFAppContext* app_context);

recomp_triangulate_t* triAlloc(SWFAppContext* app_context);

void triTessellate(recomp_triangulate_t* tess, f32* data, u32 num_vertices);

const f32* triGetVertices(recomp_triangulate_t* tess);
const int* triGetElements(recomp_triangulate_t* tess);
u32 triGetVertexCount(recomp_triangulate_t* tess);
u32 triGetElementCount(recomp_triangulate_t* tess);

void triDestroy(SWFAppContext* app_context, recomp_triangulate_t* tess);