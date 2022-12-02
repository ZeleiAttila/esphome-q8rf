import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID
from . import (
    Q8RFController, CONF_Q8RFController_ID
)

DEPENDENCIES = ['q8rf_zone_controller']

CONF_q8rf_DEVICE_ID = "q8rf_device_id"
CONF_q8rf_ZONE_ID = "q8rf_zone_id"

q8rf_ns = cg.esphome_ns.namespace("q8rf")
q8rf_button = q8rf_ns.class_("Q8RFButton", button.Button, cg.Component)

CONFIG_SCHEMA = (
    button.BUTTON_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(q8rf_button),
        cv.GenerateID(CONF_Q8RFController_ID): cv.use_id(Q8RFController), })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(
        cv.Schema(
            {
                cv.Required(CONF_q8rf_DEVICE_ID): cv.hex_uint16_t,
                cv.Required(CONF_q8rf_ZONE_ID): cv.int_range(min=1, max=4),
            }
        )
    )
)


async def to_code(config):
    controller = await cg.get_variable(config[CONF_Q8RFController_ID])

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await button.register_button(var, config)

    cg.add(var.set_q8rf_device_id(config[CONF_q8rf_DEVICE_ID]))
    cg.add(var.set_q8rf_zone_id(config[CONF_q8rf_ZONE_ID]))
    cg.add(var.set_controller(controller))
    
