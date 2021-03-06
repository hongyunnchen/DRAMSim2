/*********************************************************************************
*  Copyright (c) 2010-2011, Elliott Cooper-Balis
*                             Paul Rosenfeld
*                             Bruce Jacob
*                             University of Maryland 
*                             dramninjas [at] gmail [dot] com
*  All rights reserved.
*  
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*  
*     * Redistributions of source code must retain the above copyright notice,
*        this list of conditions and the following disclaimer.
*  
*     * Redistributions in binary form must reproduce the above copyright notice,
*        this list of conditions and the following disclaimer in the documentation
*        and/or other materials provided with the distribution.
*  
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************/








//CommandQueue.cpp
//
//Class file for command queue object
//

#include "CommandQueue.h"
#include "MemoryController.h"
#include <assert.h>

using namespace DRAMSim;

CommandQueue::CommandQueue(vector < vector < BankState > >&states, ostream & dramsim_log_):
dramsim_log(dramsim_log_),
bankStates(states),
nextBank(0),
nextRank(0),
nextBankPRE(0),
nextRankPRE(0), refreshRank(0), refreshWaiting(false), sendAct(true)
{
  //set here to avoid compile errors
  currentClockCycle = 0;

  //use numBankQueus below to create queue structure
  size_t numBankQueues;
  if (queuingStructure == PerRank) {
    numBankQueues = 1;
  } else if (queuingStructure == PerRankPerBank) {
    numBankQueues = NUM_BANKS;
  } else {
    ERROR("== Error - Unknown queuing structure");
    exit(0);
  }

  //vector of counters used to ensure rows don't stay open too long
  rowAccessCounters =
    vector < vector < unsigned > >(NUM_RANKS, vector < unsigned >(NUM_BANKS, 0));

  //create queue based on the structure we want
  BusPacket1D actualQueue;
  BusPacket2D perBankQueue = BusPacket2D();
  queues = BusPacket3D();
  for (size_t rank = 0; rank < NUM_RANKS; rank++) {
    //this loop will run only once for per-rank and NUM_BANKS times for per-rank-per-bank
    for (size_t bank = 0; bank < numBankQueues; bank++) {
      actualQueue = BusPacket1D();
      perBankQueue.push_back(actualQueue);
    }
    queues.push_back(perBankQueue);
  }


  //FOUR-bank activation window
  //      this will count the number of activations within a given window
  //      (decrementing counter)
  //
  //countdown vector will have decrementing counters starting at tFAW
  //  when the 0th element reaches 0, remove it
  tFAWCountdown.reserve(NUM_RANKS);
  for (size_t i = 0; i < NUM_RANKS; i++) {
    //init the empty vectors here so we don't seg fault later
    tFAWCountdown.push_back(vector < unsigned >());
  }
  
  // update the map data structures for last accessed row and time-stamp
  
  for (int i = 0; i < NUM_RANKS; i++) {
    bankLastRWTimeStampMapType bankLastRWTimeStampMap;
    bankLastAccessedRowMapType bankLastAccessedRowMap; 
    bankLastPrechargeTimeStampMapType bankLastPrechargeTimeStampMap;
    for (int j = 0; j < NUM_BANKS; j++) {
      // -1 denotes that no row has been accessed == idle bank
      bankLastRWTimeStampMap.insert(bankLastRWTimeStampPairType(j, 0));
   //   bankLastAccessedRowMap.insert(bankLastAccessedRowPairType(j, -1));
      bankLastPrechargeTimeStampMap.insert(bankLastPrechargeTimeStampPairType(j, 0));
    }
    rankBankTimeStampMap.insert(rankBankTimeStampPairType
                                (i, bankLastRWTimeStampMap));
    rankBankLastAccessedRowMap.insert(rankBankLastAccessedRowPairType(i, bankLastAccessedRowMap));
    rankBankLastPrechargeTimeStampMap.insert(rankBankLastPrechargeTimeStampPairType(i, bankLastPrechargeTimeStampMap));
  }
  // initial value of mistake counter
  mistakeCounter = 25;
}

CommandQueue::~CommandQueue()
{
  //ERROR("COMMAND QUEUE destructor");
  size_t bankMax = NUM_RANKS;
  if (queuingStructure == PerRank) {
    bankMax = 1;
  }
  for (size_t r = 0; r < NUM_RANKS; r++) {
    for (size_t b = 0; b < bankMax; b++) {
      for (size_t i = 0; i < queues[r][b].size(); i++) {
        delete(queues[r][b][i]);
      }
      queues[r][b].clear();
    }
  }
}

//Adds a command to appropriate queue
void CommandQueue::enqueue(BusPacket * newBusPacket)
{
  unsigned rank = newBusPacket->rank;
  unsigned bank = newBusPacket->bank;
  if (queuingStructure == PerRank) {
    queues[rank][0].push_back(newBusPacket);
    if (queues[rank][0].size() > CMD_QUEUE_DEPTH) {
      ERROR("== Error - Enqueued more than allowed in command queue");
      ERROR
        ("						Need to call .hasRoomFor(int numberToEnqueue, unsigned rank, unsigned bank) first");
      exit(0);
    }
  } else if (queuingStructure == PerRankPerBank) {
    //cout<<"\n Pushed commands into queue : "<<rank<<" " <<bank;
    queues[rank][bank].push_back(newBusPacket);
    if (queues[rank][bank].size() > CMD_QUEUE_DEPTH) {
      ERROR("== Error - Enqueued more than allowed in command queue");
      ERROR
        ("						Need to call .hasRoomFor(int numberToEnqueue, unsigned rank, unsigned bank) first");
      exit(0);
    }
  } else {
    ERROR("== Error - Unknown queuing structure");
    exit(0);
  }
}

//Removes the next item from the command queue based on the system's
//command scheduling policy
void CommandQueue::updateBankLastAccessedRow(unsigned rank, unsigned bank, int row) {
  /*
  //cout<<"\n Updating rank : " <<rank<<" bank : " <<bank<<" row :" <<row;
  // For debug, displaying contents of rankBankLastAccessedRowMap
  for (int i = 0; i < NUM_RANKS; i++) {
    bankLastAccessedRowMapType bankLastAccessedRowMap = rankBankLastAccessedRowMap[i];
    for (bankLastAccessedRowMapType::iterator it =
         bankLastAccessedRowMap.begin(), eit = bankLastAccessedRowMap.end();
         it != eit; it++) {

      cout << "\n RANK : " << i << " BANK : " << it->first << " ROW : "
      //  << it->second;
    }
  }
  // end debug
  */
  if (rankBankLastAccessedRowMap.find(rank) != rankBankLastAccessedRowMap.end()) {

    rankBankLastAccessedRowMapType::iterator rankFound =
      rankBankLastAccessedRowMap.find(rank);
    bankLastAccessedRowMapType bankLastAccessedRowMap = rankFound->second;
    bankLastAccessedRowMap[bank] = row;
    rankBankLastAccessedRowMap.erase(rankFound);
    rankBankLastAccessedRowMap.insert(rankBankLastAccessedRowPairType
                                (rank, bankLastAccessedRowMap));
  } else {
    cout << "\n ERROR : Cannot find rank time-stamp";
    exit(0);
  } 
}

void CommandQueue::updateBankLastPrecharge(unsigned rank, unsigned bank, unsigned currentClockCycle) {
  /*
  // For debug, displaying contents of rankBankTimeStampMap
  for (int i = 0; i < NUM_RANKS; i++) {
    bankLastPrechargeTimeStampMapType bankLastPrechargeTimeStampMap = rankBankLastPrechargeTimeStampMap[i];
    for (bankLastPrechargeTimeStampMapType::iterator it =
         bankLastPrechargeTimeStampMap.begin(), eit = bankLastPrechargeTimeStampMap.end();
         it != eit; it++) {

      cout << "\n RANK : " << i << " BANK : " << it->first << " TIME STAMP : "
        << it->second;
    }
  }
  // end debug
  */
  if (rankBankLastPrechargeTimeStampMap.find(rank) != rankBankLastPrechargeTimeStampMap.end()) {

    rankBankLastPrechargeTimeStampMapType::iterator rankFound =
      rankBankLastPrechargeTimeStampMap.find(rank);
    bankLastPrechargeTimeStampMapType bankLastPrechargeTimeStampMap = rankFound->second;
    bankLastPrechargeTimeStampMap[bank] = currentClockCycle;
    rankBankLastPrechargeTimeStampMap.erase(rankFound);
    rankBankLastPrechargeTimeStampMap.insert(rankBankLastPrechargeTimeStampPairType
                                (rank, bankLastPrechargeTimeStampMap));
  } else {
    cout << "\n ERROR : Cannot find rank time-stamp";
    exit(0);
  }


}
void CommandQueue::updateBankRWTimeStamp(unsigned rank, unsigned bank,
                                         unsigned currentClockCycle)
{
  /*
  // For debug, displaying contents of rankBankTimeStampMap
  for (int i = 0; i < NUM_RANKS; i++) {
    bankLastRWTimeStampMapType bankLastRWTimeStampMap = rankBankTimeStampMap[i];
    for (bankLastRWTimeStampMapType::iterator it =
         bankLastRWTimeStampMap.begin(), eit = bankLastRWTimeStampMap.end();
         it != eit; it++) {

      cout << "\n RANK : " << i << " BANK : " << it->first << " TIME STAMP : "
        << it->second;
    }
  }
  // end debug
  */
  if (rankBankTimeStampMap.find(rank) != rankBankTimeStampMap.end()) {

    rankBankTimeStampMapType::iterator rankFound =
      rankBankTimeStampMap.find(rank);
    bankLastRWTimeStampMapType bankLastRWTimeStampMap = rankFound->second;
    bankLastRWTimeStampMap[bank] = currentClockCycle;
    rankBankTimeStampMap.erase(rankFound);
    rankBankTimeStampMap.insert(rankBankTimeStampPairType
                                (rank, bankLastRWTimeStampMap));
  } else {
    cout << "\n ERROR : Cannot find rank time-stamp";
    exit(0);
  }
}

void CommandQueue::adjustTimeOut() {
  //cout<<"\n Mistake Counter : " <<mistakeCounter;
  //cout<<"\n TIME_LIMIT : " <<TIME_LIMIT;
  //cout<<"\n HIGH_THRESHOLD : " <<HIGH_THRESHOLD;
  //cout<<"\n LOW_THRESHOLD  : " <<LOW_THRESHOLD;
  if (mistakeCounter >= HIGH_THRESHOLD) {
		HIGH_THRESHOLD = mistakeCounter;
		if (HIGH_THRESHOLD - LOW_THRESHOLD > 10) {
			LOW_THRESHOLD = LOW_THRESHOLD + 1;
		}
		if (TIME_LIMIT <= 100) {
    	++TIME_LIMIT;
		}
    //cout<<"\n New Time out value : " <<TIME_LIMIT; 
  }
  else if (mistakeCounter <= LOW_THRESHOLD) {
		LOW_THRESHOLD = mistakeCounter;
		if (HIGH_THRESHOLD - LOW_THRESHOLD > 10) {
			HIGH_THRESHOLD = HIGH_THRESHOLD - 1;
		}
    if (TIME_LIMIT > 0) {
      --TIME_LIMIT;
    }
    //cout<<"\n New Time out value : " <<TIME_LIMIT;
  }
}

bool CommandQueue::pop(BusPacket ** busPacket)
{
  
  // Lets check the mistakeCounter every 30 cycles
  if (rowBufferPolicy == AdaptiveOpenPage) {
   if (currentClockCycle % 10 == 0) {
     adjustTimeOut();
   }
  }

  //this can be done here because pop() is called every clock cycle by the parent MemoryController
  //      figures out the sliding window requirement for tFAW
  //
  //deal with tFAW book-keeping
  //      each rank has it's own counter since the restriction is on a device level
  for (size_t i = 0; i < NUM_RANKS; i++) {
    //decrement all the counters we have going
    for (size_t j = 0; j < tFAWCountdown[i].size(); j++) {
      tFAWCountdown[i][j]--;
    }

    //the head will always be the smallest counter, so check if it has reached 0
    if (tFAWCountdown[i].size() > 0 && tFAWCountdown[i][0] == 0) {
      tFAWCountdown[i].erase(tFAWCountdown[i].begin());
    }
  }

  /* Now we need to find a packet to issue. When the code picks a packet, it will set
   *busPacket = [some eligible packet]

   First the code looks if any refreshes need to go
   Then it looks for data packets
   Otherwise, it starts looking for rows to close (in open page)
   */

  if (rowBufferPolicy == ClosePage) {
    bool sendingREF = false;
    //if the memory controller set the flags signaling that we need to issue a refresh
    if (refreshWaiting) {
      bool foundActiveOrTooEarly = false;
      //look for an open bank
      for (size_t b = 0; b < NUM_BANKS; b++) {
        vector < BusPacket * >&queue = getCommandQueue(refreshRank, b);
        //checks to make sure that all banks are idle
        if (bankStates[refreshRank][b].currentBankState == RowActive) {
          foundActiveOrTooEarly = true;
          //if the bank is open, make sure there is nothing else
          // going there before we close it
          for (size_t j = 0; j < queue.size(); j++) {
            BusPacket *packet = queue[j];
            if (packet->row == bankStates[refreshRank][b].openRowAddress &&
                packet->bank == b) {
              if (packet->busPacketType != ACTIVATE && isIssuable(packet)) {
                *busPacket = packet;
                queue.erase(queue.begin() + j);
                sendingREF = true;
              }
              break;
            }
          }

          break;
        }
        //      NOTE: checks nextActivate time for each bank to make sure tRP is being
        //                              satisfied.      the next ACT and next REF can be issued at the same
        //                              point in the future, so just use nextActivate field instead of
        //                              creating a nextRefresh field
        else if (bankStates[refreshRank][b].nextActivate > currentClockCycle) {
          foundActiveOrTooEarly = true;
          break;
        }
      }

      //if there are no open banks and timing has been met, send out the refresh
      //      reset flags and rank pointer
      if (!foundActiveOrTooEarly
          && bankStates[refreshRank][0].currentBankState != PowerDown) {
        *busPacket =
          new BusPacket(REFRESH, 0, 0, 0, refreshRank, 0, 0, currentClockCycle,
                        dramsim_log);
        
        refreshRank = -1;
        refreshWaiting = false;
        sendingREF = true;
        // Update last accessed banks with idle status
 
      }
    }                           // refreshWaiting

    //if we're not sending a REF, proceed as normal
    if (!sendingREF) {
      bool foundIssuable = false;
      unsigned startingRank = nextRank;
      unsigned startingBank = nextBank;
      do {
        vector < BusPacket * >&queue = getCommandQueue(nextRank, nextBank);
        //make sure there is something in this queue first
        //      also make sure a rank isn't waiting for a refresh
        //      if a rank is waiting for a refesh, don't issue anything to it until the
        //              refresh logic above has sent one out (ie, letting banks close)
        if (!queue.empty() && !((nextRank == refreshRank) && refreshWaiting)) {
          if (queuingStructure == PerRank) {
            //search from beginning to find first issuable bus packet
            for (size_t i = 0; i < queue.size(); i++) {
              if (isIssuable(queue[i])) {
                //check to make sure we aren't removing a read/write that is paired with an activate
                if (i > 0 && queue[i - 1]->busPacketType == ACTIVATE &&
                    queue[i - 1]->physicalAddress == queue[i]->physicalAddress)
                  continue;

                *busPacket = queue[i];
                queue.erase(queue.begin() + i);
                foundIssuable = true;
                break;
              }
            }
          } else {
            if (isIssuable(queue[0])) {

              //no need to search because if the front can't be sent,
              // then no chance something behind it can go instead
              *busPacket = queue[0];
              queue.erase(queue.begin());
              foundIssuable = true;
            }
          }

        }
        //if we found something, break out of do-while
        if (foundIssuable)
          break;

        //rank round robin
        if (queuingStructure == PerRank) {
          nextRank = (nextRank + 1) % NUM_RANKS;
          if (startingRank == nextRank) {
            break;
          }
        } else {
          if (schedulingPolicy == BankThenRankRoundRobin
              || schedulingPolicy == RankThenBankRoundRobin) {
            nextRankAndBank(nextRank, nextBank);
            if (startingRank == nextRank && startingBank == nextBank) {
              break;
            }
          } else {
            // fifo scheduling policy
            // scan the bank queues for the head with the earliest time-stamp
            // return the bank and rank of that requesti
            nextRankAndBank(nextRank, nextBank);
            break;
          }
        }
      }
      while (true);

      //if we couldn't find anything to send, return false
      if (!foundIssuable)
        return false;
    }
  } else if (rowBufferPolicy == OpenPageTimeOut
             || rowBufferPolicy == OpenPageNumAccessesLimit
             || rowBufferPolicy == OpenPageInFlightRequests 
             || rowBufferPolicy == AdaptiveOpenPage) {
    //cout<<"\n ROW ACCESS COUNTER : " <<TOTAL_ROW_ACCESSES;
		bool sendingREForPRE = false;
    if (refreshWaiting) {
      bool sendREF = true;
      //make sure all banks idle and timing met for a REF
      for (size_t b = 0; b < NUM_BANKS; b++) {
        //if a bank is active we can't send a REF yet
        if (bankStates[refreshRank][b].currentBankState == RowActive) {
          sendREF = false;
          bool closeRow = true;
          //search for commands going to an open row
          vector < BusPacket * >&refreshQueue = getCommandQueue(refreshRank, b);

          for (size_t j = 0; j < refreshQueue.size(); j++) {
            BusPacket *packet = refreshQueue[j];
            //if a command in the queue is going to the same row . . .
            if (bankStates[refreshRank][b].openRowAddress == packet->row &&
                b == packet->bank) {
              // . . . and is not an activate . . .
              if (packet->busPacketType != ACTIVATE) {
                closeRow = false;
                // . . . and can be issued . . .
                if (isIssuable(packet)) {
                  //send it out
                  *busPacket = packet;
                  refreshQueue.erase(refreshQueue.begin() + j);
                  sendingREForPRE = true;
                  // update bank rw time-stamp data structure with the new time-stamp
                  //updateBankRWTimeStamp(packet->rank, packet->bank,
                  //                      currentClockCycle);
                  //updateBankLastAccessedRow(packet->rank, packet->bank, packet->row);
                }
                break;
              } else            //command is an activate
              {
                //if we've encountered another act, no other command will be of interest
                break;
              }
            }
          }

          //if the bank is open and we are allowed to close it, then send a PRE
          // ANI :Have a look in this code as weell
          if (closeRow
              && currentClockCycle >=
              bankStates[refreshRank][b].nextPrecharge) {
            rowAccessCounters[refreshRank][b] = 0;
            *busPacket =
              new BusPacket(PRECHARGE, 0, 0, 0, refreshRank, b, 0,
                            currentClockCycle, dramsim_log);
            sendingREForPRE = true;
            //updateBankLastPrecharge(refreshRank, b, currentClockCycle); 
            //updateBankLastAccessedRow(refreshRank, b, -1);
          }
          break;
        }
        //      NOTE: the next ACT and next REF can be issued at the same
        //                              point in the future, so just use nextActivate field instead of
        //                              creating a nextRefresh field
        else if (bankStates[refreshRank][b].nextActivate > currentClockCycle)   //and this bank doesn't have an open row
        {
          sendREF = false;
          break;
        }
      }

      //if there are no open banks and timing has been met, send out the refresh
      //      reset flags and rank pointer
      if (sendREF && bankStates[refreshRank][0].currentBankState != PowerDown) {
        *busPacket =
          new BusPacket(REFRESH, 0, 0, 0, refreshRank, 0, 0, currentClockCycle,
                        dramsim_log);

        refreshRank = -1;
        refreshWaiting = false;
        sendingREForPRE = true;

      }
    }

    if (!sendingREForPRE) {
      unsigned startingRank = nextRank;
      unsigned startingBank = nextBank;
      bool foundIssuable = false;
      do                        // round robin over queues
      {
        vector < BusPacket * >&queue = getCommandQueue(nextRank, nextBank);
        //cout<<"\n getting rank : " <<nextRank<<" bank : " <<nextBank<<" queue size : " <<queue.size();
        //make sure there is something there first
        if (!queue.empty() && !((nextRank == refreshRank) && refreshWaiting)) {
          //search from the beginning to find first issuable bus packet								
           if (schedulingPolicy == BankThenRankRoundRobin || schedulingPolicy == Fifo) {
						// only look at the head of the queue to issue the request	 
					 		// if activate is not issuable because the row is open, then else clause with check for next request
							if(isIssuable(queue[0])){
								BusPacket *packet = queue[0];
								queue.erase(queue.begin());
								foundIssuable = true;
								*busPacket = packet;							
                //updateBankLastAccessedRow(packet->rank, packet->bank, packet->row);
								//updateBankRWTimeStamp(packet->rank, packet->bank, currentClockCycle);	
							}
							else {
								if (queue[0]->busPacketType == ACTIVATE) {
									if (isIssuable(queue[1])) {
										BusPacket *packet = queue[1];
										delete(queue[0]);
										queue.erase(queue.begin(), queue.begin()+2);
										*busPacket = packet;										
										foundIssuable = true;
                  //	updateBankLastAccessedRow(packet->rank, packet->bank, packet->row);
									//	updateBankRWTimeStamp(packet->rank, packet->bank, currentClockCycle);
									}
								}
							}
					 }
					 else { 
						for (size_t i = 0; i < queue.size(); i++) {
              BusPacket *packet = queue[i];
              //check for dependencies
              if (isIssuable(packet)) {
                // Check if the packet is not activate
                if (packet->busPacketType != ACTIVATE) {
                  //updateBankRWTimeStamp(packet->rank, packet->bank,
                    //                    currentClockCycle);
                  
                  //updateBankLastAccessedRow(packet->rank, packet->bank, packet->row);
                }
                bool dependencyFound = false;
                for (size_t j = 0; j < i; j++) {
                  BusPacket *prevPacket = queue[j];
                  if (prevPacket->busPacketType != ACTIVATE &&
                      prevPacket->bank == packet->bank &&
                      prevPacket->row == packet->row) {
                    dependencyFound = true;
                    break;
                  }
                }
                if (dependencyFound)
                  continue;

                *busPacket = packet;

                //if the bus packet before is an activate, that is the act that was
                //      paired with the column access we are removing, so we have to remove
                //      that activate as well (check i>0 because if i==0 then theres nothing before it)
                if (i > 0 && queue[i - 1]->busPacketType == ACTIVATE) {
                  //rowAccessCounters[(*busPacket)->rank][(*busPacket)->bank]++;
									// i is being returned, but i-1 is being thrown away, so must delete it here 
                  delete(queue[i - 1]);

                  // remove both i-1 (the activate) and i (we've saved the pointer in *busPacket)
                  queue.erase(queue.begin() + i - 1, queue.begin() + i + 1);
									/*
									cout<<"\n Finish clock cycle1 : " <<currentClockCycle;
									prevFinishSCycle += currentClockCycle - prevFinishSCycle + currentSCycleDiff;
									cout<<"\n prevFinishSCycle : " <<prevFinishSCycle;
									*/
                } 
								else          // there's no activate before this packet
                {
                  //or just remove the one bus packet
                  queue.erase(queue.begin() + i);
									/*
									cout<<"\n Finish clock cycle2 : " <<currentClockCycle;
									prevFinishSCycle += currentClockCycle - prevFinishSCycle + currentSCycleDiff;
									cout<<"\n prevFinishSCycle : " <<prevFinishSCycle;
									*/
                }

                foundIssuable = true;
                break;
              }
            }
					}
        }
        //if we found something, break out of do-while
        if (foundIssuable)
          break;

        //rank round robin
        if (queuingStructure == PerRank) {
          nextRank = (nextRank + 1) % NUM_RANKS;
          if (startingRank == nextRank) {
            break;
          }
        } else {
          if (schedulingPolicy == BankThenRankRoundRobin
              || schedulingPolicy == Frfcfs) {
            nextRankAndBank(nextRank, nextBank);

						if (startingRank == nextRank && startingBank == nextBank) {
              break;
            }
          } else {
            // fifo scheduling policy
            // scan the bank queues for the head with the earliest time-stamp
            // return the bank and rank of that requesti
            nextRankAndBank(nextRank, nextBank);
            break;
          }
        }
      }
      while (true);

      //if nothing was issuable, see if we can issue a PRE to an open bank
      //      that has no other commands waiting
      if (!foundIssuable) {
        //search for banks to close
        bool sendingPRE = false;
        unsigned startingRank = nextRankPRE;
        unsigned startingBank = nextBankPRE;

        do                      // round robin over all ranks and banks
        {
          vector < BusPacket * >&queue =
            getCommandQueue(nextRankPRE, nextBankPRE);

          //check if bank is open

          //if nothing found going to that bank and row or too many accesses have happend, close it
          // there are three flavors to open-page:
          // 1. issue the PRE command after a number of row accesses
          // 2. issue the PRE command after a time-limit
          // 3. issue the PRE command if no commands are in flight
          // Open page time out page-policy is a bit buggy
					if (rowBufferPolicy == OpenPageTimeOut || rowBufferPolicy == AdaptiveOpenPage) {

            bool found = false;
            if (bankStates[nextRankPRE][nextBankPRE].currentBankState ==
                RowActive) {
            
              if (schedulingPolicy == BankThenRankRoundRobin || schedulingPolicy == Fifo) {								
								if (queue[0]->bank == nextBankPRE && queue[0]->row == bankStates[nextRankPRE][nextBankPRE].openRowAddress) {
									found = true;
								}
							}
							else {
							for (size_t i = 0; i < queue.size(); i++) {
                //if there is something going to that bank and row, then we don't want to send a PRE
                if (queue[i]->bank == nextBankPRE &&
                    queue[i]->row ==
                    bankStates[nextRankPRE][nextBankPRE].openRowAddress) {
                  found = true;
                  break;
                }
              }
							}

              rankBankTimeStampMapType::iterator rankFound =
                rankBankTimeStampMap.find(nextRankPRE);
              bankLastRWTimeStampMapType bankLastRWTimeStampMap =
                rankFound->second;
              //cout << "\n TIME LIMIT : " << TIME_LIMIT;
              //cout << "\n bank : " << nextBankPRE << " last access time : " <<
               // bankLastRWTimeStampMap[nextBankPRE];

							// 2 cases
							// 1. No requests and time_limit has met, issue precharge
							// 2. Queue is not empty and no open row requests found, issue precharge
              if ((!found && queue.size() != 0) || (queue.size() == 0 && currentClockCycle > bankLastRWTimeStampMap[nextBankPRE] && currentClockCycle - bankLastRWTimeStampMap[nextBankPRE] > TIME_LIMIT )){
               	
								/*
								cout <<"\n Issuing precharge because \n";
								cout<<"\n Found : " <<found;
								cout<<"\n queue size : " <<queue.size();
								cout<<"\n currentClockCycle : " <<currentClockCycle;
								cout<<"\n bankLastRWTimeStampMap["<<nextBankPRE<<"] : " <<bankLastRWTimeStampMap[nextBankPRE];
								cout<<"\n TIME LIMIT : " <<TIME_LIMIT;
								*/
								if (currentClockCycle >=
                    bankStates[nextRankPRE][nextBankPRE].nextPrecharge) {
                  sendingPRE = true;
                  rowAccessCounters[nextRankPRE][nextBankPRE] = 0;
                  *busPacket =
                    new BusPacket(PRECHARGE, 0, 0, 0, nextRankPRE, nextBankPRE,
                                  0, currentClockCycle, dramsim_log);
                                    
            			updateBankLastPrecharge(nextRankPRE, nextBankPRE, currentClockCycle); 
									break;
                }
              }
            }
          } else if (rowBufferPolicy == OpenPageNumAccessesLimit) {
             
             bool found = false;
             if (bankStates[nextRankPRE][nextBankPRE].currentBankState ==
                RowActive) {
              if (schedulingPolicy == BankThenRankRoundRobin || schedulingPolicy == Fifo) {								
								if (queue[0]->bank == nextBankPRE && queue[0]->row == bankStates[nextRankPRE][nextBankPRE].openRowAddress) {
									found = true;
								}
							}
							else {
							for (size_t i = 0; i < queue.size(); i++) {
                //if there is something going to that bank and row, then we don't want to send a PRE
                if (queue[i]->bank == nextBankPRE &&
                    queue[i]->row ==
                    bankStates[nextRankPRE][nextBankPRE].openRowAddress) {
                  found = true;
                  break;
                }
              }
							}
              if (!found
                  || rowAccessCounters[nextRankPRE][nextBankPRE] ==
                  TOTAL_ROW_ACCESSES) {
                if (currentClockCycle >=
                    bankStates[nextRankPRE][nextBankPRE].nextPrecharge) {
                  sendingPRE = true;
                  /*
									cout<<"\n Found value : " <<found;
									cout<<"\n rwAccessCounters : " <<rowAccessCounters[nextRankPRE][nextBankPRE];
									cout<<"\n Set row access counters for bank : " <<nextBankPRE<<" rank : " <<nextRankPRE<<" to zero";
									*/
                  rowAccessCounters[nextRankPRE][nextBankPRE] = 0;
									*busPacket =
                    new BusPacket(PRECHARGE, 0, 0, 0, nextRankPRE, nextBankPRE,
                                  0, currentClockCycle, dramsim_log);
                  break;
                }
              }
            }
          } else if (rowBufferPolicy == OpenPageInFlightRequests) {            
            bool found = false;
            if (bankStates[nextRankPRE][nextBankPRE].currentBankState ==
                RowActive) {
              for (size_t i = 0; i < queue.size(); i++) {
                //if there is something going to that bank and row, then we don't want to send a PRE
                if (queue[i]->bank == nextBankPRE &&
                    queue[i]->row ==
                    bankStates[nextRankPRE][nextBankPRE].openRowAddress) {
                  found = true;
                  break;
                }
              }

              if (!found) {
                if (currentClockCycle >=
                    bankStates[nextRankPRE][nextBankPRE].nextPrecharge) {
                  sendingPRE = true;
                  rowAccessCounters[nextRankPRE][nextBankPRE] = 0;
                  *busPacket =
                    new BusPacket(PRECHARGE, 0, 0, 0, nextRankPRE, nextBankPRE,
                                  0, currentClockCycle, dramsim_log);
                  break;
                }
              }
            }
          }

          nextRankAndBank(nextRankPRE, nextBankPRE);
          if (schedulingPolicy == Fifo) {
            break;
          }
        //cout<<"\n Starting rank : " <<startingRank <<" nextRankPRE : " <<nextRankPRE<<" startingBank : " <<startingBank<<" nextBankPRE : " <<nextBankPRE;
        }
        while (!(startingRank == nextRankPRE && startingBank == nextBankPRE));
         
        //cout<<"\n Sending pres : " <<sendingPRE;
        //if no PREs could be sent, just return false
        if (!sendingPRE)
          return false;
      }
    }
  }
  //sendAct is flag used for posted-cas
  //  posted-cas is enabled when AL>0
  //  when sendAct is true, when don't want to increment our indexes
  //  so we send the column access that is paid with this act
  if (AL > 0 && sendAct) {
    sendAct = false;
  } else {
    sendAct = true;
    nextRankAndBank(nextRank, nextBank);
  }

  //if its an activate, add a tfaw counter
  if ((*busPacket)->busPacketType == ACTIVATE) {
    tFAWCountdown[(*busPacket)->rank].push_back(tFAW);
  }
    return true;
}

//check if a rank/bank queue has room for a certain number of bus packets
bool CommandQueue::hasRoomFor(unsigned numberToEnqueue, unsigned rank,
                              unsigned bank)
{
  vector < BusPacket * >&queue = getCommandQueue(rank, bank);
  return (CMD_QUEUE_DEPTH - queue.size() >= numberToEnqueue);
}

//prints the contents of the command queue
void CommandQueue::print()
{
  if (queuingStructure == PerRank) {
    PRINT(endl << "== Printing Per Rank Queue");
    for (size_t i = 0; i < NUM_RANKS; i++) {
      PRINT(" = Rank " << i << "  size : " << queues[i][0].size());
      for (size_t j = 0; j < queues[i][0].size(); j++) {
        PRINTN("    " << j << "]");
        queues[i][0][j]->print();
      }
    }
  } else if (queuingStructure == PerRankPerBank) {
    PRINT("\n== Printing Per Rank, Per Bank Queue");

    for (size_t i = 0; i < NUM_RANKS; i++) {
      PRINT(" = Rank " << i);
      for (size_t j = 0; j < NUM_BANKS; j++) {
        PRINT("    Bank " << j << "   size : " << queues[i][j].size());

        for (size_t k = 0; k < queues[i][j].size(); k++) {
          PRINTN("       " << k << "]");
          queues[i][j][k]->print();
        }
      }
    }
  }
}

/** 
 * return a reference to the queue for a given rank, bank. Since we
 * don't always have a per bank queuing structure, sometimes the bank
 * argument is ignored (and the 0th index is returned 
 */

vector < BusPacket * >&CommandQueue::getCommandQueue(unsigned rank,
                                                     unsigned bank)
{
  if (queuingStructure == PerRankPerBank) {
    return queues[rank][bank];
  } else if (queuingStructure == PerRank) {
    return queues[rank][0];
  } else {
    ERROR("Unknown queue structure");
    abort();
  }

}



//checks if busPacket is allowed to be issued
bool CommandQueue::isIssuable(BusPacket * busPacket)
{
  switch (busPacket->busPacketType) {
    case REFRESH:

      break;
    case ACTIVATE:
      if ((bankStates[busPacket->rank][busPacket->bank].currentBankState == Idle
           || bankStates[busPacket->rank][busPacket->bank].currentBankState ==
           Refreshing)
          && currentClockCycle >=
          bankStates[busPacket->rank][busPacket->bank].nextActivate
          && tFAWCountdown[busPacket->rank].size() < 4) {
                  

          bankLastAccessedRowMapType bankLastAccessedRowMap = rankBankLastAccessedRowMap[busPacket->rank];
          bankLastPrechargeTimeStampMapType bankLastPrechargeTimeStampMap = rankBankLastPrechargeTimeStampMap[busPacket->rank];
					/*	
					cout<<"\n Investigate Bank : " <<busPacket->bank<<" Rank : " <<busPacket->rank<<" Row : " <<busPacket->row;
          cout<<"\n Last accessed row : " <<bankLastAccessedRowMap[busPacket->bank]<<" packet row : " <<busPacket->row; 
          cout<<"\n Last precharge : " <<bankLastPrechargeTimeStampMap[busPacket->bank]<<" nextPrecharge : " <<bankStates[busPacket->rank][busPacket->bank].nextPrecharge;
					*/
					if (bankLastAccessedRowMap[busPacket->bank] == busPacket->row) {
            // The last accessed row of this bank matches the row of this bus packet; increase mistakeCounter
            //cout<<"\n Last accessed row : " <<bankLastAccessedRowMap[busPacket->bank]<<" busPacket row : " <<busPacket->row;
            if (mistakeCounter <= 50) {
             mistakeCounter++;             
            }
           // cout<<"\n Mistake Counter :: " <<mistakeCounter;
          }
          else if (bankLastPrechargeTimeStampMap[busPacket->bank] > bankStates[busPacket->rank][busPacket->bank].nextPrecharge) {
            if(bankLastPrechargeTimeStampMap[busPacket->bank] - bankStates[busPacket->rank][busPacket->bank].nextPrecharge > tRP) {
             	/* 
							cout<<"\n Latest precharge was far away from calculated precharge time-stamp";
              cout<<"\n bankLastPrechargeTimeStampMap["<<busPacket->bank<<"] "<<bankLastPrechargeTimeStampMap[busPacket->bank];
              cout<<"\n bankStates["<<busPacket->rank<<"]["<<busPacket->bank<<"].nextPrecharge "<<bankStates[busPacket->rank][busPacket->bank].nextPrecharge;
							*/
              if (mistakeCounter > 0){
                mistakeCounter--;
              }
             // cout<<"\n Mistake Counter : " <<mistakeCounter;
            }
          }        
        	//cout<<"\n new mistake counter : " <<mistakeCounter;
					return true;
      } else {
        return false;
      }
      break;
    case WRITE:
    case WRITE_P:
      if (bankStates[busPacket->rank][busPacket->bank].currentBankState ==
          RowActive
          && currentClockCycle >=
          bankStates[busPacket->rank][busPacket->bank].nextWrite
          && busPacket->row ==
          bankStates[busPacket->rank][busPacket->bank].openRowAddress
          ) {
					if (rowBufferPolicy == OpenPageNumAccessesLimit) {
						if (rowAccessCounters[busPacket->rank][busPacket->bank] < TOTAL_ROW_ACCESSES) {
							
							//cout<<"\n increment row access counters : " <<rowAccessCounters[(busPacket)->rank][(busPacket)->bank]++;
							updateBankLastAccessedRow(busPacket->rank, busPacket->bank, busPacket->row);
							updateBankRWTimeStamp(busPacket->rank, busPacket->bank, currentClockCycle);

							return true;
						}
					}
					else {
						updateBankLastAccessedRow(busPacket->rank, busPacket->bank, busPacket->row);
						updateBankRWTimeStamp(busPacket->rank, busPacket->bank, currentClockCycle);
						return true;
					}
	
      } else {
        return false;
      }
      break;
    case READ_P:
    case READ:
      if (bankStates[busPacket->rank][busPacket->bank].currentBankState ==
          RowActive
          && currentClockCycle >=
          bankStates[busPacket->rank][busPacket->bank].nextRead
          && busPacket->row ==
          bankStates[busPacket->rank][busPacket->bank].openRowAddress
          ) {
        	if (rowBufferPolicy == OpenPageNumAccessesLimit) {
						if (rowAccessCounters[busPacket->rank][busPacket->bank] < TOTAL_ROW_ACCESSES) {
							
							//cout<<"\n increment row access counters : " <<rowAccessCounters[(busPacket)->rank][(busPacket)->bank]++;
							updateBankLastAccessedRow(busPacket->rank, busPacket->bank, busPacket->row);
							updateBankRWTimeStamp(busPacket->rank, busPacket->bank, currentClockCycle);
							return true;
						}

					}
					else {
						updateBankLastAccessedRow(busPacket->rank, busPacket->bank, busPacket->row);
						updateBankRWTimeStamp(busPacket->rank, busPacket->bank, currentClockCycle);
						return true;
					}
	
      } else {
        return false;
      }
      break;
    case PRECHARGE:
      if (bankStates[busPacket->rank][busPacket->bank].currentBankState ==
          RowActive
          && currentClockCycle >=
          bankStates[busPacket->rank][busPacket->bank].nextPrecharge) {
          
            updateBankLastPrecharge(busPacket->rank, busPacket->bank, currentClockCycle); 

        
        return true;
      } else {
        return false;
      }
      break;
    default:
      ERROR("== Error - Trying to issue a crazy bus packet type : ");
      busPacket->print();
      exit(0);
  }
  return false;
}

//figures out if a rank's queue is empty
bool CommandQueue::isEmpty(unsigned rank)
{
  if (queuingStructure == PerRank) {
    return queues[rank][0].empty();
  } else if (queuingStructure == PerRankPerBank) {
    for (size_t i = 0; i < NUM_BANKS; i++) {
      if (!queues[rank][i].empty())
        return false;
    }
    return true;
  } else {
    DEBUG("Invalid Queueing Stucture");
    abort();
  }
}

//tells the command queue that a particular rank is in need of a refresh
void CommandQueue::needRefresh(unsigned rank)
{
  refreshWaiting = true;
  refreshRank = rank;
}

int CommandQueue::setMinTimeStamp(uint64_t & minTimeStamp, unsigned &rank,
                                  unsigned &bank)
{
  for (int i = 0; i < NUM_RANKS; i++) {

    for (int j = 0; j < NUM_BANKS; j++) {

      if (queues[i][j].size() != 0) {

        minTimeStamp = queues[i][j].at(0)->timeStamp;
        rank = i;
        bank = j;
        return 1;
      }
    }
  }
  return 0;
}


void CommandQueue::nextRankAndBank(unsigned &rank, unsigned &bank)
{
  if (schedulingPolicy == RankThenBankRoundRobin) {
    rank++;
    if (rank == NUM_RANKS) {
      rank = 0;
      bank++;
      if (bank == NUM_BANKS) {
        bank = 0;
      }
    }
  }
  //bank-then-rank round robin
  else if (schedulingPolicy == BankThenRankRoundRobin
           || schedulingPolicy == Frfcfs) {
    bank++;
    if (bank == NUM_BANKS) {
      bank = 0;
      rank++;
      if (rank == NUM_RANKS) {
        rank = 0;
      }
    }
  } else if (schedulingPolicy == Fifo) {
    // iterate over the ranks and bank queues and pick the queue with the earliest head
    uint64_t minTimeStamp;      // store the minimum time-stamp   
    minTimeStamp = 0;

    setMinTimeStamp(minTimeStamp, rank, bank);
    for (int i = 0; i < NUM_RANKS; i++) {
      for (int j = 0; j < NUM_BANKS; j++) {
        if (queues[i][j].size() != 0) {
          if (queues[i][j].at(0)->timeStamp < minTimeStamp) {
            rank = i;
            bank = j;
            minTimeStamp = queues[i][j].at(0)->timeStamp;
          }
        }
      }
    }
  }


  else {
    ERROR("== Error - Unknown scheduling policy");
    exit(0);
  }

}

void CommandQueue::update()
{
  //do nothing since pop() is effectively update(),
  //needed for SimulatorObject
  //TODO: make CommandQueue not a SimulatorObject
}
