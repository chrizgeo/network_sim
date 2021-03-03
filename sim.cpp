/*  Packet level simulation in a simplex network
    Discrete event simulator
    There are two hosts, one tx and one rx 
    Christo George<chrizgeo@gmail.com>
    Feb 2021
*/

#include<iostream>
#include<queue>
#include<list>
#include<ctime>
#include<cmath>

using namespace std;
/* The maximum number of realtime channels. */
int maxChannelNUM = 16;
/* constants */
//int packetNUM = 10000; //max number of packets we are to simulate.
int qBuffMAX = 100;   //max number of packets to keep in Q
double totalBytes = 0;
double lambda	= .01; // lambda (user-defined)
double mu	= 8.22/1000; // mu


struct channel{
    int priority;
    int period;
    int deadLine;
};

struct packet{
    int size; //packet size
    double deadline; //the deadline of this packet
    //channel* chPtr;  //RT channel for the packet //TODO use channel info for priority Queueing
    double serviceTime;
    /* overload < for prioirty Q to compare the deadlines of the channel*/
/*     bool operator<(const packet &rhs)const
    {
        return deadline < rhs.deadline;
    } */
};

struct host{ 
    queue<packet> transmitQ; // Q at the host
    int Qlength; //length of the Q
    int numDropped; // number of dropped packets

};

struct event{
    int type; //type of the event, arrival - 0 departure - 1 , channel sensing - 2
    int subType; // 0 for queue to tx 1 for tx to rx
    host *source; //source of the event
    host *dest; //destination of the event
    int size;
    double time; // time of the event
};

/* create sender and receiver hosts */
host tx, *txPtr, rx, *rxPtr; 

/* List needed for the simulator */
list<event> eventList; //store the tx and rx events
list<event> inTransitList; //store the link events

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
    hostPtr->Qlength = 0;
    hostPtr->numDropped = 0;
}

/* returns a new event to add to eventList */
event new_event(int type, int subType, host* source, host* dest, int size, double time)
{
    event newEvent;
    newEvent.type = type;
    newEvent.subType = subType;
    newEvent.source = source;
    newEvent.dest = dest;
    newEvent.size = size;
    /* the time needed for a departure event */
    //TODO check the time values
    if(type == 0) {
        newEvent.time = time + .01;
    }

    /* the time required for an arrival event */
    //TODO check the time values
    else if(type == 1) {
        newEvent.time = time + .1;
    }

    /* channel sensing event */
    //TODO check the time values
    else if(type == 2) {
        newEvent.time = time + 0.001;
    }
    /* unknown event  */
    else {
        cout << "ERR Unknown event" << endl;
    }

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

/* Add a transit event */
void add_transit(event newEvent)
{
    //cout << "add Transit event" << endl;
    list<event>::iterator it;
    event lastTransit;
    lastTransit = inTransitList.back();
    if(newEvent.time < lastTransit.time) {
        for(it = inTransitList.begin(); it != inTransitList.end(); it++) {
            if(newEvent.time < (it->time)) {
                inTransitList.insert(it, newEvent);
                break;
            }
        }
    }
    else {
        inTransitList.push_back(newEvent);
    }
}

/* Main function */
int main()
{
    cout << "Simulator start" << endl;
    /* Random number seed */
    srand48(time(NULL));
    cout << nedt(lambda) << endl;
    cout << nedt(mu) << endl;
    /* variables */
    double currentTime = 0;  //current sim time
    double throughPut = 0;
    /* Init the hosts */
    txPtr = &tx;
    rxPtr = &rx;
    init_host(txPtr);
    init_host(rxPtr);
    
    /* add the init event */
    /* Add to Q */
    event newEvent = new_event(0, 0, txPtr, txPtr, 0, currentTime);
    add_event(newEvent);
    /* Get the top of the event list */
    event lastEvent =  eventList.front();
    currentTime = lastEvent.time;
    /* Perform the iteration for the given number of packets */
    while(currentTime < 100000) {
        event currentEvent = eventList.front();
        currentTime = currentEvent.time;
        /* An arrival event */
        if(currentEvent.type == 0) {
            /* Arrival at the Tx */
            if(currentEvent.subType == 0) {
                /* Create new packet */
                packet newPacket;
                newPacket.size = (rand() % 16) + 1; 
                newPacket.serviceTime = 0.002*newPacket.size; //TODO serviceTime? is it random or fixed?
                /* Create departure event for this packet from the tx */
                if(currentEvent.source->transmitQ.size() < qBuffMAX) {
                    currentEvent.source->transmitQ.push(newPacket);
                    event newDeparture = new_event(1, 0, currentEvent.source, currentEvent.dest, newPacket.size, (currentTime + newPacket.serviceTime));
                    add_event(newDeparture);
                    currentEvent.source->Qlength++;
                }
                else {
                    currentEvent.source->numDropped +=newPacket.size;
                }
                /* Add new arrival event to generate new packet */
                event newArrival = new_event(0, 0, currentEvent.source, currentEvent.dest, 0, currentTime);
                add_event(newArrival);
            }
            /* Arrival of data packet at Rx */
            else if(currentEvent.subType == 1) {
                //cout << "Arrival at Rx" << endl;
                //TODO Update transmission time
                //cout << currentEvent.size << endl;
                totalBytes += currentEvent.size;
                /* Transmission complete */
                inTransitList.pop_front();
            }

        }
        /*Departure event */
        else if(currentEvent.type == 1) {
            //TODO update Q time data
            
            /* Data packet departure */ {
                if(currentEvent.source->Qlength > 0) {
                    packet newPacket = currentEvent.source->transmitQ.front(); // get the packet to be transmitted from the Q
                    /* check if channel is free */
                    if(inTransitList.size() == 0) {
                        /* New arrival for the destination host */
                        event newArrival = new_event(0, 1, currentEvent.source, currentEvent.dest, newPacket.size, currentTime + (newPacket.size / (11000000 / 8)));
                        //cout << "Departed to destination" << endl;
                        add_event(newArrival);
                        add_transit(newArrival);
                        currentEvent.source->transmitQ.pop();
                    }

                    else if(inTransitList.size() > 0) {
                        //line is busy
                        //add a channel sense event
                        event channelEvent = new_event(2, 0, currentEvent.source, currentEvent.dest, 0, currentTime);
                        add_event(channelEvent);
                    }
                }
            }
        }

        /* Channel  Sensing event */
        else if(currentEvent.type == 2) {
            /* Check if channel is free */
            if(inTransitList.size() == 0) {
                packet newPacket = currentEvent.source->transmitQ.front(); // get the packet to be transmitted from the Q
                /* New arrival for the destination host */
                event newArrival = new_event(0, 1, currentEvent.dest, currentEvent.source, newPacket.size, currentTime + (newPacket.size / (11000000 / 8)));
                add_event(newArrival);
                add_transit(newArrival);
                currentEvent.source->transmitQ.pop();
            }

            else if(inTransitList.size() > 0) {
                //line is busy
                //add a channel sense event
                event channelEvent = new_event(2, 0, currentEvent.source, currentEvent.dest, 0, currentTime + nedt(lambda));
                add_event(channelEvent);
            }
            }
        /* pop the processed event */    
        eventList.pop_front();    
    }
    
    throughPut = totalBytes/currentTime;
    cout << "Throughput" << throughPut << endl;
    cout << "Total bytes Txed" << totalBytes << endl;
    cout << "Dropped bytes" << tx.numDropped << endl;
    return 0;
}

