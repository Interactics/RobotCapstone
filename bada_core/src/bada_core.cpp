#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/Pose2D.h>
#include <geometry_msgs/Point.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Float64.h>
#include <std_msgs/Int16.h>
#include <std_msgs/Bool.h>
#include <std_msgs/String.h>
#include <std_msgs/Empty.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <actionlib/client/simple_action_client.h>
#include <tf/transform_listener.h>
#include <math.h>

const double PI{3.141592};

//---------------- 노드 변경할 것 ----------------------------------------
/*
	TODO
	3. 각 상태마다 디스플레이에 이미지 표현하도록. 변경
	*/
//================================================================================

enum STATE
{
	FINDING_PEPL,
	ROAMING,
	SOUND_DETECTING,
	MOVING_TO_PEPL,
	MOVING_WITH_PEPL
};

enum DISP_EVNT
{
	NOTHING = 0,
	A_UP,
	A_DOWN,
	A_LEFT,
	A_RIGHT,
	FIRE_EVENT,
	WATER_EVENT,
	DOOR_EVENT,
	BELL_EVENT,
	BOILING_EVENT,
	CRYING_EVENT
};

struct Position
{
	double x;
	double y;
	double orien_z;
	double orien_w;
};

typedef std_msgs::Bool BoolMsg;
typedef std_msgs::String StringMsg;

std_msgs::Bool HEAD_STATUS;
std_msgs::Int16 MAT_STATUS;

void bada_set_state(STATE &present_state, STATE target_state);
void bada_next_state(STATE &present_state);
void bada_roaming();
void bada_send_destination(double x, double y, double orien_z, double orien_w); //맵 위치 x,y quaternion z,w 를 설정해주고 그 위치로 이동							// 배회하나 소리가 나면 다음으로 넘어간다.
void bada_go_destination_blocking(double duration, double x, double y, double orien_z, double orien_w);
void bada_save_current_position();					 // calculate the person's position on map from robot position using detected angle and theta.
bool bada_rounding();								 // 회전하며 사람이 있는지를 검사한다.
void bada_head_UP_cmd(bool STATUS);					 // 카메라달린 모터 위로 들기 for 사람 위치 확인용
void getCurrentRobotPositionTODO();					 // get current transform position(pose, quaternion) of robot
void bada_change_pos(float LinePos, float AnglePos); // 로봇에게 직선거리 혹은 회전 명령 주기
													 // 특정 위치만큼만 이동하기.
													 /* 리니어, 앵귤러에 도달할 때까지 회전하도록하기. 기본 속도는 정해져있다. 
													   보내는 것은 cmd_vel, 받는 것은 오도메트리 정보. */
void bada_aligned_pepl();							 // 사람 찾고 로봇과 사람 위치 정렬하기.
void go_until_touch();								 // 버튼이 눌릴 때까지 전진.
void bada_go_to_pepl();
void bada_go_until_touch();
void bada_save_sound_PT(); //로봇의 현재 위치와 소리나는 방향 저장하기.
void bada_go_to_soundPT(); // 소리가 발생한 지점으로 이동하기.
void bada_display_inform();
void bada_emotion();

void bada_wait_button();

void bada_save_sound_odom();
void bada_go_to_sound(); //소리 발생하는 방향으로 충분히 이동하기.
Position bada_get_robot_pos();

void bada_open_eyes_cmd(bool Status);									// Open Eyes Function.
void bada_display_cmd(DISP_EVNT status);								// Display Command
void bada_vel_cmd(const float XLineVel = 0, const float ZAngleVel = 0); // commendation of Publishing Velocity
void bada_log(std::string str1);										// commendation of Publishing Velocity
bool bada_go_to_sound2();												//로봇 base_link 기준, 소리 나는 방향보고 1m 전진

void waitSec(float sec);

ros::Publisher pub_cmdvel;
ros::Publisher pub_camera;
ros::Publisher pub_eyes_open;
ros::Publisher pub_head_up;
ros::Publisher pub_display_cmd;
ros::Publisher pub_logger;
ros::Publisher pub_pose;

ros::Subscriber sub_odometry;
ros::Subscriber sub_pepl_checker;
ros::Subscriber sub_sig_checker;
ros::Subscriber sub_switch_checker;
ros::Subscriber sub_signal;
ros::Subscriber sub_sound_localization;

/*--------------------------------------Callback----------------------------------------------*/

void sub_pepl_checker_callback(const geometry_msgs::Point &msg);
void sub_odometry_callback(const nav_msgs::Odometry &msg);
void sub_sig_checker_callback(const std_msgs::Empty &msg);					 // Roaming 단계에서 사용. 소리가 발생할 경우에 쓸모가 있다.
void sub_switch_checker_callback(const std_msgs::Bool &msgs);				 // Roaming 단계에서 사용. 소리가 발생할 경우에 쓸모가 있다.
void sub_signal_callback(const std_msgs::String &msg);						 //
void sub_sound_localization_callback(const geometry_msgs::PoseStamped &msg); //

//==============================================================================================

//사람이 일정 ROI에 들어오는 것을 검사함. 만약 ROI에 들어온다면, Checker는 True로 바뀜.

bool SIG_CHECK = false;	   // Roaming 단계에서 사용.
bool SWITCH_CHECK = false; // is Switch on?  T/F
bool PPL_CHECK = false;	   // Is there PPL?
float PPL_ANGLE = -90;	   // Angle of PPL respect to camera
float PPL_DIST = -1.0;	   // Distnace to PPL from

int CURRENT_POINT = 0;		//ROAMING CURRNET POINT

//geometry_msgs::Pose2D PERSON_POSITION;
nav_msgs::Odometry CURRENT_ROBOT_POSITION;
geometry_msgs::PoseStamped CURRENT_SOUND_DIRECTION;
std_msgs::String SIGNAL;
std_msgs::String LastSignal;

Position SAVED_SOUND_POSITION = {0.0f, 1.0f, 2.0f, 3.0f};
Position SAVED_HUMAN_POSITION = {0, 1, 2, 3};

double wayPoint[][4] = {
	{3.293, 1.023, 0.028, 1.000}, //way1
	{-0.097, 0.548, 1.000, -0.006},	//way2
	{-0.854, -1.400, -0.716, 0.699}	//way3
};									//roaming 장소 저장

typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveBaseClient;
MoveBaseClient *actionClient;

// sound position
// aligning: cam up
// button - twice?
// 

int main(int argc, char **argv)
{
	STATE state = FINDING_PEPL;
	ros::init(argc, argv, "bada_core");
	ROS_INFO("starting bada_core... 06010352");

	ros::NodeHandle nh;
	pub_cmdvel = nh.advertise<geometry_msgs::Twist>("/cmd_vel", 1);
	pub_camera = nh.advertise<std_msgs::Bool>("/bada/duino/camera_cmd", 1);
	pub_eyes_open = nh.advertise<std_msgs::Bool>("/bada/eyes/open", 1);
	pub_head_up = nh.advertise<std_msgs::Bool>("/bada/duino/camera_cmd", 1);
	pub_display_cmd = nh.advertise<std_msgs::Int16>("/bada/duino/display_cmd", 1);
	pub_logger = nh.advertise<std_msgs::String>("/bada/log", 1);
	// pub_pose = nh.advertise<geometry_msgs::PoseStamped>("/bada/pose", 1);

	// sub_odometry           = nh.subscribe("/bada/odom", 1, sub_odometry_callback);
	sub_pepl_checker = nh.subscribe("/bada/eyes/distance", 1, sub_pepl_checker_callback); //TODO: FIX CALLBACK FUNCTION
	sub_sig_checker = nh.subscribe("/bada/audio/checker", 1, sub_sig_checker_callback);
	sub_signal = nh.subscribe("/bada/audio/signal", 1, sub_signal_callback);
	sub_switch_checker = nh.subscribe("Button_State", 1, sub_switch_checker_callback);
	sub_sound_localization = nh.subscribe("/bada/audio/localization_filtered", 1, sub_sound_localization_callback);

	bada_log("bada started");
	// std_msgs::String msg1;
	// msg1.data="what";
	// pub_logger.publish(msg1);

	actionClient = new MoveBaseClient("move_base", true); //move_base client 선언

	bool is_there_pepl = false;
	bool is_sound_same = false;

	int current = 0;
	int target;

	//bada_aligned_pepl(); // 사람의 위치고 로봇 사람을 가운데로
	//waitSec(2);
	
	// bada_roaming();
	// return 0;

	while (ros::ok())
	{
		switch (state)
		{
		case FINDING_PEPL:
			ROS_INFO("starting FINDING_PEOPLE");
			do
			{
				target = current % 3;
				// TODO: wait until action client success
				bada_go_destination_blocking(50.0, wayPoint[target][0], wayPoint[target][1], wayPoint[target][2], wayPoint[target][3]); // Go to POINT of ROOM
				bada_head_UP_cmd(true);																									// HEAD_UP
				is_there_pepl = bada_rounding();
				bada_head_UP_cmd(false); // HEAD_DOWN
				if (is_there_pepl)
				{	ROS_INFO("break");
					break;
				}
				current++;
			} while (ros::ok());
			bada_next_state(state);
			break;
		case ROAMING:
			ROS_INFO("starting ROAMING");
			// bada_display_cmd(DISP_EVNT::A_UP);
			// waitSec(1);
			// bada_display_cmd(DISP_EVNT::NOTHING);
			bada_roaming(); // 카테고리에 등록된 소리가 나올 때까지 배회하기

			bada_display_cmd(DISP_EVNT::A_DOWN);
			waitSec(1);
			bada_display_cmd(DISP_EVNT::NOTHING);
			ROS_INFO("roaming done");
			LastSignal = SIGNAL;
			// if(LastSignal.data == "Cry"){
			// 	bada_display_cmd(DISP_EVNT::CRYING_EVENT);
			// } else if(LastSignal.data == "Alarm") {
			// 	bada_display_cmd(DISP_EVNT::FIRE_EVENT);
			// }
			waitSec(1);
			bada_display_cmd(DISP_EVNT::NOTHING);
			/////save souud pos
			bada_save_sound_odom();

			bada_next_state(state);
			break;
		case SOUND_DETECTING:
			ROS_INFO("pass go to sound");
			bada_next_state(state);
			break;

			// ROS_INFO("starting SOUND_DETECTING");
			// // 소리 난 방향 서브스크라이빙하기
			// // 소리 발생한 방향으로 이동하기
			// // 저장하기
			// // TODO:
			// is_sound_same = bada_go_to_sound2(); //소리 발생하는 방향으로 충분히 이동하기.

			// if (!is_sound_same)
			// {
			// 	ROS_INFO("sound detecting different");
			// 	bada_set_state(state, STATE::ROAMING);
			// 	break;
			// }
			// bada_save_sound_odom();
			// // bada_save_sound_PT();					//로봇의 현재 위치와 소리나는 방향 저장하기.
			// bada_next_state(state);
			// ROS_INFO("sound detecting same");
			// break;
		case MOVING_TO_PEPL:

			bada_display_cmd(DISP_EVNT::A_UP);
			waitSec(1);
			bada_display_cmd(DISP_EVNT::NOTHING);
			ROS_INFO("starting MOVING_TO_PEPL");
			waitSec(1);
			bada_go_to_pepl(); // 반경 2m 이내 도달 검사하기. 그렇지 않으면 계속 접근
			ROS_INFO("go to ppl done");
			waitSec(1);
			bada_aligned_pepl(); // 사람의 위치고 로봇 사람을 가운데로
			ROS_INFO("align ppl done");
			bada_go_until_touch(); // 버튼 눌리기 전까지 전진하기
			ROS_INFO("touched. moving back.");
			bada_change_pos(-0.3, 0); // 뒤로 1m 이동

			//bada_change_pos(0, PI); // 180도 회전
			ROS_INFO("change pos done");
			bada_next_state(state);
			break;
		case MOVING_WITH_PEPL:
			bada_display_cmd(DISP_EVNT::A_DOWN);
			//waitSec(1);
			
			ROS_INFO("starting MOVING_WITH_PEPL");
			waitSec(1);
			ROS_INFO("waiting for button input");
			bada_wait_button(); // 버튼 눌리기 전까지 대기하기.
			bada_display_cmd(DISP_EVNT::FIRE_EVENT);
			ROS_INFO("wait button done");
			bada_go_to_soundPT(); // 소리가 발생한 지점으로 이동하기.
			ROS_INFO("go to sound pt done");
			bada_display_inform(); // 해당 지점에서 소리
			bada_display_cmd(DISP_EVNT::NOTHING);
			bada_emotion();		   // 완료함 표현하기.
			ROS_INFO("finished");
			//bada_next_state(state);
			return 0;
			break;
		} // END_SWITCH
		ros::spinOnce();
	} // END_WHILE
} // END_MAIN

void bada_set_state(STATE &present_state, STATE target_state)
{
	int temp_int = int(target_state);
	present_state=static_cast<STATE>(temp_int);
}

void bada_next_state(STATE &present_state)
{
	int temp_int = int(present_state);
	STATE temp_state;
	if (temp_int != int(MOVING_WITH_PEPL))
	{
		temp_int++;
		temp_state = static_cast<STATE>(temp_int);
		present_state = temp_state;
	}
	else if (temp_int != int(MOVING_WITH_PEPL))
	{
		present_state = ROAMING;
	}
}

// send simple goal
void bada_send_destination(double x, double y, double orien_z, double orien_w)
{ //맵 위치 x,y quaternion z,w 를 설정해주고 그 위치로 이동
	//- ACTION MSG 퍼블리시한다.
	// ROS_INFO("waiting for move base");
	actionClient->waitForServer();
	// ROS_INFO("connected to move base");
	move_base_msgs::MoveBaseGoal goal;

	goal.target_pose.header.frame_id = "map";
	goal.target_pose.header.stamp = ros::Time::now();

	goal.target_pose.pose.position.x = x;
	goal.target_pose.pose.position.y = y;
	goal.target_pose.pose.orientation.z = orien_z;
	goal.target_pose.pose.orientation.w = orien_w;

	actionClient->sendGoal(goal);
	return;
}

void bada_go_destination_blocking(double duration, double x, double y, double orien_z, double orien_w)
{ //맵 위치 x,y quaternion z,w 를 설정해주고 그 위치로 이동
	//- ACTION MSG 퍼블리시한다.
	// ROS_INFO("waiting for move base");
	actionClient->waitForServer();
	// ROS_INFO("connected to move base");
	move_base_msgs::MoveBaseGoal goal;

	goal.target_pose.header.frame_id = "map";
	goal.target_pose.header.stamp = ros::Time::now();

	goal.target_pose.pose.position.x = x;
	goal.target_pose.pose.position.y = y;
	goal.target_pose.pose.orientation.z = orien_z;
	goal.target_pose.pose.orientation.w = orien_w;

	actionClient->sendGoal(goal);

	//http://wiki.ros.org/actionlib_tutorials/Tutorials/SimpleActionClient
	bool finished_before_timeout = actionClient->waitForResult(ros::Duration(duration));

	if (finished_before_timeout)
	{
		actionlib::SimpleClientGoalState state = actionClient->getState();
		ROS_INFO("Action finished: %s", state.toString().c_str());
	}
	else
		ROS_INFO("Action did not finish before the time out.");

	return;
}

// void bada_save_current_position(){ //호출되면 로봇 현재 위치 저장
// 	// TODO: use realsense topic to get angle and theta
// 	tf::StampedTransforms tranform;
// 	geometry_msgs::Pose2D pose2d;

// 	distance, theta=REALSENSEANGLETHETATODO();
// 	// theta: radian
// s	tranform=getCurrentRobotPositionTODO();
// 	double robotX=transform.pose.x;
// 	double robotY=transform.pose.y;

// 	tf::Matrix3x3 m(q);
// 	// https://gist.github.com/marcoarruda/f931232fe3490b7fa20dbb38da1195ac
// 	double roll, pitch, yaw;
// 	m.getRPY(roll, pitch, yaw);

//     pose2d.theta = yaw+theta;
// 	double deltaX=distance*cos(pose2d.theta);
// 	double deltaY=distance*sin(pose2d.theta);

//     pose2d.x = robotX+deltaX;
//     pose2d.y = robotY+deltaY;

// 	return pose2d;
// }

// void getCurrentRobotPositionTODO(){
// 	ros::NodeHandle n;
// 	tf::TransformListener listener;
// 	while(n.ok())
// 	{
// 		tf::StampedTransform transform;
// 		try{
// 			listener.lookupTransform("/map", "/base_link",  ros::Time(0), transform);
// 			CURRENT_ROBOT_POSITION.pose.pose.position.x = transform.getOrigin().x();
// 			CURRENT_ROBOT_POSITION.pose.pose.position.y = transform.getOrigin().y();
// 			CURRENT_ROBOT_POSITION.pose.pose.orientation.z = transform.getRotation().getZ();
// 			CURRENT_ROBOT_POSITION.pose.pose.orientation.w =  transform.getRotation().getW();
// 		}
// 		catch (tf::TransformException ex){
// 			ROS_ERROR("%s",ex.what());
// 			ros::Duration(1.0).sleep();
//     	}
// 	}
// };

bool bada_rounding()
{
	geometry_msgs::Twist msg; // 회전하기 위해 퍼블리시 용도로 만들어진 변수
	//subscribing_odometry
	//Save present angle
	// geometry_msgs::Quaternion initial_angle = CURRENT_ROBOT_POSITION.pose.pose.orientation;        // 현재 각도 정보를 저장
	//http://docs.ros.org/melodic/api/nav_msgs/html/msg/Odometry.html

	msg.angular.z = (3.14f / 4.0f); // 회전하도록하기

	bada_open_eyes_cmd(true); // 눈 뜨기. (정보 받기 시작)
	ros::Rate rate(5);		  // ROS Rate at 5Hz 0.2 sec

	int time = 0;
	float Ang_Position = 0;

	do
	{
		// ROS_INFO("spinning");

		pub_cmdvel.publish(msg); //각속도 정보 pub, 통신 실패를 방지하기 위해 while문에 넣어놓음.
		ros::spinOnce();		 // bada/eyes로부터 토픽 서브스크라이빙, 현재 오도메트리 정보 서브스크라이빙 목적으로 스핀
		// msg.angular.z;
		if (PPL_CHECK)
		{ // 만약 사람 정보가 ROI에 들어왔다면 true
			ROS_INFO("person detected.");
			//BOOKMARK1
			// TODO : USE ROBOT POSITION
			ROS_INFO("get ROBOT pose");
			SAVED_HUMAN_POSITION = bada_get_robot_pos();
			ROS_INFO("get ROBOT pose done");
			/** TODO : 각도와 거리를 이용하여 포인트를 저장한다.  **/
			// ~~맵에 사람의 위치 포인트를 저장하는 방법, 즉 데이터타입이 무엇인지 알아볼 것. ~~<<-- 사람 위치 저장하지 말 것
			// 지금 위치를 저장한다. (로봇의 위치) <<-- 이것을 사용할것

			// init values
			PPL_ANGLE = -90;	   // Angle of PPL respect to camera
			PPL_DIST = -1.0;	   // Distnace to PPL from
			break;
		}
		rate.sleep(); // 6헤르츠가 적당할 듯. 연산 과부화 방지용.
		time++;
		Ang_Position = (time * 0.2) * msg.angular.z;
		// ROS_INFO("%f",Ang_Position);
	} while (ros::ok() && Ang_Position < 6.28); //한바퀴 돌았는지?
	bada_open_eyes_cmd(false);

	msg.angular.z = 0.0;
	pub_cmdvel.publish(msg); //스탑
	if (PPL_CHECK){
		ROS_INFO("Person CHECKED");
		PPL_CHECK = false;	   // Is there PPL?
		return true;
	}
	else{
		ROS_INFO("Person no");
		PPL_CHECK = false;	   // Is there PPL?
		return false;
	}
}

void sub_sig_checker_callback(const std_msgs::Empty &msg)
{
	SIG_CHECK = true;
} // 소리가 발생하는지 체크하는 콜백함수

void bada_roaming()
{	//현재위치에서 마지막 지점까지 이동
	// bada_go_destination_blocking();			        //다음 지점으로 이동하기.

	ros::Rate loop_rate(1);
	int t=0;
	CURRENT_POINT += 1;
	for (; CURRENT_POINT <= 3; CURRENT_POINT++)
	{
		CURRENT_POINT %= 3;

		//while(ros::Duration(30))
		bada_send_destination(wayPoint[CURRENT_POINT][0], wayPoint[CURRENT_POINT][1], wayPoint[CURRENT_POINT][2], wayPoint[CURRENT_POINT][3]);
		// TODO : Investigate why while lo op breaks..
		ROS_INFO("SENDING DESTINATION");
		t=0;
		while (ros::ok() && t<30)
		{
			t++;
			loop_rate.sleep();
			ros::spinOnce(); //소리 검사 결과 받기.
			//ROS_INFO("roaming");
			if (SIG_CHECK)
			{
				SIG_CHECK = false;
				ROS_INFO("sig check");

				// bada_save_sound_odom();	                    //- robot pos 정보 오도메트리 저장하기.
				//-  배회 중단. 액션 메시지 취소 보내기
				actionClient->cancelGoal();
				return;
			}
			if (actionClient->getState() == actionlib::SimpleClientGoalState::SUCCEEDED /*--목표도착-- ACTION*/)
			{
				ROS_INFO("%d goal reach", CURRENT_POINT);
				// bada_go_destination_blocking();			//다음 지점으로 이동하기.
				break;
			}
		}
		if(!ros::ok()) break;
	}
	ROS_INFO("roaming done");
}

void bada_go_to_pepl()
{
	// while
	bada_go_destination_blocking(
		30.0,
		SAVED_HUMAN_POSITION.x,
		SAVED_HUMAN_POSITION.y,
		SAVED_HUMAN_POSITION.orien_w,
		SAVED_HUMAN_POSITION.orien_z); //사람에게 가기.
									   // FUTURE: what if the person gone??

	//예전 알고리즘
	// do{
	// 	// 현재 로봇의 위치 받아오기
	//
	// }while(/*-이동한 거리가 사람의 반경 2m 이내가 아닐 경우까지 이동. -*/);
	// 	/*- 행동 중단 -*/
	// return;
	// //사람에게 가기.
	// do{
	// 	// 현재 로봇의 위치 받아오기
	// }while(/*-이동한 거리가 사람의 반경 2m 이내가 아닐 경우까지 이동. -*/);
	// 	/*- 행동 중단 -*/
	// return;
} // END

void bada_aligned_pepl()
{
	bada_head_UP_cmd(true); // 2m 에 도달하면 카메라 위로 들기
	bada_open_eyes_cmd(true);
	float AngleV = 3.14f / 4.0f;
	while (ros::ok())
	{
		ros::spinOnce();
		ROS_INFO("%f", PPL_ANGLE);
		if (abs(PPL_ANGLE) < 80)
		{
			if (PPL_ANGLE < 0)
				bada_vel_cmd(0, AngleV); // CCW   Object is on Left side
			else
				bada_vel_cmd(0, -AngleV); // CW,   Object is on right side
		}
		if (abs(PPL_ANGLE) < (5 * PI / 180.0))
		{
			bada_vel_cmd(0, 0);
			break;
		}
	} //가운데로 맞추기
	bada_open_eyes_cmd(false);
	bada_head_UP_cmd(false); // 2m 에 도달하면 카메라 위로 들기
}

void bada_go_until_touch()
{ // 버튼 눌리기 전까지 전진하기
	geometry_msgs::Twist msg;
	msg.linear.x = 0.1;

	do
	{
		pub_cmdvel.publish(msg); // 앞으로 전진
		ros::spinOnce();
	} while (ros::ok() && !SWITCH_CHECK);
	SWITCH_CHECK = false;
	
	msg.linear.x = 0.0;
	pub_cmdvel.publish(msg); //스탑
} //END

void bada_change_pos(float LinePos, float AnglePos)
{
	ros::Rate loop_rate(20);

	int t = 0;
	float vel = 0;
	float target = (abs(LinePos) > abs(AnglePos)) ? LinePos : AnglePos;
	do
	{
		t++;
		if (abs(LinePos) > abs(AnglePos))
		{
			bada_vel_cmd(-0.15, 0);
			vel = -0.15;
		}
		else
		{
			bada_vel_cmd(0, PI / 4.0f);
			vel = PI / 4.0f;
		}
		loop_rate.sleep();
	} while (ros::ok() && float(t) * 0.05 * abs(vel) <= abs(target));
	bada_vel_cmd(0,0);
}

void sub_odometry_callback(const nav_msgs::Odometry &msg)
{
	// CURRENT_ROBOT_POSITION = msg;
}

// void bada_cancelAllGoal(){
// 	actionClient->cancelAllGoals();
// }

void bada_go_to_soundPT()
{ //사람 데리고 가는용
	bada_go_destination_blocking(30, SAVED_SOUND_POSITION.x, SAVED_SOUND_POSITION.y, SAVED_SOUND_POSITION.orien_z, SAVED_SOUND_POSITION.orien_w);
}
void bada_display_inform()
{
}

void bada_emotion()
{
}

void bada_wait_button()
{	
	ROS_INFO("button in)");
	bool pressed = false;
	ros::Rate loop_rate(6);
	while (ros::ok() && !pressed)
	{
		ros::spinOnce();
		pressed = SWITCH_CHECK;
		loop_rate.sleep();
	}
	ROS_INFO("out!");

}

void bada_go_to_sound()
{ //소리나는 방향으로 이동
	// topic:
	//
	// - sub sound localization,
	//
	// - pub cmd vel
	// go to sound while tracking, stop when "enough"
	ros::Rate loop_rate(6); // 과부하방지로 멈추기
	int count = 0;
	while (ros::ok())
	{
		// move base go to : CURRENT_SOUND_DIRECTION
		// to align direction of the robot.
		// TODO:: cmd_vel or simple_goal
		bada_vel_cmd(0.3f, 0);
		count++;

		loop_rate.sleep(); // 6헤르츠가 적당할 듯. 연산 과부화 방지용.
		if (count > 30)
		{
			break;
		}
		ros::spinOnce();
	}
}

Position bada_get_robot_pos()
{
	tf::TransformListener listener;
	tf::StampedTransform transform;
	try
	{
		listener.waitForTransform("map","base_link",ros::Time(0),ros::Duration(1.0));
		listener.lookupTransform("map", "base_link", ros::Time(0), transform);
		CURRENT_ROBOT_POSITION.pose.pose.position.x = transform.getOrigin().x();
		CURRENT_ROBOT_POSITION.pose.pose.position.y = transform.getOrigin().y();
		CURRENT_ROBOT_POSITION.pose.pose.orientation.z = transform.getRotation().getZ();
		CURRENT_ROBOT_POSITION.pose.pose.orientation.w = transform.getRotation().getW();
	}
	catch (tf::TransformException ex)
	{
		ROS_ERROR("%s", ex.what());
		ROS_INFO("tf ERROR");
		ros::Duration(1.0).sleep();
	}

	Position pos = {CURRENT_ROBOT_POSITION.pose.pose.position.x, CURRENT_ROBOT_POSITION.pose.pose.position.y, CURRENT_ROBOT_POSITION.pose.pose.orientation.z, CURRENT_ROBOT_POSITION.pose.pose.orientation.w};
	return pos;
}

void bada_save_sound_odom()
{
	Position pos = bada_get_robot_pos();
	SAVED_SOUND_POSITION = pos;
}
//https://opentutorials.org/module/2894/16661

void bada_open_eyes_cmd(bool status)
{
	std_msgs::Bool BoolStatus;
	BoolStatus.data = status;
	pub_eyes_open.publish(BoolStatus);
	waitSec(0.5); // wait 0.5
} // True -> Eyes UP, False -> Eyes Down

void bada_head_UP_cmd(bool status)
{
	std_msgs::Bool BoolStatus;
	BoolStatus.data = status;
	pub_head_up.publish(BoolStatus);
	waitSec(1);
} // True -> UP, False -> DOWN

void bada_display_cmd(DISP_EVNT status)
{
	std_msgs::Int16 IntStatus;
	IntStatus.data = int(status);
	pub_display_cmd.publish(IntStatus);
}

void bada_vel_cmd(const float XLineVel, const float ZAngleVel)
{
	geometry_msgs::Twist msg;
	msg.linear.x = XLineVel;
	msg.angular.z = ZAngleVel;
	pub_cmdvel.publish(msg);
}

/*--------------------------------------Callback----------------------------------------------*/
void sub_pepl_checker_callback(const geometry_msgs::Point &msg)
{
	PPL_CHECK = true;
	PPL_ANGLE = msg.y; // [rad]
	ROS_INFO("PPL_CHECK");
}

void sub_switch_checker_callback(const std_msgs::Bool &msgs)
{
	SWITCH_CHECK = msgs.data;
}

void sub_signal_callback(const std_msgs::String &msg)
{
	SIGNAL = msg;
}

void sub_sound_localization_callback(const geometry_msgs::PoseStamped &msg)
{
	CURRENT_SOUND_DIRECTION = msg;
}

bool bada_go_to_sound2()
{ //로봇 base_link 기준, 소리 나는 방향보고 1m 전진
	actionClient->waitForServer();
	ROS_INFO("going to sound");
	move_base_msgs::MoveBaseGoal sound_goal;
	move_base_msgs::MoveBaseGoal sound_goal2;

	sound_goal.target_pose.header.frame_id = "base_link";
	sound_goal.target_pose.header.stamp = ros::Time::now();

	sound_goal.target_pose.pose.position.x = 0;
	sound_goal.target_pose.pose.orientation.z = CURRENT_SOUND_DIRECTION.pose.orientation.z;
	sound_goal.target_pose.pose.orientation.w = CURRENT_SOUND_DIRECTION.pose.orientation.w;
	actionClient->sendGoal(sound_goal);
	actionClient->waitForResult(ros::Duration(30.0));

	sound_goal2.target_pose.header.frame_id = "base_link";
	sound_goal2.target_pose.header.stamp = ros::Time::now();

	sound_goal2.target_pose.pose.position.x = 1.0;
	sound_goal2.target_pose.pose.orientation.w = 1.0;
	actionClient->sendGoal(sound_goal2);

	bool finished_before_timeout = actionClient->waitForResult(ros::Duration(30.0));
	ROS_INFO("go to sound done.");

	if (finished_before_timeout)
	{
		actionlib::SimpleClientGoalState state = actionClient->getState();
		ROS_INFO("go to sound Action finished: %s", state.toString().c_str());
	}
	else
		ROS_INFO("go to sound Action did not finish before the time out.");

	if (SIGNAL.data == LastSignal.data)
		return true;
	else
		return false;
}

void waitSec(float sec)
{
	ros::Duration(sec).sleep();
}

void bada_log(std::string str1)
{
	// StringMsg pubData;
	// pubData.data=msg;
	// pub_logger.publish(pubData);

	std_msgs::String msg;

	std::stringstream ss;
	ss << str1;
	msg.data = ss.str();

	ROS_INFO("%s", msg.data.c_str());

	/**
     * The publish() function is how you send messages. The parameter
     * is the message object. The type of this object must agree with the type
     * given as a template parameter to the advertise<>() call, as was done
     * in the constructor above.
     */
	pub_logger.publish(msg);

	// ros::spinOnce();
}
