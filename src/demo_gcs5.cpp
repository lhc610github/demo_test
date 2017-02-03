#include <stdlib.h>
#include "ros/ros.h"
#include "demo_test/pos_data.h"
#include "demo_test/pos_write_data.h"
#include "demo_test/pos_status.h"
#include <string.h>
#include <boost/thread.hpp>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <termios.h>
#include <stdlib.h>
#include <time.h>
#include"checksum.h"
using namespace std;
#define BAUDRATE B57600
#define MODEMDEVICE "/dev/ttyUSB0"

typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
boost::mutex pos_mutex;
boost::mutex pos_sp_mutex;
boost::mutex send_mutex;
demo_test::pos_data xyz;
demo_test::pos_data xyz_old;
demo_test::pos_status pos_status1;
ros::Publisher status_upd_sub;
float ref_data[5];
char *ref_status1 =0;
char *ref_status;
float send_currentpos_freq=25;
float send_desirepos_freq=10;
class myGroundCS 
{
public:
	myGroundCS();
	struct timespec time1;
	struct timespec time2;
	//static bool flag = true;
	//int send_write(int fd,char *buffer,int length);
	int open_port(const char * tty_port);
	int setup_port(int fd);
	void udpcallback(const demo_test::pos_data& pos);
 	void writecallback(const demo_test::pos_write_data& pos1);
	void send_current_pos(const int& fd);
	void send_desire_pos(const int& fd);
	void status_publish_thrd();

	bool setup_port_flag;
	int fd_setup_port;

private:
};
myGroundCS::myGroundCS()
{
	 timespec time1 = {0, 0};
	 timespec time2 = {0, 0};
 	 setup_port_flag=true;
// setup_port
	
	 fd_setup_port=myGroundCS::open_port(MODEMDEVICE);

	if (fd_setup_port < 0)
	{
		setup_port_flag=false;
	}else{
	if (myGroundCS::setup_port(fd_setup_port) < 0)
	{
		setup_port_flag=false;
	}else
	{
	printf("%s is open...\n", MODEMDEVICE);
	}
    }

}


int myGroundCS::open_port(const char * tty_port)
{
    int fd;
    fd = open(tty_port, O_RDWR | O_NOCTTY);
    if (fd == -1)
    {
        printf("open tty error\n");
        return (-1);
    }
    else
    {
        printf("open %s successfully\n", tty_port);
    }
    if (!isatty(fd))
    {
        printf("%s is not a serial port\n", tty_port);
        return (-1);
    }
    return (fd);
}

int myGroundCS::setup_port(int fd)
{
    struct termios config;
    if (tcgetattr(fd, &config) < 0)
    {
        printf("get serial pot attribute error\n");
        return (-1);
    }
	config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);
	config.c_oflag &= ~(OCRNL | ONLCR | ONLRET | ONOCR | OFILL | OPOST);
#ifdef OLCUC
  config.c_oflag &= ~OLCUC;
#endif
#ifdef ONOEOT
  config.c_oflag &= ~ONOEOT;
#endif
  config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
  config.c_cflag &= ~(CSIZE | PARENB);
  config.c_cflag |= CS8;
  config.c_cc[VMIN] = 0;
  config.c_cc[VTIME] = 0; 
   if (cfsetispeed(&config, BAUDRATE) < 0 || cfsetospeed(&config, BAUDRATE) < 0)
    {
		printf("\nERROR: Could not set desired baud rate of %d Baud\n", BAUDRATE);
		return (-1);
    }
   if (tcsetattr(fd, TCSAFLUSH, &config) < 0)
    {
		fprintf(stderr, "\nERROR: could not set configuration of fd %d\n", fd);
		return (-1);
    }
    return 1;
}


void myGroundCS::udpcallback(const demo_test::pos_data& pos)
{
   
    boost::mutex::scoped_lock lock(pos_mutex);
    memcpy(&xyz, &pos, sizeof(pos));
   lock.unlock();
}
void myGroundCS::writecallback(const demo_test::pos_write_data& pos1)
{   	boost::mutex::scoped_lock possp_lock(pos_sp_mutex);
	 ref_data[0]=pos1.pos_d[0];
	 ref_data[1]=pos1.pos_d[1];
	 ref_data[2]=pos1.pos_d[2];
    	 ref_data[3]=pos1.pos_d[3];
	 ref_data[4]=pos1.pos_d[4];
         send_currentpos_freq=pos1.send_currentpos_freq;
	 send_desirepos_freq=pos1.send_desirepos_freq;
	possp_lock.unlock();
}
// to be complete------------------
void myGroundCS::send_current_pos(const int& fd)
{
    //to be complete
	 
    
	
  

    char buffer[20]={0xFE,12,0,1,50,232};
	unsigned int seq=1;
	float temp_freq;
	typedef struct _mocap_data
	{
		float x;
		float y;
		float z;
	}mocap_data;
	mocap_data _pos_data;
	uint8_t ck[2];
   	int write_n;
	
	float delay_time;
		while(ros::ok()){
			ros::Rate loop_rate_send_current(send_currentpos_freq);	
			temp_freq=send_currentpos_freq;	
				while(ros::ok())
				{
					if(temp_freq == send_currentpos_freq)
						{
							boost::mutex::scoped_lock lock(pos_mutex);
							_pos_data.x = xyz.pos[0];
							_pos_data.y = xyz.pos[2];
							_pos_data.z = -xyz.pos[1];
							lock.unlock();
						//ROS_INFO("sending position_feedback to uav1: x=%.4f, y=%.4f, z=%.4f", _pos_data.x, _pos_data.y, _pos_data.z);
							ROS_INFO("!");  
						  	memcpy(buffer+6, &_pos_data, sizeof(_pos_data));
							buffer[2]=seq++;		
							buffer[18]=88;
							uint16_t crcTmp = crc_calculate((uint8_t *)(buffer+1),18);
	
							ck[0] = (uint8_t)(crcTmp & 0xFF);
							ck[1] = (uint8_t)(crcTmp >> 8);
							buffer[18]=ck[0];
							buffer[19]=ck[1];

							write_n = 0;
							boost::mutex::scoped_lock send_lock(send_mutex);
							write_n=write(fd,buffer,20);
							//write_n=send_write(fd,buffer,20);	
							send_lock.unlock();
							if(write_n!=20)
								{
									printf("send current position feedback error\n");
									///////////////////
								}
						}
					else
						break;
						loop_rate_send_current.sleep();
				}
			}
}

//-------------------------------------------------------------------------------
//
void myGroundCS::send_desire_pos(const int& fd)
{

 


    char buffer2[28]={0xFE,20,0,1,50,233};
	unsigned int seq2=1;
	float temp_freq;
	typedef struct _pos_sp_data
	{
		float x_d;
		float y_d;
		float z_d;
        	float yaw_d;
		float type;
	}pos_sp_data;
	pos_sp_data _xyz_sp;
	
	uint8_t ck2[2];
	int write_n2;
	float delay_time;
	while(ros::ok())
	{
	ros::Rate loop_rate_send_desire(send_desirepos_freq);
		temp_freq = send_desirepos_freq;
		while(ros::ok()){
			//delay_time=1000000/send_desirepos_freq-20;
			if(temp_freq == send_desirepos_freq)
				{	
				boost::mutex::scoped_lock possp_lock(pos_sp_mutex);
				_xyz_sp.x_d = ref_data[0];
				_xyz_sp.y_d = ref_data[1];
				_xyz_sp.z_d = ref_data[2];
			    	_xyz_sp.yaw_d = ref_data[3];
				_xyz_sp.type = ref_data[4];
				possp_lock.unlock();
			 // ROS_INFO("sending position setpoint to uav1: x_d=%.4f, y_d=%.4f, z_d=%.4f, yaw_d=%.4f\n", _xyz_sp.x_d, _xyz_sp.y_d, _xyz_sp.z_d, _xyz_sp.yaw_d);
				ROS_INFO(".");			   
			 	memcpy(buffer2+6, &_xyz_sp, sizeof(_xyz_sp));
				buffer2[2]=seq2++;		
				buffer2[26]=119;
				uint16_t crcTmp2 = crc_calculate((uint8_t *)(buffer2+1),26);
	
				ck2[0] = (uint8_t)(crcTmp2 & 0xFF);
				ck2[1] = (uint8_t)(crcTmp2 >> 8);
				buffer2[26]=ck2[0];
				buffer2[27]=ck2[1];
				write_n2 = 0;
				boost::mutex::scoped_lock send_lock(send_mutex);
				write_n2=write(fd,buffer2,28);
				//write_n2=send_write(fd,buffer2,24);	
				send_lock.unlock();
				if(write_n2!=28)
				{
					printf("send desire position setpoint  error\n");
					//return -1;
				}
				}
			else 
				break;
				loop_rate_send_desire.sleep();
			}
	}
}
////////////////////////////////////////////////////////////////////////

void myGroundCS::status_publish_thrd()
{

ros::Rate loop_pos_status(5);
while(ros::ok()){
boost::mutex::scoped_lock possp_lock(pos_sp_mutex);
	pos_status1.pos_d[0] = ref_data[0];
	pos_status1.pos_d[1] = ref_data[1];
	pos_status1.pos_d[2] = ref_data[2];
    	pos_status1.pos_d[3] = ref_data[3];
	pos_status1.pos_d[4] = ref_data[4];
	pos_status1.send_currentpos_freq=send_currentpos_freq;
	pos_status1.send_desirepos_freq=send_desirepos_freq;
	possp_lock.unlock();
	status_upd_sub.publish(pos_status1);
loop_pos_status.sleep();
}
}


//////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    
	if(argc<6)
    {
        printf("missing arguments\n");
        return -1;
    }
	myGroundCS myGCS;
    if(!myGCS.setup_port_flag)
    {  return -1;
	}
    memset(&xyz, 0, sizeof(xyz));
    memset(&xyz_old, 0,sizeof(xyz_old));

   
    //ros init
    ros::init(argc, argv, "demo_gcs5");
    ros::NodeHandle n;
    ros::Subscriber sub = n.subscribe("demo_udp", 1000, &myGroundCS::udpcallback,&myGCS);
    ros::Subscriber sub2 = n.subscribe("pos_write_topic", 1000, &myGroundCS::writecallback,&myGCS);
    //read trajectory data from file argv[1]
    ROS_INFO("argv[0]:%s",argv[0]);
    ROS_INFO("argv[1]:%s",argv[1]);
    ROS_INFO("argv[2]:%s",argv[2]);
    ROS_INFO("argv[3]:%s",argv[3]);
    ROS_INFO("argv[4]:%s",argv[4]);
    ROS_INFO("argv[5]:%s",argv[5]);
    boost::mutex::scoped_lock possp_lock(pos_sp_mutex);
    ref_data[0] = atof(argv[1]);
    ref_data[1] = atof(argv[2]);
    ref_data[2] = atof(argv[3]);
    ref_data[3] = atof(argv[4]);
    ref_status = new char[sizeof(argv[5])];
					memset(ref_status, 0, sizeof(argv[5]));
					strcpy(ref_status,argv[5]);
    if(!strcmp(ref_status,"idel"))
	ref_data[4]=1;
	else
		if(!strcmp(ref_status,"land"))
		ref_data[4]=2;
		else
			if(!strcmp(ref_status,"takeoff"))
			ref_data[4]=3;
			else
			ref_data[4]=0;
	possp_lock.unlock();
    //motion capture postion data
    demo_test::pos_data temp_xyz;
    memset(&temp_xyz, 0, sizeof(temp_xyz));
 //   float temp_vx = 0;
 //   float temp_vy = 0;
  //  float temp_vz = 0;
	
    int i = 0;
status_upd_sub=n.advertise<demo_test::pos_status>("pos_status_topic",1000);

boost::thread send_current(boost::bind(&myGroundCS::send_current_pos,&myGCS,myGCS.fd_setup_port));
boost::thread send_desire(boost::bind(&myGroundCS::send_desire_pos,&myGCS,myGCS.fd_setup_port));
boost::thread status_publish(&myGroundCS::status_publish_thrd,&myGCS);
    ros::AsyncSpinner s(3);
    s.start();

    ros::Rate loop_rate(3);
    while(ros::ok())
    {
        			switch((int)ref_data[4])
						{
						case 0:
						ref_status1 = new char[sizeof("normal")];
						memset(ref_status1, 0, sizeof("normal"));
						strcpy(ref_status1,"normal");
						break;
						case 1:
						ref_status1 = new char[sizeof("idel")];
						memset(ref_status1, 0, sizeof("idel"));
						strcpy(ref_status1,"idel");
						break;
						case 2:
						ref_status1 = new char[sizeof("land")];
						memset(ref_status1, 0, sizeof("land"));
						strcpy(ref_status1,"land");
						break;
						case 3:
						ref_status1 = new char[sizeof("takeoff")];
						memset(ref_status1, 0, sizeof("takeoff"));
						strcpy(ref_status1,"takeoff");
						break;
						default:
						break;
						}		
	printf("RUNING... \n");
	printf("ref_data[4] : %s \n",ref_status1);
	printf("sending position setpoint to uav1: x_d=%.4f, y_d=%.4f, z_d=%.4f, yaw_d=%.4f\n", ref_data[0],ref_data[1], ref_data[2], ref_data[3]);
	printf("sending position_feedback to uav1: x=%.4f, y=%.4f, z=%.4f\n", xyz.pos[0], xyz.pos[2], -xyz.pos[1]);  
	printf("sending freq:  feedback = %.1f HZ, setpoint= %.1f HZ\n", send_currentpos_freq, send_desirepos_freq);     
	loop_rate.sleep();
    }
    return 0;
}
