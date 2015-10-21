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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
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

#ifndef KWIVER_VITAL_KLV_0601_H_
#define KWIVER_VITAL_KLV_0601_H_

#include <klv/klv_key.h>
#include <vxl_config.h>
#include <vector>
#include <string>
#include <cstddef>
#include <boost/any.hpp>


namespace kwiver {
namespace vital {

/// Validate a KLV 0601 data packet using the checksum at the end
/// @param[in] data is the klv packet to checksum
bool klv_0601_checksum( klv_data const& data );


/// Enumeration of tags in the MISB 0601 KLV standard
enum klv_0601_tag {KLV_0601_UNKNOWN                 = 0,
                   KLV_0601_CHECKSUM                = 1,
                   KLV_0601_UNIX_TIMESTAMP          = 2,
                   KLV_0601_MISSION_ID              = 3,
                   KLV_0601_PLATFORM_TAIL_NUMBER    = 4,
                   KLV_0601_PLATFORM_HEADING_ANGLE  = 5,
                   KLV_0601_PLATFORM_PITCH_ANGLE    = 6,
                   KLV_0601_PLATFORM_ROLL_ANGLE     = 7,
                   KLV_0601_PLATFORM_TRUE_AIRSPEED  = 8,
                   KLV_0601_PLATFORM_IND_AIRSPEED   = 9,
                   KLV_0601_PLATFORM_DESIGNATION    = 10,
                   KLV_0601_IMAGE_SOURCE_SENSOR     = 11,
                   KLV_0601_IMAGE_COORDINATE_SYSTEM = 12,
                   KLV_0601_SENSOR_LATITUDE         = 13,
                   KLV_0601_SENSOR_LONGITUDE        = 14,
                   KLV_0601_SENSOR_TRUE_ALTITUDE    = 15,
                   KLV_0601_SENSOR_HORIZONTAL_FOV   = 16,
                   KLV_0601_SENSOR_VERTICAL_FOV     = 17,
                   KLV_0601_SENSOR_REL_AZ_ANGLE     = 18,
                   KLV_0601_SENSOR_REL_EL_ANGLE     = 19,
                   KLV_0601_SENSOR_REL_ROLL_ANGLE   = 20,
                   KLV_0601_SLANT_RANGE             = 21,
                   KLV_0601_TARGET_WIDTH            = 22,
                   KLV_0601_FRAME_CENTER_LAT        = 23,
                   KLV_0601_FRAME_CENTER_LONG       = 24,
                   KLV_0601_FRAME_CENTER_ELEV       = 25,
                   KLV_0601_OFFSET_CORNER_LAT_PT_1  = 26,
                   KLV_0601_OFFSET_CORNER_LONG_PT_1 = 27,
                   KLV_0601_OFFSET_CORNER_LAT_PT_2  = 28,
                   KLV_0601_OFFSET_CORNER_LONG_PT_2 = 29,
                   KLV_0601_OFFSET_CORNER_LAT_PT_3  = 30,
                   KLV_0601_OFFSET_CORNER_LONG_PT_3 = 31,
                   KLV_0601_OFFSET_CORNER_LAT_PT_4  = 32,
                   KLV_0601_OFFSET_CORNER_LONG_PT_4 = 33,
                   KLV_0601_ICING_DETECTED          = 34,
                   KLV_0601_WIND_DIRECTION          = 35,
                   KLV_0601_WIND_SPEED              = 36,
                   KLV_0601_STATIC_PRESSURE         = 37,
                   KLV_0601_DENSITY_ALTITUDE        = 38,
                   KLV_0601_OUTSIDE_AIR_TEMPERATURE = 39,
                   KLV_0601_TARGET_LOCATION_LAT     = 40,
                   KLV_0601_TARGET_LOCATION_LONG    = 41,
                   KLV_0601_TARGET_LOCATION_ELEV    = 42,
                   KLV_0601_TARGET_TRK_GATE_WIDTH   = 43,
                   KLV_0601_TARGET_TRK_GATE_HEIGHT  = 44,
                   KLV_0601_TARGET_ERROR_EST_CE90   = 45,
                   KLV_0601_TARGET_ERROR_EST_LE90   = 46,
                   KLV_0601_GENERIC_FLAG_DATA_01    = 47,
                   KLV_0601_SECURITY_LOCAL_MD_SET   = 48,
                   KLV_0601_DIFFERENTIAL_PRESSURE   = 49,
                   KLV_0601_PLATFORM_ANG_OF_ATTACK  = 50,
                   KLV_0601_PLATFORM_VERTICAL_SPEED = 51,
                   KLV_0601_PLATFORM_SIDESLIP_ANGLE = 52,
                   KLV_0601_AIRFIELD_BAROMET_PRESS  = 53,
                   KLV_0601_AIRFIELD_ELEVATION      = 54,
                   KLV_0601_RELATIVE_HUMIDITY       = 55,
                   KLV_0601_PLATFORM_GROUND_SPEED   = 56,
                   KLV_0601_GROUND_RANGE            = 57,
                   KLV_0601_PLATFORM_FUEL_REMAINING = 58,
                   KLV_0601_PLATFORM_CALL_SIGN      = 59,
                   KLV_0601_WEAPON_LOAD             = 60,
                   KLV_0601_WEAPON_FIRED            = 61,
                   KLV_0601_LASER_PRF_CODE          = 62,
                   KLV_0601_SENSOR_FOV_NAME         = 63,
                   KLV_0601_PLATFORM_MAGNET_HEADING = 64,
                   KLV_0601_UAS_LDS_VERSION_NUMBER  = 65,
                   // TODO Add the rest of the fields here
                   KLV_0601_ENUM_END };



/// Return a string representation of the name of a KLV 0601 tag
std::string
klv_0601_tag_to_string(klv_0601_tag t);


/// Test to see if a 0601 key
bool is_klv_0601_key( klv_uds_key const& key);


/// Return 0601 key
klv_uds_key klv_0601_key();


/// Extract the appropriate data type from raw bytes as a boost::any
boost::any
klv_0601_value(klv_0601_tag t, const vxl_byte* data, std::size_t length);


/// Return the tag data as a double
double
klv_0601_value_double(klv_0601_tag t, const boost::any& data);


/// Format the tag data as a string
std::string
klv_0601_value_string(klv_0601_tag t, const boost::any& data);


/// Format the tag data as a hex string
std::string
klv_0601_value_hex_string(klv_0601_tag t, const boost::any& data);

} } // end namespace


#endif
