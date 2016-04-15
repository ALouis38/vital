"""
ckwg +31
Copyright 2016 by Kitware, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Kitware, Inc. nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

==============================================================================

Interface to VITAL Eigen matrix classes through numpy

"""
import ctypes

import numpy

from vital.util import (
    VitalErrorHandle,
    VitalObject,
)


__author__ = 'paul.tunison@kitware.com'


if ctypes.sizeof(ctypes.c_void_p) == 4:
    c_ptrdiff_t = ctypes.c_int32
elif ctypes.sizeof(ctypes.c_void_p) == 8:
    c_ptrdiff_t = ctypes.c_int64
else:
    raise RuntimeError("Invalid c_void_p size? =%d"
                       % ctypes.sizeof(ctypes.c_void_p))


class VitalEigenNumpyArray (numpy.ndarray, VitalObject):

    # Valid dtype possibilities
    MAT_TYPE_KEYS = (numpy.double, numpy.float32)

    # C library function component template
    FUNC_SPEC = "{rows:s}x{cols:s}{type:s}"

    # noinspection PyMethodOverriding
    @classmethod
    def from_c_pointer(cls, ptr, rows=2, cols=1,
                       dynamic_rows=False, dynamic_cols=False,
                       dtype=numpy.double, owns_data=True,
                       shallow_copy_of=None):
        """
        Special implementation from C pointer due to both needing more
        information and because we are sub-classing numpy.ndarray.

        :param ptr: C API opaque structure pointer type instance
        :type ptr: VitalAlgorithm.C_TYPE_PTR

        :param rows: Number of rows in the matrix
        :param cols: Number of columns in the matrix
        :param dynamic_rows: If we should not use compile-time generated types
            in regards to the row specification.
        :param dynamic_cols: If we should not use compile-time generated types
            in regards to the column specification.
        :param dtype: numpy dtype to use
        :param owns_data: When given a c-pointer, if we should take ownership of
            the underlying data.

        :param shallow_copy_of: Optional parent object instance when the ptr
            given is coming from an existing python object.
        :type shallow_copy_of: VitalObject or None

        """
        m = VitalEigenNumpyArray(rows, cols, dynamic_rows, dynamic_cols,
                                 dtype, ptr, owns_data)
        m._parent = shallow_copy_of
        return m

    @classmethod
    def _init_func_map(cls, rows, cols, d_rows, d_cols, dtype):
        """
        Initialize C function naming map for the given shape and type
        information.

        :returns: Function name mapping and associated ctypes data type
        :rtype: (dict[str, str], _ctypes._SimpleCData)

        """
        if dtype == cls.MAT_TYPE_KEYS[0]:  # C double
            type_char = 'd'
            c_type = ctypes.c_double
        elif dtype == cls.MAT_TYPE_KEYS[1]:  # C float
            type_char = 'f'
            c_type = ctypes.c_float
        else:
            raise ValueError("Invalid data type given ('%s'). "
                             "Must be one of %s."
                             % (dtype, cls.MAT_TYPE_KEYS))
        func_spec = cls.FUNC_SPEC.format(
            rows=(d_rows and 'X') or str(rows),
            cols=(d_cols and 'X') or str(cols),
            type=type_char,
        )
        func_map = {
            'new': 'vital_eigen_matrix{}_new'.format(func_spec),
            'new_sized': 'vital_eigen_matrix{}_new_sized'.format(func_spec),
            'destroy': 'vital_eigen_matrix{}_destroy'.format(func_spec),
            'get': 'vital_eigen_matrix{}_get'.format(func_spec),
            'set': 'vital_eigen_matrix{}_set'.format(func_spec),
            'rows': 'vital_eigen_matrix{}_rows'.format(func_spec),
            'cols': 'vital_eigen_matrix{}_cols'.format(func_spec),
            'row_stride': 'vital_eigen_matrix{}_row_stride'.format(func_spec),
            'col_stride': 'vital_eigen_matrix{}_col_stride'.format(func_spec),
            'data': 'vital_eigen_matrix{}_data'.format(func_spec),
        }
        return func_map, c_type

    @classmethod
    def _get_data_components(cls, ptr, c_type, func_map):
        """
        Get underlying Eigen matrix shape, stride and data pointer
        """
        v_rows = cls.VITAL_LIB[func_map['rows']]
        v_cols = cls.VITAL_LIB[func_map['cols']]
        v_row_stride = cls.VITAL_LIB[func_map['row_stride']]
        v_col_stride = cls.VITAL_LIB[func_map['col_stride']]
        v_data = cls.VITAL_LIB[func_map['data']]

        v_rows.argtypes = [cls.C_TYPE_PTR, VitalErrorHandle.C_TYPE_PTR]
        v_cols.argtypes = [cls.C_TYPE_PTR, VitalErrorHandle.C_TYPE_PTR]
        v_row_stride.argtypes = [cls.C_TYPE_PTR, VitalErrorHandle.C_TYPE_PTR]
        v_col_stride.argtypes = [cls.C_TYPE_PTR, VitalErrorHandle.C_TYPE_PTR]
        v_data.argtypes = [cls.C_TYPE_PTR, VitalErrorHandle.C_TYPE_PTR]

        v_rows.restype = c_ptrdiff_t
        v_cols.restype = c_ptrdiff_t
        v_row_stride.restype = c_ptrdiff_t
        v_col_stride.restype = c_ptrdiff_t
        v_data.restype = ctypes.POINTER(c_type)

        with VitalErrorHandle() as eh:
            rows = v_rows(ptr, eh)
            cols = v_cols(ptr, eh)
            row_stride = v_row_stride(ptr, eh)
            col_stride = v_col_stride(ptr, eh)
            data = v_data(ptr, eh)

        return rows, cols, row_stride, col_stride, data

    def __new__(cls, rows=2, cols=1, dynamic_rows=False, dynamic_cols=False,
                dtype=numpy.double, c_ptr=None, owns_data=True):
        """
        Create a new Vital Eigen matrix and interface

        :param rows: Number of rows in the matrix
        :param cols: Number of columns in the matrix
        :param dynamic_rows: If we should not use compile-time generated types
            in regards to the row specification.
        :param dynamic_cols: If we should not use compile-time generated types
            in regards to the column specification.
        :param dtype: numpy dtype to use
        :param c_ptr: Optional existing C Eigen matrix instance pointer to use
            instead of constructing a new one.
        :param owns_data: When given a c-pointer, if we should take ownership of
            the underlying data.

        :return: Interface to a new or existing Eigen matrix instance.

        """
        func_map, c_type = cls._init_func_map(rows, cols,
                                              dynamic_rows, dynamic_cols,
                                              dtype)

        # Create new Eigen matrix
        if c_ptr is None:
            c_new = cls.VITAL_LIB[func_map['new_sized']]
            c_new.argtypes = [c_ptrdiff_t, c_ptrdiff_t]
            c_new.restype = cls.C_TYPE_PTR
            inst_ptr = c_new(c_ptrdiff_t(rows), c_ptrdiff_t(cols))
            if not bool(inst_ptr):
                raise RuntimeError("Failed to construct new Eigen matrix")
            owns_data = True
        else:
            inst_ptr = c_ptr

        # Get information, data pointer and base transformed array
        rows, cols, row_stride, col_stride, data = \
            cls._get_data_components(inst_ptr, c_type, func_map)
        # Might have to swap out the use of ``dtype_bytes`` for
        # inner/outer size values from Eigen if matrices are ever NOT
        # densely packed.
        dtype_bytes = numpy.dtype(c_type).alignment
        strides = (row_stride * dtype_bytes,
                   col_stride * dtype_bytes)
        if data:
            b = numpy.ctypeslib.as_array(data, (rows * cols,))
        else:
            b = buffer('')

        # args: (subclass, shape, dtype, buffer, offset, strides, order)
        # TODO: Get offset from eigen matrix, too.
        # print "Construction ({}, {}, {}, {}, {})".format(
        #     (rows, cols), dtype, b, 0, strides
        # )
        obj = numpy.ndarray.__new__(cls, (rows, cols), dtype, b, 0, strides)

        # local properties
        obj._dynamic_rows = dynamic_rows
        obj._dynamic_cols = dynamic_cols
        obj._func_map = func_map
        obj._owns_data = owns_data

        VitalObject.__init__(obj)

        obj._inst_ptr = inst_ptr
        obj._parent = None

        return obj

    # noinspection PyMissingConstructor
    def __init__(self, *args, **kwds):
        # initialization handled in __new__
        pass

    def __array_finalize__(self, obj):
        """
        Where numpy finalizes instance properties of an array instance when
        created due to __new__, casting or new-from-template.
        """
        # got here from __new__, nothing to transfer
        if obj is None:
            return

        # copy/move over attributes from parent as necessary
        #   self => New class of this type
        #   obj  => other class MAYBE this type
        if isinstance(obj, VitalEigenNumpyArray):
            self._dynamic_rows = obj._dynamic_rows
            self._dynamic_cols = obj._dynamic_cols
            self._func_map = obj._func_map
            # Always false because we are view of obj. See parent switch below.
            self._owns_data = False

            self._inst_ptr = obj._inst_ptr
            if obj._parent is None:
                # obj is the root parent object
                self._parent = obj
            else:
                # transfer parent reference
                self._parent = obj._parent
        else:
            raise RuntimeError("Finalizing VitalEigenNumpyArray whose parent "
                               "is not of the same type (%s). Cannot inherit "
                               "required information." % type(obj))

    # def __array_prepare__(self, obj, context=None):
    #     # Don't propagate this class and its stored references needlessly
    #     return obj
    #
    # def __array_wrap__(self, out_arr, context=None):
    #     # Don't propagate this class and its stored references needlessly
    #     return out_arr

    def _destroy(self):
        # Not smart-pointer controlled in C++. We might not own the data we're
        # viewing.
        if self.c_pointer and self._owns_data:
            # print "Destroying"
            m_del = self.VITAL_LIB[self._func_map['destroy']]
            m_del.argtypes = [self.C_TYPE_PTR, VitalErrorHandle.C_TYPE_PTR]
            with VitalErrorHandle() as eh:
                m_del(self, eh)
            self._inst_ptr = self.C_TYPE_PTR()

    def at_eigen_base_index(self, row, col=0):
        """
        Get the value at the specified index in the base Eigen matrix.

        **Note:** *The base Eigen matrix may not be the same shape as the
        current instance as this might be a sliced view of the base matrix.*

        :param row: Row of the value
        :param col: Column of the value

        :return: Value at the specified index.

        """
        assert 0 <= row < self.shape[0], "Row out of range"
        assert 0 <= col < self.shape[1], "Col out of range"
        f = self.VITAL_LIB[self._func_map['get']]
        f.argtypes = [self.C_TYPE_PTR, c_ptrdiff_t, c_ptrdiff_t,
                      VitalErrorHandle.C_TYPE_PTR]
        f.restype = self.c_type
        with VitalErrorHandle() as eh:
            return f(self, row, col, eh)
