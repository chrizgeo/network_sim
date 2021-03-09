/*  Packet level simulation in a simplex network
    Discrete event simulator
    There are two hosts, one tx and one rx 
    Christo George<chrizgeo@gmail.com>
    Feb 2021
*/
//TODO add file and  plot the throughput for many number of simulations
#include<iostream>
#include<queue>
#include<list>
#include<ctime>
#include<cmath>
#include<fstream>
#include<random>
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

using namespace std;

/* The bitrate of the processor in bits per time unit*/
//double BITRATE = 0.001;
/* The maximum number of realtime channels. */
const int maxChannelNUM = 16;
/* Edit if channel is to be distributed with different priorities */
discrete_distribution< int> channelDistribution;
/* Number of times to run the simulator events to get an average value */
int simulatorRUNS = 10;
/* constants */
long unsigned int qBuffMAX = 10000;   //max number of packets to keep in Q
double lambda	= .001; //to get some kind of uncertainity
int packetDuration = 125;

struct channel{
    int priority;
    int period;
    int deadLine;
    int capacity;
};

channel RTChannelList[maxChannelNUM];

struct packet{
    int size; //packet size
    double arrivalTime;
    double deadline; //the deadline of this packet
    int channelNum;  //RT channel for the packet 
    double txDelay;
    /* overload < for prioirty Q to compare the deadlines of the channel*/
    bool operator<(const packet &rhs)const
    {
        return channelNum > rhs.channelNum;
    }
};

struct host{ 
    priority_queue<packet> transmitQ; // Q at the host
    //int Qlength; //length of the Q - we dont need this when not considering propogation delays
    float numDropped; // number of dropped packets

};

struct event{
    int type; //type of the event, arrival - 0 departure - 1 , channel sensing - 2
    int rtChannel; // 0 for queue to tx 1 for tx to rx
    host *source; //source of the event
    host *dest; //destination of the event
    double time; // time of the event
};

/* create sender and receiver hosts */
host tx, *txPtr, rx, *rxPtr; 

/* List needed for the simulator */
list<event> eventList; //store the tx and rx events
//list<event> inTransitList; //store the link events

/* Helper function to find Negetive exponentially distributed time */
double nedt(double rate)
{
	double u;
	u = drand48();
	return ((-1/rate)*log(1-u));
}


/* Init host function - set the properties for the host */
void init_host(host* hostPtr)
{
    while(!hostPtr->transmitQ.empty()) hostPtr->transmitQ.pop();
    //hostPtr->Qlength = 0;
    hostPtr->numDropped = 0;
}

/* Init RT channels */
void init_channels(void) {
    // WE have a priority distribution of 1 for each channel 
    for(int i = 0; i < maxChannelNUM; i++) {
        RTChannelList[i].priority = i;
        RTChannelList[i].period = 500;
        RTChannelList[i].deadLine = 500;
        RTChannelList[i].capacity = ((rand() % 160) + 1);
        cout << "Channel num " << i << " Priority " << RTChannelList[i].priority << endl;
        cout << " Period " << RTChannelList[i].period << " Deadline " << RTChannelList[i].deadLine << " Capacity " << RTChannelList[i].capacity << endl;
    }
}

/* returns a new event to add to eventList */
event new_event(int type, int rtChannel, host* source, host* dest, double time)
{
    event newEvent;
    newEvent.type = type;
    newEvent.rtChannel = rtChannel;
    newEvent.source = source;
    newEvent.dest = dest;
    newEvent.time = time;
    return newEvent;
}

/*   Add a general event */
void add_event(event newEvent)
{
    list<event>::iterator it;
    event lastEvent; //the last event in the list
    lastEvent = eventList.back(); 
    
    if(newEvent.time < lastEvent.time) {
        for(it = eventList.begin(); it != eventList.end(); it++) {
            if(newEvent.time < (it->time)) {
                eventList.insert(it, newEvent);
                break;
            }           
        }
    }
    else {
        eventList.push_back(newEvent);
    }
}

/* Main function */
int main()
{
    cout << "Simulator start" << endl;
    /* Random number seed */
    srand48(time(NULL));
    default_random_engine generator;
    double simRunTime = 1000;
    /* Get how much time to run the simulator from the user */
    cout <<"Enter run time for simulator" << endl;
    cin >> simRunTime;
    cout << "Enter number of runs to smoothen simulator" << endl;
    cin >> simulatorRUNS;

    vector<double> runs, tp, avqd, chNum, chnDlMiss;
    /* variables */
    double currentTime = 0;  //current sim time
    double throughPut[maxChannelNUM] = {0};
    double totalPackets = 0;
    double totalDeadlineMisses = 0;
    double queueingDelay = 0;
    double deadlineMisses[maxChannelNUM] = {0};
    double deadlineMissratio[maxChannelNUM] = {0};
    double averageQueueingDelay[maxChannelNUM] = {0};
    int RTChannelNum = 0;
    
    /* Init the hosts */
    txPtr = &tx;
    rxPtr = &rx;

    /* Init channels */
    init_channels();
    /* Each run of the simulation */
    for(RTChannelNum = 0; RTChannelNum < maxChannelNUM; RTChannelNum++) {
        cout << RTChannelNum + 1 << " channels active " << endl;
        // Clear all data 
        totalPackets = 0;
        for(int ii = 0; ii <= RTChannelNum; ii++) {
             deadlineMisses[ii] = 0;
        }
        throughPut[RTChannelNum] = 0;
        totalDeadlineMisses = 0;
        deadlineMisses[maxChannelNUM] = {0};
        deadlineMissratio[RTChannelNum] = 0;
        currentTime = 0;
        queueingDelay = 0;
        averageQueueingDelay[maxChannelNUM] = 0;
        eventList.clear();
        /* Init the hosts */
        init_host(txPtr);
        init_host(rxPtr);
        /* Add to Q */
        event newEvent = new_event(0, 0, txPtr, txPtr, currentTime);
        add_event(newEvent);
        for(int ii = 1; ii <= RTChannelNum; ii++) {
            double nextTime = currentTime + nedt(lambda);
            //cout << nextTime << endl;
            event newEvent = new_event(0, ii, txPtr, txPtr, nextTime);
            add_event(newEvent);
            //cout << "added another channel" << endl;
        }
        /* Get the top of the event list */
        event lastEvent =  eventList.front();
        currentTime = lastEvent.time;
        /* Perform the iteration for the given number of packets */
        while(currentTime < simRunTime) {
            event currentEvent = eventList.front();
            currentTime = currentEvent.time;
            /* An arrival event */
            if(currentEvent.type == 0) {
                /* Arrival at the Tx */
                /* Create new packet */
                packet newPacket;
                //1-16 bytes of data, in random
                newPacket.channelNum = currentEvent.rtChannel;
                newPacket.size = RTChannelList[newPacket.channelNum].capacity;
                newPacket.txDelay = (double)newPacket.size*(double)packetDuration;
                //newPacket.txDelay = (double)newPacket.size/BITRATE; //Tx delay is Number of bits/bitrate
                newPacket.deadline = currentTime + RTChannelList[newPacket.channelNum].deadLine; //TODO deadline is fixed now. will have to change
                newPacket.arrivalTime  = currentTime;
                /* Create departure event for this packet from the tx */
                if(currentEvent.source->transmitQ.size() < qBuffMAX) {
                    currentEvent.source->transmitQ.push(newPacket);
                    event newDeparture = new_event(1, currentEvent.rtChannel, currentEvent.source, currentEvent.dest, (currentTime + newPacket.txDelay));
                    add_event(newDeparture);
                }
                else {
                    currentEvent.source->numDropped += newPacket.size;
                }

                // Add new arrival event to generate new packet 
                event newArrival = new_event(0, currentEvent.rtChannel, currentEvent.source, currentEvent.dest, (currentTime + RTChannelList[newPacket.channelNum].period ));
                add_event(newArrival);

            }
            /*receive event */
            else if(currentEvent.type == 1) {
                //TODO update Q time data
                    /* rx has received the data */ 
                        packet newPacket = currentEvent.source->transmitQ.top(); // get the packet to be transmitted from the Q
                        totalPackets += newPacket.size;
                        if(newPacket.deadline < currentTime) deadlineMisses[newPacket.channelNum]++;
                        queueingDelay += currentTime - newPacket.deadline;
                        currentEvent.source->transmitQ.pop();
            }
            eventList.pop_front();  
        }
        
        for(int ii = 0; ii <= RTChannelNum; ii++) {
            totalDeadlineMisses += deadlineMisses[ii];
            //chNum.push_back(ii);
            //chnDlMiss.push_back(deadlineMisses[ii]);
            cout << "Channel " << ii << " missed " << deadlineMisses[ii] << " packets " << endl;
        }

        averageQueueingDelay[RTChannelNum] = queueingDelay/totalPackets;
        throughPut[RTChannelNum] = totalPackets/currentTime;
        deadlineMissratio[RTChannelNum] = totalDeadlineMisses/totalPackets;
        cout << "Throughput " << throughPut[RTChannelNum] << endl;
        cout << "Total packets Txed " << totalPackets << endl;
        cout << "Deadlines missed " << totalDeadlineMisses << endl;
        cout << "Deadline miss ratio " << deadlineMissratio[RTChannelNum] << endl;
        cout << "Average queuing delay " << averageQueueingDelay[RTChannelNum] << endl;
        cout << "Dropped packets " << tx.numDropped << endl;
        /* Write simulation data to file */
        //TODO
//        runs.push_back(i);
//        tp.push_back(throughPut);
//        avqd.push_back(averageQueueingDelay);

    }

    return 0;

}

