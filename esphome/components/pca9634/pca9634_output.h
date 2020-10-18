#pragma once

#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace pca9634 {

// Sleep mode, disables oscillator
extern const uint8_t PCA9634_MODE1_SLEEP;
// Enable Subaddress 1
extern const uint8_t PCA9634_MODE1_SUB1E;
// Enable Subaddress 2
extern const uint8_t PCA9634_MODE1_SUB2E;
// Enable Subaddress 3
extern const uint8_t PCA9634_MODE1_SUB3E;
// Enable All Call address
extern const uint8_t PCA9634_MODE1_ALLCE;

// Set group control to blinking instead of group duty cycle control
extern const uint8_t PCA9634_MODE2_DMBLNK_BLNK;
/// Inverts polarity of channel output signal
extern const uint8_t PCA9634_MODE2_INVRT;
/// Channel update happens upon ACK (post-set) rather than on STOP (endTransmission)
extern const uint8_t PCA9634_MODE2_OCH_ONACK;
/// Use a totem-pole (push-pull) style output rather than an open-drain structure.
extern const uint8_t PCA9634_MODE2_OUTDRV_TOTEM_POLE;
    
extern const uint8_t PCA9634_MODE2_OUTNE_LOW;
extern const uint8_t PCA9634_MODE2_OUTNE_OUTDRV;
extern const uint8_t PCA9634_MODE2_OUTNE_HIGHZ;

class PCA9634Output;

class PCA9634Channel : public output::FloatOutput {
 public:
    PCA9634Channel(PCA9634Output *parent, uint8_t channel) :
	parent_(parent), channel_(channel) {}

 protected:
  void write_state(float state) override;

  PCA9634Output *parent_;
  uint8_t channel_;
};

/// PCA9634 float output component.
class PCA9634Output : public Component, public i2c::I2CDevice {
 public:
    PCA9634Output(
	bool invert,
	bool outdrv,
	bool outchange,
	uint8_t mode1 = 0
        )
	: mode1_(mode1)
    {
	this->mode2_ = 0;
	if (invert)    this->mode2_ |= PCA9634_MODE2_INVRT;
	if (outdrv)    this->mode2_ |= PCA9634_MODE2_OUTDRV_TOTEM_POLE;
	if (outchange) this->mode2_ |= PCA9634_MODE2_OCH_ONACK;
    }

    PCA9634Channel *create_channel(uint8_t channel, bool groupmem);

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  void loop() override;

 protected:
  friend PCA9634Channel;

  void set_channel_value_(uint8_t channel, uint16_t value) {
    if (this->pwm_amounts_[channel] != value)
      this->update_ = true;
    this->pwm_amounts_[channel] = value;
  }

  uint8_t mode1_, mode2_;
  uint16_t ledout_{0};
    
  uint16_t pwm_amounts_[10] = {
      0,
  };
  bool group_member_[8] = {
      0,
  };
  
  bool update_{true};
};

}  // namespace pca9634
}  // namespace esphome
