/******************************************************************************
 * Copyright 2020 The Firmament Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include <Controller.h>
#include <firmament.h>

#include "module/log/mlog.h"
#include "module/param/param.h"

#define TAG "Controller"

/* controller input topic */
MCN_DECLARE(fms_output);
MCN_DECLARE(ins_output);

/* controller output topic */
MCN_DEFINE(control_output, sizeof(Control_Out_Bus));

/* define parameters */
static param_t __param_list[] = {
    PARAM_FLOAT(VEL_XY_P, 1.4),
    PARAM_FLOAT(VEL_XY_I, 0.2),
    PARAM_FLOAT(VEL_XY_D, 0.2),
    PARAM_FLOAT(VEL_Z_P, 0.6),
    PARAM_FLOAT(VEL_Z_I, 0.1),
    PARAM_FLOAT(VEL_Z_D, 0.0),
    PARAM_FLOAT(VEL_XY_I_MIN, -1),
    PARAM_FLOAT(VEL_XY_I_MAX, 1),
    PARAM_FLOAT(VEL_XY_D_MIN, -1),
    PARAM_FLOAT(VEL_XY_D_MAX, 1),
    PARAM_FLOAT(VEL_Z_I_MIN, -0.15),
    PARAM_FLOAT(VEL_Z_I_MAX, 0.15),
    PARAM_FLOAT(VEL_Z_D_MIN, -0.1),
    PARAM_FLOAT(VEL_Z_D_MAX, 0.1),
    PARAM_FLOAT(ROLL_P, 5),
    PARAM_FLOAT(PITCH_P, 5),
    PARAM_FLOAT(ROLL_PITCH_CMD_LIM, PI / 6),
    PARAM_FLOAT(ROLL_RATE_P, 0.08),
    PARAM_FLOAT(PITCH_RATE_P, 0.08),
    PARAM_FLOAT(YAW_RATE_P, 0.15),
    PARAM_FLOAT(ROLL_RATE_I, 0.1),
    PARAM_FLOAT(PITCH_RATE_I, 0.1),
    PARAM_FLOAT(YAW_RATE_I, 0.2),
    PARAM_FLOAT(ROLL_RATE_D, 0.002),
    PARAM_FLOAT(PITCH_RATE_D, 0.002),
    PARAM_FLOAT(YAW_RATE_D, 0.001),
    PARAM_FLOAT(RATE_I_MIN, -0.1),
    PARAM_FLOAT(RATE_I_MAX, 0.1),
    PARAM_FLOAT(RATE_D_MIN, -0.1),
    PARAM_FLOAT(RATE_D_MAX, 0.1),
    PARAM_FLOAT(P_Q_CMD_LIM, PI / 2),
    PARAM_FLOAT(R_CMD_LIM, PI),
};
PARAM_GROUP_DEFINE(CONTROL, __param_list);

/* define log data */
static mlog_elem_t Control_Out_Elems[] = {
    MLOG_ELEMENT(timestamp, MLOG_UINT32),
    MLOG_ELEMENT_VEC(actuator_cmd, MLOG_UINT16, 16),
};
MLOG_BUS_DEFINE(Control_Out, Control_Out_Elems);

static McnNode_t fms_out_nod;
static McnNode_t ins_out_nod;

static int Control_Out_ID;

fmt_model_info_t control_model_info;

static int control_out_echo(void* param)
{
    Control_Out_Bus control_out;
    if (mcn_copy_from_hub((McnHub*)param, &control_out) == FMT_EOK) {
        console_printf("timestamp:%d actuator: %d %d %d %d\n", control_out.timestamp, control_out.actuator_cmd[0],
            control_out.actuator_cmd[1], control_out.actuator_cmd[2], control_out.actuator_cmd[3]);
    }
    return 0;
}

static void init_parameter(void)
{
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "VEL_XY_P"), &CONTROL_PARAM.VEL_XY_P));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "VEL_XY_I"), &CONTROL_PARAM.VEL_XY_I));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "VEL_XY_D"), &CONTROL_PARAM.VEL_XY_D));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "VEL_Z_P"), &CONTROL_PARAM.VEL_Z_P));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "VEL_Z_I"), &CONTROL_PARAM.VEL_Z_I));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "VEL_Z_D"), &CONTROL_PARAM.VEL_Z_D));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "VEL_XY_I_MIN"), &CONTROL_PARAM.VEL_XY_I_MIN));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "VEL_XY_I_MAX"), &CONTROL_PARAM.VEL_XY_I_MAX));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "VEL_XY_D_MIN"), &CONTROL_PARAM.VEL_XY_D_MIN));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "VEL_XY_D_MAX"), &CONTROL_PARAM.VEL_XY_D_MAX));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "VEL_Z_I_MIN"), &CONTROL_PARAM.VEL_Z_I_MIN));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "VEL_Z_I_MAX"), &CONTROL_PARAM.VEL_Z_I_MAX));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "VEL_Z_D_MIN"), &CONTROL_PARAM.VEL_Z_D_MIN));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "VEL_Z_D_MAX"), &CONTROL_PARAM.VEL_Z_D_MAX));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "ROLL_P"), &CONTROL_PARAM.ROLL_P));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "PITCH_P"), &CONTROL_PARAM.PITCH_P));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "ROLL_PITCH_CMD_LIM"), &CONTROL_PARAM.ROLL_PITCH_CMD_LIM));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "ROLL_RATE_P"), &CONTROL_PARAM.ROLL_RATE_P));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "PITCH_RATE_P"), &CONTROL_PARAM.PITCH_RATE_P));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "YAW_RATE_P"), &CONTROL_PARAM.YAW_RATE_P));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "ROLL_RATE_I"), &CONTROL_PARAM.ROLL_RATE_I));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "PITCH_RATE_I"), &CONTROL_PARAM.PITCH_RATE_I));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "YAW_RATE_I"), &CONTROL_PARAM.YAW_RATE_I));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "ROLL_RATE_D"), &CONTROL_PARAM.ROLL_RATE_D));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "PITCH_RATE_D"), &CONTROL_PARAM.PITCH_RATE_D));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "YAW_RATE_D"), &CONTROL_PARAM.YAW_RATE_D));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "RATE_I_MIN"), &CONTROL_PARAM.RATE_I_MIN));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "RATE_I_MAX"), &CONTROL_PARAM.RATE_I_MAX));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "RATE_D_MIN"), &CONTROL_PARAM.RATE_D_MIN));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "RATE_D_MAX"), &CONTROL_PARAM.RATE_D_MAX));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "P_Q_CMD_LIM"), &CONTROL_PARAM.P_Q_CMD_LIM));
    FMT_CHECK(param_link_object(param_get_by_full_name("CONTROL", "R_CMD_LIM"), &CONTROL_PARAM.R_CMD_LIM));
}

void control_interface_step(uint32_t timestamp)
{
    if (mcn_poll(fms_out_nod)) {
        mcn_copy(MCN_HUB(fms_output), fms_out_nod, &Controller_U.FMS_Out);
    }

    if (mcn_poll(ins_out_nod)) {
        mcn_copy(MCN_HUB(ins_output), ins_out_nod, &Controller_U.INS_Out);
    }

    Controller_step();

    mcn_publish(MCN_HUB(control_output), &Controller_Y.Control_Out);

    DEFINE_TIMETAG(control_output, 100);
    /* Log Control output bus data */
    if (check_timetag(TIMETAG(control_output))) {
        /* Log Control out data */
        mlog_push_msg((uint8_t*)&Controller_Y.Control_Out, Control_Out_ID, sizeof(Control_Out_Bus));
    }
}

void control_interface_init(void)
{
    control_model_info.period = CONTROL_EXPORT.period;
    control_model_info.info = (char*)CONTROL_EXPORT.model_info;

    mcn_advertise(MCN_HUB(control_output), control_out_echo);

    fms_out_nod = mcn_subscribe(MCN_HUB(fms_output), NULL, NULL);
    ins_out_nod = mcn_subscribe(MCN_HUB(ins_output), NULL, NULL);

    Control_Out_ID = mlog_get_bus_id("Control_Out");
    FMT_ASSERT(Control_Out_ID >= 0);

    Controller_init();

    init_parameter();
}