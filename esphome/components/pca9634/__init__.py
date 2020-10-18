import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_FREQUENCY, CONF_ID, CONF_INVERTED

DEPENDENCIES = ['i2c']
MULTI_CONF = True

pca9634_ns = cg.esphome_ns.namespace('pca9634')
PCA9634Output = pca9634_ns.class_('PCA9634Output', cg.Component, i2c.I2CDevice)

CONF_OUTDRV = "structure"
CONF_OUTDRV_OPTIONS = {"opendrain" : False, "totempole": True}

CONF_OUTCHANGE = "changeon"
CONF_OUTCHANGE_OPTIONS = {"stop" : False, "ack": True}


CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(PCA9634Output),
    cv.Optional(CONF_INVERTED, False): cv.boolean,
    cv.Optional(CONF_OUTDRV, "opendrain"): cv.enum(CONF_OUTDRV_OPTIONS, lower=True),
    cv.Optional(CONF_OUTCHANGE, "stop"): cv.enum(CONF_OUTCHANGE_OPTIONS, lower=True)
}).extend(cv.COMPONENT_SCHEMA).extend(i2c.i2c_device_schema(None))


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID],
                           config[CONF_INVERTED],
                           config[CONF_OUTDRV],
                           config[CONF_OUTCHANGE],
                           )
    yield cg.register_component(var, config)
    yield i2c.register_i2c_device(var, config)
