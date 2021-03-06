/*
 * Copyright (C) 2008, Morgan Quigley and Willow Garage, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the names of Stanford University or Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "ros/ros.h"
#include <demo_test/pos_data.h>


#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<netinet/in.h>


#define MCAST_PORT 1511 
#define MCAST_ADDR "239.255.42.99"


int unpack(char *pdata, demo_test::pos_data *pos);

int main(int argc, char **argv)
{
    //ros init 
    ros::init(argc, argv, "demo_udp");
    ros::NodeHandle node;
    ros::Publisher chatter_pub = node.advertise<demo_test::pos_data>("demo_udp", 1000);
    ROS_INFO("Hello ROS, ready to send rigidbody pos_data!!!");

    //socket init
    int sock;
    struct sockaddr_in local_addr;
    struct sockaddr_in temp_addr;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
        // cout<<"socket error"<<endl;
        ROS_INFO("socket error ");
        return -1;
    }
    memset(&local_addr, 0, sizeof(local_addr));
    memset(&temp_addr, 0, sizeof(temp_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //    local_addr.sin_addr.s_addr = inet_addr("169.254.6.162");
    local_addr.sin_port = htons(MCAST_PORT);

    int err = 0;
    int value = 1;
    //    err = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &local_addr.sin_addr.s_addr, sizeof(local_addr.sin_addr.s_addr));
    err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value));
    if (err < 0)
    {
        //cout<<"set reuseaddr error"<<endl;
        ROS_INFO("set reuseaddr error");
        return -1;
    }
    err = bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr));
    if (err < 0)
    {
        //cout<<"bind error"<<endl;
        ROS_INFO("bind error");
        return -1;
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MCAST_ADDR);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    //    mreq.imr_interface.s_addr = inet_addr("169.254.6.162");
    err = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    if (err < 0)
    {
        //cout<<"set sock ip_add_membership error"<<endl;
        ROS_INFO("set sock ip_add_membership error");
        return -1;
    }

    //receive--------------
    socklen_t addr_len = 0;
    addr_len = sizeof(temp_addr);
    char buff[1000];
    demo_test::pos_data pos;
    int n = 0;
    int i = 0;
    int j = 0;
    int send_seq = 0;
    while(ros::ok())
    {
        //i--;
        n = recvfrom(sock, buff, 1000, 0, (struct sockaddr*)&temp_addr, &addr_len);
        //    n = read(sock, buff, sizeof(buff));
        //    if (n < 0)
        //    {
        //        cout<<"recvfrom() errro"<<endl;
        //        return -2;
        //    }
        ROS_INFO("recevive %d bytes from %s", n, inet_ntoa(temp_addr.sin_addr));
        int test = 0;
        test = unpack(buff, &pos);
        if (test>0)
        {
            j++;
            ROS_INFO("received:%d    decoeded:%d", ++i, j);
            ROS_INFO("rigid_pos.num: %d", pos.num);
            for(int k=0; k<pos.num; k++)
            {
                ROS_INFO("rigidbody_pos::    id:%d, x:%f, y:%f, z%f", k+1, pos.pos[k*3], pos.pos[k*3+1], pos.pos[k*3+2]);
                ROS_INFO("rigidbody_q::    id:%d, qx:%f, qy:%f, qz%f, qw%f", k+1, pos.q[k*4], pos.q[k*4+1], pos.q[k*4+2], pos.q[k*4+3]);
            }
            pos.header.stamp = ros::Time::now();
            send_seq ++;
            pos.header.seq = send_seq;
            chatter_pub.publish(pos);
        }
        //

    }

    return 0;
}



int unpack(char *pdata, demo_test::pos_data *pos)
{
    int major = 2;
    int minor = 7;
    //decoding
    char *ptr =pdata;
    int msgid = 0;
    memcpy(&msgid, ptr, 2);

    if (msgid == 5)
    {
        printf("\nunpack Data Descriptions\n")
        int datasetCount = 0;
        memcpy(&datasetCount, ptr, 4);
        ptr += 4;
        printf("datasetCount: %d\n", datasetCount);
        for(int i = 1; i <= datasetCount; i++)
        {
            int type = 0; 
            memcpy(&type, ptr, 4);
            ptr += 4;
            printf("type : %d\n", type);
            if(type == 0)
            {
                char partition = 'a';
                do{
                    memcpy(&partition, ptr, 1);
                    ptr ++;

                }while(partition != '\0');
            }

        }
    }





    if (msgid == 7)
    {
        printf("\nBegin Packet\n---------------------\n");
        //1 msgid
        msgid = 0;
        memcpy(&msgid, ptr, 2); ptr += 2;
        printf("msgid: %d\n", msgid);
        //2 bytes count
        int nbytes = 0;
        memcpy(&nbytes, ptr, 2); ptr += 2;
        printf("bytes count: %d\n", nbytes);
        //3 frame number
        int framenumber = 0;
        memcpy(&framenumber, ptr, 4); ptr += 4;
        printf("Frame number: %d\n", framenumber);
        //4 number of data sets (markersets, rigidbodies, etc)
        int nmarksets = 0;
        memcpy(&nmarksets, ptr, 4); ptr += 4;
        printf("marker sets count: %d\n", nmarksets);
        //5 unidentified markers
        int othermarkers = 0;
        memcpy(&othermarkers, ptr, 4); ptr += 4;
        printf("unidentified marker count: %d\n", othermarkers);
        //6 rigid bodies
        int nRigidBodies = 0;
        memcpy(&nRigidBodies, ptr, 4); ptr += 4;
        printf("rigid body count: %d\n", nRigidBodies);
        //6.1 rigid bodies 
        for (int j=0; j < nRigidBodies; j++)
        {
            //6.1 rigid body pos/ori 32bytes
            int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
            float x = 0.0f; memcpy(&x, ptr, 4); ptr += 4;
            float y = 0.0f; memcpy(&y, ptr, 4); ptr += 4;
            float z = 0.0f; memcpy(&z, ptr, 4); ptr += 4;
            float qx = 0; memcpy(&qx, ptr, 4); ptr += 4;
            float qy = 0; memcpy(&qy, ptr, 4); ptr += 4;
            float qz = 0; memcpy(&qz, ptr, 4); ptr += 4;
            float qw = 0; memcpy(&qw, ptr, 4); ptr += 4;
            printf("ID : %d\n", ID);
            printf("pos: [%4.3f,%4.3f,%4.3f]\n", x,y,z);
            printf("ori: [%4.3f,%4.3f,%4.3f,%4.3f]\n", qx,qy,qz,qw);

            pos->pos[j*3] = x;
            pos->pos[j*3+1] = y;
            pos->pos[j*3+2] = z;
            pos->num = j+1;
            pos->q[j*4] = qx;
            pos->q[j*4+1] = qy;
            pos->q[j*4+2] = qz;
            pos->q[j*4+3] = qw;
            //6.2 associated marker positions  40bytes
            int nRigidMarkers = 0;
            memcpy(&nRigidMarkers, ptr, 4); ptr += 4;
            printf("Marker Count: %d\n", nRigidMarkers);
            int nBytes = nRigidMarkers*3*sizeof(float);
            float* markerData = (float*)malloc(nBytes);
            memcpy(markerData, ptr, nBytes); ptr += nBytes;
            printf("11\n");
            if(major >= 2)
            {
                //6.3 associated marker IDs  12bytes
                nBytes = nRigidMarkers*sizeof(int);
                int* markerIDs = (int*)malloc(nBytes);
                memcpy(markerIDs, ptr, nBytes);
                ptr += nBytes;

                //6.4 associated marker sizes  12bytes
                nBytes = nRigidMarkers*sizeof(float);
                float* markerSizes = (float*)malloc(nBytes);
                memcpy(markerSizes, ptr, nBytes);
                ptr += nBytes;

                for(int k=0; k < nRigidMarkers; k++)
                {
                    printf("\tMarker %d: id=%d\tsize=%3.1f\tpos=[%3.2f,%3.2f,%3.2f]\n", k, markerIDs[k], markerSizes[k], markerData[k*3], markerData[k*3+1],markerData[k*3+2]);
                }

                if(markerIDs)
                    free(markerIDs);
                if(markerSizes)
                    free(markerSizes);

            }
            else
            {
                for(int k=0; k < nRigidMarkers; k++)
                {
                    printf("\tMarker %d: pos = [%3.2f,%3.2f,%3.2f]\n", k, markerData[k*3], markerData[k*3+1],markerData[k*3+2]);
                }
            }
            printf("12\n");
            if(markerData)
                free(markerData);

            if(major >= 2)
            {
                //6.5 Mean marker error
                float fError = 0.0f; memcpy(&fError, ptr, 4); ptr += 4;
                printf("Mean marker error: %3.2f\n", fError);
            }

            // 2.6 and later
            if( ((major == 2)&&(minor >= 6)) || (major > 2) || (major == 0) ) 
            {
                //6.6 params
                short params = 0; memcpy(&params, ptr, 2); ptr += 2;
                bool bTrackingValid = params & 0x01; // 0x01 : rigid body was successfully tracked in this frame
            }

        } // next rigid body
        //
            printf("13\n");
        //7 skeletons
        if ( ((major == 2)&&(minor > 0)) || (major > 2) )
        {
            int nSkeletons = 0;
            memcpy(&nSkeletons, ptr, 4); ptr+=4;
            printf("Skeleton count: %d\n", nSkeletons);
            if (nSkeletons > 0)
                printf("error: Skeleton count > 0\n");
        }
            printf("14\n");
        //8 Labeled Markers
        int nLabeledMarkers = 0;
        memcpy(&nLabeledMarkers, ptr, 4); ptr += 4;
        printf("Labeled Marker count: %d\n", nLabeledMarkers);
            printf("15\n");
        //9 latency
        float latency = 0.0f;
        memcpy(&latency, ptr, 4); ptr += 4;
        printf("latency: %3.3f\n", latency);
        //10 timecode
        unsigned int timecode = 0;
        memcpy(&timecode, ptr, 4); ptr += 4;
        unsigned int timecodesub = 0;
        memcpy(&timecodesub, ptr, 4); ptr +=4;
        //11 timestamp
        double timestamp = 0.0f;
        if ( ((major == 2)&&(minor>=7)) || (major>2) )
        {
            memcpy(&timestamp, ptr, 8); ptr += 8;
        }
        else
        {
            float ftemp = 0.0f;
            memcpy(&ftemp, ptr, 4); ptr += 4;
            timestamp = (double)ftemp;
        }
        // frame param
        short params = 0;
        memcpy(&params, ptr, 2); ptr += 2;
        bool bIsRecording = params & 0x01;             //0x01  Motive is recording 
        bool bTrackedModelsChanged = params & 0x02;    //0x02  Actively tracked model list has changed 

        //13 end of data tag
        int eod = 0; 
        memcpy(&eod, ptr, 4); ptr += 4;
        printf("eod: %d\n", eod);
        printf("End Packet\n----------------\n");
        return 1;

    }
    else
    {
        printf("unrecognized packet\n");
        return -1;
    }

}
