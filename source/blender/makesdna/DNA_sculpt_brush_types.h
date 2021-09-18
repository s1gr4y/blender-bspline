/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2011 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup DNA
 *
 * Structs used for the sculpt brush system
 */
#pragma once

typedef struct BrushMapping {
  char name[64];
  CurveMapping curve;
  float factor;
  short blendmode;
  short input_channel;
  int flag, _pad[1];
} BrushMapping;

typedef struct BrushChannel {
  char idname[64];
  char name[64];

  struct BrushChannelType *def;

  float fvalue;
  int ivalue;
  BrushMapping mappings[5];  // should always be BRUSH_MAPPING_MAX

  int type, flag;
} BrushChannel;

typedef struct BrushChannelSet {
  BrushChannel *channels;
  int totchannel, _pad[1];
} BrushChannelSet;

// mapping flags
enum { BRUSH_MAPPING_ENABLED = 1 << 0, BRUSH_MAPPING_INVERT = 1 << 1 };

// mapping types
enum {
  BRUSH_MAPPING_PRESSURE = 0,
  BRUSH_MAPPING_XTILT = 1,
  BRUSH_MAPPING_YTILT = 2,
  BRUSH_MAPPING_ANGLE = 3,
  BRUSH_MAPPING_SPEED = 4,
  BRUSH_MAPPING_MAX = 5  // see BrushChannel.mappings
};

static_assert(offsetof(BrushChannel, type) - offsetof(BrushChannel, mappings) ==
                  sizeof(BrushMapping) * BRUSH_MAPPING_MAX,
              "BrushChannel.mappings must == BRUSH_MAPPING_MAX");

// BrushChannel->flag
enum {
  BRUSH_CHANNEL_INHERIT = 1 << 0,
  BRUSH_CHANNEL_INHERIT_IF_UNSET = 1 << 1,
  BRUSH_CHANNEL_NO_MAPPINGS = 1 << 2
};

// BrushChannelType->type
enum {
  BRUSH_CHANNEL_FLOAT = 1 << 0,
  BRUSH_CHANNEL_INT = 1 << 1,
  BRUSH_CHANNEL_ENUM = 1 << 2,
  BRUSH_CHANNEL_BITMASK = 1 << 3,
};