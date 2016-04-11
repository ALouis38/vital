/*ckwg +29
 * Copyright 2016 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 * \brief C Interface implementation to Vital use of Eigen vector and matrix
 *        classes
 *
 * Eigen is Column-major by default, i.e. (rows, cols) indexing
 */

#include "eigen.h"

#include <vital/bindings/c/helpers/c_utils.h>
#include <vital/types/matrix.h>
#include <vital/types/vector.h>


#define REINTERP_TYPE( new_type, c_ptr, var )           \
  new_type *var = reinterpret_cast<new_type*>( c_ptr ); \
  do                                                    \
  {                                                     \
    if( var == 0 )                                      \
    {                                                   \
      throw "Null pointer";                             \
    }                                                   \
  } while(0)


/// Define Eigen matrix interface functions for use with MAPTK
/**
 * \param T The data storage type like double or float
 * \param S The character suffix to use for naming of functions.
 * \param R Number of rows in the matrix. "Vector" types use this as the size
 *          parameter.
 * \param C Number of columns in the matrix. "Vector" types have a value of 1
 *          here.
 */
#define DEFINE_EIGEN_OPERATIONS( T, S, R, C ) \
/** Create a new Eigen type-based Matrix of the given shape */ \
vital_eigen_matrix##R##x##C##S##_t* \
vital_eigen_matrix##R##x##C##S##_new() \
{ \
  STANDARD_CATCH( \
    "vital_eigen_matrix" #R "x" #C #S ".new.", 0, \
    return reinterpret_cast<vital_eigen_matrix##R##x##C##S##_t*>( \
      new Eigen::Matrix< T, R, C > \
    ); \
  ); \
  return 0; \
} \
\
/** Destroy a given Eigen matrix instance */ \
void \
vital_eigen_matrix##R##x##C##S##_destroy( vital_eigen_matrix##R##x##C##S##_t *m, \
                                          vital_error_handle_t *eh ) \
{ \
  typedef Eigen::Matrix< T, R, C > matrix_t; \
  STANDARD_CATCH( \
    "vital_eigen_matrix" #R "x" #C #S ".destroy", eh, \
    REINTERP_TYPE( matrix_t, m, mp ); \
    if( mp ) \
    { \
      delete( mp ); \
    } \
  ); \
} \
\
/** Get the value at a location */ \
T \
vital_eigen_matrix##R##x##C##S##_get( vital_eigen_matrix##R##x##C##S##_t *m, \
                                      unsigned int row, unsigned int col, \
                                      vital_error_handle_t *eh ) \
{ \
  typedef Eigen::Matrix< T, R, C > matrix_t; \
  STANDARD_CATCH( \
    "vital_eigen_matrix" #R "x" #C #S ".get", eh, \
    REINTERP_TYPE( matrix_t, m, mp ); \
    return (*mp)(row, col); \
  ); \
  return 0; \
} \
\
/** Set the value at a location */ \
void \
vital_eigen_matrix##R##x##C##S##_set( vital_eigen_matrix##R##x##C##S##_t *m, \
                                      unsigned int row, unsigned int col, \
                                      T value, \
                                      vital_error_handle_t *eh ) \
{ \
  typedef Eigen::Matrix< T, R, C > matrix_t; \
  STANDARD_CATCH( \
    "vital_eigen_matrix" #R "x" #C #S ".set", eh, \
    REINTERP_TYPE( matrix_t, m, mp ); \
    (*mp)(row, col) = value; \
  ); \
} \
\
/** Get the pointer to the vector's data array */ \
void \
vital_eigen_matrix##R##x##C##S##_data( vital_eigen_matrix##R##x##C##S##_t *m, \
                                       unsigned int *rows, \
                                       unsigned int *cols, \
                                       unsigned int *inner_stride, \
                                       unsigned int *outer_stride, \
                                       unsigned int *is_row_major, \
                                       T **data, \
                                       vital_error_handle_t *eh ) \
{ \
  typedef Eigen::Matrix< T, R, C > matrix_t; \
  STANDARD_CATCH( \
    "vital_eigen_matrix" #R "x" #C #S ".data", eh, \
    REINTERP_TYPE( matrix_t, m, mp ); \
    *rows = mp->rows(); \
    *cols = mp->cols(); \
    *inner_stride = mp->innerStride(); \
    *outer_stride = mp->outerStride(); \
    *is_row_major = (unsigned int)(mp->Flags & Eigen::RowMajorBit); \
    *data = mp->data(); \
  ); \
}


/// DEFINE operations for all shapes
/**
 * \param T Data type
 * \param S Type suffix
 */
#define DEFINE_EIGEN_ALL_SHAPES( T, S ) \
/* "Vector" types */                    \
DEFINE_EIGEN_OPERATIONS( T, S, 2, 1 )   \
DEFINE_EIGEN_OPERATIONS( T, S, 3, 1 )   \
DEFINE_EIGEN_OPERATIONS( T, S, 4, 1 )   \
/* Other matrix shapes */               \
DEFINE_EIGEN_OPERATIONS( T, S, 2, 2 )   \
DEFINE_EIGEN_OPERATIONS( T, S, 2, 3 )   \
DEFINE_EIGEN_OPERATIONS( T, S, 3, 2 )   \
DEFINE_EIGEN_OPERATIONS( T, S, 3, 3 )   \
DEFINE_EIGEN_OPERATIONS( T, S, 3, 4 )   \
DEFINE_EIGEN_OPERATIONS( T, S, 4, 3 )   \
DEFINE_EIGEN_OPERATIONS( T, S, 4, 4 )


DEFINE_EIGEN_ALL_SHAPES( double, d )
DEFINE_EIGEN_ALL_SHAPES( float,  f )


#undef DEFINE_EIGEN_OPERATIONS
#undef DEFINE_EIGEN_ALL_SHAPES
