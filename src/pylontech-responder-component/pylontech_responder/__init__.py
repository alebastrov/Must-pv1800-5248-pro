import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import modbus, uart
from esphome.const import CONF_ID, CONF_UART_ID

pylontech_ns = cg.esphome_ns.namespace('pylontech')
PylontechResponder = pylontech_ns.class_('PylontechResponder', cg.Component)

DEPENDENCIES = ['modbus', 'uart']

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(PylontechResponder),
    cv.Required("modbus_id"): cv.use_id(modbus.Modbus),
    cv.Required(CONF_UART_ID): cv.use_id(uart.UARTComponent),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Pass the parent Modbus hub reference pointer
    parent_modbus = await cg.get_variable(config["modbus_id"])
    cg.add(var.set_modbus_parent(parent_modbus))

    # Pass the raw parent UART stream component reference pointer
    parent_uart = await cg.get_variable(config[CONF_UART_ID])
    cg.add(var.set_uart_parent(parent_uart))
