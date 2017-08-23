#version 450

layout (local_size_x = WG_WIDTH) in;

layout (std430, binding = 0) buffer bTEST	{ float	_G_test[]; };
layout (std430, binding = 1) buffer bDBG	{ uint	_G_dbg[]; };
layout (std430, binding = 2) buffer bDBG2	{ uint	_G_dbg2[]; };



shared float _lds[WG_WIDTH];


void main ()
{

//#if (TASK == 0)
//for (int i=0; i<5; i++)
//{
//#endif

	uint lID = gl_LocalInvocationID.x;
	uint index = (((gl_WorkGroupID.x) * WG_WIDTH + lID) & 0xFFF) + TASK * 0x1000; // note: buffer too small for high settings in 'wavefronts per dispatch' but we don't care
	
	_lds[lID] = _G_test[index];
	memoryBarrierShared(); barrier();

	if (lID<32) _lds[(((lID >> 0) << 1) | (lID &  0) |  1) ]	+= _lds[(((lID >> 0) << 1) |  0) ];	memoryBarrierShared(); barrier();
	if (lID<32) _lds[(((lID >> 1) << 2) | (lID &  1) |  2) ]	+= _lds[(((lID >> 1) << 2) |  1) ];	memoryBarrierShared(); barrier();
	if (lID<32) _lds[(((lID >> 2) << 3) | (lID &  3) |  4) ]	+= _lds[(((lID >> 2) << 3) |  3) ];	memoryBarrierShared(); barrier();
	if (lID<32) _lds[(((lID >> 3) << 4) | (lID &  7) |  8) ]	+= _lds[(((lID >> 3) << 4) |  7) ];	memoryBarrierShared(); barrier();
	if (lID<32) _lds[(((lID >> 4) << 5) | (lID & 15) | 16) ]	+= _lds[(((lID >> 4) << 5) | 15) ];	memoryBarrierShared(); barrier();
	if (lID<32) _lds[(((lID >> 5) << 6) | (lID & 31) | 32) ]	+= _lds[(((lID >> 5) << 6) | 31) ];	memoryBarrierShared(); barrier();

	_G_test[index] = _lds[lID]; // write prefix sum results (they are not used anywhere)
	
	if (lID==0) atomicAdd(_G_dbg[TASK], 1); // count wavefronts on common memory for all tasks. This gives wrong results when using multiple queues and interleving task 3!

//#if (TASK == 0)
//memoryBarrier(); barrier();
//}
//#endif

}



/*
shared uint _lds[WG_WIDTH];


void main ()
{
	uint lID = gl_LocalInvocationID.x;
	uint index = lID;
	
	_lds[lID] = 1;
	memoryBarrierShared(); barrier();

#if 1 // wrong result: (1...128), (1...128) instead (1...256)
	if (lID<(WG_WIDTH>>1)) _lds[(((lID >> 0) << 1) | (lID &   0) |   1) ]	+= _lds[(((lID >> 0) << 1) |   0) ];	memoryBarrierShared(); barrier();
	if (lID<(WG_WIDTH>>1)) _lds[(((lID >> 1) << 2) | (lID &   1) |   2) ]	+= _lds[(((lID >> 1) << 2) |   1) ];	memoryBarrierShared(); barrier();
	if (lID<(WG_WIDTH>>1)) _lds[(((lID >> 2) << 3) | (lID &   3) |   4) ]	+= _lds[(((lID >> 2) << 3) |   3) ];	memoryBarrierShared(); barrier();
	if (lID<(WG_WIDTH>>1)) _lds[(((lID >> 3) << 4) | (lID &   7) |   8) ]	+= _lds[(((lID >> 3) << 4) |   7) ];	memoryBarrierShared(); barrier();
	if (lID<(WG_WIDTH>>1)) _lds[(((lID >> 4) << 5) | (lID &  15) |  16) ]	+= _lds[(((lID >> 4) << 5) |  15) ];	memoryBarrierShared(); barrier();
	if (lID<(WG_WIDTH>>1)) _lds[(((lID >> 5) << 6) | (lID &  31) |  32) ]	+= _lds[(((lID >> 5) << 6) |  31) ];	memoryBarrierShared(); barrier();
	if (lID<(WG_WIDTH>>1)) _lds[(((lID >> 6) << 7) | (lID &  63) |  64) ]	+= _lds[(((lID >> 6) << 7) |  63) ];	memoryBarrierShared(); barrier();
	if (lID<(WG_WIDTH>>1)) _lds[(((lID >> 7) << 8) | (lID & 127) | 128) ]	+= _lds[(((lID >> 7) << 8) | 127) ];	memoryBarrierShared(); barrier();
#else // wrong result: (1...64), (1...64), (1...32), (1...32), (1...32), (1...32)
	if (lID<(WG_WIDTH>>1)) _lds[(((lID >> 0) << 1) | (lID &   0) |   1) ]	+= _lds[(((lID >> 0) << 1) |   0) ];	memoryBarrierShared(); barrier();
	if (lID<(WG_WIDTH>>1)) _lds[(((lID >> 1) << 2) | (lID &   1) |   2) ]	+= _lds[(((lID >> 1) << 2) |   1) ];	memoryBarrierShared(); barrier();
	if (lID<(WG_WIDTH>>1)) _lds[(((lID >> 2) << 3) | (lID &   3) |   4) ]	+= _lds[(((lID >> 2) << 3) |   3) ];	memoryBarrierShared(); barrier();
	if (lID<(WG_WIDTH>>1)) _lds[(((lID >> 3) << 4) | (lID &   7) |   8) ]	+= _lds[(((lID >> 3) << 4) |   7) ];	memoryBarrierShared(); barrier();
	if (lID<(WG_WIDTH>>1)) _lds[(((lID >> 4) << 5) | (lID &  15) |  16) ]	+= _lds[(((lID >> 4) << 5) |  15) ];	memoryBarrierShared(); barrier();
	if (lID<(WG_WIDTH>>1)) _lds[(((lID >> 5) << 6) | (lID &  31) |  32) ]	+= _lds[(((lID >> 5) << 6) |  31) ];	memoryBarrierShared(); barrier();
#endif

	_G_test[index] = float(_lds[lID]);
}
*/