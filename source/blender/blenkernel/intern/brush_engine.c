#include "MEM_guardedalloc.h"

#include "BLI_alloca.h"
#include "BLI_array.h"
#include "BLI_bitmap.h"
#include "BLI_compiler_attrs.h"
#include "BLI_compiler_compat.h"
#include "BLI_listbase.h"
#include "BLI_math.h"
#include "BLI_memarena.h"

#include "DNA_brush_enums.h"
#include "DNA_brush_types.h"
#include "DNA_color_types.h"
#include "DNA_curveprofile_types.h"
#include "DNA_material_types.h"
#include "DNA_node_types.h"
#include "DNA_sculpt_brush_types.h"

#include "BKE_brush.h"
#include "BKE_colorband.h"
#include "BKE_colortools.h"
#include "BKE_context.h"
#include "BKE_node.h"
#include "BKE_paint.h"

#include "BKE_brush_engine.h"
#include "BKE_curveprofile.h"

#include "BLO_read_write.h"

#define ICON_NONE -1

/*
Brush command lists.

Command lists are built dynamically from
brush flags, pen input settings, etc.

Eventually they will be generated by node
networks.  BrushCommandPreset will be
generated from the node group inputs.
*/

/* clang-format off */
BrushChannelType brush_builtin_channels[] = {
  {
    .name = "Radius",
    .idname = "RADIUS",
    .min = 0.001f,
    .type = BRUSH_CHANNEL_FLOAT,
    .max = 2048.0f,
    .soft_min = 0.1f,
    .soft_max = 1024.0f,
    .mappings = {
        .pressure = {.curve = CURVE_PRESET_SMOOTH, .min = 0.0f, .max = 1.0f, .enabled = false},
    }
  },
    {
    .name = "Strength",
    .idname = "STRENGTH",
    .min = -1.0f,
    .type = BRUSH_CHANNEL_FLOAT,
    .max = 4.0f,
    .soft_min = 0.0f,
    .soft_max = 1.0f,
    .mappings = {
        .pressure = {.curve = CURVE_PRESET_SMOOTH, .min = 0.0f, .max = 1.0f, .enabled = true},
    }
  },
   {
    .name = "Spacing",
    .idname = "SPACING",
    .min = 0.001f,
    .type = BRUSH_CHANNEL_FLOAT,
    .max = 4.0f,
    .soft_min = 0.005f,
    .soft_max = 2.0f,
    .mappings = {
        .pressure = {.curve = CURVE_PRESET_SMOOTH, .min = 0.0f, .max = 1.0f, .enabled = true},
    }
  },
    {
    .name = "Autosmooth",
    .idname = "AUTOSMOOTH",
    .type = BRUSH_CHANNEL_FLOAT,
    .min = -1.0f,
    .max = 4.0f,
    .soft_min = 0.0f,
    .soft_max = 1.0f,
    .mappings = {
        .pressure = {.curve = CURVE_PRESET_SMOOTH, .min = 0.0f, .max = 1.0f, .enabled = false, .inv = true},
    }
  },
    {
    .name = "Topology Rake",
    .idname = "TOPOLOGY_RAKE",
    .type = BRUSH_CHANNEL_FLOAT,
    .min = -1.0f,
    .max = 4.0f,
    .soft_min = 0.0f,
    .soft_max = 1.0f,
    .mappings = {
        .pressure = {.curve = CURVE_PRESET_SMOOTH, .min = 0.0f, .max = 1.0f, .enabled = false},
    }
  },
  {
    .name = "Autosmooth Radius Scale",
    .idname = "AUTOSMOOTH_RADIUS_SCALE",
    .type = BRUSH_CHANNEL_FLOAT,
    .min = 0.0001f,
    .max = 25.0f,
    .soft_min = 0.1f,
    .soft_max = 4.0f,
    .mappings = {
        .pressure = {.curve = CURVE_PRESET_SMOOTH, .min = 0.0f, .max = 1.0f, .enabled = false},
    }
  },
  {
    .name = "Rake Radius Scale",
    .idname = "TOPOLOGY_RAKE_RADIUS_SCALE",
    .type = BRUSH_CHANNEL_FLOAT,
    .min = 0.0001f,
    .max = 25.0f,
    .soft_min = 0.1f,
    .soft_max = 4.0f,
    .mappings = {
        .pressure = {.curve = CURVE_PRESET_SMOOTH, .min = 0.0f, .max = 1.0f, .enabled = false},
    }
  },
  {
    .name = "Face Set Slide",
    .idname = "FSET_SLIDE",
    .type = BRUSH_CHANNEL_FLOAT,
    .min = 0.0001f,
    .max = 1.0f,
    .soft_min = 0.1f,
    .soft_max = 1.0f,
    .mappings = {
        .pressure = {.curve = CURVE_PRESET_SMOOTH, .min = 0.0f, .max = 1.0f, .enabled = false},
    }
  },
  {
    .name = "Boundary Smooth",
    .idname = "BOUNDARY_SMOOTH",
    .type = BRUSH_CHANNEL_FLOAT,
    .min = 0.0001f,
    .max = 1.0f,
    .soft_min = 0.1f,
    .soft_max = 1.0f,
    .mappings = {
        .pressure = {.curve = CURVE_PRESET_SMOOTH, .min = 0.0f, .max = 1.0f, .enabled = false},
    }
  },
   {
    .name = "Projection",
    .idname = "PROJECTION",
    .type = BRUSH_CHANNEL_FLOAT,
    .min = 0.0001f,
    .max = 1.0f,
    .soft_min = 0.1f,
    .soft_max = 1.0f,
    .mappings = {
        .pressure = {.curve = CURVE_PRESET_SMOOTH, .min = 0.0f, .max = 1.0f, .enabled = false},
    }
  },
  {
    .name = "Topology Rake Mode",
    .idname = "TOPOLOGY_RAKE_MODE",
    .type = BRUSH_CHANNEL_ENUM,
    .enumdef = {.items = {
      {0, "BRUSH_DIRECTION", ICON_NONE, "Stroke", "Stroke Direction"},
      {1, "CURVATURE", ICON_NONE, "Curvature", "Follow mesh curvature"},
      {-1, 0}
    }}
  },
   {
     .name = "Automasking",
     .idname = "AUTOMASKING",
     .flag = BRUSH_CHANNEL_INHERIT_IF_UNSET | BRUSH_CHANNEL_INHERIT,
     .type = BRUSH_CHANNEL_BITMASK,
     .enumdef = {.items = {
         {BRUSH_AUTOMASKING_BOUNDARY_EDGES, "BOUNDARY_EDGE", ICON_NONE, "Boundary Edges", ""},
         {BRUSH_AUTOMASKING_BOUNDARY_FACE_SETS, "BOUNDARY_FACE_SETS", ICON_NONE, "Boundary Face Sets", ""},
         {BRUSH_AUTOMASKING_CONCAVITY, "CONCAVITY", ICON_NONE, "Concave", ""},
         {BRUSH_AUTOMASKING_INVERT_CONCAVITY, "INVERT_CONCAVITY", ICON_NONE, "Invert Concave", "Invert Concave Map"},
         {BRUSH_AUTOMASKING_FACE_SETS, "FACE_SETS", ICON_NONE, "Face Sets", ""},
         {BRUSH_AUTOMASKING_TOPOLOGY, "TOPOLOGY", ICON_NONE, "Topology", ""}
      }}
   },
   {
     .name = "Disable Dyntopo",
     .idname = "DYNTOPO_DISABLED",
     .type = BRUSH_CHANNEL_INT,
     .flag = BRUSH_CHANNEL_NO_MAPPINGS,
     .ivalue = 0
   },
   {
     .name = "Detail Range",
     .idname = "DYNTOPO_DETAIL_RANGE",
     .type = BRUSH_CHANNEL_FLOAT,
     .min = 0.001,
     .max = 0.99,
     .ivalue = 0
   },
};

/* clang-format on */
const int builtin_channel_len = ARRAY_SIZE(brush_builtin_channels);

void BKE_brush_channel_free(BrushChannel *ch)
{
  for (int i = 0; i < BRUSH_MAPPING_MAX; i++) {
    BKE_curvemapping_free_data(&ch->mappings[i].curve);
  }
}

ATTR_NO_OPT void BKE_brush_channel_copy(BrushChannel *dst, BrushChannel *src)
{
  *dst = *src;

  for (int i = 0; i < BRUSH_MAPPING_MAX; i++) {
    BKE_curvemapping_copy_data(&dst->mappings[i].curve, &src->mappings[i].curve);
  }
}

ATTR_NO_OPT void BKE_brush_channel_init(BrushChannel *ch, BrushChannelType *def)
{
  memset(ch, 0, sizeof(*ch));

  strcpy(ch->name, def->name);
  strcpy(ch->idname, def->idname);

  ch->flag = def->flag;
  ch->fvalue = def->fvalue;
  ch->ivalue = def->ivalue;

  ch->def = def;

  for (int i = 0; i < BRUSH_MAPPING_MAX; i++) {
    BrushMapping *map = ch->mappings + i;
    CurveMapping *curve = &map->curve;

    memset(curve, 0, sizeof(*curve));

    float min, max;

    BrushMappingDef *mdef = (&def->mappings.pressure) + i;

    if (mdef->min != mdef->max) {
      min = mdef->min;
      max = mdef->max;
    }
    else {
      min = 0.0f;
      max = 1.0f;
    }

    if (mdef->inv) {
      ch->mappings[i].flag |= BRUSH_MAPPING_INVERT;
    }

    int slope = CURVEMAP_SLOPE_POSITIVE;

    for (int i = 0; i < 4; i++) {
      BKE_curvemap_reset(&curve->cm[i],
                         &(struct rctf){.xmax = 0, .ymax = min, .xmax = 1, .ymax = max},
                         mdef->curve,
                         slope);
    }

    BKE_curvemapping_init(curve);

    map->blendmode = mdef->blendmode;
    map->factor = 1.0f;

    if (mdef->enabled) {
      map->flag |= BRUSH_MAPPING_ENABLED;
    }
  }
}

BrushChannelSet *BKE_brush_channelset_create()
{
  return (BrushChannelSet *)MEM_callocN(sizeof(BrushChannelSet), "BrushChannelSet");
}

void BKE_brush_channelset_free(BrushChannelSet *chset)
{
  if (chset->channels) {
    for (int i = 0; i < chset->totchannel; i++) {
      BKE_brush_channel_free(chset->channels + i);
    }

    MEM_freeN(chset->channels);
  }
  MEM_freeN(chset);
}

void BKE_brush_channelset_add(BrushChannelSet *chset, BrushChannel *ch)
{
  chset->totchannel++;

  if (!chset->channels) {
    chset->channels = MEM_callocN(sizeof(BrushChannel) * chset->totchannel, "chset->channels");
  }
  else {
    chset->channels = MEM_recallocN(chset->channels, sizeof(BrushChannel) * chset->totchannel);
  }

  memcpy(chset->channels + chset->totchannel - 1, ch, sizeof(BrushChannel));
}

ATTR_NO_OPT BrushChannel *BKE_brush_channelset_lookup(BrushChannelSet *chset, const char *idname)
{
  for (int i = 0; i < chset->totchannel; i++) {
    if (STREQ(chset->channels[i].idname, idname)) {
      return chset->channels + i;
    }
  }

  return NULL;
}

bool BKE_brush_channelset_has(BrushChannelSet *chset, const char *idname)
{
  return BKE_brush_channelset_lookup(chset, idname) != NULL;
}

BrushChannelType brush_default_channel_type = {
    .name = "Channel",
    .idname = "CHANNEL",
    .min = 0.0f,
    .max = 1.0f,
    .soft_min = 0.0f,
    .soft_max = 1.0f,
    .type = BRUSH_CHANNEL_FLOAT,
    .flag = 0,
    .ivalue = 0,
    .fvalue = 0.0f,
    .mappings = {.pressure = {.curve = CURVE_PRESET_LINE,
                              .enabled = false,
                              .inv = false,
                              .blendmode = MA_RAMP_BLEND}}};

BrushChannelType *BKE_brush_default_channel_def()
{
  return &brush_default_channel_type;
}

void BKE_brush_channel_def_copy(BrushChannelType *dst, BrushChannelType *src)
{
  *dst = *src;
}

ATTR_NO_OPT BrushChannelType *BKE_brush_builtin_channel_def_find(const char *name)
{
  for (int i = 0; i < builtin_channel_len; i++) {
    BrushChannelType *def = brush_builtin_channels + i;

    if (STREQ(def->idname, name)) {
      return def;
    }
  }

  return NULL;
}

ATTR_NO_OPT void BKE_brush_channelset_add_builtin(BrushChannelSet *chset, const char *idname)
{
  BrushChannelType *def = BKE_brush_builtin_channel_def_find(idname);

  if (!def) {
    printf("%s: Could not find brush %s\n", __func__, idname);
    return;
  }

  BrushChannel ch;

  BKE_brush_channel_init(&ch, def);
  BKE_brush_channelset_add(chset, &ch);
}

bool BKE_brush_channelset_ensure_builtin(BrushChannelSet *chset, const char *idname)
{
  if (!BKE_brush_channelset_has(chset, idname)) {
    BKE_brush_channelset_add_builtin(chset, idname);
    return true;
  }

  return false;
}

#define ADDCH(name) BKE_brush_channelset_ensure_builtin(chset, name)
#define GETCH(name) BKE_brush_channelset_lookup(chset, name)

void BKE_brush_channelset_merge(BrushChannelSet *dst,
                                BrushChannelSet *child,
                                BrushChannelSet *parent)
{
  // first add missing channels

  for (int step = 0; step < 2; step++) {
    BrushChannelSet *chset = step ? parent : child;

    for (int i = 0; i < chset->totchannel; i++) {
      BrushChannel *ch = chset->channels + i;

      if (BKE_brush_channelset_has(dst, ch->idname)) {
        continue;
      }

      BrushChannel ch2;
      BKE_brush_channel_copy(&ch2, ch);
      BKE_brush_channelset_add(chset, &ch2);
    }
  }

  for (int i = 0; i < child->totchannel; i++) {
    BrushChannel *ch = child->channels + i;
    BrushChannel *mch = BKE_brush_channelset_lookup(dst, ch->idname);
    BrushChannel *pch = BKE_brush_channelset_lookup(parent, ch->name);

    bool ok = ch->flag & BRUSH_CHANNEL_INHERIT;

    if (ch->flag & BRUSH_CHANNEL_INHERIT) {
      BKE_brush_channel_free(mch);
      BKE_brush_channel_copy(mch, pch);
      continue;
    }

    if (ch->type == BRUSH_CHANNEL_BITMASK && (ch->flag & BRUSH_CHANNEL_INHERIT_IF_UNSET)) {
      mch->ivalue = ch->ivalue | pch->ivalue;
    }
  }
}

void BKE_brush_resolve_channels(Brush *brush, Sculpt *sd)
{
  if (brush->channels_final) {
    BKE_brush_channelset_free(brush->channels_final);
  }

  brush->channels_final = BKE_brush_channelset_create();

  BKE_brush_channelset_merge(brush->channels_final, brush->channels, sd->channels);

  if (!brush->commandlist) {
    return;
  }

  BrushCommandList *cl = brush->commandlist;

  for (int i = 0; i < cl->totcommand; i++) {
    BrushCommand *command = cl->commands + i;

    if (command->params_final) {
      BKE_brush_channelset_free(command->params_final);
    }

    command->params_final = BKE_brush_channelset_create();
    BKE_brush_channelset_merge(command->params_final, command->params, brush->channels_final);
  }
}

int BKE_brush_channel_get_int(BrushChannelSet *chset, char *idname)
{
  BrushChannel *ch = BKE_brush_channelset_lookup(chset, idname);

  if (!ch) {
    printf("%s, unknown channel %s", __func__, idname);
    return 0;
  }

  return ch->ivalue;
}

float BKE_brush_channel_get_float(BrushChannelSet *chset, char *idname)
{
  BrushChannel *ch = BKE_brush_channelset_lookup(chset, idname);

  if (!ch) {
    printf("%s, unknown channel %s", __func__, idname);
    return 0;
  }

  return ch->fvalue;
}

float BKE_brush_channel_set_float(BrushChannelSet *chset, char *idname, float val)
{
  BrushChannel *ch = BKE_brush_channelset_lookup(chset, idname);

  if (!ch) {
    printf("%s, unknown channel %s", __func__, idname);
    return 0;
  }

  float old = ch->fvalue;

  ch->fvalue = val;

  return old;
}

void BKE_brush_init_toolsettings(Sculpt *sd)
{
  BrushChannelSet *chset = sd->channels = BKE_brush_channelset_create();

  ADDCH("RADIUS");
  ADDCH("STRENGTH");
  ADDCH("AUTOMASKING");
  ADDCH("DYNTOPO_DISABLED");
  ADDCH("DYNTOPO_DETAIL_RANGE");
}

void BKE_brush_builtin_create(Brush *brush, int tool)
{
  if (brush->channels) {
    BKE_brush_channelset_free(brush->channels);
  }

  BrushChannelSet *chset = brush->channels = BKE_brush_channelset_create();

  ADDCH("RADIUS");
  ADDCH("STRENGTH");
  ADDCH("AUTOSMOOTH");
  ADDCH("TOPOLOGY_RAKE");
  ADDCH("AUTOSMOOTH_RADIUS_SCALE");
  ADDCH("TOPOLOGY_RAKE_RADIUS_SCALE");

  ADDCH("AUTOMASKING");

  switch (tool) {
    case SCULPT_TOOL_DRAW: {
      BrushChannel *ch = GETCH("STRENGTH");

      ch->mappings[BRUSH_MAPPING_PRESSURE].flag &= ~BRUSH_MAPPING_ENABLED;
      ch->flag = BRUSH_CHANNEL_INHERIT;
      break;
    }
    default: {
      // implement me!
      BKE_brush_channelset_free(chset);
      brush->channels = NULL;
      break;
    }
  }
}

BrushCommandList *BKE_brush_commandlist_create()
{
  return MEM_callocN(sizeof(BrushCommandList), "BrushCommandList");
}
void BKE_brush_commandlist_free(BrushCommandList *cl)
{
  for (int i = 0; i < cl->totcommand; i++) {
    BrushCommand *cmd = cl->commands + i;

    if (cmd->params) {
      BKE_brush_channelset_free(cmd->params);
    }

    if (cmd->params_final) {
      BKE_brush_channelset_free(cmd->params_final);
    }
  }

  MEM_SAFE_FREE(cl->commands);

  MEM_freeN(cl);
}
BrushCommand *BKE_brush_commandlist_add(BrushCommandList *cl)
{
  cl->totcommand++;

  if (!cl->commands) {
    cl->commands = MEM_callocN(sizeof(BrushCommand) * cl->totcommand, "BrushCommand");
  }
  else {
    cl->commands = MEM_recallocN(cl->commands, sizeof(BrushCommand) * cl->totcommand);
  }

  BrushCommand *cmd = cl->commands + cl->totcommand - 1;
  cmd->params = BKE_brush_channelset_create();
  cmd->params_final = NULL;

  return cmd;
}

BrushCommand *BKE_brush_command_init(BrushCommand *command, int tool)
{
  BrushChannelSet *chset = command->params;

  ADDCH("SPACING");

  switch (tool) {
    case SCULPT_TOOL_DRAW:
      ADDCH("RADIUS");
      ADDCH("STRENGTH");
      break;
    case SCULPT_TOOL_SMOOTH:
      ADDCH("RADIUS");
      ADDCH("STRENGTH");
      ADDCH("FSET_SLIDE");
      ADDCH("BOUNDARY_SMOOTH");
      ADDCH("PROJECTION");
      break;
    case SCULPT_TOOL_TOPOLOGY_RAKE:
      ADDCH("RADIUS");
      ADDCH("STRENGTH");
      // ADDCH("FSET_SLIDE");
      // ADDCH("BOUNDARY_SMOOTH");
      ADDCH("PROJECTION");
      ADDCH("TOPOLOGY_RAKE_MODE");
      break;
    case SCULPT_TOOL_DYNTOPO:
      break;
  }

  return command;
}

void BKE_builtin_commandlist_create(BrushChannelSet *chset, BrushCommandList *cl, int tool)
{
  BrushCommand *cmd;

  cmd = BKE_brush_commandlist_add(cl);
  BKE_brush_command_init(cmd, tool);

  for (int i = 0; i < cmd->totparam; i++) {
    // inherit from brush channels for main tool
    cmd->params->channels[i].flag |= BRUSH_CHANNEL_INHERIT;
  }

  float radius = BKE_brush_channel_get_float(chset, "RADIUS");
  float autosmooth_scale = BKE_brush_channel_get_float(chset, "AUTOSMOOTH_RADIUS_SCALE");

  float autosmooth = BKE_brush_channel_get_float(chset, "AUTOSMOOTH");
  if (autosmooth > 0.0f) {
    cmd = BKE_brush_command_init(BKE_brush_commandlist_add(cl), SCULPT_TOOL_SMOOTH);
    BKE_brush_channel_set_float(cmd->params, "STRENGTH", autosmooth);
    BKE_brush_channel_set_float(cmd->params, "RADIUS", radius * autosmooth_scale);
    BKE_brush_channel_set_float(
        cmd->params, "PROJECTION", BKE_brush_channel_get_float(chset, "AUTOSMOOTH_PROJECTION"));
  }
}

void BKE_brush_channelset_read(BlendDataReader *reader, BrushChannelSet *cset)
{
  BLO_read_data_address(reader, &cset->channels);

  for (int i = 0; i < cset->totchannel; i++) {
    BrushChannel *ch = cset->channels + i;

    for (int j = 0; j < BRUSH_MAPPING_MAX; j++) {
      BKE_curvemapping_blend_read(reader, &ch->mappings[j].curve);
    }

    ch->def = BKE_brush_builtin_channel_def_find(ch->idname);

    if (!ch->def) {
      printf("failed to find brush definition");
      ch->def = BKE_brush_default_channel_def();
    }
  }
}

void BKE_brush_channelset_write(BlendWriter *writer, BrushChannelSet *cset)
{
  BLO_write_struct(writer, BrushChannelSet, cset);
  BLO_write_struct_array_by_name(writer, "BrushChannel", cset->totchannel, cset->channels);

  for (int i = 0; i < cset->totchannel; i++) {
    BrushChannel *ch = cset->channels + i;

    for (int j = 0; j < BRUSH_MAPPING_MAX; j++) {
      BKE_curvemapping_blend_write(writer, &ch->mappings[j].curve);
    }
  }
}

/* clang-format on */

/* idea for building built-in preset node graphs:
from brush_builder import Builder;

def build(input, output):
  input.add("Strength", "float", "strength").range(0.0, 3.0)
  input.add("Radius", "float", "radius").range(0.01, 1024.0)
  input.add("Autosmooth", "float", "autosmooth").range(0.0, 4.0)
  input.add("Topology Rake", "float", "topology rake").range(0.0, 4.0)
  input.add("Smooth Radius Scale", "float", "autosmooth_radius_scale").range(0.01, 5.0)
  input.add("Rake Radius Scale", "float", "toporake_radius_scale").range(0.01, 5.0)

  draw = input.make.tool("DRAW")
  draw.radius = input.radius
  draw.strength = input.strength

  smooth = input.make.tool("SMOOTH")
  smooth.radius = input.radius * input.autosmooth_radius_scale
  smooth.strength = input.autosmooth;
  smooth.flow = draw.outflow

  rake = input.make.tool("TOPORAKE")
  rake.radius = input.radius * input.toporake_radius_scale
  rake.strength = input.topology;
  rake.flow = smooth.outflow

  output.out = rake.outflow

preset = Builder(build)

*/

/*
bNodeType sculpt_tool_node = {
  .idname = "SculptTool",
  .ui_name = "SculptTool",
};*/
/* cland-format on */