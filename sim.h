#include<iostream>
#include<queue>
#include<list>
#include<ctime>
#include<cmath>
#include<fstream>
using namespace std;

//comment this to use FCFS Q
#define PRIORITY_Q
/* The maximum number of realtime channels. */
const int maxChannelNUM = 16;
/* Number of times to run the simulator events to get an average value */
int simulatorRUNS = 5;
/* constants */
long unsigned int qBuffMAX = 10000000;   //max number of packets to keep in Q

double lambda	= .001; //to get some kind of uncertainity for RTC channel transmission start

int packetDuration = 125; //corresponding to a bitrate of 64Mbps for 1 packet of one byte 

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
    int priority;
#ifdef PRIORITY_Q
    /* overload < for prioirty Q to compare priorities of the channel*/
    bool operator<(const packet &rhs)const
    {
        if(priority > rhs.priority) {
            return true;
        }
        else if(priority == rhs.priority && deadline > rhs.deadline) {
            return true;
        }
        else if (deadline > rhs.deadline){
            return true;
        }
        else if(deadline == rhs.deadline) {
            if(arrivalTime > rhs.arrivalTime)
                return true;
            else
                return arrivalTime+txDelay > rhs.arrivalTime + rhs.txDelay;
        }
        else {
            return false;
        }
    }
#else
    /* overload < for prioirty Q to compare the deadlines of the channel*/
    bool operator<(const packet &rhs)const
    {
        if(deadline > rhs.deadline) {
            return true;
        }
        else if(deadline == rhs.deadline) {
            if(arrivalTime > rhs.arrivalTime)
                return true;
            else
                return arrivalTime+txDelay > rhs.arrivalTime + rhs.txDelay;
        }
        else {
            return false;
        }
    }
#endif

};

struct host{ 
    priority_queue<packet> transmitQ; // Q at the host
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
list<event> FEL; //store the tx and rx events
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
        RTChannelList[i].period = 5000;
        RTChannelList[i].deadLine = 5000;
        RTChannelList[i].capacity = ((rand() % 4) + 1);
        cout << "Channel num " << i << " Priority " << RTChannelList[i].priority << endl;
        cout << " Period " << RTChannelList[i].period << " Deadline " << RTChannelList[i].deadLine << " Capacity " << RTChannelList[i].capacity << endl;
    }
}

/* returns a new event to add to FEL */
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
    lastEvent = FEL.back(); 
    
    if(newEvent.time < lastEvent.time) {
        for(it = FEL.begin(); it != FEL.end(); it++) {
            if(newEvent.time < (it->time)) {
                FEL.insert(it, newEvent);
                break;
            }           
        }
    }
    else {
        FEL.push_back(newEvent);
    }
}
