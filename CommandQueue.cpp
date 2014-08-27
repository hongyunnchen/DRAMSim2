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

CommandQueue::CommandQueue(vector< vector<BankState> > &states, ostream &dramsim_log_) :
		dramsim_log(dramsim_log_),
		bankStates(states),
		nextBank(0),
		nextRank(0),
		nextBankPRE(0),
		nextRankPRE(0),
		refreshRank(0),
		refreshWaiting(false),
		sendAct(true),
		isCurrentQueueRead(true),
		rwDependencyFound(false),
		indexAddress(0),
		isWriteDrained(false)
{
	//set here to avoid compile errors
	currentClockCycle = 0;

	//use numBankQueus below to create queue structure
	size_t numBankQueues;
	if (queuingStructure==PerRank) 	{
		numBankQueues = 1;
	}
	else if (queuingStructure==PerRankPerBank|| queuingStructure == Queue) {
		numBankQueues = NUM_BANKS;
	}
	else
	{
		ERROR("== Error - Unknown queuing structure");
		exit(0);
	}

	//vector of counters used to ensure rows don't stay open too long
	rowAccessCounters = vector< vector<unsigned> >(NUM_RANKS, vector<unsigned>(NUM_BANKS,0));
	
	// Allocate space for activeRowPerBank
	
	for (unsigned int i = 0; i<NUM_RANKS; i++) {
		unsigned int *rowActivePerBank = new unsigned int[NUM_BANKS];
		if (rankActiveBanksMap.find(i) == rankActiveBanksMap.end()) {
			rankActiveBanksMap.insert(rankActiveBanksPairType(i, rowActivePerBank));			
		}
		else {
		}
	}
		
	//create queue based on the structure we want
	BusPacket1D actualQueue;
	BusPacket2D perBankQueue = BusPacket2D();
	queues = BusPacket3D();
	
	fifo_queue = BusPacket1D();
	frfcs_queue = BusPacket1D();	
	
	fifo_read_queue = BusPacket1D();
	fifo_write_queue = BusPacket1D();

	for (size_t rank=0; rank<NUM_RANKS; rank++)
	{
		//this loop will run only once for per-rank and NUM_BANKS times for per-rank-per-bank
		for (size_t bank=0; bank<numBankQueues; bank++)
		{
			actualQueue	= BusPacket1D();
			perBankQueue.push_back(actualQueue);
		}
		queues.push_back(perBankQueue);

	}
	//FOUR-bank activation window
	//	this will count the number of activations within a given window
	//	(decrementing counter)
	//
	//countdown vector will have decrementing counters starting at tFAW
	//  when the 0th element reaches 0, remove it
	tFAWCountdown.reserve(NUM_RANKS);
	for (size_t i=0;i<NUM_RANKS;i++)
	{
		//init the empty vectors here so we don't seg fault later
		tFAWCountdown.push_back(vector<unsigned>());
	}
	
}
CommandQueue::~CommandQueue()
{
	//ERROR("COMMAND QUEUE destructor");
	size_t bankMax = NUM_RANKS;
	if(queuingStructure == Queue) {
		for (unsigned int i = 0; i <fifo_queue.size(); i++) {
			delete fifo_queue.at(i);
			delete frfcs_queue.at(i);
		}
		fifo_queue.clear();
		frfcs_queue.clear();
		fifo_read_queue.clear();
		fifo_write_queue.clear();
	}
	else {
		if (queuingStructure == PerRank) {
			bankMax = 1; 
		}
		for (size_t r=0; r< NUM_RANKS; r++)
		{
			for (size_t b=0; b<bankMax; b++) 
			{
				for (size_t i=0; i<queues[r][b].size(); i++)
				{
					delete(queues[r][b][i]);
				}
				queues[r][b].clear();
			}
		}
	}
}

void CommandQueue::rearrangeFRFCS() {
	cout<<"\n IN function rearrangeFRFCS";	
	vector<BusPacket *> openBusPackets;
	vector<BusPacket *> newBusPackets;
	for (unsigned int i = 0; i<frfcs_queue.size(); i++) {
		cout<<"\n BusPacket : ";
		cout<<"\n PA : "<<hex<<frfcs_queue.at(i)->physicalAddress<<dec;
		cout<<"\n Type : "<<frfcs_queue.at(i)->busPacketType;
		if (rankActiveBanksMap.find(frfcs_queue.at(i)->rank) != rankActiveBanksMap.end()) {
			rankActiveBanksMapType::iterator rankFound = rankActiveBanksMap.find(frfcs_queue.at(i)->rank);
			unsigned int *rowActive = rankFound->second;
			if(frfcs_queue.at(i)->row == rowActive[frfcs_queue.at(i)->bank]) { // same as opened row
				openBusPackets.push_back(frfcs_queue.at(i));
			}
			else {
				newBusPackets.push_back(frfcs_queue.at(i));
			}
		}
		else {
			ERROR("\n Bus Packet has rank more than number of ranks present in the memory system");
		}
	}
	

	frfcs_queue.clear();
	for (unsigned int i = 0; i<openBusPackets.size(); i++) {
		frfcs_queue.push_back(openBusPackets.at(i));
	}
	for (unsigned int i = 0; i<newBusPackets.size(); i++) {
		frfcs_queue.push_back(newBusPackets.at(i));
	}

	/*
	vector <BusPacket*> diffVector;
	vector <BusPacket*> sameVector;
			
	unsigned int headBank = frfcs_queue[0]->bank;
	unsigned int headRow = frfcs_queue[0]->row;
	unsigned int index;
	
	for (unsigned int i = 0; i<frfcs_queue.size(); i++) {
		BusPacket *compare = frfcs_queue.at(i);
		if(compare->bank != headBank || compare->row != headRow) {
			diffVector.push_back(compare);
		}
		else {
			sameVector.push_back(compare);
		}
	}
	unsigned int i;
	for (i = 0; i<sameVector.size(); i++) {
		frfcs_queue.at(i) = sameVector.at(i);
	}
	index = i;
	for (i = 0; i<diffVector.size(); i++) {
		frfcs_queue.at(index+i) = diffVector.at(i);
	}

	*/

}

//Adds a command to appropriate queue
void CommandQueue::enqueue(BusPacket *newBusPacket)
{
	unsigned rank = newBusPacket->rank;
	unsigned bank = newBusPacket->bank;
	
	static BusPacket *tmpBusPacket = NULL; // Buffer activate buspacket to determine next packet is read or write

	if (queuingStructure==PerRank)
	{
		queues[rank][0].push_back(newBusPacket);
		if (queues[rank][0].size()>CMD_QUEUE_DEPTH)
		{
			ERROR("== Error - Enqueued more than allowed in command queue");
			ERROR("Need to call .hasRoomFor(int numberToEnqueue, unsigned rank, unsigned bank) first");
			exit(0);
		}
	}
	else if (queuingStructure==PerRankPerBank)
	{
		queues[rank][bank].push_back(newBusPacket);

		if (queues[rank][bank].size()>CMD_QUEUE_DEPTH)
		{
			ERROR("== Error - Enqueued more than allowed in command queue");
			ERROR("						Need to call .hasRoomFor(int numberToEnqueue, unsigned rank, unsigned bank) first");
			exit(0);
		}
	}
	else if (queuingStructure == Queue) {
		if(schedulingPolicy == fifo){
			fifo_queue.push_back(newBusPacket);	
			if (fifo_queue.size() > CMD_QUEUE_DEPTH) {
				ERROR("== Error - Enqueued more than allowed in command queue");
				ERROR("Need to call .hasRoomFor(int numberToEnqueue, unsigned rank, unsigned bank) first");
				exit(0);
			}
		}
		else if (schedulingPolicy == fiforw) {
			cout<<"\n Enqueuing new bus packet : \n";
			newBusPacket->print();
			if (newBusPacket->busPacketType == ACTIVATE) {
				tmpBusPacket = newBusPacket;
				cout<<"\n setting buspacket";

			}
			else if (newBusPacket->busPacketType == READ || newBusPacket->busPacketType == READ_P) {
				if (tmpBusPacket) {
					if(fifo_read_queue.size() > CMD_QUEUE_DEPTH) {
						ERROR("== Error - Enqueued more than allowed in command queue");
						ERROR("						Need to call .hasRoomFor(int numberToEnqueue, unsigned rank, unsigned bank) first");
						exit(0);										
					}
					fifo_read_queue.push_back(tmpBusPacket);
					fifo_read_queue.push_back(newBusPacket);
					cout<<"\n FIFO READ SIZE : " <<fifo_read_queue.size();	
					uint64_t address = newBusPacket->physicalAddress;
					for (unsigned int i = 0; i<fifo_write_queue.size(); i++) {
						if (address == fifo_write_queue.at(i)->physicalAddress) {
							newBusPacket->dependency = true;
							fifo_write_queue.at(i)->dependency = true;
							rwDependencyFound = true;
							break;
						}
					}
				}
			}
			else if (newBusPacket->busPacketType == WRITE || newBusPacket->busPacketType == WRITE_P) {
				if (tmpBusPacket) {
					if (fifo_write_queue.size() > CMD_QUEUE_DEPTH) {
						ERROR("== Error - Enqueued more than allowed in command queue");
						ERROR("						Need to call .hasRoomFor(int numberToEnqueue, unsigned rank, unsigned bank) first");
						exit(0);
					}
					fifo_write_queue.push_back(tmpBusPacket);
					fifo_write_queue.push_back(newBusPacket);	
					cout<<"\n FIFO WRITE SIZE : " <<fifo_write_queue.size();	
				}
			}
			else {
				ERROR("== ERROR - What buspacket is this .... ?");
				cout<<" " <<newBusPacket->busPacketType;
				exit(0);
			}
		}
		else if (schedulingPolicy == frfcs) {
			if(frfcs_queue.size() <= 2) {
				frfcs_queue.push_back(newBusPacket);
			}
			else {
				frfcs_queue.push_back(newBusPacket);	
				rearrangeFRFCS();	
			}
		}
	}
	else
	{
		ERROR("== Error - Unknown queuing structure");
		exit(0);
	}
}

//Removes the next item from the command queue based on the system's
//command scheduling policy
bool CommandQueue::pop(BusPacket **busPacket)
{
	//this can be done here because pop() is called every clock cycle by the parent MemoryController
	//	figures out the sliding window requirement for tFAW
	//
	//deal with tFAW book-keeping
	//	each rank has it's own counter since the restriction is on a device level
	for (size_t i=0;i<NUM_RANKS;i++)
	{
		//decrement all the counters we have going
		for (size_t j=0;j<tFAWCountdown[i].size();j++)
		{
			tFAWCountdown[i][j]--;
		}

		//the head will always be the smallest counter, so check if it has reached 0
		if (tFAWCountdown[i].size()>0 && tFAWCountdown[i][0]==0)
		{
			tFAWCountdown[i].erase(tFAWCountdown[i].begin());
		}
	}

	/* Now we need to find a packet to issue. When the code picks a packet, it will set
		 *busPacket = [some eligible packet]
		 
		 First the code looks if any refreshes need to go
		 Then it looks for data packets
		 Otherwise, it starts looking for rows to close (in open page)
	*/

	if (rowBufferPolicy==ClosePage)
	{
		// Check active rows
		updateActiveRowPerBank();
		

		// no refresh is happening i belive: ANI			
		bool sendingREF = false;
		//if the memory controller set the flags signaling that we need to issue a refresh
		if (refreshWaiting)
		{
			//cout<<"\n REFRESH WAITING SET";
			bool foundActiveOrTooEarly = false;
			//look for an open bank
			for (size_t b=0;b<NUM_BANKS;b++)
			{
				vector<BusPacket *> &queue = getCommandQueue(refreshRank,b);

				//checks to make sure that all banks are idle
				if (bankStates[refreshRank][b].currentBankState == RowActive)
				{
					foundActiveOrTooEarly = true;
					//if the bank is open, make sure there is nothing else
					// going there before we close it
					for (size_t j=0;j<queue.size();j++)
					{
						BusPacket *packet = queue[j];
						if (packet->row == bankStates[refreshRank][b].openRowAddress &&
								packet->bank == b && packet->rank == refreshRank)
						{
						//	cout<<"\nCHECKING WHETHER TO ISSUE REFRESH";
							if (packet->busPacketType != ACTIVATE && isIssuable(packet))
							{
							//	cout<<"\nISSUING REFRESH";
								*busPacket = packet;
								if (packet->dependency == true) {
									rwDependencyFound = false;
								}
								queue.erase(queue.begin() + j);
								sendingREF = true;
							}
							break;
						}
					}
					break;
				}
				//	NOTE: checks nextActivate time for each bank to make sure tRP is being
				//				satisfied.	the next ACT and next REF can be issued at the same
				//				point in the future, so just use nextActivate field instead of
				//				creating a nextRefresh field
				else if (bankStates[refreshRank][b].nextActivate > currentClockCycle)
				{
					foundActiveOrTooEarly = true;
					break;
				}
			}

			//if there are no open banks and timing has been met, send out the refresh
			//	reset flags and rank pointer
			if (!foundActiveOrTooEarly && bankStates[refreshRank][0].currentBankState != PowerDown)
			{
				*busPacket = new BusPacket(REFRESH, 0, 0, 0, refreshRank, 0, 0, dramsim_log);
				refreshRank = -1;
				refreshWaiting = false;
				sendingREF = true;
			}
		} // refreshWaiting

		//if we're not sending a REF, proceed as normal
		if (!sendingREF)
		{
			bool foundIssuable = false;
			unsigned startingRank;
			unsigned startingBank;			
			
			if(schedulingPolicy == fifo) {

				if(!fifo_queue.empty()) {
					startingRank = fifo_queue.front()->rank;
					startingBank = fifo_queue.front()->bank;
				}
				else {
					startingRank = 0;
					startingBank = 0;
				}
			}
	//////////////////////////////////////////////////////
			else if (schedulingPolicy == fiforw) {
				if (isCurrentQueueRead) {
					if (!fifo_read_queue.empty()){
						startingRank = fifo_read_queue.front()->rank;
						startingBank = fifo_read_queue.front()->bank;
					}
					else {
						cout<<"\n I am here ...... read condition ";
						startingRank = 0;
						startingBank = 0;
					}
				}
				else {
					if(!fifo_write_queue.empty()) {
						startingRank = fifo_write_queue.front()->rank;
						startingBank = fifo_write_queue.front()->bank;
					}
					else {

						cout<<"\n I am here ...... read condition ";
						startingRank = 0;
						startingBank = 0;
					}
				}
			}
	//////////////////////////////////////////////////////
			else if (schedulingPolicy == frfcs) {
				if(!frfcs_queue.empty()) {
					startingRank = frfcs_queue.front()->rank;
					startingBank = frfcs_queue.front()->bank;
				}
				else {
					startingRank = 0; 
					startingBank = 0;
				}
			}
			else {
				startingRank = nextRank;
				startingBank = nextBank;
			}

			do
			{
			//	cout<<"\n IN HERE ALWAYS";
				vector<BusPacket *> &queue = getCommandQueue(nextRank, nextBank);
				//cout<<"\n QUEUE SIZE : "<<queue.size()<<" fifo " <<fifo_queue.size();
				//make sure there is something in this queue first
				//	also make sure a rank isn't waiting for a refresh
				//	if a rank is waiting for a refesh, don't issue anything to it until the
				//		refresh logic above has sent one out (ie, letting banks close)

					if (queuingStructure == PerRank)
					{						
							if (!queue.empty() && !((nextRank == refreshRank) && refreshWaiting)){
							//search from beginning to find first issuable bus packet
							for (size_t i=0;i<queue.size();i++)
							{
								if (isIssuable(queue[i]))
								{
								//check to make sure we aren't removing a read/write that is paired with an activate
									if (i>0 && queue[i-1]->busPacketType==ACTIVATE &&
											queue[i-1]->physicalAddress == queue[i]->physicalAddress)
										continue;

									*busPacket = queue[i];

									queue.erase(queue.begin()+i);
									foundIssuable = true;
									break;
								}
							}
						}
					}
					else if(queuingStructure == PerRankPerBank)
					{	
						if (!queue.empty() && !((nextRank == refreshRank) && refreshWaiting)){
							if (isIssuable(queue[0]))
							{
								//no need to search because if the front can't be sent,
								// then no chance something behind it can go instead
								*busPacket = queue[0];
								queue.erase(queue.begin());
								foundIssuable = true;
							}
						}
					}
					else {							
						if (!queue.empty() && !((queue[0]->rank == refreshRank) && refreshWaiting)){
							for (size_t i=0;i<queue.size();i++)
							{
								if (isIssuable(queue[i]))
								{
								//check to make sure we aren't removing a read/write that is paired with an activate
									if (i>0 && queue[i-1]->busPacketType==ACTIVATE &&
											queue[i-1]->physicalAddress == queue[i]->physicalAddress)
										continue;

									*busPacket = queue[i];

									queue.erase(queue.begin()+i);
									foundIssuable = true;
									break;
								}
							}
							/*
							if(isIssuable(queue[0])) {
								*busPacket = queue[0];
								if (queue[0]->dependency == true) {
										rwDependencyFound = false;
								}
								queue.erase(queue.begin());
								foundIssuable = true;
							}
							*/
						}
					}
				
				//if we found something, break out of do-while
				if (foundIssuable) break;

				//rank round robin
				if (queuingStructure == PerRank)
				{
					nextRank = (nextRank + 1) % NUM_RANKS;
					if (startingRank == nextRank)
					{
						break;
					}
				}
				else if(queuingStructure == PerRankPerBank) 
				{
					nextRankAndBank(nextRank, nextBank);
					if (startingRank == nextRank && startingBank == nextBank)
					{
						break;
					}
				}
				else {
					if(queue.size() != 0 && bankStates[queue.front()->rank][queue.front()->bank].nextActivate <= currentClockCycle) {
						nextRank = queue.front()->rank;
						nextBank = queue.front()->bank;
						break;
					}
					else {
						break;
					}
				}
			}
			while (true);

			//if we couldn't find anything to send, return false
			if (!foundIssuable) return false;
		}
	}
	/*
	else if (rowBufferPolicy == LookAheadClosePage) {

		bool sendingREF = false;
		//if the memory controller set the flags signaling that we need to issue a refresh
		if (refreshWaiting)
		{
			//cout<<"\n REFRESH WAITING SET";
			bool foundActiveOrTooEarly = false;
			//look for an open bank
			for (size_t b=0;b<NUM_BANKS;b++)
			{
				vector<BusPacket *> &queue = getCommandQueue(refreshRank,b);

				//checks to make sure that all banks are idle
				if (bankStates[refreshRank][b].currentBankState == RowActive)
				{
					foundActiveOrTooEarly = true;
					//if the bank is open, make sure there is nothing else
					// going there before we close it
					for (size_t j=0;j<queue.size();j++)
					{
						BusPacket *packet = queue[j];
						if (packet->row == bankStates[refreshRank][b].openRowAddress &&
								packet->bank == b && packet->rank == refreshRank)
						{
						//	cout<<"\nCHECKING WHETHER TO ISSUE REFRESH";
							if (packet->busPacketType != ACTIVATE && isIssuable(packet))
							{
							//	cout<<"\nISSUING REFRESH";
								*busPacket = packet;
								queue.erase(queue.begin() + j);
								sendingREF = true;
							}
							break;
						}
					}
					break;
				}
				//	NOTE: checks nextActivate time for each bank to make sure tRP is being
				//				satisfied.	the next ACT and next REF can be issued at the same
				//				point in the future, so just use nextActivate field instead of
				//				creating a nextRefresh field
				else if (bankStates[refreshRank][b].nextActivate > currentClockCycle)
				{
					foundActiveOrTooEarly = true;
					break;
				}
			}

			//if there are no open banks and timing has been met, send out the refresh
			//	reset flags and rank pointer
			if (!foundActiveOrTooEarly && bankStates[refreshRank][0].currentBankState != PowerDown)
			{
				*busPacket = new BusPacket(REFRESH, 0, 0, 0, refreshRank, 0, 0, dramsim_log);
				refreshRank = -1;
				refreshWaiting = false;
				sendingREF = true;
			}
		} // refreshWaiting

		//if we're not sending a REF, proceed as normal
		if (!sendingREF)
		{
			bool foundIssuable = false;
			unsigned startingRank;
			unsigned startingBank;			
			
			if(schedulingPolicy == fifo) {

				if(!fifo_queue.empty()) {
					startingRank = fifo_queue.front()->rank;
					startingBank = fifo_queue.front()->bank;
				}
				else {
					startingRank = 0;
					startingBank = 0;
				}
			}
			else if (schedulingPolicy == frfcs) {
				if(!frfcs_queue.empty()) {
					startingRank = frfcs_queue.front()->rank;
					startingBank = frfcs_queue.front()->bank;
				}
				else {
					startingRank = 0; 
					startingBank = 0;
				}
			}
			else {
				startingRank = nextRank;
				startingBank = nextBank;
			}

			do
			{
			//	cout<<"\n IN HERE ALWAYS";
				vector<BusPacket *> &queue = getCommandQueue(nextRank, nextBank);
				//cout<<"\n QUEUE SIZE : "<<queue.size()<<" fifo " <<fifo_queue.size();
				//make sure there is something in this queue first
				//	also make sure a rank isn't waiting for a refresh
				//	if a rank is waiting for a refesh, don't issue anything to it until the
				//		refresh logic above has sent one out (ie, letting banks close)

					if (queuingStructure == PerRank)
					{						
							if (!queue.empty() && !((nextRank == refreshRank) && refreshWaiting)){
							//search from beginning to find first issuable bus packet
							for (size_t i=0;i<queue.size();i++)
							{
								if (isIssuable(queue[i]))
								{
								//check to make sure we aren't removing a read/write that is paired with an activate
									if (i>0 && queue[i-1]->busPacketType==ACTIVATE &&
											queue[i-1]->physicalAddress == queue[i]->physicalAddress)
										continue;

									*busPacket = queue[i];
									queue.erase(queue.begin()+i);
									foundIssuable = true;
									break;
								}
							}
						}
					}
					else if(queuingStructure == PerRankPerBank)
					{
						
						if (!queue.empty() && !((nextRank == refreshRank) && refreshWaiting)){
							if (isIssuable(queue[0]))
							{
								//no need to search because if the front can't be sent,
								// then no chance something behind it can go instead
								*busPacket = queue[0];
								queue.erase(queue.begin());
								foundIssuable = true;
							}
						}
					}
					else {							
						if (!queue.empty() && !((queue[0]->rank == refreshRank) && refreshWaiting)){
							//cout<<"\n DOING THIS";
							if(isIssuable(queue[0])) {
								*busPacket = queue[0];
								queue.erase(queue.begin());
								foundIssuable = true;
							}
						}
					}
				
				//if we found something, break out of do-while
				if (foundIssuable) break;

				//rank round robin
				if (queuingStructure == PerRank)
				{
					nextRank = (nextRank + 1) % NUM_RANKS;
					if (startingRank == nextRank)
					{
						break;
					}
				}
				else if(queuingStructure == PerRankPerBank) 
				{
					nextRankAndBank(nextRank, nextBank);
					if (startingRank == nextRank && startingBank == nextBank)
					{
						break;
					}
				}
				else {
					if(queue.size() != 0 && bankStates[queue.front()->rank][queue.front()->bank].nextActivate <= currentClockCycle) {
						nextRank = queue.front()->rank;
						nextBank = queue.front()->bank;
						break;
					}
					else {
						break;
					}
				}
			}
			while (true);

			//if we couldn't find anything to send, return false
			if (!foundIssuable) return false;
		}
	}
	*/
	else if (rowBufferPolicy==OpenPage)
	{
		// call function to print state of banks;
		dumpActiveRowPerBank();	
		
		bool sendingREForPRE = false;
		if (refreshWaiting)
		{
			bool sendREF = true;
			//make sure all banks idle and timing met for a REF
			for (size_t b=0;b<NUM_BANKS;b++)
			{
				//if a bank is active we can't send a REF yet
				if (bankStates[refreshRank][b].currentBankState == RowActive)
				{
					sendREF = false;
					bool closeRow = true;
					//search for commands going to an open row
					vector <BusPacket *> &refreshQueue = getCommandQueue(refreshRank,b);

					for (size_t j=0;j<refreshQueue.size();j++)
					{
						BusPacket *packet = refreshQueue[j];
						//if a command in the queue is going to the same row . . .
						if (bankStates[refreshRank][b].openRowAddress == packet->row &&
								b == packet->bank && packet->rank == refreshRank)
						{
							// . . . and is not an activate . . .
							if (packet->busPacketType != ACTIVATE)
							{
								closeRow = false;
								// . . . and can be issued . . .

								if (isIssuable(packet))
								{
									//send it out
									*busPacket = packet;
									if (packet->dependency == true) {
										rwDependencyFound = false;
									}
									refreshQueue.erase(refreshQueue.begin()+j);
									sendingREForPRE = true;
								}
								break;
							}
							else //command is an activate
							{
								//if we've encountered another act, no other command will be of interest
								break;
							}
						}
					}

					//if the bank is open and we are allowed to close it, then send a PRE
					if (closeRow && currentClockCycle >= bankStates[refreshRank][b].nextPrecharge)
					{
						rowAccessCounters[refreshRank][b]=0;
						*busPacket = new BusPacket(PRECHARGE, 0, 0, 0, refreshRank, b, 0, dramsim_log);
						sendingREForPRE = true;
					}
					break;
				}
				//	NOTE: the next ACT and next REF can be issued at the same
				//				point in the future, so just use nextActivate field instead of
				//				creating a nextRefresh field
				else if (bankStates[refreshRank][b].nextActivate > currentClockCycle) //and this bank doesn't have an open row
				{
					sendREF = false;
					break;
				}
			}

			//if there are no open banks and timing has been met, send out the refresh
			//	reset flags and rank pointer
			if (sendREF && bankStates[refreshRank][0].currentBankState != PowerDown)
			{
				*busPacket = new BusPacket(REFRESH, 0, 0, 0, refreshRank, 0, 0, dramsim_log);
				refreshRank = -1;
				refreshWaiting = false;
				sendingREForPRE = true;
			}
		}

		if (!sendingREForPRE)
		{
			unsigned startingRank;
			unsigned startingBank;
			if(schedulingPolicy == fifo ) {
				if(!fifo_queue.empty()) {
					startingRank = fifo_queue.front()->rank;
					startingBank = fifo_queue.front()->bank;
				}
				else {
					startingRank = 0;
					startingBank = 0;
				}
			}
			else if (schedulingPolicy == frfcs) {
				if(!frfcs_queue.empty()) {
					startingRank = frfcs_queue.front()->rank;
					startingBank = frfcs_queue.front()->bank;
				}
				else {
					startingRank = 0;
					startingBank = 0;
				}
			}

			else {
				startingRank = nextRank;
				startingBank = nextBank;
			}
			bool foundIssuable = false;
			do // round robin over queues
			{
				vector<BusPacket *> &queue = getCommandQueue(nextRank,nextBank);
				/*
				cout<<"\n FIFO QUEUE ORDERED\n";
				for (int i = 0; i<queue.size(); i++) {
				cout<<"BANK : " <<queue.at(i)->bank<<" ROW : " <<queue.at(i)->row<<" BUS PACKET TYPE : " <<queue.at(i)->busPacketType<<endl;
				}
				*/
				//make sure there is something there first
				// this might create a problem for FIFO.... 
				if(queuingStructure == PerRank || queuingStructure == PerRankPerBank) {
					if (!queue.empty() && !((nextRank == refreshRank) && refreshWaiting))
					{
						//search from the beginning to find first issuable bus packet
						for (size_t i=0;i<queue.size();i++)
						{
							BusPacket *packet = queue[i];
							if (isIssuable(packet))
							{
								//check for dependencies
								bool dependencyFound = false;
								
								for (size_t j=0;j<i;j++)
								{
									BusPacket *prevPacket = queue[j];
									if (prevPacket->busPacketType != ACTIVATE &&
											prevPacket->bank == packet->bank &&
											prevPacket->row == packet->row)
									{
										
										dependencyFound = true;
										break;
									}
								}

								if (dependencyFound) continue;							
								*busPacket = packet;
								//cout<<"\n VALUE OF i : "<<i;	
								//if the bus packet before is an activate, that is the act that was
								//	paired with the column access we are removing, so we have to remove
								//	that activate as well (check i>0 because if i==0 then theres nothing before it)
								if (i>0 && queue[i-1]->busPacketType == ACTIVATE)
								{
									rowAccessCounters[(*busPacket)->rank][(*busPacket)->bank]++;
									// i is being returned, but i-1 is being thrown away, so must delete it here 
									delete (queue[i-1]);
	
									// remove both i-1 (the activate) and i (we've saved the pointer in *busPacket)

									queue.erase(queue.begin()+i-1,queue.begin()+i+1);
								}
								else // there's no activate before this packet
								{
									//or just remove the one bus packet
									
									queue.erase(queue.begin()+i);
								}

								foundIssuable = true;
								break;
							}
							
							if(schedulingPolicy != fairfrfcs) {
								break;
							}
							
						}
					}
				}
				else if(queuingStructure == Queue) {
					if (!queue.empty() && !((queue[0]->rank == refreshRank) && refreshWaiting))
					{
						//search from the beginning to find first issuable bus packet
						for (size_t i=0;i<queue.size();i++)
						{
							BusPacket *packet = queue[i];
	
							if (isIssuable(packet) && packet->rank == nextRank && packet->bank == nextBank)
							{
								//check for dependencies
								cout<<"\nISSUING PACKET : " <<packet->bank<<" "<<packet->busPacketType;
								bool dependencyFound = false;
								
								for (size_t j=0;j<i;j++)
								{
									BusPacket *prevPacket = queue[j];
									if (prevPacket->busPacketType != ACTIVATE &&
											prevPacket->bank == packet->bank &&
											prevPacket->row == packet->row &&
											prevPacket->rank == packet->rank)
									{
										dependencyFound = true;
										break;
									}
								}
								
								if (dependencyFound) continue;
	
								*busPacket = packet;
	
								//if the bus packet before is an activate, that is the act that was
								//	paired with the column access we are removing, so we have to remove
								//	that activate as well (check i>0 because if i==0 then theres nothing before it)
								if (i>0 && queue[i-1]->busPacketType == ACTIVATE)
								{
									rowAccessCounters[(*busPacket)->rank][(*busPacket)->bank]++;
									// i is being returned, but i-1 is being thrown away, so must delete it here 
									if (queue[i-1]->dependency == true || queue[i]->dependency == true){
										rwDependencyFound = false;
									}
									
									delete (queue[i-1]);
	
									// remove both i-1 (the activate) and i (we've saved the pointer in *busPacket)
									queue.erase(queue.begin()+i-1,queue.begin()+i+1);
								}
								else // there's no activate before this packet
								{
									//or just remove the one bus packet
									if(queue[i]->dependency == true) {
										rwDependencyFound = false;
									}									
									queue.erase(queue.begin()+i);
								}

								foundIssuable = true;
								break;
							}
							else {
								//break;
							}
						}
					}
				}
				//if we found something, break out of do-while
				if (foundIssuable) break;

				//rank round robin
				if (queuingStructure == PerRank)
				{
					nextRank = (nextRank + 1) % NUM_RANKS;
					if (startingRank == nextRank)
					{
						break;
					}
				}
				else if(queuingStructure == PerRankPerBank) 
				{
					nextRankAndBank(nextRank, nextBank); 
					if (startingRank == nextRank && startingBank == nextBank)
					{
						break;
					}
				}
				else if (schedulingPolicy == fifo) {
					if(!fifo_queue.empty() && bankStates[queue.front()->rank][queue.front()->bank].nextActivate <= currentClockCycle) {
						nextRank = fifo_queue.front()->rank;
						nextBank = fifo_queue.front()->bank;
						break;
					}
					else {
						break;
					}
				}
				else if (schedulingPolicy == frfcs) {
					if(!frfcs_queue.empty() && bankStates[queue.front()->rank][queue.front()->bank].nextActivate <= currentClockCycle) {
						nextRank = frfcs_queue.front()->rank;
						nextBank = frfcs_queue.front()->bank;
						break;
					}
					else {
						break;
					}
				}

				else if (schedulingPolicy == fiforw) {
					if(isCurrentQueueRead) {
						if(!fifo_read_queue.empty() && bankStates[queue.front()->rank][queue.front()->bank].nextActivate
						<= currentClockCycle) {
							nextRank = fifo_read_queue.front()->rank;
							nextBank = fifo_read_queue.front()->bank;
							break;
						}
						else {
							break;
						}
					}
					else {
						if(!fifo_write_queue.empty() && bankStates[queue.front()->rank][queue.front()->bank].nextActivate
						<= currentClockCycle) {
							nextRank = fifo_write_queue.front()->rank;
							nextBank = fifo_write_queue.front()->bank;
							break;
						}
						else {
							break;
						}		
					}
				}
			}
			while (true);

			//if nothing was issuable, see if we can issue a PRE to an open bank
			//	that has no other commands waiting
			if (!foundIssuable)
			{
				//search for banks to close
				bool sendingPRE = false;
				unsigned startingRank = nextRankPRE;
				unsigned startingBank = nextBankPRE;
	
		             //   bool found = true; // changed

				do // round robin over all ranks and banks
				{

				//	cout<<"\n ALWAYS HERE";
					
					vector <BusPacket *> &queue = getCommandQueue(nextRankPRE, nextBankPRE);
				//	cout<<"\nQUEUE SIZE : " <<queue.size();
					bool found = true; // original
					//check if bank is  open or if all banks have nothing
					/*	
					if (! (bankStates[nextRankPRE][nextBankPRE].currentBankState == RowActive) )
					{
						
						for (size_t i=0;i<queue.size();i++)
						{
							//if there is something going to other banks  then we want to send a PRE							
							if (((queue[i]->bank == nextBankPRE) && (queue[i]->rank == nextRankPRE))||
									(queue[i]->row == bankStates[nextRankPRE][nextBankPRE].openRowAddress))
							{
								found = false;
								break;
							}
						}

					}
					
         	else {
        		for (size_t i=0;i<queue.size();i++) {
            //if there is something going to same rank and  banks but different row  then we want to send a PRE
            	if (queue[i]->row != 
							bankStates[nextRankPRE][nextBankPRE].openRowAddress  && queue[i]->bank == nextBankPRE && queue[i]->rank == nextRankPRE) {
              	found = false;
                break;
             	}
            }
          }
					*/
					if(bankStates[nextRankPRE][nextBankPRE].currentBankState == RowActive) {
						for (size_t i = 0; i<queue.size(); i++) {
							if(queue[0]->bank == nextBankPRE && queue[0]->rank == nextRankPRE && queue[0]->row != bankStates[nextRankPRE][nextBankPRE].openRowAddress) {
								found = false;
								break;
							}
						}
					}
					//if nothing found going to that bank and row or too many accesses have happend, close it
					//BAGGY-CHANGED
          if ( (bankStates[nextRankPRE][nextBankPRE].currentBankState == RowActive) &&  ( !found )) { //
       	//if (rowAccessCounters[nextRankPRE][nextBankPRE]==TOTAL_ROW_ACCESSES) // changed
						
						if (currentClockCycle >= 
						bankStates[nextRankPRE][nextBankPRE].nextPrecharge) { // changed 
          	//	cout<<"Baggy powup "<<bankStates[nextRankPRE][nextBankPRE].nextPowerUp<<" clock  "<<currentClockCycle<<endl;
							sendingPRE = true;
							rowAccessCounters[nextRankPRE][nextBankPRE] = 0;
							*busPacket = new BusPacket(PRECHARGE, 0, 0, 0, nextRankPRE, nextBankPRE, 0, dramsim_log);
							break;
					 	}
					}

					// need to put condition here......
					
					nextRankAndBankPRE(nextRankPRE, nextBankPRE);
					//if(schedulingPolicy == fifo || schedulingPolicy == frfcs) {
					//	break;	
					//}
				}
				while (!(startingRank == nextRankPRE && startingBank == nextBankPRE));

				//if no PREs could be sent, just return false
				if (!sendingPRE) return false;
			}
		}
	}

	//sendAct is flag used for posted-cas
	//  posted-cas is enabled when AL>0
	//  when sendAct is true, when don't want to increment our indexes
	//  so we send the column access that is paid with this act
	if (AL>0 && sendAct)
	{
		sendAct = false;
	}
	else
	{
		sendAct = true;
		nextRankAndBank(nextRank, nextBank);
	}

	//if its an activate, add a tfaw counter
	if ((*busPacket)->busPacketType==ACTIVATE)
	{
		tFAWCountdown[(*busPacket)->rank].push_back(tFAW);
	}

	return true;
}

//check if a rank/bank queue has room for a certain number of bus packets
bool CommandQueue::hasRoomFor(unsigned numberToEnqueue, unsigned rank, unsigned bank)
{
	vector<BusPacket *> &queue = getCommandQueue(rank, bank); 
	return (CMD_QUEUE_DEPTH - queue.size() >= numberToEnqueue);
}

//prints the contents of the command queue
void CommandQueue::print()
{
	if (queuingStructure==PerRank)
	{
		PRINT(endl << "== Printing Per Rank Queue" );
		for (size_t i=0;i<NUM_RANKS;i++)
		{
			PRINT(" = Rank " << i << "  size : " << queues[i][0].size() );
			for (size_t j=0;j<queues[i][0].size();j++)
			{
				PRINTN("    "<< j << "]");
				queues[i][0][j]->print();
			}
		}
	}
	else if (queuingStructure==PerRankPerBank)
	{
		PRINT("\n== Printing Per Rank, Per Bank Queue" );

		for (size_t i=0;i<NUM_RANKS;i++)
		{
			PRINT(" = Rank " << i );
			for (size_t j=0;j<NUM_BANKS;j++)
			{
				PRINT("    Bank "<< j << "   size : " << queues[i][j].size() );

				for (size_t k=0;k<queues[i][j].size();k++)
				{
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
vector<BusPacket *> &CommandQueue::getCommandQueue(unsigned rank, unsigned bank)
{
	if (queuingStructure == PerRankPerBank)
	{
		return queues[rank][bank];
	}
	else if (queuingStructure == PerRank)
	{
		return queues[rank][0];
	}
	else if(queuingStructure == Queue) {
		if(schedulingPolicy == fifo) {
			return fifo_queue;
		}
		
		else if(schedulingPolicy == frfcs) {			
			return frfcs_queue;
		}

		else if (schedulingPolicy == fiforw) {
			cout<<"\n Conditions : \n";
			cout<<"rwDependencyFound : " <<rwDependencyFound<<"\n";
			cout<<"fifo_write_queue.size() : " <<fifo_write_queue.size()<<"\n";
			cout<<"isWriteDrained : " <<isWriteDrained<<"\n";
			cout<<"fifo_read_queue.size() : " <<fifo_read_queue.size()<<"\n";
			cout<<"fifo_write_queue.size() : " <<fifo_write_queue.size()<<"\n";
			if (rwDependencyFound || fifo_write_queue.size() > THRESH_HIGH || isWriteDrained || 
			fifo_read_queue.size() == 0 || fifo_write_queue.size() % 2 == 1) {
				isCurrentQueueRead = false;
				if(fifo_write_queue.size() == THRESH_LOW) {
					isWriteDrained = false;
				}
				cout<<"\n Returning write queue";
				return fifo_write_queue;
			}
			else {
				isCurrentQueueRead = true;
				cout<<"\n Returning read queue";	
				return fifo_read_queue;
			}

		}
		
		else {
			cout<<"\n WRONG SCHEDULING POLICY WITH QUEUE DATA STRUCTURE";
			abort();
		}	
	}
		

	else
	{
		ERROR("Unknown queue structure");
		abort(); 
	}

}

//checks if busPacket is allowed to be issued
bool CommandQueue::isIssuable(BusPacket *busPacket)
{
	switch (busPacket->busPacketType)
	{
	case REFRESH:

		break;
	case ACTIVATE:
		if ((bankStates[busPacket->rank][busPacket->bank].currentBankState == Idle ||
		        bankStates[busPacket->rank][busPacket->bank].currentBankState == Refreshing) &&
		        currentClockCycle >= bankStates[busPacket->rank][busPacket->bank].nextActivate &&
		        tFAWCountdown[busPacket->rank].size() < 4)
		{
			return true;
		}
		else
		{
		/*	
		if (bankStates[busPacket->rank][busPacket->bank].currentBankState == Idle ||
		        bankStates[busPacket->rank][busPacket->bank].currentBankState == Refreshing){
						cout<<"\n First condition true";

						}
						cout<<"\n Current clock cycle : " <<currentClockCycle<<" " <<" next activate : " <<bankStates[busPacket->rank][busPacket->bank].nextActivate<<" bank : "<<busPacket->bank<<" " <<busPacket->rank;
		        if(currentClockCycle >= bankStates[busPacket->rank][busPacket->bank].nextActivate) {
							cout<<"\n Second condition true";
						}
						
		        if(tFAWCountdown[busPacket->rank].size() < 4){
							cout <<"\n Third condition true";
						}
			*/			
			return false;
		}
		break;
	case WRITE:
	case WRITE_P:
		if (bankStates[busPacket->rank][busPacket->bank].currentBankState == RowActive &&
		        currentClockCycle >= bankStates[busPacket->rank][busPacket->bank].nextWrite &&
		        busPacket->row == bankStates[busPacket->rank][busPacket->bank].openRowAddress &&
		        rowAccessCounters[busPacket->rank][busPacket->bank] < TOTAL_ROW_ACCESSES)
		{
			return true;
		}
		else
		{
			return false;
		}
		break;
	case READ_P:
	case READ:
		if (bankStates[busPacket->rank][busPacket->bank].currentBankState == RowActive &&
		        currentClockCycle >= bankStates[busPacket->rank][busPacket->bank].nextRead &&
		        busPacket->row == bankStates[busPacket->rank][busPacket->bank].openRowAddress &&
		        rowAccessCounters[busPacket->rank][busPacket->bank] < TOTAL_ROW_ACCESSES)
		{
			return true;
		}
		else
		{
			return false;
		}
		break;
	case PRECHARGE:
		if (bankStates[busPacket->rank][busPacket->bank].currentBankState == RowActive &&
		        currentClockCycle >= bankStates[busPacket->rank][busPacket->bank].nextPrecharge)
		{
			return true;
		}
		else
		{
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
	if (queuingStructure == PerRank)
	{
		return queues[rank][0].empty();
	}
	else if (queuingStructure == PerRankPerBank)
	{
		for (size_t i=0;i<NUM_BANKS;i++)
		{
			if (!queues[rank][i].empty()) return false;
		}
		return true;
	}
	else if (queuingStructure == Queue) {
		return fifo_queue.empty();
	}
	else
	{
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

void CommandQueue::nextRankAndBankPRE(unsigned &rank, unsigned &bank)
{
	if (schedulingPolicy == RankThenBankRoundRobin)
	{
		rank++;
		if (rank == NUM_RANKS)
		{
			rank = 0;
			bank++;
			if (bank == NUM_BANKS)
			{
				bank = 0;
			}
		}
	}
	//bank-then-rank round robin
	else if (schedulingPolicy == BankThenRankRoundRobin)
	{
		bank++;
		if (bank == NUM_BANKS)
		{
			bank = 0;
			rank++;
			if (rank == NUM_RANKS)
			{
				rank = 0;
			}
		}
	}
	else if (schedulingPolicy == fifo || schedulingPolicy == frfcs || schedulingPolicy == fiforw) {
		rank++;
		if (rank == NUM_RANKS)
		{
			rank = 0;
			bank++;
			if (bank == NUM_BANKS)
			{
				bank = 0;
			}
		}
	}
	else
	{
								ERROR("== Error - Unknown scheduling policy");
		exit(0);
	}

}


void CommandQueue::nextRankAndBank(unsigned &rank, unsigned &bank)
{
	if (schedulingPolicy == RankThenBankRoundRobin)
	{
		rank++;
		if (rank == NUM_RANKS)
		{
			rank = 0;
			bank++;
			if (bank == NUM_BANKS)
			{
				bank = 0;
			}
		}
	}
	//bank-then-rank round robin
	else if (schedulingPolicy == BankThenRankRoundRobin)
	{
		bank++;
		if (bank == NUM_BANKS)
		{
			bank = 0;
			rank++;
			if (rank == NUM_RANKS)
			{
				rank = 0;
			}
		}
	}
	else if (schedulingPolicy == fifo) {
		if(fifo_queue.empty()) {
			bank = 0;
			rank = 0;
		}
		else {
			rank = fifo_queue.front()->rank;
			bank = fifo_queue.front()->bank;
		}
	}
	else if (schedulingPolicy == fiforw) {
		if (fifo_read_queue.empty() && fifo_write_queue.empty()) {
			bank = 0;
			rank = 0;
		}
		else if (isCurrentQueueRead) {
			if (!fifo_read_queue.empty()){
				rank = fifo_read_queue.front()->rank;
				bank = fifo_read_queue.front()->bank;
			}
			else {
				rank = 0;
				bank = 0;
			}
		}
		else {
			if(!fifo_write_queue.empty()) {
				rank = fifo_write_queue.front()->rank;
				bank = fifo_write_queue.front()->bank;
			}
			else {
				rank = 0;
				bank = 0;
			}
		}
	}

	else if (schedulingPolicy == frfcs) {
		if(frfcs_queue.empty()) {
			bank = 0; 
			rank = 0;
		}
		else {
			rank = frfcs_queue.front()->rank;
			bank = frfcs_queue.front()->bank;
		}
	}
	else
	{
								ERROR("== Error - Unknown scheduling policy");
		exit(0);
	}

}


void CommandQueue::dumpActiveRowPerBank() {
	unsigned int numBanks;
	
	if (queuingStructure==PerRank ) {
		numBanks = 1;
	}
	else if (queuingStructure==PerRankPerBank|| queuingStructure == Queue) {
		numBanks = NUM_BANKS;
	}

	for (unsigned int i = 0; i<NUM_RANKS; i++) {
		for (unsigned int j = 0; j<numBanks; j++) {
			cout<<"\n Current Bank State of rank : " <<i<<" bank : "<<j;
			if (bankStates[i][j].currentBankState == 0) {
				cout<<" IDLE ";
			}
			else if (bankStates[i][j].currentBankState == 1) {
				cout<<" ROW ACTIVE";
			}
			else if (bankStates[i][j].currentBankState == 2) {
				cout<<" PRECHARGING";
			}
			else if (bankStates[i][j].currentBankState == 3) {
				cout<<" REFRESHING";
			}
			else if (bankStates[i][j].currentBankState == 4) {
				cout<<" POWER DOWN";
			}
			else {
				cout<<" UNKNOWN STATE";
			}
		}
	}
}

void CommandQueue::updateActiveRowPerBank() {
	unsigned int numBanks;
	unsigned int *rowActive;

	if(queuingStructure==PerRank) {
		numBanks = 1;
	}
	else if (queuingStructure==PerRankPerBank || queuingStructure == Queue){
		numBanks = NUM_BANKS;
	}

	rowActive = new unsigned int [numBanks];
	for (unsigned int i = 0; i<NUM_RANKS; i++) {
		for (unsigned int j = 0; j<numBanks; j++) {
			if(bankStates[i][j].currentBankState == 1) { // row is active in this bank
				if (rankActiveBanksMap.find(i) != rankActiveBanksMap.end()) {
					rankActiveBanksMapType::iterator rankActiveFound = rankActiveBanksMap.find(i);
					memcpy(rowActive, rankActiveFound->second, sizeof(unsigned int)*numBanks);	
					rowActive[j] = bankStates[i][j].openRowAddress;
					rankActiveBanksMap.erase(i);
					rankActiveBanksMap.insert(rankActiveBanksPairType(i, rowActive));
				}
			}
			else {
			}
		}
	}
}

void CommandQueue::update()
{
	//do nothing since pop() is effectively update(),
	//needed for SimulatorObject
	//TODO: make CommandQueue not a SimulatorObject
}
