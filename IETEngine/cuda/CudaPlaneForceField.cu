/******************************************************************************
*       SOFA, Simulation Open-Framework Architecture, version 1.0 beta 4      *
*                (c) 2006-2009 MGH, INRIA, USTL, UJF, CNRS                    *
*                                                                             *
* This library is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as published by    *
* the Free Software Foundation; either version 2.1 of the License, or (at     *
* your option) any later version.                                             *
*                                                                             *
* This library is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License *
* for more details.                                                           *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this library; if not, write to the Free Software Foundation,     *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.          *
*******************************************************************************
*                               SOFA :: Modules                               *
*                                                                             *
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/
#include <cuda/CudaCommon.h>
#include <cuda/CudaMath.h>
#include "cuda.h"

template<class real>
class GPUPlane
{
public:
    //CudaVec3<real> normal;
    real normal_x, normal_y, normal_z;
    real d;
    real stiffness;
    real damping;
};

typedef GPUPlane<float> GPUPlane3f;
typedef GPUPlane<double> GPUPlane3d;

extern "C"
{
void CudaPlaneForceField3f_addForce(unsigned int size, GPUPlane3f* plane, float* penetration, void* f, const void* x, const void* v);
void CudaPlaneForceField3f_addDForce(unsigned int size, GPUPlane3f* plane, const float* penetration, void* f, const void* dx); //, const void* dfdx);

#ifdef SOFA_GPU_CUDA_DOUBLE

void CudaPlaneForceField3d_addForce(unsigned int size, GPUPlane3d* plane, double* penetration, void* f, const void* x, const void* v);
void CudaPlaneForceField3d_addDForce(unsigned int size, GPUPlane3d* plane, const double* penetration, void* f, const void* dx); //, const void* dfdx);

#endif // SOFA_GPU_CUDA_DOUBLE
}

//////////////////////
// GPU-side methods //
//////////////////////

template<class real>
__global__ void CudaPlaneForceField3t_addForce_kernel(int size, GPUPlane<real> plane, real* penetration, real* f, const real* x, const real* v)
{
	int index0 = fastmul(blockIdx.x,BSIZE);
	int index0_3 = fastmul(blockIdx.x,BSIZE*3); //index0*3;

	penetration += index0;
	f += index0_3;
	x += index0_3;
	v += index0_3;
	
	int index = threadIdx.x;
	int index_3 = fastmul(index,3); //index*3;

	//! Dynamically allocated shared memory to reorder global memory access
	__shared__  real temp[BSIZE*3];

	temp[index        ] = x[index        ];
	temp[index+  BSIZE] = x[index+  BSIZE];
	temp[index+2*BSIZE] = x[index+2*BSIZE];

	__syncthreads();
	
	CudaVec3<real> xi = CudaVec3<real>::make(temp[index_3  ], temp[index_3+1], temp[index_3+2]);
	real d = dot(xi,CudaVec3<real>::make(plane.normal_x,plane.normal_y,plane.normal_z))-plane.d;
	
	penetration[index] = d;
	
	__syncthreads();
	
	temp[index        ] = v[index        ];
	temp[index+  BSIZE] = v[index+  BSIZE];
	temp[index+2*BSIZE] = v[index+2*BSIZE];
	
	__syncthreads();
	
	CudaVec3<real> vi = CudaVec3<real>::make(temp[index_3  ], temp[index_3+1], temp[index_3+2]);
	CudaVec3<real> force = CudaVec3<real>::make(0,0,0);
	
	if (d<0)
	{
		real forceIntensity = -plane.stiffness*d;
		real dampingIntensity = -plane.damping*d;
		force = CudaVec3<real>::make(plane.normal_x,plane.normal_y,plane.normal_z)*forceIntensity - vi*dampingIntensity;
	}	
	
	__syncthreads();
	
	temp[index        ] = f[index        ];
	temp[index+  BSIZE] = f[index+  BSIZE];
	temp[index+2*BSIZE] = f[index+2*BSIZE];
	
	__syncthreads();
	
	temp[index_3+0] += force.x;
	temp[index_3+1] += force.y;
	temp[index_3+2] += force.z;
	
	__syncthreads();
	
	f[index        ] = temp[index        ];
	f[index+  BSIZE] = temp[index+  BSIZE];
	f[index+2*BSIZE] = temp[index+2*BSIZE];
}

template<class real>
__global__ void CudaPlaneForceField3t_addDForce_kernel(int size, GPUPlane<real> plane, const real* penetration, real* df, const real* dx)
{
	int index0 = fastmul(blockIdx.x,BSIZE);
	int index0_3 = fastmul(blockIdx.x,BSIZE*3); //index0*3;

	penetration += index0;
	df += index0_3;
	dx += index0_3;
	
	int index = threadIdx.x;
	int index_3 = fastmul(index,3); //index*3;

	//! Dynamically allocated shared memory to reorder global memory access
	__shared__  real temp[BSIZE*3];

	temp[index        ] = dx[index        ];
	temp[index+  BSIZE] = dx[index+  BSIZE];
	temp[index+2*BSIZE] = dx[index+2*BSIZE];

	__syncthreads();
	
	CudaVec3<real> dxi = CudaVec3<real>::make(temp[index_3  ], temp[index_3+1], temp[index_3+2]);
	real d = penetration[index];
	
	__syncthreads();
	
	temp[index        ] = df[index        ];
	temp[index+  BSIZE] = df[index+  BSIZE];
	temp[index+2*BSIZE] = df[index+2*BSIZE];
	
	CudaVec3<real> dforce = CudaVec3<real>::make(0,0,0);

	if (d<0)
	{
		dforce = CudaVec3<real>::make(plane.normal_x,plane.normal_y,plane.normal_z) * (-plane.stiffness * dot(dxi, CudaVec3<real>::make(plane.normal_x,plane.normal_y,plane.normal_z)));
	}	
	
	__syncthreads();
	
	temp[index_3+0] += dforce.x;
	temp[index_3+1] += dforce.y;
	temp[index_3+2] += dforce.z;
	
	__syncthreads();
	
	df[index        ] = temp[index        ];
	df[index+  BSIZE] = temp[index+  BSIZE];
	df[index+2*BSIZE] = temp[index+2*BSIZE];
}

//////////////////////
// CPU-side methods //
//////////////////////

void CudaPlaneForceField3f_addForce(unsigned int size, GPUPlane3f* plane, float* penetration, void* f, const void* x, const void* v)
{
	dim3 threads(BSIZE,1);
	dim3 grid((size+BSIZE-1)/BSIZE,1);
	CudaPlaneForceField3t_addForce_kernel<float><<< grid, threads >>>(size, *plane, penetration, (float*)f, (const float*)x, (const float*)v);
}

void CudaPlaneForceField3f_addDForce(unsigned int size, GPUPlane3f* plane, const float* penetration, void* df, const void* dx) //, const void* dfdx)
{
	dim3 threads(BSIZE,1);
	dim3 grid((size+BSIZE-1)/BSIZE,1);
	CudaPlaneForceField3t_addDForce_kernel<float><<< grid, threads >>>(size, *plane, penetration, (float*)df, (const float*)dx);
}

#ifdef SOFA_GPU_CUDA_DOUBLE

void CudaPlaneForceField3d_addForce(unsigned int size, GPUPlane3d* plane, double* penetration, void* f, const void* x, const void* v)
{
	dim3 threads(BSIZE,1);
	dim3 grid((size+BSIZE-1)/BSIZE,1);
	CudaPlaneForceField3t_addForce_kernel<double><<< grid, threads >>>(size, *plane, penetration, (double*)f, (const double*)x, (const double*)v);
}

void CudaPlaneForceField3d_addDForce(unsigned int size, GPUPlane3d* plane, const double* penetration, void* df, const void* dx) //, const void* dfdx)
{
	dim3 threads(BSIZE,1);
	dim3 grid((size+BSIZE-1)/BSIZE,1);
	CudaPlaneForceField3t_addDForce_kernel<double><<< grid, threads >>>(size, *plane, penetration, (double*)df, (const double*)dx);
}

#endif // SOFA_GPU_CUDA_DOUBLE
