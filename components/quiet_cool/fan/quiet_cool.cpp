#include "quiet_cool.h"
#include "esphome/core/log.h"
#include "quietcool.h"

namespace esphome {
    namespace quiet_cool {
        
        static const char *TAG = "quiet_cool.fan";

        void QuietCoolFan::setup() {
            if (!this->pins_set_) {
                ESP_LOGE(TAG, "QuietCool pins not configured via YAML; radio not initialised");
                return;
            }

            if (this->qc_ == nullptr) {
                // Use standard VSPI pins (CLK18, MISO19, MOSI23) for ESP32 dev boards
                this->qc_.reset(new QuietCool(this->csn_pin_, this->gdo0_pin_, this->gdo2_pin_, 18, 19, 23, remote_id_.data(), center_freq_mhz, deviation_khz));
            }

            this->qc_->begin();
            ESP_LOGD(TAG, "QuietCool initialized");
        }

        fan::FanTraits QuietCoolFan::get_traits() {
            return fan::FanTraits(false, true, false, 3);
        }

        void QuietCoolFan::control(const fan::FanCall &call) {
            float inc_speed = call.get_speed().value_or(-1.0f);
            ESP_LOGD(TAG, "Control called: state=%s, speed=%s", 
                     call.get_state().has_value() ? (*call.get_state() ? "ON" : "OFF") : "<unchanged>",
                     call.get_speed().has_value() ? (std::to_string(inc_speed)).c_str() : "<unchanged>");
            bool old_state = this->state;
            if (call.get_state().has_value())
                this->state = *call.get_state();

            QuietCoolSpeed qcspd = QUIETCOOL_SPEED_LOW;
            QuietCoolDuration qcdur = QUIETCOOL_DURATION_ON;
            if (call.get_speed().has_value()) {
                this->speed_ = *call.get_speed();
                if (this->speed_ < 0.5) qcdur = QUIETCOOL_DURATION_OFF;
                else if (this->speed_ < 1.5) qcspd = QUIETCOOL_SPEED_LOW;
                else if (this->speed_ < 2.5) qcspd = QUIETCOOL_SPEED_MEDIUM;
                else if (this->speed_ < 3.5) qcspd = QUIETCOOL_SPEED_HIGH;
            } else {
		qcdur = QUIETCOOL_DURATION_OFF;
	    }
            if (this->qc_) this->qc_->send(qcspd, qcdur);


            ESP_LOGV(TAG, "Post-update internal state: state=%s speed=%s", 
                     (this->state ? "ON" : "OFF"),
                     (std::to_string(this->speed_)).c_str());

            this->write_state_();
            this->publish_state();
        }
	// NEW: Setter for duration
	void QuietCoolFan::set_current_duration(QuietCoolDuration duration) {
  		// 1. Store the new duration value in a member variable
  		this->current_duration_ = duration; 

  		// 2. Send the combined command (current speed + new duration) via the RF module
  		if (this->quiet_cool_instance_ != nullptr) { // Make sure the RF module is initialized
		    // Assuming you have a member variable 'this->current_speed_'
		    // that stores the fan's current speed (Low, Medium, High).
		    // The 'send' method of your QuietCool object takes both speed and duration.
    			this->quiet_cool_instance_->send(this->current_speed_, this->current_duration_);
    
		    // Optional: Log for debugging
		    ESP_LOGD("quiet_cool_fan", "Set duration to: %d. Sending command with speed: %d", 
		             (int)this->current_duration_, (int)this->current_speed_);
		  } else {
		    ESP_LOGE("quiet_cool_fan", "QuietCool RF module not initialized when setting duration.");
		  }
	}
	// Updated control() method to use current_duration_
        void QuietCoolFan::write_state_() {
            ESP_LOGVV(TAG, "write_state_: driving pins: state=%s ", 
                      (this->state ? "ON" : "OFF"));
            ESP_LOGVV(TAG, "write_state_: output calls completed");
        }

        void QuietCoolFan::dump_config() { LOG_FAN("", "QuietCool fan", this); }
    }  // namespace quiet_cool
}  // namespace esphome
