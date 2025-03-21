#pragma once

#include "ScopedAllocator.h"
#include "ObjectPool.h"

namespace jl {

/**
 * Object pool wrappers that provide ScopedAllocator interfaces
 */

class PreallocatedObjectPoolAllocator : public ScopedAllocator
{
public:
    // Initialize object pool with preallocated buffer.
    // If you set the PreallocatedObjectPool::eFlag_ManageBuffer flag, make sure
    // that the buffer was created using array-new.
    void Init( void* pBuffer, unsigned nCapacity, unsigned nStride, unsigned nFlags )
    {
        m_oPool.Init( pBuffer, nCapacity, nStride, nFlags );
    }

    void Deinit()
    {
        m_oPool.Deinit();
    }

    unsigned CountAllocations() const
    {
        return m_oPool.CountAllocations();
    }

    // Virtual overrides
    void* Alloc( size_t nBytes ) override
    {
        JL_ASSERT( nBytes <= m_oPool.GetStride() );
        return m_oPool.Alloc();
    }

    void Free( void* pObject ) override
    {
        m_oPool.Free( pObject );
    }

private:
    PreallocatedObjectPool m_oPool;
};

template<unsigned _Stride, unsigned _Capacity>
class StaticObjectPoolAllocator : public ScopedAllocator
{
public:
    typedef StaticObjectPool<_Stride, _Capacity> TObjectPool;

    unsigned CountAllocations() const
    {
        return m_oPool.CountAllocations();
    }

    // Virtual overrides
    void* Alloc( size_t nBytes ) final
    {
        JL_ASSERT( nBytes <= m_oPool.GetStride() );
        return m_oPool.Alloc();
    }

    void Free( void* pObject ) final
    {
        m_oPool.Free( pObject );
    }

private:
    TObjectPool m_oPool;
};

} // namespace jl
