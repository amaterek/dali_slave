/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "bccu_config.h"

const XMC_BCCU_GLOBAL_CONFIG_t kBCCUGlobalConfig = {
  trig_mode: XMC_BCCU_TRIGMODE0,
  trig_delay: XMC_BCCU_TRIGDELAY_NO_DELAY,
  maxzero_at_output: 256,
  fclk_ps: 346,
  bclk_sel: XMC_BCCU_BCLK_MODE_NORMAL,
  dclk_ps: 154,
  global_dimlevel: 0,
};

const XMC_BCCU_DIM_CONFIG_t kBCCUDimmingConfig = {
  dim_div: 0,
  dither_en: 0,
  cur_sel: XMC_BCCU_DIM_CURVE_FINE,
};

const XMC_BCCU_CH_CONFIG_t kLampBCCUChannelConfig = {
  pack_thresh: 3,
  pack_en: 0,
  dim_sel: XMC_BCCU_CH_DIMMING_SOURCE_GLOBAL,
  dim_bypass: XMC_BCCU_CH_DIMMING_ENGINE_BYPASS_DISABLE,
  gate_en: XMC_BCCU_CH_GATING_FUNC_DISABLE,
  flick_wd_en: XMC_BCCU_CH_FLICKER_WD_EN,
  trig_edge: XMC_BCCU_CH_TRIG_EDGE_PASS_TO_ACT,
  force_trig_en: 0,
  pack_offcmp_lev: 55,
  pack_oncmp_lev: 55,
  pack_offcnt_val: 0,
  pack_oncnt_val: 0,
};
