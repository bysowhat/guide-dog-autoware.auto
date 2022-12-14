// Copyright 2021 The Autoware Foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "vesc_interface/vesc_interface.hpp"

#include <iostream>

namespace autoware
{
namespace vesc_interface
{
using VSC = autoware_auto_vehicle_msgs::msg::VehicleStateCommand;
VESCInterface::VESCInterface(
  rclcpp::Node & node,
  float64_t speed_to_erpm_gain,
  float64_t speed_to_erpm_offset,
  float64_t max_erpm_positive_delta,
  float64_t max_erpm_negative_delta,
  float64_t steering_to_servo_gain,
  float64_t steering_to_servo_offset
)
: m_logger{node.get_logger()},
  speed_to_erpm_gain_{speed_to_erpm_gain},
  speed_to_erpm_offset_{speed_to_erpm_offset},
  max_erpm_positive_delta_{max_erpm_positive_delta},
  max_erpm_negative_delta_{max_erpm_negative_delta},
  steering_to_servo_gain_{steering_to_servo_gain},
  steering_to_servo_offset_{steering_to_servo_offset}
{
  // create publishers to vesc electric-RPM (speed) and servo commands
  erpm_pub_ = node.create_publisher<Float64>("commands/motor/speed", 10);
  servo_pub_ = node.create_publisher<Float64>("commands/servo/position", 10);

  // Subscribers to data from VESC
  vesc_motor_state_ = node.create_subscription<VescStateStamped>(
    "sensors/core",
    10,
    [this](VescStateStamped::SharedPtr msg) {on_motor_state_report(msg);}
  );

  servo_state_ = node.create_subscription<Float64>(
    "sensors/servo_position_command",
    10,
    [this](Float64::SharedPtr msg) {on_servo_state_report(msg);}
  );
}

bool8_t VESCInterface::update(std::chrono::nanoseconds timeout)
{
  (void)timeout;
  return true;
}

bool8_t VESCInterface::send_state_command(const VehicleStateCommand & msg)
{
  // If the GEAR is in reverse, set direction to -1
  if (msg.gear == VSC::GEAR_REVERSE) {
    direction = -1;
  }

  if (msg.gear == VSC::GEAR_DRIVE || msg.gear == VSC::GEAR_LOW) {
    direction = 1;
  }

  return true;
}

bool8_t VESCInterface::send_control_command(const VehicleControlCommand & msg)
{
  static float64_t prev_erpm_value = 0.0;
  if (msg.velocity_mps == 0.0f || !run_autonomous) {
    seen_zero_speed = true;
  } else {
    seen_zero_speed = false;
  }

  // calc vesc electric RPM (speed)
  Float64 erpm_msg;
  if (seen_zero_speed) {
    erpm_msg.data = 0.0f;
  } else {
    erpm_msg.data = speed_to_erpm_gain_ * static_cast<double>(msg.velocity_mps) +
      speed_to_erpm_offset_;
  }

  // limit vesc electric RPM
  erpm_msg.data =
    (erpm_msg.data <
    prev_erpm_value + max_erpm_positive_delta_) ? erpm_msg.data : prev_erpm_value +
    max_erpm_positive_delta_;
  erpm_msg.data =
    (erpm_msg.data >
    prev_erpm_value - max_erpm_negative_delta_) ? erpm_msg.data : prev_erpm_value -
    max_erpm_negative_delta_;
  prev_erpm_value = erpm_msg.data;

  // calc steering angle (servo)
  Float64 servo_msg;
  if (run_autonomous) {
    servo_msg.data = steering_to_servo_gain_ *
      static_cast<double>(msg.front_wheel_angle_rad) + steering_to_servo_offset_;
  } else {
    servo_msg.data = steering_to_servo_offset_;
  }


  if (rclcpp::ok()) {
    erpm_pub_->publish(erpm_msg);
    servo_pub_->publish(servo_msg);
    return true;
  }
  return false;
}

bool8_t VESCInterface::send_control_command(const AckermannControlCommand & msg)
{
  VehicleControlCommand vehicle_control_command_msg;
  vehicle_control_command_msg.velocity_mps = msg.longitudinal.speed;
  vehicle_control_command_msg.front_wheel_angle_rad = msg.lateral.steering_tire_angle;
  vehicle_control_command_msg.stamp = msg.stamp;
  return send_control_command(vehicle_control_command_msg);
}

/// TODO(jacobjj): Add comments
bool8_t VESCInterface::handle_mode_change_request(
  autoware_auto_vehicle_msgs::srv::AutonomyModeChange_Request::SharedPtr request)
{
  if (request->mode == ModeChangeRequest::MODE_MANUAL) {
    run_autonomous = false;
  } else if (request->mode == ModeChangeRequest::MODE_AUTONOMOUS) {
    run_autonomous = true;
  } else {
    RCLCPP_ERROR(m_logger, "Got invalid autonomy mode request value.");
    return false;
  }
  return true;
}

bool8_t VESCInterface::send_control_command(const RawControlCommand & msg)
{
  (void)msg;
  RCLCPP_WARN(m_logger, "Cannot control the VESC using RawControlCommand");
  return false;
}

void VESCInterface::on_motor_state_report(const VescStateStamped::SharedPtr & msg)
{
  float64_t current_speed = (-msg->state.speed - speed_to_erpm_offset_) / speed_to_erpm_gain_;
  if (std::fabs(current_speed) < 0.05) {
    current_speed = 0.0;
  }
  odometry().velocity_mps = static_cast<float>(current_speed);

  if (last_servo_cmd) {
    float64_t current_front_wheel_angle(0.0);
    current_front_wheel_angle = (last_servo_cmd->data - steering_to_servo_offset_) /
      steering_to_servo_gain_;
    odometry().front_wheel_angle_rad = static_cast<float>(current_front_wheel_angle);
  }
}

void VESCInterface::on_servo_state_report(const Float64::SharedPtr & msg)
{
  last_servo_cmd = msg;
}
}  // namespace vesc_interface
}  // namespace autoware
