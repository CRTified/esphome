#include "pca9634_output.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace pca9634 {

static const char *TAG = "pca9634";

const uint8_t PCA9634_MODE1_SLEEP = 0x10;
const uint8_t PCA9634_MODE1_SUB1E = 0x08;
const uint8_t PCA9634_MODE1_SUB2E = 0x04;
const uint8_t PCA9634_MODE1_SUB3E = 0x02;
const uint8_t PCA9634_MODE1_ALLCE = 0x01;

const uint8_t PCA9634_MODE2_DMBLNK_BLNK = 0x20;
const uint8_t PCA9634_MODE2_INVRT = 0x10;
const uint8_t PCA9634_MODE2_OCH_ONACK = 0x08;
const uint8_t PCA9634_MODE2_OUTDRV_TOTEM_POLE = 0x04;
const uint8_t PCA9634_MODE2_OUTNE_LOW = 0x00;
const uint8_t PCA9634_MODE2_OUTNE_OUTDRV = 0x01;
const uint8_t PCA9634_MODE2_OUTNE_HIGHZ = 0x02;

const uint8_t PCA9634_LEDOUT_OFF = 0x00;
const uint8_t PCA9634_LEDOUT_ON = 0x01;
const uint8_t PCA9634_LEDOUT_INDIV = 0x02;
const uint8_t PCA9634_LEDOUT_GROUP = 0x03;
    
static const uint8_t PCA9634_ADDRESS_SOFTWARE_RESET = 0x03;
static const uint8_t PCA9634_ADDRESS_ALLCALL = 0x70;

// Configuration register
static const uint8_t PCA9634_REGISTER_MODE1 = 0x00;
static const uint8_t PCA9634_REGISTER_MODE2 = 0x01;
// First individual LED register
static const uint8_t PCA9634_REGISTER_LED0 = 0x02;
// Group PWM register, controlling the group duty cycle
static const uint8_t PCA9634_REGISTER_GRPPWM = 0x0A;
// Group frequency register, controlling the group blinking period
static const uint8_t PCA9634_REGISTER_GRPFREQ = 0x0B;
// Configuration registers for LEDs
static const uint8_t PCA9634_REGISTER_LEDOUT0 = 0x0C;
static const uint8_t PCA9634_REGISTER_LEDOUT1 = 0x0D;

// Registers to configure addresses
static const uint8_t PCA9634_REGISTER_SUBADDR1 = 0x0E;
static const uint8_t PCA9634_REGISTER_SUBADDR2 = 0x0F;
static const uint8_t PCA9634_REGISTER_SUBADDR3 = 0x10;
static const uint8_t PCA9634_REGISTER_ALLCALLADR = 0x11;

// Register modifier for autoincrements
static const uint8_t PCA9634_REGISTER_AUTOINC_NO = 0x00;
static const uint8_t PCA9634_REGISTER_AUTOINC_ALL = 0x80;
static const uint8_t PCA9634_REGISTER_AUTOINC_INDIV = 0xA0;
static const uint8_t PCA9634_REGISTER_AUTOINC_GLOBAL = 0xC0;
static const uint8_t PCA9634_REGISTER_AUTOINC_ALLCNTRL = 0xE0;
    
// Software reset sequence
static const uint8_t PCA9634_RESET_SEQUENCE[2] = {0xA5, 0x5A};

static bool global_init_done_ = false;
    
void PCA9634Output::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PCA9634OutputComponent...");

  if(!global_init_done_)
  {
    ESP_LOGV(TAG, "  Resetting devices...");
    // Backup the address
    uint8_t main_addr = this->address_;
    this->set_i2c_address(PCA9634_ADDRESS_SOFTWARE_RESET);
  
    
    if (!this->write_bytes_raw(PCA9634_RESET_SEQUENCE, 2)) {
      this->mark_failed();
      return;
    }
    // Restore address and wait for boot
    this->set_i2c_address(main_addr);
    
    global_init_done_ = true;
  }

  // Setup modes
  for(int tries = 0; tries < 10; tries++)
  {
    if (!this->write_byte(PCA9634_REGISTER_MODE1, this->mode1_ | PCA9634_MODE1_ALLCE)) {
  	continue;
    }
    if (!this->write_byte(PCA9634_REGISTER_MODE2, this->mode2_)) {
  	continue;
    }
    delayMicroseconds(500);
    this->loop();
    return;
  }
  
  this->mark_failed();
}

void PCA9634Output::dump_config() {
  ESP_LOGCONFIG(TAG, "PCA9634:");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02X", this->address_);
  ESP_LOGCONFIG(TAG, "  Mode1: 0x%02X", this->mode1_);
  ESP_LOGCONFIG(TAG, "  Mode2: 0x%02X", this->mode2_);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Setting up PCA9634 failed!");
  }
}

void PCA9634Output::loop() {
  if (!this->update_)
    return;

  
  uint8_t data[12];
  uint16_t ledout = 0;

  for (uint8_t channel = 0; channel <= 7; channel++) {
    ESP_LOGVV(TAG, "Channel %02u: amount=%04u", channel, this->pwm_amounts_[channel]);
    
    if(this->pwm_amounts_[channel] == 256)
    {
	data[channel] = 255;
	ledout |= (this->group_member_[channel] ? PCA9634_LEDOUT_GROUP : PCA9634_LEDOUT_ON) << (2 * channel);
    }
    else if (this->pwm_amounts_[channel] == 0)
    {
	data[channel] = 0;
	ledout |= PCA9634_LEDOUT_OFF << (2 * channel);
    }
    else
    {
	data[channel] = uint8_t(this->pwm_amounts_[channel]);
	ledout |= (this->group_member_[channel] ? PCA9634_LEDOUT_GROUP : PCA9634_LEDOUT_INDIV) << (2 * channel);
    }
  }
  ESP_LOGVV(TAG, "ledout: %04u", ledout);

  // Group control registers
  data[8] = uint8_t(this->pwm_amounts_[8]);
  data[9] = uint8_t(this->pwm_amounts_[9]);
  // LEDOUT register
  data[10] = 0xFF & ledout;
  data[11] = 0xFF & (ledout >> 8);
  
  // Update all individual LED registers and the group registers
  if(!this->write_bytes(PCA9634_REGISTER_AUTOINC_ALL | PCA9634_REGISTER_LED0, data, 12)) {
      this->status_set_warning();
  }

  this->status_clear_warning();
  this->update_ = false;
}

PCA9634Channel *PCA9634Output::create_channel(uint8_t channel, bool groupmem) {
  if(0 <= channel && channel < 8)
  {
    this->group_member_[channel] = groupmem;
  }
  auto *c = new PCA9634Channel(this, channel);
  return c;
}

void PCA9634Channel::write_state(float state) {
  const uint16_t max_duty = 256;
  const float duty_rounded = roundf(state * max_duty);
  auto duty = static_cast<uint16_t>(duty_rounded);
  this->parent_->set_channel_value_(this->channel_, duty);
}

}  // namespace pca9634
}  // namespace esphome
