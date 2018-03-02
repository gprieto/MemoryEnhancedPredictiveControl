#ifndef _CAR_CONTROL_PLUGIN_HH_
#define _CAR_CONTROL_PLUGIN_HH_

#include <boost/bind.hpp>
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/common/common.hh>
#include <gazebo/math/Vector3.hh>
#include <gazebo/math/Pose.hh>
#include <gazebo/math/Quaternion.hh>

#include <stdio.h>

#include <thread>
#include <ros/ros.h>
#include <ros/callback_queue.h>
#include <utils/Command.h>

#include <math.h>

#define PI 3.14159265

namespace gazebo

{
  class CarControlPlugin : public ModelPlugin
  {

  	public: CarControlPlugin() {}

    public: virtual void Load(physics::ModelPtr _model, sdf::ElementPtr _sdf)
    {
			this->model = _model;

			this->velocity = math::Vector3();
			//this->velocity.Set(_sdf->Get<double>("X_velocity"), _sdf->Get<double>("Y_velocity"), _sdf->Get<double>("Z_velocity"));
			this->SetVelocity(this->velocity);

			this->orientation = math::Vector3();
			this->orientation.Set(0,0,0);

			this->rosNode.reset(new ros::NodeHandle("car_control"));
			this->rosSub = this->rosNode->subscribe("command", 10 , &CarControlPlugin::OnRosMsg, this);
			this->rosQueueThread = std::thread(std::bind(&CarControlPlugin::QueueThread, this));
			
			time_step = 0.001;
    }

    public: void SetVelocity(const gazebo::math::Vector3 &_vel)
    {
			this->model->SetLinearVel(_vel);
			gzmsg << "Linear velocity set to: " << _vel.GetLength() << "\n";
    }

    /*public: void SetOrientation(const float &_theta)
    {
    	this->orientation.Set(0,0,_theta *PI/180.0);
    	math::Pose initPose(this->model->GetWorldPose().pos, math::Quaternion(0, 0, _theta *PI/180.0));
			this->model->SetWorldPose(initPose);
			gzmsg << "Orientation set to: " << _theta << "\n";
    }*/
    
    public: void SetAngularVelocity(const float &_omega)
    {
    	this->model->SetAngularVel({0, 0, _omega});
    	gzmsg << "Angular velocity set to: " << _omega << "\n";
    }

    public: void OnRosMsg(const utils::CommandConstPtr &_msg)
	{
		//gazebo::common:Time gz_time_now = _model->GetWorld()->GetSimTime();
		//ros_time = ros::Time(gz_time_now.sec, gz_time_now.nsec);
		
		//ros::Duration time_step = ros_time - old_ros_time;
		
		//this->SetOrientation(_msg->theta);
		this->SetAngularVelocity(_msg->theta);
		
		math::Quaternion pose = this->model->GetWorldPose().rot;
		double yaw = pose.GetYaw();
		//double yaw_next = yaw + _msg->theta * time_step;
		
		this->velocity.Set(-_msg->velocity*sin(yaw), _msg->velocity*cos(yaw), 0);
    this->SetVelocity(this->velocity);
	}

    /// \brief ROS helper function that processes messages
	private: void QueueThread()
	{
	  static const double timeout = 0.01;
	  while (this->rosNode->ok())
	  {
	    this->rosQueue.callAvailable(ros::WallDuration(timeout));
	  }
	}

	private: gazebo::math::Vector3 velocity;
	private: gazebo::math::Vector3 orientation;
  private: physics::ModelPtr model;
	private: std::unique_ptr<ros::NodeHandle> rosNode;
	private: ros::Subscriber rosSub;
	private: ros::CallbackQueue rosQueue;
	private: std::thread rosQueueThread;
	private: double time_step;

  };

  GZ_REGISTER_MODEL_PLUGIN(CarControlPlugin)
}

#endif