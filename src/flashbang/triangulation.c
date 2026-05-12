#include <heap.h>
#include <triangulation.h>
#include <utils.h>

static TESSalloc libtess2AllocCtx;

void triInit(SWFAppContext* app_context)
{
	libtess2AllocCtx.memalloc = (void* (*)(void*, unsigned int)) heap_alloc;
	libtess2AllocCtx.memrealloc = (void* (*)(void*, void*, unsigned int)) heap_realloc;
	libtess2AllocCtx.memfree = (void (*)(void*, void*)) heap_free;
	libtess2AllocCtx.userData = app_context;
	libtess2AllocCtx.meshEdgeBucketSize = 512;
	libtess2AllocCtx.meshVertexBucketSize = 512;
	libtess2AllocCtx.meshFaceBucketSize = 256;
	libtess2AllocCtx.dictNodeBucketSize = 512;
	libtess2AllocCtx.regionBucketSize = 256;
	libtess2AllocCtx.extraVertices = 0;
}

recomp_triangulate_t* triAlloc(SWFAppContext* app_context)
{
	return tessNewTess(&libtess2AllocCtx);
}

void triTessellate(recomp_triangulate_t* tess, f32* data, u32 num_vertices)
{
	tessAddContour(tess, 2, data, 2*sizeof(f32), num_vertices);
	
	if (UNLIKELY(!tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, NULL)))
	{
		EXC("tessellation failed");
	}
}

const f32* triGetVertices(recomp_triangulate_t* tess)
{
	return tessGetVertices(tess);
}

const int* triGetElements(recomp_triangulate_t* tess)
{
	return tessGetElements(tess);
}

u32 triGetVertexCount(recomp_triangulate_t* tess)
{
	return tessGetVertexCount(tess);
}

u32 triGetElementCount(recomp_triangulate_t* tess)
{
	return tessGetElementCount(tess);
}

void triDestroy(SWFAppContext* app_context, recomp_triangulate_t* tess)
{
	tessDeleteTess(tess);
}