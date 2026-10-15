import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

# Define the C++ namespace and class name
pylontech_ns = cg.esphome_ns.namespace('pylontech')
PylontechResponder = pylontech_ns.class_('PylontechResponder', cg.Component, uart.UARTDevice)

DEPENDENCIES = ['uart']

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(PylontechResponder),
}).extend(uart.UART_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
