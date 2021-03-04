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

using namespace std;

/* The bitrate of the processor in bits per time unit*/
int BITRATE = 10;
/* The maximum number of realtime channels. */
int maxChannelNUM = 16;
/* Number of times to run the simulator events */
int simulatorRUNS = 10;
/* constants */
long unsigned int qBuffMAX = 100;   //max number of packets to keep in Q
double totalBytes = 0;
double lambda	= .01; //to get some kind of uncertainity


struct channel{
    int priority;
    int period;
    int deadLine;
};

struct packet{
    int size; //packet size
    double arrivalTime;
    double deadline; //the deadline of this packet
    //channel* chPtr;  //RT channel for the packet //TODO use channel info for priority Queueing
    double txDelay;
    /* overload < for prioirty Q to compare the deadlines of the channel*/
    bool operator<(const packet &rhs)const
    {
        return deadline > rhs.deadline;
    }
};

struct host{ 
    priority_queue<packet> transmitQ; // Q at the host
    int Qlength; //length of the Q
    int numDropped; // number of dropped packets

};

struct event{
    int type; //type of the event, arrival - 0 departure - 1 , channel sensing - 2
    int subType; // 0 for queue to tx 1 for tx to rx
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
    hostPtr->Qlength = 0;
    hostPtr->numDropped = 0;
}

/* returns a new event to add to eventList */
event new_event(int type, int subType, host* source, host* dest, double time)
{
    event newEvent;
    newEvent.type = type;
    newEvent.subType = subType;
    newEvent.source = source;
    newEvent.dest = dest;
    newEvent.time = time;
    return newEvent;
}

/* Add a general event */
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

    /* variables */
    double currentTime = 0;  //current sim time
    double throughPut = 0;
    double deadlineMisses = 0;
    double deadlineMissratio = 0;
    double queueingDelay = 0;
    double averageQueueingDelay = 0;
    double simRunTime = 1000;
    /* Init the hosts */
    txPtr = &tx;
    rxPtr = &rx;
    cout <<"Enter run time for simulator" << endl;
    cin >> simRunTime;

    /* Each run of the simulation */
    for(int i=0; i < simulatorRUNS; i++) {
        // Clear all data 
        totalBytes = 0;
        throughPut = 0;
        deadlineMisses = 0;
        deadlineMissratio = 0;
        currentTime = 0;
        queueingDelay = 0;
        averageQueueingDelay = 0;
        eventList.clear();
        /* Init the hosts */
        init_host(txPtr);
        init_host(rxPtr);
        /* Add to Q */
        event newEvent = new_event(0, 0, txPtr, txPtr, currentTime);
        add_event(newEvent);
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
                newPacket.size = 8*((rand() % 16) + 1);  
                newPacket.txDelay = (double)newPacket.size/BITRATE; //Tx delay is Number of bits/bitrate
                newPacket.deadline = currentTime + .00000001; //TODO deadline is fixed now. will have to change
                newPacket.arrivalTime  = currentTime;
                /* Create departure event for this packet from the tx */
                if(currentEvent.source->transmitQ.size() < qBuffMAX) {
                    currentEvent.source->transmitQ.push(newPacket);
                    event newDeparture = new_event(1, 0, currentEvent.source, currentEvent.dest, (currentTime + newPacket.txDelay));
                    add_event(newDeparture);
                }
                else {
                    currentEvent.source->numDropped +=newPacket.size;
                }

                // Add new arrival event to generate new packet 
                event newArrival = new_event(0, 0, currentEvent.source, currentEvent.dest, (currentTime + 0.0001));
                add_event(newArrival);

            }
            /*receive event */
            else if(currentEvent.type == 1) {
                //TODO update Q time data
                    /* rx has received the data */ 
                        packet newPacket = currentEvent.source->transmitQ.top(); // get the packet to be transmitted from the Q
                        totalBytes += newPacket.size;
                        if(newPacket.deadline < currentTime) deadlineMisses++;
                        queueingDelay += currentTime - newPacket.arrivalTime;
                        currentEvent.source->transmitQ.pop();
            } 
            eventList.pop_front();    
        }
        
        averageQueueingDelay = queueingDelay/totalBytes;
        throughPut = totalBytes/currentTime;
        deadlineMissratio = deadlineMisses/totalBytes;
        cout << "Throughput " << throughPut << endl;
        cout << "Total bits Txed " << totalBytes << endl;
        cout << "Deadlines missed " << deadlineMisses << endl;
        cout << "Deadline miss ratio " << deadlineMissratio << endl;
        cout << "Average queuing delay " << averageQueueingDelay << endl;
        cout << "Dropped bits " << tx.numDropped << endl;
        /* Write simulation data to file */
        //TODO
    }
    return 0;

}

