/*ckwg +29
 * Copyright 2015 by Kitware, Inc.
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
 * \brief C interface common error handle structure
 */

#ifndef VITAL_C_ERROR_HANDLE_H_
#define VITAL_C_ERROR_HANDLE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <vital/bindings/c/vital_c_export.h>


/// Common error handle structure
/**
 * When an instance of this structure is to be used multiple times, if the
 * message from a previous error is to be retained, it should be copied before
 * being used again. The message, if non-null, is freed when used as an error
 * handle.
 */
typedef struct vital_error_handle_s {
  int error_code;
  char *message;
} vital_error_handle_t;


/// Return a new, empty error handle object.
/**
 * \returns New error handle whose error code is set to 0 and message to NULL.
 */
VITAL_C_EXPORT
vital_error_handle_t* vital_eh_new();


/// Destroy the given non-null error handle structure pointer
VITAL_C_EXPORT
void vital_eh_destroy( vital_error_handle_t *eh );


#ifdef __cplusplus
}
#endif

#endif // VITAL_C_ERROR_HANDLE_H_
