
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/mutex.h>

#define CW2217_RSENSE_MOHM (10)

#define CW2217_VOLTAGE_HIGH_REGISTER (0x02)
#define CW2217_VOLTAGE_LOW_REGISTER (0x03)

#define CW2217_SOC_HIGH_REGISTER (0x04)
#define CW2217_SOC_LOW_REGISTER (0x05)

#define CW2217_TEMPERATURE_REGISTER (0x06)

#define CW2217_CURRENT_HIGH_REGISTER (0x0E)
#define CW2217_CURRENT_LOW_REGISTER (0x0F)

typedef int cw2217_result_t;
typedef s8 cw2217_int8_t;
typedef u8 cw2217_uint8_t;
typedef s16 cw2217_int16_t;
typedef u16 cw2217_uint16_t;
typedef s32 cw2217_int32_t;
typedef u32 cw2217_uint32_t;
typedef s64 cw2217_int64_t;
typedef u64 cw2217_uint64_t;

typedef struct _cw2217_battery_t {
    struct i2c_client * i2c_client;
	struct power_supply * power_supply;
	struct mutex mutex;
    struct delayed_work poll_work;

    int voltage_level;
    int soc_level;
    int current_level;
    int temp_level;
} cw2217_battery_t;

//
// cw2217b.h
//
int cw2217_battery_get_property(struct power_supply * psy, enum power_supply_property psp, union power_supply_propval * val);
static const enum power_supply_property cw2217_powersupply_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_TEMP,
};
static const struct power_supply_desc cw2217_powersupply_desc = {
    .name = "BAT0",
	.type = POWER_SUPPLY_TYPE_BATTERY,
    .get_property = cw2217_battery_get_property,
    .properties = cw2217_powersupply_props,
	.num_properties	= ARRAY_SIZE(cw2217_powersupply_props),
};

//
// cw2217b-work.h
//
void cw2217_poll_workfn(struct work_struct * work);

//
// cw2217b-i2c.h
//
cw2217_result_t cw2217_i2c_probe(struct i2c_client * client);
void cw2217_i2c_remove(struct i2c_client * client);
cw2217_result_t cw2217_i2c_read_register(struct i2c_client * client, cw2217_uint8_t reg, cw2217_uint8_t * val);

//
// cw2217b.h
//
cw2217_result_t cw2217_battery_get_voltage(struct i2c_client * client, int * voltage);
cw2217_result_t cw2217_battery_get_soc(struct i2c_client * client, int * soc);
cw2217_result_t cw2217_battery_get_current(struct i2c_client * client, int * curr);
cw2217_result_t cw2217_battery_get_temperature(struct i2c_client * client, int * temperature);

//
// cw2217b.c
//
int cw2217_battery_get_property(struct power_supply * psy, enum power_supply_property psp, union power_supply_propval * val) {
    cw2217_battery_t * battery = power_supply_get_drvdata(psy);
    if (battery == NULL) {
		return -EINVAL;
    }

    mutex_lock(&battery->mutex);
    int voltage_level = battery->voltage_level;
    int soc_level = battery->soc_level;
    int current_level = battery->current_level;
    int temp_level = battery->temp_level;
	mutex_unlock(&battery->mutex);

    switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = current_level <= 0 ? POWER_SUPPLY_STATUS_DISCHARGING : POWER_SUPPLY_STATUS_CHARGING;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = soc_level;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = voltage_level;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = current_level;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = temp_level;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

//
// cw2217b-work.c
//
void cw2217_poll_workfn(struct work_struct * work) {
    cw2217_battery_t * battery = container_of(to_delayed_work(work), cw2217_battery_t, poll_work);

    if (battery == NULL) {
        dev_err(&battery->i2c_client->dev, "error: unable to get client data from delayed work!\n");
        return;
    }

    int voltage_level = 0;
    cw2217_result_t voltage_result = cw2217_battery_get_voltage(battery->i2c_client, &voltage_level);
    if (voltage_result) {
        voltage_level = 0;
    }

    int soc_level = 0;
    cw2217_result_t soc_result = cw2217_battery_get_soc(battery->i2c_client, &soc_level);
    if (soc_result) {
        soc_level = 0;
    }

    int current_level = 0;
    cw2217_result_t current_result = cw2217_battery_get_current(battery->i2c_client, &current_level);
    if (current_result) {
        current_level = 0;
    }

    int temperature_level = 0;
    cw2217_result_t temperature_result = cw2217_battery_get_temperature(battery->i2c_client, &temperature_level);
    if (temperature_result) {
        temperature_level = 0;
    }

    mutex_lock(&battery->mutex);
    battery->voltage_level = voltage_level;
    battery->soc_level = soc_level;
    battery->current_level = current_level;
    battery->temp_level = temperature_level;
	mutex_unlock(&battery->mutex);

    power_supply_changed(battery->power_supply);

    schedule_delayed_work(&battery->poll_work, msecs_to_jiffies(1000));
}

//
// cw2217b-i2c.c
//
cw2217_result_t cw2217_i2c_probe(struct i2c_client * client) {
    dev_info(&client->dev, "CC2217 I2C device probed at address 0x%02x\n", client->addr);

    // Allocate memory for battery management
    cw2217_battery_t * battery = kzalloc(sizeof(cw2217_battery_t), GFP_KERNEL);
    if (battery == NULL) {
        return -ENOMEM;
    }

    i2c_set_clientdata(client, battery);

    struct power_supply_config psy_cfg = {
        .drv_data = battery,
    };


    battery->i2c_client = client;
    battery->power_supply = power_supply_register(NULL, &cw2217_powersupply_desc, &psy_cfg);
    mutex_init(&battery->mutex);

    mutex_lock(&battery->mutex);
    battery->voltage_level = 0;
    battery->soc_level = 0;
    battery->current_level = 0;
    battery->temp_level = 0;
	INIT_DELAYED_WORK(&battery->poll_work, cw2217_poll_workfn);
	mutex_unlock(&battery->mutex);

	schedule_delayed_work(&battery->poll_work, 0);

    if (IS_ERR(battery->power_supply)) {
		int ret = PTR_ERR(battery->power_supply);

        i2c_set_clientdata(client, NULL);
        kfree(battery);

		return ret;
	} else {
        return 0;
    }
}
void cw2217_i2c_remove(struct i2c_client * client) {
    dev_info(&client->dev, "CC2217 I2C device removed\n");

    cw2217_battery_t * battery = (cw2217_battery_t *)i2c_get_clientdata(client);
    if (battery == NULL) {
        dev_info(&client->dev, "error: unable to get client data!\n");
        return;
    }

    if (battery->power_supply) {
        power_supply_unregister(battery->power_supply);
    }

    cancel_delayed_work_sync(&battery->poll_work);

    i2c_set_clientdata(client, NULL);
    kfree(battery);
}

cw2217_result_t cw2217_i2c_read_register(struct i2c_client * client, cw2217_uint8_t reg, cw2217_uint8_t * val) {
    if (client == NULL) {
        return -ENXIO;
    }

    int ret = i2c_smbus_read_byte_data(client, reg);
    if (ret < 0) {
        return ret;
    }

    cw2217_uint8_t value = ret & 0xFF;
    if (val == NULL) {
        return 0;
    }

    *val = value;
    return 0;
}

//
// cw2217b.c
//
cw2217_result_t cw2217_battery_get_voltage(struct i2c_client * client, int * voltage) {
    // Request voltage high value
    cw2217_uint8_t voltage_high = 0x00;
    cw2217_result_t voltage_high_result = cw2217_i2c_read_register(client, CW2217_VOLTAGE_HIGH_REGISTER, &voltage_high);
    if (voltage_high_result) {
        dev_err(&client->dev, "Unable to read voltage high register from CW2217 (%i)\n", voltage_high_result);

        return voltage_high_result;
    }

    // Request voltage low value
    cw2217_uint8_t voltage_low = 0x00;
    cw2217_result_t voltage_low_result = cw2217_i2c_read_register(client, CW2217_VOLTAGE_LOW_REGISTER, &voltage_low);
    if (voltage_low_result) {
        dev_err(&client->dev, "Unable to read voltage low register from CW2217 (%i)\n", voltage_low_result);

        return voltage_low_result;
    }

    // Safe values according with datasheet
    cw2217_uint8_t voltage_high_s = voltage_high & 0x3F;
    cw2217_uint8_t voltage_low_s = voltage_low & 0xFF;

    // Calculate voltage
    cw2217_uint16_t raw_voltage = ((cw2217_uint16_t)voltage_high_s << 8) | ((cw2217_uint16_t)voltage_low_s << 0);
    cw2217_uint64_t calculated_voltage = (cw2217_uint64_t)raw_voltage * 625 / 2;

    if (calculated_voltage > (cw2217_uint64_t)INT_MAX) {
        *voltage = INT_MAX;
    } else {
        *voltage = (int)calculated_voltage;
    }

    return 0;
}
cw2217_result_t cw2217_battery_get_soc(struct i2c_client * client, int * soc) {
    // Request SoC high value
    cw2217_uint8_t soc_high = 0x00;
    cw2217_result_t soc_high_result = cw2217_i2c_read_register(client, CW2217_SOC_HIGH_REGISTER, &soc_high);
    if (soc_high_result) {
        dev_err(&client->dev, "Unable to read soc high register from CW2217 (%i)\n", soc_high_result);

        return soc_high_result;
    }

    // Request SoC low value
    // cw2217_uint8_t soc_low = 0x00;
    // cw2217_result_t soc_low_result = cw2217_i2c_read_register(client, CW2217_SOC_LOW_REGISTER, &soc_low);
    // if (soc_low_result) {
    //     dev_err(&client->dev, "Unable to read soc low register from CW2217 (%i)\n", soc_low_result);
    //
    //     return soc_low_result;
    // }

    // Calculate SoC
    int calculated_soc = (int)soc_high;

    if (calculated_soc > 100) {
        *soc = 100;
    } else {
        *soc = calculated_soc;
    }

    return 0;
}
cw2217_result_t cw2217_battery_get_current(struct i2c_client * client, int * curr) {
    // Request current high value
    cw2217_uint8_t current_high = 0x00;
    cw2217_result_t current_high_result = cw2217_i2c_read_register(client, CW2217_CURRENT_HIGH_REGISTER, &current_high);
    if (current_high_result) {
        dev_err(&client->dev, "Unable to read current high register from CW2217 (%i)\n", current_high_result);

        return current_high_result;
    }

    // Request current low value
    cw2217_uint8_t current_low = 0x00;
    cw2217_result_t current_low_result = cw2217_i2c_read_register(client, CW2217_CURRENT_LOW_REGISTER, &current_low);
    if (current_low_result) {
        dev_err(&client->dev, "Unable to read current low register from CW2217 (%i)\n", current_low_result);

        return current_low_result;
    }

    // Calculate current
    cw2217_int16_t raw_current = ((cw2217_int16_t)current_high << 8) | ((cw2217_uint16_t)current_low << 0);

    cw2217_int64_t num = (cw2217_int64_t)raw_current * (cw2217_int64_t)52400000;
    cw2217_int64_t denom = (cw2217_int64_t)32768 * (cw2217_int64_t)(CW2217_RSENSE_MOHM);
    num += (num >= 0) ? (denom / 2) : -(denom / 2);

    int calculated_current = (int)(num / denom);

    *curr = calculated_current;

    return 0;
}

cw2217_result_t cw2217_battery_get_temperature(struct i2c_client * client, int * temp) {
    // Request temperature value
    cw2217_uint8_t temperature_value = 0x00;
    cw2217_result_t temperature_result = cw2217_i2c_read_register(client, CW2217_TEMPERATURE_REGISTER, &temperature_value);
    if (temperature_result) {
        dev_err(&client->dev, "Unable to read temperature register from CW2217 (%i)\n", temperature_result);

        return temperature_result;
    }

    // Calculate temperature
    cw2217_uint64_t calculated_temperature = (cw2217_uint64_t)0 - (cw2217_uint64_t)400 + (cw2217_uint64_t)5 * (cw2217_uint64_t)temperature_value;

    *temp = (int)calculated_temperature;

    return 0;
}


static const struct i2c_device_id cw2217_i2c_device_id[] = {
    { "cw2217b", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, cw2217_i2c_device_id);

static struct i2c_driver cw2217_i2c_driver = {
    .driver = {
        .name = "cw2217b",
    },
    .probe = cw2217_i2c_probe,
    .remove = cw2217_i2c_remove,
    .id_table = cw2217_i2c_device_id,
};
module_i2c_driver(cw2217_i2c_driver);

MODULE_AUTHOR("Diego Fernandez <diego@diegofer.com>");
MODULE_AUTHOR("Ralf Miunske <rbm78bln@github.com>");
MODULE_DESCRIPTION("Linux power_supply driver for Cellwise CW2217B I2C battery fuel gauge with mains and battery support");
MODULE_LICENSE("GPL");
