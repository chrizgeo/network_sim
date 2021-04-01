/*  Packet level simulation in a simplex network
    Discrete event simulator
    There are two hosts, one tx and one rx 
    Christo George<chrizgeo@gmail.com>
    Feb 2021
*/
//TODO add file and  plot the throughput for many number of simulations
#include "sim.h"
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

using namespace std;

/* Main function */
int main()
{
    cout << "Simulator start" << endl;
    /* Random number seed */
    srand48(time(NULL));
    unsigned long long int simRunTime = 150000;
    string fileName;
    string fileRoot;

    vector<double>  tp, dmr, avqd, chNum;
    vector<double*> perChanDmr;
    /* variables */
    /* simulator variables */
    unsigned long long int currentTime = 0;  //current sim time
    int lineBusy = 0;
    unsigned int waitTime = 0;
    int RTChannelNum = 0;
    int maxAcceptedChannels = maxChannelNUM;
    txPtr = &tx;
    rxPtr = &rx;

    /* Statistics variables */
    unsigned long long int totalPackets = 0; //total number of packets txd
    unsigned long long int perChannelTotalPackets[maxChannelNUM] = {0}; //total number of packets sent per channel
    unsigned long long int queueingDelay = 0; //total queing delay
    unsigned long long int deadlineMisses = 0; // total number of packets which missed deadlines
    unsigned long long int perChannelDeadlineMisses[maxChannelNUM] = {0}; //total number of packets which missed deadlines for each channel
    double averageThroughput = 0;
    double averageUtilisation = 0;
    double averageDeadlineMissRatio = 0;
    double perChannelAverageDeadlineMissRatio[maxChannelNUM] = {0};
    unsigned long long int averageQueueingDelay = 0;

    /* Init channels */
    init_channels();
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
        perChannelAverageDeadlineMissRatio[maxChannelNUM] = {0};
        averageQueueingDelay = 0;
        for(int runs = 0; runs < simulatorRUNS; runs++) {
            // Clear all data 
            currentTime = 0;
            totalPackets = 0;
            perChannelTotalPackets[maxChannelNUM] = {0};
            deadlineMisses = 0;
            perChannelDeadlineMisses[maxChannelNUM] = {0};
            queueingDelay = 0;

            lineBusy = 0;
            FEL.clear();

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
                        packet topPacket = currentEvent.source->transmitQ.top();
                        event newSent = new_event(1, currentEvent.rtChannel, currentEvent.source, currentEvent.dest, currentTime+topPacket.txDelay);
                        add_event(newSent);
                        //cout << "Queueing delay " << queueingDelay << endl;
                        queueingDelay += currentTime - topPacket.arrivalTime;
                        waitTime = topPacket.txDelay;
                        //cout << "Sent packet " << endl;
                        lineBusy = 1;
                    }
                    else {
                        event newWaiting = new_event(2, currentEvent.rtChannel, currentEvent.source, currentEvent.dest, currentTime+waitTime);
                        add_event(newWaiting);
                        //cout << "New waiting event " << currentTime+125 << endl;
                    }
                }
                /*receive event */
                else if(currentEvent.type == 1) {
                    //TODO update Q time data
                        /* rx has received the data */ 
                            packet newPacket = currentEvent.source->transmitQ.top(); // get the packet to be transmitted from the
                            totalPackets += newPacket.size;
                            perChannelTotalPackets[newPacket.channelNum] += newPacket.size;
                            if(newPacket.deadline < currentTime) { 
                                deadlineMisses += newPacket.size;
                                perChannelDeadlineMisses[newPacket.channelNum] += newPacket.size;
                            }
                            currentEvent.source->transmitQ.pop();
                            lineBusy = 0;
                            
                }
            FEL.pop_front();
            }
            

            averageQueueingDelay += queueingDelay;
            averageThroughput += (double)totalPackets/(double)currentTime;
            //cout << "Deadline misses " << deadlineMisses << " total packets " << totalPackets << endl;
            averageDeadlineMissRatio += (double)deadlineMisses/(double)totalPackets;
            for(int ii = 0; ii <= RTChannelNum; ii++ ) {
                perChannelAverageDeadlineMissRatio[ii] += perChannelDeadlineMisses[ii]/perChannelTotalPackets[ii];
            }
        }

        averageThroughput = averageThroughput/simulatorRUNS;
        averageQueueingDelay = averageQueueingDelay/simulatorRUNS;
        averageDeadlineMissRatio = averageDeadlineMissRatio/simulatorRUNS;

        for(int ii = 0; ii <= RTChannelNum; ii++ ) {
                perChannelAverageDeadlineMissRatio[ii] = perChannelAverageDeadlineMissRatio[ii]/simulatorRUNS;
        }

        perChanDmr.push_back(perChannelAverageDeadlineMissRatio);
        cout << " Throughput " << averageThroughput << endl;
        cout << " Queueing Delay " << averageQueueingDelay << endl;
        cout << " Deadline Miss Ratio " << averageDeadlineMissRatio << endl;
        double* ptr = perChanDmr.front();
        for(int ii = 0; ii <= RTChannelNum; ii++) {
            cout << " Deadline Miss Ratio " << ii << " " << ptr[ii] << endl;
        }
        chNum.push_back(RTChannelNum);
        tp.push_back(averageThroughput);
        dmr.push_back(averageDeadlineMissRatio);
        avqd.push_back(averageQueueingDelay);
    }

    plt::plot(chNum, tp);
    plt::title("Number of channels vs throughput");
    fileName = "../images/PQ_" + to_string(maxChannelNUM) + "tp_chan.png";
    plt::save(fileName);
    plt::close();
    plt::plot(chNum, dmr);
    //for(int ii = 0; ii < maxChannelNUM; ii++ ) {
    //    plt::plot(chNum, dmrPerChan(ii));
    //    }
    plt::title(" Number of channels vs Deadline miss ratio");
    fileName = "../images/PQ_" + to_string(maxChannelNUM) + "DMR_chan.png";
    plt::save(fileName);
    plt::close();
    plt::plot(chNum, avqd);
    plt::title(" Number of channels vs Average Queueing Delay");
    fileName = "../images/PQ_" + to_string(maxChannelNUM) + "AQD_chan.png";
    plt::save(fileName);
    plt::close();
    return 0;

}

