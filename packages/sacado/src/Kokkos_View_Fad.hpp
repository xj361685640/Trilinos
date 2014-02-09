// @HEADER
// ***********************************************************************
//
//                           Sacado Package
//                 Copyright (2006) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact David M. Gay (dmgay@sandia.gov) or Eric T. Phipps
// (etphipp@sandia.gov).
//
// ***********************************************************************
// @HEADER

#ifndef KOKKOS_VIEW_FAD_HPP
#define KOKKOS_VIEW_FAD_HPP

// Make sure the user really wants these View specializations
#include "Sacado_ConfigDefs.h"
#if defined(HAVE_SACADO_KOKKOSCORE) && defined(HAVE_SACADO_VIEW_SPEC) && !defined(SACADO_DISABLE_FAD_VIEW_SPEC)

#include "Kokkos_View.hpp"
#include "impl/Kokkos_Error.hpp"
#if defined(__CUDACC__) && defined(__CUDA_ARCH__)
#include "Cuda/Kokkos_Cuda_abort.hpp"
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace Sacado {
namespace Fad {

/**\brief  Define a partition of a View of Sacado::MP::Vector type */
struct VectorPartition {
  unsigned begin ;
  unsigned end ;

  template< typename iType0 , typename iType1 >
  KOKKOS_INLINE_FUNCTION
  VectorPartition( const iType0 & i0 , const iType1 & i1 ) : begin(i0), end(i1) {}
};

}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace Kokkos {
namespace Impl {

struct ViewSpecializeSacadoFad {};
template <typename T,unsigned,unsigned> struct ViewFadType {};

template< class ValueType , class MemorySpace , class MemoryTraits >
struct ViewSpecialize
  < ValueType
  , ViewSpecializeSacadoFad
  , LayoutLeft
  , MemorySpace
  , MemoryTraits >
{
  typedef ViewSpecializeSacadoFad type ;
};

template< class ValueType , class MemorySpace , class MemoryTraits >
struct ViewSpecialize
  < ValueType
  , ViewSpecializeSacadoFad
  , LayoutRight
  , MemorySpace
  , MemoryTraits >
{
  typedef ViewSpecializeSacadoFad type ;
};

//----------------------------------------------------------------------------

} // namespace Impl
} // namespace Kokkos

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace Kokkos {
namespace Impl {
namespace ViewError {

struct sacado_fad_partition_constructor_requires_unmanaged_view {};

} // namespace ViewError
} // namespace Impl
} // namespace Kokkos

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace Kokkos {

/*
 * \brief Specialization of Kokkos::View for any Sacado Fad value type.
 *
 * Usage is to append another dimension to the view of size fad_size+1 where
 * fad_size is the size of the desired derivative array.
*/
template< class DataType ,
          class Arg1Type ,
          class Arg2Type ,
          class Arg3Type >
class View< DataType , Arg1Type , Arg2Type , Arg3Type ,
            Impl::ViewSpecializeSacadoFad >
  : public ViewTraits< DataType , Arg1Type , Arg2Type, Arg3Type >
{
public:

  typedef ViewTraits< DataType , Arg1Type , Arg2Type, Arg3Type > traits ;

private:

  // Assignment of compatible views requirement:
  template< class , class , class , class , class > friend class View ;

  // Assignment of compatible subview requirement:
  template< class , class , class > friend struct Impl::ViewAssignment ;

  typedef typename traits::value_type fad_type ;

  enum { FadStaticDimension = Sacado::StaticSize<fad_type>::value };

  /* LayoutRight has stride-one storage */
  enum { FadStaticStride = ( Impl::is_same< typename traits::array_layout , LayoutRight >::value ? 1 : 0 ) };

  typedef Impl::LayoutStride< typename traits::shape_type ,
                              typename traits::array_layout > stride_type ;

  typename fad_type::value_type              * m_ptr_on_device ;
  typename traits::shape_type                  m_shape ;
  stride_type                                  m_stride ;
  typename traits::device_type::size_type      m_storage_size ;

public:

  // This needs to be public so that we know what the return type of () is
  typedef typename Impl::ViewFadType<fad_type, FadStaticDimension, FadStaticStride>::type fad_view_type ;

  typedef View< typename traits::const_data_type ,
                typename traits::array_layout ,
                typename traits::device_type ,
                typename traits::memory_traits > const_type ;

  typedef View< typename traits::non_const_data_type ,
                typename traits::array_layout ,
                typename traits::device_type ,
                typename traits::memory_traits > non_const_type ;

  typedef View< typename traits::array_type ,
                typename traits::array_layout ,
                typename traits::device_type ,
                typename traits::memory_traits > array_type ;

  typedef View< typename traits::data_type ,
                typename traits::array_layout ,
                typename traits::device_type::host_mirror_device_type ,
                void > HostMirror ;

  //------------------------------------
  // Shape

  // Rank for multidimensional array of the Fad value_type
  // is one less than the rank of the array of intrinsic scalar_type defined by the shape.
  enum { Rank = traits::rank - 1 };

  KOKKOS_FORCEINLINE_FUNCTION typename traits::shape_type shape() const { return m_shape ; }
  KOKKOS_FORCEINLINE_FUNCTION typename traits::size_type dimension_0() const { return m_shape.N0 ; }
  KOKKOS_FORCEINLINE_FUNCTION typename traits::size_type dimension_1() const { return m_shape.N1 ; }
  KOKKOS_FORCEINLINE_FUNCTION typename traits::size_type dimension_2() const { return m_shape.N2 ; }
  KOKKOS_FORCEINLINE_FUNCTION typename traits::size_type dimension_3() const { return m_shape.N3 ; }
  KOKKOS_FORCEINLINE_FUNCTION typename traits::size_type dimension_4() const { return m_shape.N4 ; }
  KOKKOS_FORCEINLINE_FUNCTION typename traits::size_type dimension_5() const { return m_shape.N5 ; }
  KOKKOS_FORCEINLINE_FUNCTION typename traits::size_type dimension_6() const { return m_shape.N6 ; }
  KOKKOS_FORCEINLINE_FUNCTION typename traits::size_type dimension_7() const { return m_shape.N7 ; }
  KOKKOS_FORCEINLINE_FUNCTION typename traits::size_type size() const
  {
    return   m_shape.N0
           * m_shape.N1
           * m_shape.N2
           * m_shape.N3
           * m_shape.N4
           * m_shape.N5
           * m_shape.N6
           * m_shape.N7
           ;
  }

  template< typename iType >
  KOKKOS_FORCEINLINE_FUNCTION
  typename traits::size_type dimension( const iType & i ) const
    { return Impl::dimension( m_shape , i ); }

  //------------------------------------

private:

  // Restrict allocation to 'FadStaticDimension'
  inline
  void verify_dimension_storage_static_size() const
  {
    if ( dimension( unsigned(Rank) ) % ( FadStaticDimension ? FadStaticDimension+1 : 1 ) ) {
      std::ostringstream msg ;
      msg << "Kokkos::View< FadType , ... > allocation dimension ("
          << dimension( unsigned(Rank) )
          << ") must be a multiple of StorageType::static_size ("
          << FadStaticDimension+1
          << ")" ;
#if defined(__CUDACC__) && defined(__CUDA_ARCH__)
      cuda_abort( msg.str().c_str() );
#else
      Impl::throw_runtime_exception( msg.str() );
#endif
    }
  }

public:

  //------------------------------------
  // Destructor, constructors, assignment operators:

  KOKKOS_INLINE_FUNCTION
  ~View() { Impl::ViewTracking< traits >::decrement( m_ptr_on_device ); }

  KOKKOS_INLINE_FUNCTION
  View() : m_ptr_on_device(0)
    {
      traits::shape_type::assign(m_shape,0,0,0,0,0,0,0,0);
      stride_type::assign(m_stride,0);
    }

  KOKKOS_INLINE_FUNCTION
  View( const View & rhs ) : m_ptr_on_device(0)
    {
      (void) Impl::ViewAssignment<
        typename traits::specialize ,
        typename traits::specialize >( *this , rhs );
    }

  KOKKOS_INLINE_FUNCTION
  View & operator = ( const View & rhs )
    {
      (void) Impl::ViewAssignment<
        typename traits::specialize ,
        typename traits::specialize >( *this , rhs );
      return *this ;
    }

  //------------------------------------
  // Construct or assign compatible view:

  template< class RT , class RL , class RD , class RM >
  KOKKOS_INLINE_FUNCTION
  View( const View<RT,RL,RD,RM,typename traits::specialize> & rhs )
    : m_ptr_on_device(0)
    {
      (void) Impl::ViewAssignment<
        typename traits::specialize ,
        typename traits::specialize >( *this , rhs );
    }

  template< class RT , class RL , class RD , class RM >
  KOKKOS_INLINE_FUNCTION
  View & operator = ( const View<RT,RL,RD,RM,typename traits::specialize> & rhs )
    {
      (void) Impl::ViewAssignment<
        typename traits::specialize ,
        typename traits::specialize >( *this , rhs );
      return *this ;
    }

  //------------------------------------
  // Allocation of a managed view with possible alignment padding.

  typedef Impl::if_c< traits::is_managed ,
                      std::string ,
                      Impl::ViewError::allocation_constructor_requires_managed >
   if_allocation_constructor ;

  explicit inline
  View( const typename if_allocation_constructor::type & label ,
        size_t n0 = 0 ,
        size_t n1 = 0 ,
        size_t n2 = 0 ,
        size_t n3 = 0 ,
        size_t n4 = 0 ,
        size_t n5 = 0 ,
        size_t n6 = 0 ,
        size_t n7 = 0 )
    : m_ptr_on_device(0)
    {
      typedef typename traits::memory_space  memory_space ;
      typedef typename traits::shape_type    shape_type ;
      typedef typename fad_type::value_type  scalar_type ;

      shape_type ::assign( m_shape, n0, n1, n2, n3, n4, n5, n6, n7 );
      stride_type::assign_with_padding( m_stride , m_shape );

      verify_dimension_storage_static_size();

      m_storage_size  = Impl::dimension( m_shape , unsigned(Rank) );
      m_ptr_on_device = (scalar_type *)
        memory_space::allocate( if_allocation_constructor::select( label ) ,
                                typeid(scalar_type) ,
                                sizeof(scalar_type) ,
                                Impl::capacity( m_shape , m_stride ) );

      (void) Impl::ViewFill< array_type >( *this , typename array_type::value_type() );
    }

  explicit inline
  View( const AllocateWithoutInitializing & ,
        const typename if_allocation_constructor::type & label ,
        size_t n0 = 0 ,
        size_t n1 = 0 ,
        size_t n2 = 0 ,
        size_t n3 = 0 ,
        size_t n4 = 0 ,
        size_t n5 = 0 ,
        size_t n6 = 0 ,
        size_t n7 = 0 )
    : m_ptr_on_device(0)
    {
      typedef typename traits::memory_space  memory_space ;
      typedef typename traits::shape_type    shape_type ;
      typedef typename fad_type::value_type  scalar_type ;

      shape_type ::assign( m_shape, n0, n1, n2, n3, n4, n5, n6, n7 );
      stride_type::assign_with_padding( m_stride , m_shape );

      verify_dimension_storage_static_size();

      m_storage_size  = Impl::dimension( m_shape , unsigned(Rank) );
      m_ptr_on_device = (scalar_type *)
        memory_space::allocate( if_allocation_constructor::select( label ) ,
                                typeid(scalar_type) ,
                                sizeof(scalar_type) ,
                                Impl::capacity( m_shape , m_stride ) );
    }

  //------------------------------------
  // Assign an unmanaged View from pointer, can be called in functors.
  // No alignment padding is performed.

  template< typename T >
  View( T * ptr ,
        size_t n0 = 0 ,
        size_t n1 = 0 ,
        size_t n2 = 0 ,
        size_t n3 = 0 ,
        size_t n4 = 0 ,
        size_t n5 = 0 ,
        size_t n6 = 0 ,
        typename Impl::enable_if<(
          ( Impl::is_same<T,typename traits::value_type>::value ||
            Impl::is_same<T,typename traits::const_value_type>::value ) &&
          ! traits::is_managed ),
        const size_t >::type n7 = 0 )
    : m_ptr_on_device(ptr)
    {
      typedef typename traits::shape_type  shape_type ;

      shape_type ::assign( m_shape, n0, n1, n2, n3, n4, n5, n6, n7 );
      stride_type::assign_no_padding( m_stride , m_shape );

      verify_dimension_storage_static_size();

      m_storage_size = Impl::dimension( m_shape , unsigned(Rank) );
    }

  //------------------------------------
  // Assign unmanaged View to portion of Device shared memory

  typedef Impl::if_c< ! traits::is_managed ,
                      typename traits::device_type ,
                      Impl::ViewError::device_shmem_constructor_requires_unmanaged >
      if_device_shmem_constructor ;

  explicit KOKKOS_INLINE_FUNCTION
  View( typename if_device_shmem_constructor::type & dev ,
        const unsigned n0 = 0 ,
        const unsigned n1 = 0 ,
        const unsigned n2 = 0 ,
        const unsigned n3 = 0 ,
        const unsigned n4 = 0 ,
        const unsigned n5 = 0 ,
        const unsigned n6 = 0 ,
        const unsigned n7 = 0 )
    : m_ptr_on_device(0)
    {
      typedef typename traits::shape_type   shape_type ;
      typedef typename fad_type::value_type scalar_type ;

      enum { align = 8 };
      enum { mask  = align - 1 };

      shape_type::assign( m_shape, n0, n1, n2, n3, n4, n5, n6, n7 );
      stride_type::assign_no_padding( m_stride , m_shape );

      typedef Impl::if_c< ! traits::is_managed ,
                          scalar_type * ,
                          Impl::ViewError::device_shmem_constructor_requires_unmanaged >
        if_device_shmem_pointer ;

      verify_dimension_storage_static_size();

      m_storage_size  = Impl::dimension( m_shape , unsigned(Rank) );

      // Select the first argument:
      m_ptr_on_device = if_device_shmem_pointer::select(
        (scalar_type *) dev.get_shmem( shmem_size(n0,n1,n2,n3,n4,n5,n6,n7) ) );
    }

  static KOKKOS_INLINE_FUNCTION
  unsigned shmem_size( const unsigned n0 = 0 ,
                       const unsigned n1 = 0 ,
                       const unsigned n2 = 0 ,
                       const unsigned n3 = 0 ,
                       const unsigned n4 = 0 ,
                       const unsigned n5 = 0 ,
                       const unsigned n6 = 0 ,
                       const unsigned n7 = 0 )
  {
    enum { align = 8 };
    enum { mask  = align - 1 };

    typedef typename traits::shape_type   shape_type ;
    typedef typename fad_type::value_type scalar_type ;

    shape_type  shape ;
    stride_type stride ;

    traits::shape_type::assign( shape, n0, n1, n2, n3, n4, n5, n6, n7 );
    stride_type::assign_no_padding( stride , shape );

    return unsigned( sizeof(scalar_type) * Impl::capacity( shape , stride ) + unsigned(mask) ) & ~unsigned(mask) ;
  }

  //------------------------------------
  // Is not allocated

  KOKKOS_FORCEINLINE_FUNCTION
  bool is_null() const { return 0 == m_ptr_on_device ; }

  //------------------------------------
  //------------------------------------
  // Scalar operator on traits::rank == 1

  typedef Impl::if_c< ( traits::rank == 1 ),
                      fad_view_type ,
                      Impl::ViewError::scalar_operator_called_from_non_scalar_view >
    if_scalar_operator ;

  KOKKOS_FORCEINLINE_FUNCTION
  typename if_scalar_operator::type
    operator()() const
    {
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return fad_view_type( m_ptr_on_device , m_storage_size-1 , 1 );
    }

  //------------------------------------
  //------------------------------------
  // Array operators, traits::rank 2:

  template< typename iType0 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type , traits, LayoutLeft, 2, iType0 >::type
    operator() ( const iType0 & i0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_2( m_shape, i0, 0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      // Strided storage
      return fad_view_type( m_ptr_on_device + i0 ,
                            m_storage_size-1 ,
                            m_stride.value );
    }

  template< typename iType0 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, LayoutRight, 2, iType0 >::type
    operator() ( const iType0 & i0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_2( m_shape, i0, 0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      // Contiguous storage with right-most index as the fad dimension
      return fad_view_type( m_ptr_on_device + ( m_stride.value * i0 ) ,
                            m_storage_size-1 , 1 );
    }

  template< typename iType0 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type , traits, typename traits::array_layout, 2, iType0 >::type
    operator[] ( const iType0 & i0 ) const
    { return operator()( i0 ); }

  template< typename iType0 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, typename traits::array_layout, 2,
                                      iType0 >::type
    at( const iType0 & i0 , int , int , int , int , int , int , int ) const
    { return operator()(i0); }

  //------------------------------------
  //------------------------------------
  // Array operators, traits::rank 3:

  template< typename iType0 , typename iType1 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, LayoutLeft, 3, iType0, iType1 >::type
    operator() ( const iType0 & i0 , const iType1 & i1 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_3( m_shape, i0, i1, 0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      // Strided storage with right-most index as the fad dimension
      return fad_view_type( m_ptr_on_device + ( i0 + m_stride.value * ( i1 )),
                            m_storage_size-1 ,
                            m_stride.value * m_shape.N1 );
    }

  template< typename iType0 , typename iType1 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, LayoutRight, 3, iType0, iType1 >::type
    operator() ( const iType0 & i0 , const iType1 & i1 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_3( m_shape, i0, i1, 0);
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      // Contiguous storage with right-most index as the fad dimension
      return fad_view_type(
        m_ptr_on_device + ( m_storage_size * ( i1 ) + m_stride.value * i0 ) ,
        m_storage_size-1 , 1 );
    }

  template< typename iType0 , typename iType1 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, typename traits::array_layout, 3,
                                      iType0, iType1 >::type
    at( const iType0 & i0 , const iType1 & i1 , int , int , int , int , int , int ) const
    { return operator()(i0,i1); }

  //------------------------------------
  //------------------------------------
  // Array operators, traits::rank 4:

  template< typename iType0 , typename iType1 , typename iType2 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, LayoutLeft, 4, iType0, iType1, iType2 >::type
    operator() ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_4( m_shape, i0, i1, i2, 0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      // Strided storage with right-most index as the fad dimension
      return fad_view_type(
        m_ptr_on_device + ( i0 + m_stride.value * (
                            i1 + m_shape.N1 * (
                            i2 ))),
        m_storage_size-1 ,
        m_stride.value * m_shape.N1 * m_shape.N2 );
    }

  template< typename iType0 , typename iType1 , typename iType2 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, LayoutRight, 4, iType0, iType1, iType2 >::type
    operator() ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_4( m_shape, i0, i1, i2, 0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      // Contiguous storage with right-most index as the fad dimension
      return fad_view_type(
        m_ptr_on_device + ( m_storage_size * ( i2 +
                            m_shape.N2 * ( i1 )) +
                            m_stride.value * i0 ) ,
        m_storage_size-1 , 1 );
    }

  template< typename iType0 , typename iType1 , typename iType2 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, typename traits::array_layout, 4,
                                      iType0, iType1, iType2 >::type
    at( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , int , int , int , int , int ) const
    { return operator()(i0,i1,i2); }

  //------------------------------------
  //------------------------------------
  // Array operators, traits::rank 5:

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, LayoutLeft, 5, iType0, iType1, iType2, iType3 >::type
    operator() ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_5( m_shape, i0, i1, i2, i3, 0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      // Strided storage with right-most index as the fad dimension
      return fad_view_type(
        m_ptr_on_device + ( i0 + m_stride.value * (
                            i1 + m_shape.N1 * (
                            i2 + m_shape.N2 * (
                            i3 )))),
        m_storage_size-1 ,
        m_stride.value * m_shape.N1 * m_shape.N2 * m_shape.N3 );
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, LayoutRight, 5, iType0, iType1, iType2, iType3 >::type
    operator() ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_5( m_shape, i0, i1, i2, i3, 0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      // Contiguous storage with right-most index as the fad dimension
      return fad_view_type(
        m_ptr_on_device + ( m_storage_size * ( i3 +
                            m_shape.N3 * ( i2 +
                            m_shape.N2 * ( i1 ))) +
                            m_stride.value * i0 ) ,
        m_storage_size-1 , 1 );
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, typename traits::array_layout, 5,
                                      iType0, iType1, iType2, iType3 >::type
    at( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 , int , int , int , int ) const
    { return operator()(i0,i1,i2,i3); }

  //------------------------------------
  //------------------------------------
  // Array operators, traits::rank 6:

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 , typename iType4 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, LayoutLeft, 6, iType0, iType1, iType2, iType3, iType4 >::type
    operator() ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 , const iType4 & i4 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_6( m_shape, i0, i1, i2, i3, i4, 0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      // Strided storage with right-most index as the fad dimension
      return fad_view_type(
        m_ptr_on_device + ( i0 + m_stride.value * (
                            i1 + m_shape.N1 * (
                            i2 + m_shape.N2 * (
                            i3 + m_shape.N3 * (
                            i4 ))))),
        m_storage_size-1 ,
        m_stride.value * m_shape.N1 * m_shape.N2 * m_shape.N3 * m_shape.N4 );
    }

  template< typename iType0 , typename iType1 , typename iType2 ,
            typename iType3 , typename iType4 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, LayoutRight, 6, iType0, iType1, iType2, iType3, iType4 >::type
    operator() ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
                 const iType4 & i4 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_6( m_shape, i0, i1, i2, i3, i4, 0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      // Contiguous storage with right-most index as the fad dimension
      return fad_view_type(
        m_ptr_on_device + ( m_storage_size * ( i4 +
                            m_shape.N4 * ( i3 +
                            m_shape.N3 * ( i2 +
                            m_shape.N2 * ( i1 )))) +
                            m_stride.value * i0 ) ,
        m_storage_size-1 , 1 );
    }

  template< typename iType0 , typename iType1 , typename iType2 ,
            typename iType3 , typename iType4 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, typename traits::array_layout, 6,
                                      iType0, iType1, iType2, iType3, iType4 >::type
    at( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
        const iType4 & i4 , int , int , int ) const
    { return operator()(i0,i1,i2,i3,i4); }

  //------------------------------------
  //------------------------------------
  // Array operators, traits::rank 7:

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 , typename iType4 , typename iType5 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, LayoutLeft, 7, iType0, iType1, iType2, iType3, iType4, iType5 >::type
    operator() ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 ,
                 const iType3 & i3 , const iType4 & i4 , const iType5 & i5 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_7( m_shape, i0, i1, i2, i3, i4, i5, 0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      // Strided storage with right-most index as the fad dimension
      return fad_view_type(
        m_ptr_on_device + ( i0 + m_stride.value * (
                            i1 + m_shape.N1 * (
                            i2 + m_shape.N2 * (
                            i3 + m_shape.N3 * (
                            i4 + m_shape.N4 * (
                            i5 )))))),
        m_storage_size-1 ,
        m_stride.value * m_shape.N1 * m_shape.N2 * m_shape.N3 * m_shape.N4 * m_shape.N5 );
    }

  template< typename iType0 , typename iType1 , typename iType2 ,
            typename iType3 , typename iType4 , typename iType5 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, LayoutRight, 7, iType0, iType1, iType2, iType3, iType4, iType5 >::type
    operator() ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
                 const iType4 & i4 , const iType5 & i5 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_7( m_shape, i0, i1, i2, i3, i4, i5, 0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      // Contiguous storage with right-most index as the fad dimension
      return fad_view_type(
        m_ptr_on_device + ( m_storage_size * ( i5 +
                            m_shape.N5 * ( i4 +
                            m_shape.N4 * ( i3 +
                            m_shape.N3 * ( i2 +
                            m_shape.N2 * ( i1 ))))) +
                            m_stride.value * i0 ) ,
        m_storage_size-1 , 1 );
    }

  template< typename iType0 , typename iType1 , typename iType2 ,
            typename iType3 , typename iType4 , typename iType5 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, typename traits::array_layout, 7,
                                      iType0, iType1, iType2, iType3, iType4, iType5 >::type
    at( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
        const iType4 & i4 , const iType5 & i5 , int , int ) const
    { return operator()(i0,i1,i2,i3,i4,i5); }

  //------------------------------------
  //------------------------------------
  // Array operators, traits::rank 8:

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 , typename iType6 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, LayoutLeft, 8, iType0, iType1, iType2, iType3, iType4, iType5, iType6 >::type
    operator() ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
                 const iType4 & i4 , const iType5 & i5 , const iType6 & i6 ) const
    {
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );
      KOKKOS_ASSERT_SHAPE_BOUNDS_8( m_shape, i0, i1, i2, i3, i4, i5, i6, 0 );

      // Strided storage with right-most index as the fad dimension
      return fad_view_type(
        m_ptr_on_device + ( i0 + m_stride.value * (
                            i1 + m_shape.N1 * (
                            i2 + m_shape.N2 * (
                            i3 + m_shape.N3 * (
                            i4 + m_shape.N4 * (
                            i5 + m_shape.N5 * (
                            i6 ))))))),
        m_storage_size-1 ,
        m_stride.value * m_shape.N1 * m_shape.N2 * m_shape.N3 * m_shape.N4 * m_shape.N5 * m_shape.N6 );
    }

  template< typename iType0 , typename iType1 , typename iType2 ,
            typename iType3 , typename iType4 , typename iType5, typename iType6 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, LayoutRight, 8, iType0, iType1, iType2, iType3, iType4, iType5, iType6 >::type
    operator() ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
                 const iType4 & i4 , const iType5 & i5 , const iType6 & i6 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_8( m_shape, i0, i1, i2, i3, i4, i5, i6, 0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      // Contiguous storage with right-most index as the fad dimension
      return fad_view_type(
        m_ptr_on_device + ( m_storage_size * ( i6 +
                            m_shape.N6 * ( i5 +
                            m_shape.N5 * ( i4 +
                            m_shape.N4 * ( i3 +
                            m_shape.N3 * ( i2 +
                            m_shape.N2 * ( i1 )))))) +
                            m_stride.value * i0 ) ,
        m_storage_size-1 , 1 );
    }

  template< typename iType0 , typename iType1 , typename iType2 ,
            typename iType3 , typename iType4 , typename iType5, typename iType6 >
  KOKKOS_FORCEINLINE_FUNCTION
  typename Impl::ViewEnableArrayOper< fad_view_type ,
                                      traits, typename traits::array_layout, 8,
                                      iType0, iType1, iType2, iType3, iType4, iType5, iType6 >::type
    at( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
        const iType4 & i4 , const iType5 & i5 , const iType6 & i6 , int ) const
    { return operator()(i0,i1,i2,i3,i4,i5,i6); }

  //------------------------------------
  // Access to the underlying contiguous storage of this view specialization.
  // These methods are specific to specialization of a view.

  KOKKOS_FORCEINLINE_FUNCTION
  typename traits::value_type::value_type *
    ptr_on_device() const { return m_ptr_on_device ; }

  // Stride of physical storage, dimensioned to at least Rank
  template< typename iType >
  KOKKOS_FORCEINLINE_FUNCTION
  void stride( iType * const s ) const
  { Impl::stride( s , m_shape , m_stride ); }

  // Count of contiguously allocated data members including padding.
  KOKKOS_FORCEINLINE_FUNCTION
  typename traits::size_type capacity() const
  { return Impl::capacity( m_shape , m_stride ); }

  // Static storage size
  KOKKOS_FORCEINLINE_FUNCTION
  typename traits::size_type storage_size() const
  { return m_storage_size; }
};

/** \brief  A deep copy between views of the same specialization, compatible type,
 *          same rank, same layout are handled by that specialization.
 */
template< class DT , class DL , class DD , class DM ,
          class ST , class SL , class SD , class SM >
inline
void deep_copy( const View<DT,DL,DD,DM,Impl::ViewSpecializeSacadoFad> & dst ,
                const View<ST,SL,SD,SM,Impl::ViewSpecializeSacadoFad> & src ,
                typename Impl::enable_if<(
                  Impl::is_same< typename View<DT,DL,DD,DM,Impl::ViewSpecializeSacadoFad>::value_type::value_type ,
                                 typename View<ST,SL,SD,SM,Impl::ViewSpecializeSacadoFad>::value_type::value_type >::value
                  &&
                  Impl::is_same< typename View<DT,DL,DD,DM,Impl::ViewSpecializeSacadoFad>::array_layout ,
                                 typename View<ST,SL,SD,SM,Impl::ViewSpecializeSacadoFad>::array_layout >::value
                  &&
                  ( unsigned(View<DT,DL,DD,DM,Impl::ViewSpecializeSacadoFad>::rank) ==
                    unsigned(View<ST,SL,SD,SM,Impl::ViewSpecializeSacadoFad>::rank) )
                )>::type * = 0 )
{
  typedef  View<DT,DL,DD,DM,Impl::ViewSpecializeSacadoFad>  dst_type ;
  typedef  View<ST,SL,SD,SM,Impl::ViewSpecializeSacadoFad>  src_type ;

  typedef typename dst_type::memory_space  dst_memory_space ;
  typedef typename src_type::memory_space  src_memory_space ;

  if ( dst.ptr_on_device() != src.ptr_on_device() ) {

    Impl::assert_shapes_are_equal( dst.shape() , src.shape() );

    const size_t nbytes = sizeof(typename dst_type::value_type::value_type) * dst.capacity();

    Impl::DeepCopy< dst_memory_space , src_memory_space >( dst.ptr_on_device() , src.ptr_on_device() , nbytes );
  }
}

} // namespace Kokkos

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace Kokkos {
namespace Impl {

template<>
struct ViewAssignment< ViewSpecializeSacadoFad , ViewSpecializeSacadoFad , void >
{
  //------------------------------------
  /** \brief  Compatible value and shape */

  template< class DT , class DL , class DD , class DM ,
            class ST , class SL , class SD , class SM >
  KOKKOS_INLINE_FUNCTION
  ViewAssignment(       View<DT,DL,DD,DM,ViewSpecializeSacadoFad> & dst
                , const View<ST,SL,SD,SM,ViewSpecializeSacadoFad> & src
                , const typename enable_if<(
                    ViewAssignable< ViewTraits<DT,DL,DD,DM> ,
                                    ViewTraits<ST,SL,SD,SM> >::value
                    )>::type * = 0
                  )
  {
    typedef ViewTraits<DT,DL,DD,DM>                         dst_traits ;
    typedef View<DT,DL,DD,DM,ViewSpecializeSacadoFad>       dst_type ;
    typedef typename dst_type::shape_type                   shape_type ;
    typedef typename dst_type::stride_type                  stride_type ;

    ViewTracking< dst_traits >::decrement( dst.m_ptr_on_device );

    shape_type::assign( dst.m_shape,
                        src.m_shape.N0 , src.m_shape.N1 , src.m_shape.N2 , src.m_shape.N3 ,
                        src.m_shape.N4 , src.m_shape.N5 , src.m_shape.N6 , src.m_shape.N7 );

    stride_type::assign( dst.m_stride , src.m_stride.value );

    dst.m_storage_size  = src.m_storage_size ;
    dst.m_ptr_on_device = src.m_ptr_on_device ;

    Impl::ViewTracking< dst_traits >::increment( dst.m_ptr_on_device );
  }

  //------------------------------------
  /** \brief  Partition of compatible value and shape */

  template< class DT , class DL , class DD , class DM ,
            class ST , class SL , class SD , class SM >
  KOKKOS_INLINE_FUNCTION
  ViewAssignment(       View<DT,DL,DD,DM,ViewSpecializeSacadoFad> & dst
                , const View<ST,SL,SD,SM,ViewSpecializeSacadoFad> & src
                , const Sacado::Fad::VectorPartition & part
                , const typename enable_if<(
                    ViewAssignable< ViewTraits<DT,DL,DD,DM> ,
                                    ViewTraits<ST,SL,SD,SM> >::value
                    &&
                    ! ViewTraits<DT,DL,DD,DM>::is_managed
                    )>::type * = 0
                  )
  {
    typedef ViewTraits<DT,DL,DD,DM>                           dst_traits ;
    typedef View<DT,DL,DD,DM,ViewSpecializeSacadoFad>         dst_type ;
    typedef typename dst_type::shape_type                     dst_shape_type ;
    typedef typename dst_type::stride_type                    dst_stride_type ;
    typedef typename dst_traits::value_type                   dst_fad_type ;

    enum { DstRank         = dst_type::Rank };
    enum { DstStaticLength = Sacado::StaticSize<dst_fad_type>::value };

    const int length = part.end - part.begin ;

    if ( DstStaticLength && DstStaticLength != length ) {
      const char msg[] = "Kokkos::View< Fad ... > incompatible partitioning" ;
#if defined(__CUDACC__) && defined(__CUDA_ARCH__)
      cuda_abort(msg);
#else
      throw std::runtime_error(msg);
#endif
    }

    dst_shape_type::assign( dst.m_shape ,
                            ( DstRank == 0 ? length : src.m_shape.N0 ) ,
                            ( DstRank == 1 ? length : src.m_shape.N1 ) ,
                            ( DstRank == 2 ? length : src.m_shape.N2 ) ,
                            ( DstRank == 3 ? length : src.m_shape.N3 ) ,
                            ( DstRank == 4 ? length : src.m_shape.N4 ) ,
                            ( DstRank == 5 ? length : src.m_shape.N5 ) ,
                            ( DstRank == 6 ? length : src.m_shape.N6 ) ,
                            ( DstRank == 7 ? length : src.m_shape.N7 ) );

    dst_stride_type::assign( dst.m_stride , src.m_stride.value );

    dst.m_storage_size = src.m_storage_size ;

    if ( Impl::is_same< typename dst_traits::array_layout , LayoutLeft >::value ) {
      dst.m_ptr_on_device = src.m_ptr_on_device + part.begin *
                      ( 0 == DstRank ? 1 : dst.m_stride.value * (
                      ( 1 == DstRank ? 1 : dst.m_shape.N1 * (
                      ( 2 == DstRank ? 1 : dst.m_shape.N2 * (
                      ( 3 == DstRank ? 1 : dst.m_shape.N3 * (
                      ( 4 == DstRank ? 1 : dst.m_shape.N4 * (
                      ( 5 == DstRank ? 1 : dst.m_shape.N5 * (
                      ( 6 == DstRank ? 1 : dst.m_shape.N6 )))))))))))));
    }
    else { // if ( Impl::is_same< typename traits::array_layout , LayoutRight >::value )
      dst.m_ptr_on_device = src.m_ptr_on_device + part.begin ;
    }
  }
};


template<>
struct ViewAssignment< ViewDefault , ViewSpecializeSacadoFad , void >
{
  //------------------------------------
  /** \brief  Compatible value and shape */

  template< class ST , class SL , class SD , class SM >
  KOKKOS_INLINE_FUNCTION
  ViewAssignment( typename View<ST,SL,SD,SM,ViewSpecializeSacadoFad>::array_type & dst
                , const    View<ST,SL,SD,SM,ViewSpecializeSacadoFad> & src )
  {
    typedef View<ST,SL,SD,SM,ViewSpecializeSacadoFad>    src_type ;

    typedef typename src_type::array_type   dst_type ;
    typedef typename dst_type::shape_type   dst_shape_type ;
    typedef typename dst_type::stride_type  dst_stride_type ;

    ViewTracking< dst_type >::decrement( dst.m_ptr_on_device );

    dst_shape_type::assign( dst.m_shape,
                            src.m_shape.N0 , src.m_shape.N1 , src.m_shape.N2 , src.m_shape.N3 ,
                            src.m_shape.N4 , src.m_shape.N5 , src.m_shape.N6 , src.m_shape.N7 );

    dst_stride_type::assign( dst.m_stride , src.m_stride.value );

    dst.m_ptr_on_device = reinterpret_cast< typename dst_type::value_type *>( src.m_ptr_on_device );

    Impl::ViewTracking< dst_type >::increment( dst.m_ptr_on_device );
  }
};

} // namespace Impl
} // namespace Kokkos

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#endif

#endif /* #ifndef KOKKOS_VIEW_FAD_HPP */