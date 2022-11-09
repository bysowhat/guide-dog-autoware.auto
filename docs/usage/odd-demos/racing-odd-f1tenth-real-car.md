F1Tenth Demonstration {#f1tenth-demo-real-car}
=================================================

@tableofcontents

# Setup Vehicle {#f1tenth-real-car}

## Use a Stock F1Tenth Car

If you do not have a working F1Tenth vehicle yet, head over to [F1Tenth build instructions](https://f1tenth.org/build.html) for a bill of materials and detailed build procedures. You need to complete the following sections in the build instructions:

- Building the F1TENTH Car
- Configure F1TENTH System
- Install F1TENTH Software (Configuring the VESC, and Hokuyo Setup), skipping ROS Workspace Setup

## Build Your Own

It is okay to have custom parts such as:

- RC chassis, with servo and BLDC motor compatable with VESC;
- VESC, any version configured properly to control both motor and servo;
- LiDAR, as long as it has a ROS2 driver;
- Onboard computer (ARM64 or X64 Linux with Docker), preferably more powerful than the Jetson Nano board; Jetson Xavier NX and AGX are recommended.

## Remote Desktop

Since Rviz is used in the demo, you need a remote desktop solution on the Jetson. This demo was tested with [No Machine](https://www.nomachine.com/). You need a dummy HDMI pluged into the Jetson's HDMI port while driving. If you haven't got one yet, proceed with the rest of the setup by connecting your Jetson to a monitor.

1.  Download and install MoMachine on both Jetson (ARM64) and your host computer.
2.  Make sure the two machines are under the same local network. Create a new remote desktop connection from your host computer by entering the Jetson's host name or IP.

# F1Tenth RecordReplay Trajectory Demo

## Preparing Autoware on Jetson

### Using Custom LiDAR

If you have custom LiDAR:

1. Check the content of `AutowareAuto/.aderc-jetson-f1tenth`. You would most likely need to modify some device mounting options when launching the container, if using ADE;
2. Check the content of `AutowareAuto/autoware.auto.foxy.repos`. Add the URL to the ROS2 driver for your LiDAR (put branch name under `version`);
3. Add your lidar package as a dependency to `AutowareAuto/src/launch/f1tenth_launch/package.xml`;
4. Add your lidar node in `AutowareAuto/src/launch/f1tenth_launch/launch/f1tenth_vehicle_vesc.launch.py`; `LaserScan` topic name should be `scan`, `frame_id` should be `lidar`, and the node namespace should be `lidar`.

### Option 1: Installing with ADE Container

Using ADE container will give you a quick start and clean system. Expect a performance decrease especially with RVIZ.

1. Follow the standard [Autoware installation with ADE](https://autowarefoundation.gitlab.io/autoware.auto/AutowareAuto/installation-ade.html) but stop before executing `ade start --update --enter`.
    - Note when downloading ADE: you need to check the ADE's GitLab release page and `wget` the correct binary for AArch64 than what is listed on the ADE website for X86_64.
2. Start the ADE container: `cd adehome/AutowareAuto && ade --rc .aderc-jetson-f1tenth start --update --enter`

The ADE container comes with everything pre-built. If you prefer using your own build inside the ADE container, follow these:

```{bash}
# import external dependencies
ade$ cd AutowareAuto
ade$ vcs import . < autoware.auto.foxy.repos
ade$ sudo apt update; rosdep update; rosdep install --from-paths src --ignore-src --rosdistro ${ROS_DISTRO} -yr

# build Autoware
ade$ colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release --packages-up-to f1tenth_launch
```

### Option 2: Installing with native build

For optimal performance, build Autoware locally. Follow the [build instructions without ADE](https://autowarefoundation.gitlab.io/autoware.auto/AutowareAuto/installation-no-ade.html). When executing `colcon build`, use `--packages-up-to f1tenth_launch` and `--symlink-install` to skip unnecessary packages and change parameteres easily. The build time is approximately 15-30 minutes on a Jetson Xavier NX.

## Creating Map

```{bash}
# (Terminal 1)
$ ade enter
ade$ source ~/AutowareAuto/install/setup.bash
ade$ ros2 launch f1tenth_launch f1tenth_mapping_demo.launch.py with_joy:=True vehicle_interface:=vesc
```

Press the right shoulder button to start driving. Visualize the map building process in Rviz.

Save the map for AMCL by running:

```{bash}
# (Terminal 2)
$ ade enter
ade$ mkdir ${HOME}/map
ade$ ros2 run nav2_map_server map_saver_cli -f $HOME/map/mymap
```
Alternatively, save the map for SLAM by running:

```{bash}
# (Terminal 2)
ade$ mkdir ${HOME}/map
ade$ ros2 service call /slam_toolbox/serialize_map slam_toolbox/srv/SerializePoseGraph "{filename: "$HOME/map/mymap"}"
```

This will save a pose graph for `slam_toolbox`'s localization.

After saving maps, stop mapping nodes by pressing `Ctrl+C` in terminal 1.

## Recording and replaying trajectory
### Record a trajectory

```{bash}
# (Terminal 1)
$ ade enter
ade$ source AutowareAuto/install/setup.bash

# add 'map:=/path/to/map.yaml' to select your original map
ade$ ros2 launch f1tenth_launch f1tenth_recordreplay_demo.launch.py with_joy:=True vehicle_interface:=vesc map:=$HOME/map/mymap
```

Press the right shoulder button to start driving. Set a intial pose with correct orientation in Rviz using `2D pose estimate`

Now start recording:

```{bash}
# (Terminal 2)
ade$ source /opt/AutowareAuto/setup.bash
ade$ ros2 action send_goal /planning/recordtrajectory autoware_auto_planning_msgs/action/RecordTrajectory "{record_path: "/tmp/path"}" --feedback
```
Drive around the vehicle and stop recording with Ctrl + C in terminal 2.

### Replay a trajectory

```{bash}
# (Terminal 1)
$ ade enter
ade$ source AutowareAuto/install/setup.bash

# add 'localization:='slam'' to use slam_toolbox for localization (default is amcl)
# add 'map:=/path/to/map.yaml' (AMCL) or 'map:=/path/to/map.posegraph' (SLAM) to select your original map
ade$ ros2 launch f1tenth_launch f1tenth_recordreplay_demo.launch.py with_joy:=True vehicle_interface:=vesc map:=$HOME/map/mymap
```
Press the right shoulder button to start driving.

Set a intial pose with correct orientation in Rviz using `2D pose estimate`.

Press the right shoulder button again to go to autonomous mode.

Now start replaying:

```{bash}
(Terminal 2)
ade$ source /opt/AutowareAuto/setup.bash
ade$ ros2 action send_goal /planning/replaytrajectory autoware_auto_planning_msgs/action/ReplayTrajectory "{replay_path: "/tmp/path"}" --feedback
```

To stop the car, press the right shoulder button again to switch back to manual mode.

### Running in a Loop

To let the vehicle run continuously in a loop, first record a trajectory finished with a stop position just behind the start position. Modify Autoware/src/launch/f1tenth_launch/param/recordreplay_planner.param.yaml by setting `loop_trajectory` to `True`. Build and run the replay.

## Known issues
The replayed velocity might not be exactly the same with the recorded one. This is due to limitation of `Pure Pursuit` algorithm. The implementation does not take delay and external force into consideration, which means that it assumes constant speed when acceleration is released even if break is not pressed. This causes velocity of the vehicle to be wrong. Improvements will be made in the future.

# Troubleshoot

## No Map in Rviz; LiDAR Message Dropped

X button needs to be pressed once for the map to show in Rviz when building the map or recording/replaying the trajectory.

If you have custom LiDAR with its own ROS2 driver, make sure the `LaserScan`'s `frame_id` is `lidar`, topic name is `scan`, and the node namespace is `lidar`. If the driver outputs `PointCloud2`, use the conversion node to republish it as a `LaserScan`.

## Terrible Map; Odometry Drifting

Please make sure there are enough obstacles around the track for the LiDAR. For example, an outdoor playground with a few cones would usually not work, Whereas an indoor space with nearby walls would be preferable.

A map is often bad due to uncalibrated odometry, although slight difference is acceptable. You can visualize the vehicle odometry by adding a visualization in Rviz for `/vehicle/odom`. Check the trajectory of the vehicle, and look for flipped travel direction and/or flipped steering direction. Correct them in the following two files.

Your vehicle odometry may need to be recalibrated, and values put into `AutowareAuto/src/launch/f1tenth_launch/param/vesc_to_odom_node.param.yaml` and `AutowareAuto/src/launch/f1tenth_launch/param/vesc_config.param.yaml`. Refer to the F1Tenth documentation for calibrating odometry (Driving the F1TENTH Car -> Calibrating the Odometry).

## Vehicle not Moving When Replaying

Press the right shoulder button to stop manual joystick output. Otherwise the vehicle would not move.

If the car starting position is too close to the final trajectory point and the looping feature is off, the replay goal would be reached instantly and the car will not drive.