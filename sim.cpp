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

using namespace std;

/* The maximum number of realtime channels. */
int maxChannelNUM = 16;
/* constants */
int packetNUM = 10000; //max number of packets we are to simulate.
int qBuffMAX = 100;   //max number of packets to keep in Q



struct channel{
    int priority;
    int period;
    double deadLine;
};

struct packet{
    double size; //packet size
    double deadline; //TODO remove deadline and use deadline info from channel
    //channel channelNum;  //RT channel for the packet //TODO use channel info for priority Queueing
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
    double time; // time for arrival or departure.
    host *source; //source of the event
    host *dest; //destination of the event
    int subType; // 0 for queue to tx 1 for tx to rx
};

/* create sender and receiver hosts */
host tx, *txPtr, rx, *rxPtr; 
/* List needed for the simulator */
list<event> eventList; //store the simulator events
list<event> inTransitList; //store the link events


/* Init host function - set the properties for the host */
void init_host(host* hostPtr)
{
    hostPtr->Qlength = 0;
    hostPtr->numDropped = 0;
}

/* returns a new event to add to eventList */
event new_event(int type, int subType, host* source, host* dest, double time)
{
    event newEvent;
    newEvent.type = type;
    newEvent.source = source;
    newEvent.dest = dest;
    newEvent.subType = subType;
    /* the time needed for a departure event */
    //TODO check the time values
    if(type == 0) {
        newEvent.time = time + 0.01;
    }

    /* the time required for an arrival event */
    //TODO check the time values
    else if(type == 1) {
        newEvent.time = time + 0.01;
    }

    /* channel sensing event */
    //TODO check the time values
    else if(type == 2) {
        newEvent.time = time + 0.01;
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
    cout << "Transit" << endl;
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
    
    /* variables */
    double currentTime = 0;  //current sim time

    /* Init the hosts */
    txPtr = &tx;
    rxPtr = &rx;
    init_host(txPtr);
    init_host(rxPtr);
    
    /* add the init event */
    /* Add to Q */
    event newEvent = new_event(0, 0, txPtr, txPtr, currentTime);
    add_event(newEvent);
    /* Get the top of the event list */
    //event lastEvent =  eventList.front();
    //oldTime = lastEvent.time;

    /* Perform the iteration for the given number of packets */
    for(int i = 0; i < 100; i++) {
        event currentEvent = eventList.front();
        currentTime = currentEvent.time;
        /* An arrival event */
        if(currentEvent.type == 0) {

            /* Arrival at the Tx */
            if(currentEvent.subType == 0) {
                cout << "Arrival at Tx" << endl;
                /* Add new arrival event */
                event newArrival = new_event(0, 0, currentEvent.source, currentEvent.dest, currentTime);
                add_event(newArrival);

                /* Create new packet */
                packet newPacket;
                newPacket.size = (rand() % 4) + 1; 
                newPacket.serviceTime = 0.001*newPacket.size; //TODO serviceTime? is it random or fixed?

                /* Create departure event for this packet from the tx */
                if(currentEvent.source->Qlength == 0) {
                    currentEvent.source->transmitQ.push(newPacket);
                    event newDeparture = new_event(1, 0, currentEvent.source, currentEvent.dest, currentTime + newPacket.serviceTime);
                    add_event(newDeparture);
                    currentEvent.source->Qlength++;
                }
                else if(currentEvent.source->Qlength > 0 && (currentEvent.source->Qlength < qBuffMAX)) {
                    currentEvent.source->transmitQ.push(newPacket);
                    currentEvent.source->Qlength++;
                }
                else {
                    currentEvent.source->numDropped++;
                }
            }
            /* Arrival of data packet at Rx */
            else if(currentEvent.subType == 1) {
                cout << "Arrival at tx" << endl;
                //TODO Update transmission time
                /* Transmission complete */
                inTransitList.pop_front();
            }

        }
        /*Departure event */
        else if(currentEvent.type == 1) {
            packet newPacket = currentEvent.source->transmitQ.front(); // get the packet to be transmitted from the Q
            //TODO update Q time data
            
            /* Data packet departure */ {
                if(currentEvent.source->Qlength > 0) {
                    /* check if channel is free */
                    if(inTransitList.size() == 0) {
                        /* New arrival for the destination host */
                        event newArrival = new_event(0, 1, currentEvent.dest, currentEvent.source, currentTime + (newPacket.size / (11000000 / 8)));
                        cout << "Departure from destination" << endl;
                        add_event(newArrival);
                        add_transit(newArrival);
                    }

                    else if(inTransitList.size() > 0) {
                        //line is busy
                        //add a channel sense event
                        event channelEvent = new_event(2, 0, currentEvent.source, currentEvent.dest, currentTime);
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
                        event newArrival = new_event(0, 1, currentEvent.dest, currentEvent.source, currentTime + (newPacket.size / (11000000 / 8)));
                        add_event(newArrival);
                        add_transit(newArrival);
                    }

            else if(inTransitList.size() > 0) {
                //line is busy
                //add a channel sense event
                event channelEvent = new_event(2, 0, currentEvent.source, currentEvent.dest, currentTime);
                add_event(channelEvent);
            }
            }
        }

    return 0;
}

