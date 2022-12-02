from esphome.cpp_generator import RawExpression
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID
from . import (
    Q8RFController, CONF_Q8RFController_ID
)

DEPENDENCIES = ['q8rf_zone_controller']

CONF_Q8RF_DEVICE_ID = "q8rf_device_id"
CONF_Q8RF_ZONE_ID = "q8rf_zone_id"
CONF_Q8RF_RESEND_INTERVAL = "q8rf_resend_interval"
CONF_Q8RF_TURN_ON_WATCHDOG_INTERVAL = "q8rf_turn_on_watchdog_interval"

q8rf_ns = cg.esphome_ns.namespace("q8rf")
q8rf_SWITCH = q8rf_ns.class_("Q8RFSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = (
    switch.SWITCH_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(q8rf_SWITCH),
        cv.GenerateID(CONF_Q8RFController_ID): cv.use_id(Q8RFController),
    }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(
        cv.Schema(
            {
                cv.Required(CONF_Q8RF_DEVICE_ID): cv.hex_uint16_t,
                cv.Required(CONF_Q8RF_ZONE_ID): cv.int_range(min=1, max=4),
                cv.Optional(CONF_Q8RF_RESEND_INTERVAL): cv.uint32_t,
                cv.Optional(CONF_Q8RF_TURN_ON_WATCHDOG_INTERVAL): cv.uint32_t
            }
        )
    )
)


async def to_code(config):

    controller = await cg.get_variable(config[CONF_Q8RFController_ID])

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    cg.add(var.set_q8rf_device_id(config[CONF_Q8RF_DEVICE_ID]))
    cg.add(var.set_q8rf_zone_id(config[CONF_Q8RF_ZONE_ID]))
    cg.add(var.set_controller(controller))
    cg.add(var.setup())

    if CONF_Q8RF_RESEND_INTERVAL in config:
        cg.add(var.set_q8rf_resend_interval(config[CONF_Q8RF_RESEND_INTERVAL]))
    if CONF_Q8RF_TURN_ON_WATCHDOG_INTERVAL in config:
        cg.add(
            var.set_q8rf_turn_on_watchdog_interval(
                config[CONF_Q8RF_TURN_ON_WATCHDOG_INTERVAL]
            )
        )