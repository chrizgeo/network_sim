/*  Packet level simulation in a simplex network
    Discrete event simulator
    There are two hosts, one tx and one rx 
    Christo George<chrizgeo@gmail.com>
    Feb 2021
*/

#include "sim.h"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <cstdio>
#include <chrono>
#include <ctime>
using namespace std;

/* Main function */
int main(int argc, char* argv[])
{
    freopen( "log.log", "w", stdout );
    time_t startTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
    cout << " Program start at " << ctime(&startTime) <<endl;
    if(argc==1)
        cout<<"No Arguments Passed- Using defaults for period and deadline"<<endl;
    if(argc>=2)
    {
        cout<<argc<<" Arguments Passed: "<<endl;

        string arg = argv[1];
        try {
            size_t pos;
            PERIOD = stoi(arg, &pos);
            if (pos < arg.size()) {
                cerr << "Trailing characters after number: " << arg << '\n';
            }
        } catch (invalid_argument const &ex) {
            cerr << "Invalid number: " << arg << '\n';
        } catch (out_of_range const &ex) {
            cerr << "Number out of range: " << arg << '\n';
        }

        arg = argv[2];
        try {
            size_t pos;
            DEADLINE = stoi(arg, &pos);
            if (pos < arg.size()) {
                cerr << "Trailing characters after number: " << arg << '\n';
            }
        } catch (invalid_argument const &ex) {
            cerr << "Invalid number: " << arg << '\n';
        } catch (out_of_range const &ex) {
            cerr << "Number out of range: " << arg << '\n';
        }
    }
    cout << "Simulator start" << endl;
    /* Random number seed */
    srand48(time(NULL));

    ofstream statsFile;

    #ifdef PRIORITY_Q 
    char const *qMethod = "PRIO";
    #else
    char const *qMethod = "FCFS";
    #endif  

    /* variables */
    /* simulator variables */
    unsigned long long int currentTime = 0;  //current sim time
    int lineBusy = 0;
    unsigned int waitTime = 0;
    int RTChannelNum = 0;

    txPtr = &tx;
    rxPtr = &rx;

    /* Statistics variables */
    unsigned long long int totalPackets = 1; //total number of packets txd
    unsigned long long int perChannelTotalPackets[maxChannelNUM] = {}; //total number of packets sent per channel
    unsigned long long int queueingDelay = 0; //total queing delay
    unsigned long long int deadlineMisses = 0; // total number of packets which missed deadlines
    unsigned long long int perChannelDeadlineMisses[maxChannelNUM] = {0}; //total number of packets which missed deadlines for each channel
    unsigned long long int txTime = 0;
    double averageThroughput = 0;
    double averageUtilisation = 0;
    double averageDeadlineMissRatio = 0;
    double perChannelAverageDeadlineMissRatio[maxChannelNUM] = {0};
    unsigned long long int averageQueueingDelay = 0;

    /* Init channels */
    cout << PERIOD << " " << DEADLINE <<  endl;
    init_channels(PERIOD, DEADLINE);
    /* Find max acceptable channels */
/*     for(RTChannelNum = 0; RTChannelNum < maxChannelNUM; RTChannelNum++) {
        double utilisation = 0;
        //cout << "Utilisation " << utilisation << endl;
        for(int i = 0; i<RTChannelNum; i++) {
            utilisation += (double)(RTChannelList[i].capacity*packetDuration)/(double)RTChannelList[i].period;
            //cout << "Utilisation " << utilisation << endl;
            if(utilisation >= 1.0)  {
                maxAcceptedChannels = RTChannelNum - 1 ;
                break;
            }
        }
        
        if(maxAcceptedChannels) break;
        cout << endl;
    } */
    
    cout << "Maximum   " << maxAcceptedChannels << "    " << " accepted " << endl;
    /* Each run of the simulation */
    for(RTChannelNum = 0; RTChannelNum < maxAcceptedChannels; RTChannelNum++) {
        cout << RTChannelNum + 1 << " channels active " << endl;
        averageThroughput = 0;
        averageDeadlineMissRatio = 0;
        averageUtilisation = 0;
        perChannelAverageDeadlineMissRatio[maxChannelNUM] = {0};
        averageQueueingDelay = 0;
        /* clear per channel data */
        for(int i = 0; i <= RTChannelNum; i++) {
            perChannelTotalPackets[i] = 1;
            perChannelDeadlineMisses[i] = 0;
        }
        for(int runs = 0; runs < simulatorRUNS; runs++) {
            // Clear all data 
            currentTime = 0;
            totalPackets = 1;
            deadlineMisses = 0;
            queueingDelay = 0;
            txTime = 0;
            lineBusy = 0;
            FEL.clear();
            //cout << txTime << endl;
            /* Init the hosts */
            init_host(txPtr);
            init_host(rxPtr);

            /* Add to Q the first event, channel 0*/
            event newEvent = new_event(0, 0, txPtr, txPtr, currentTime);
            add_event(newEvent);
            //cout << "Event list size " << FEL.size() << endl;
            double nextTime = 0;
            for(int ii = 1; ii <= RTChannelNum; ii++) {
                nextTime = currentTime + (int)nedt(lambda);
                event newEvent = new_event(0, ii, txPtr, txPtr, nextTime);
                add_event(newEvent);
                currentTime = nextTime;
            }

            /* Get the top of the event list and start simulation */
            event lastEvent =  FEL.front();
            currentTime = lastEvent.time;
            packet sentPacket;
            //cout << "start " << lastEvent.time << endl;
            /* Perform the iteration for the given number of packets */
            while(currentTime < simRunTime) {
                event currentEvent = FEL.front();
                //cout << "current Event time " << currentEvent.time << endl;
                //cout << "Event type " << currentEvent.type << endl;
                //cout << "Event list size " << FEL.size() << endl;
                /* An arrival event */
                currentTime = currentEvent.time;
                if(currentEvent.type == 0) {
                    //cout << currentTime << endl;
                    /* Arrival at the Tx, add to transmitQ */
                    /* Create new packet */
                    packet newPacket;
                    newPacket.channelNum = currentEvent.rtChannel;
                    newPacket.size = RTChannelList[newPacket.channelNum].capacity;
                    newPacket.txDelay = (double)newPacket.size*(double)packetDuration; //packets * size is the time taken for Txn
                    newPacket.deadline = currentTime + RTChannelList[newPacket.channelNum].deadLine; 
                    newPacket.arrivalTime = currentEvent.time;
                    newPacket.priority = newPacket.channelNum < (RTChannelNum+1)/2 ? 0 : 1; //channel numbers in the first half has more priority
                    /* Create departure event for this packet from the tx */
                    if(currentEvent.source->transmitQ.size() < qBuffMAX) {
                        currentEvent.source->transmitQ.push(newPacket);
                        event newDeparture = new_event(2, currentEvent.rtChannel, currentEvent.source, currentEvent.dest, currentTime);
                        add_event(newDeparture);
                        //cout << "Added departure" << currentTime << " " << newDeparture.time <<endl;
                        
                    }

                    else {
                        currentEvent.source->numDropped += newPacket.size;
                        //cout << "Dropped a packet due to buffer overflow";
                    }

                    // Add new arrival event to generate new packet 
                    event newArrival = new_event(0, currentEvent.rtChannel, currentEvent.source, currentEvent.dest, (currentEvent.time + RTChannelList[newPacket.channelNum].period ));
                    add_event(newArrival);
                    //cout << "Added new packet gen for " << currentEvent.rtChannel << " at " << newArrival.time << endl;
                }
                else if(currentEvent.type == 2) 
                {
                    if(!lineBusy) {
                        sentPacket = currentEvent.source->transmitQ.top();
                        event newSent = new_event(1, currentEvent.rtChannel, currentEvent.source, currentEvent.dest, currentTime+sentPacket.txDelay);
                        add_event(newSent);
                        //cout << "Queueing delay " << queueingDelay << endl;
                        queueingDelay += currentTime - sentPacket.arrivalTime;
                        waitTime = sentPacket.txDelay;
                        //cout << "Sent packet " << endl;
                        //cout << "Tx time " << sentPacket.txDelay << endl;
                        lineBusy = 1;
                        currentEvent.source->transmitQ.pop();
                    }
                    else {
                        event newWaiting = new_event(2, currentEvent.rtChannel, currentEvent.source, currentEvent.dest, currentTime+waitTime);
                        add_event(newWaiting);
                        //cout << "New waiting " << currentTime+waitTime << endl;
                    }
                }
                /*receive event */
                else if(currentEvent.type == 1) {
                    //TODO update Q time data
                        /* rx has received the data */ 
                            //cout << "New arrival " << currentTime << endl;
                            packet recvdPacket = sentPacket;
                            //packet newPacket = currentEvent.source->transmitQ.top(); // get the packet to be transmitted from the
                            totalPackets += recvdPacket.size;
                            perChannelTotalPackets[recvdPacket.channelNum] += recvdPacket.size;
                            txTime += recvdPacket.txDelay;
                            //cout << "Tx time " << recvdPacket.txDelay << endl;
                            //cout << "Total tx time " << txTime << endl;
                            if(recvdPacket.deadline < currentTime) { 
                                deadlineMisses += recvdPacket.size;
                                perChannelDeadlineMisses[recvdPacket.channelNum] += recvdPacket.size;
                            }
                            //currentEvent.source->transmitQ.pop();
                            lineBusy = 0;
                        }
            FEL.pop_front();
            }
            
            //cout << txTime << endl;
            averageUtilisation += (double)txTime/(double)currentTime;
            averageQueueingDelay += queueingDelay;
            averageThroughput += (double)totalPackets/(double)currentTime;
            //cout << "Deadline misses " << deadlineMisses << " total packets " << totalPackets << endl;
            averageDeadlineMissRatio += (double)deadlineMisses/(double)totalPackets;
            for(int ii = 0; ii <= RTChannelNum; ii++ ) {
                perChannelAverageDeadlineMissRatio[ii] += (double)perChannelDeadlineMisses[ii]/(double)perChannelTotalPackets[ii];
            }
        }

        averageThroughput = averageThroughput/simulatorRUNS;
        averageUtilisation = averageUtilisation/simulatorRUNS;
        averageQueueingDelay = averageQueueingDelay; ///(double)US_IN_S;
        averageQueueingDelay = averageQueueingDelay/simulatorRUNS;
        averageDeadlineMissRatio = averageDeadlineMissRatio/simulatorRUNS;

        for(int ii = 0; ii <= RTChannelNum; ii++ ) {
                perChannelAverageDeadlineMissRatio[ii] = perChannelAverageDeadlineMissRatio[ii]/simulatorRUNS;
        }

        
        cout << " Throughput " << averageThroughput << endl;
        cout << " Utilisation " << averageUtilisation << endl;
        cout << " Queueing Delay " << averageQueueingDelay << endl;
        cout << " Deadline Miss Ratio " << averageDeadlineMissRatio << endl;

        ofstream statsFile;
        statsFile.open("statistics.csv", std::ios_base::app);
        if(statsFile.is_open()) {
            statsFile<<qMethod<<","<<RTChannelNum<<","<<RTChannelList[RTChannelNum].period<<","<<RTChannelList[RTChannelNum].deadLine<<","<<RTChannelList[RTChannelNum].capacity<<","<<averageThroughput<<","<<averageUtilisation<<","<<averageQueueingDelay<<","<<averageDeadlineMissRatio;
            for(int ii = 0; ii < maxAcceptedChannels; ii++) {
                cout << " Deadline Miss Ratio " << ii << " " << perChannelAverageDeadlineMissRatio[ii] << endl;
                statsFile<<","<<perChannelAverageDeadlineMissRatio[ii]; 
            }
            statsFile<<endl;    
            statsFile.close();
        }
        else {
            cout << "Unable to open file" << endl;
        }
    }

    fclose(stdout);
    return 0;

}

