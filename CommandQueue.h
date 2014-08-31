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






#ifndef CMDQUEUE_H
#define CMDQUEUE_H

//CommandQueue.h
//
//Header
//

#include "BusPacket.h"
#include "BankState.h"
#include "Transaction.h"
#include "SystemConfiguration.h"
#include "SimulatorObject.h"
#include "defines.h"
#include <map>
using namespace std;

namespace DRAMSim
{
class CommandQueue : public SimulatorObject
{
	CommandQueue();
	ostream &dramsim_log;
public:
	//typedefs
	typedef vector<BusPacket *> BusPacket1D;
	typedef vector<BusPacket1D> BusPacket2D;
	typedef vector<BusPacket2D> BusPacket3D;
 
 // Dta structure to hold bank's last rw time-stamp 
 typedef pair<unsigned int, uint64_t> bankLastRWTimeStampPairType;
 typedef map<unsigned int, uint64_t> bankLastRWTimeStampMapType;

 // Data structure to hold rank-bank timestamp mapping
 typedef pair<unsigned int, bankLastRWTimeStampMapType> rankBankTimeStampPairType;
 typedef map<unsigned int, bankLastRWTimeStampMapType> rankBankTimeStampMapType;

 // Data structure to hold bank's last accessed row
 typedef pair<unsigned int, int> bankLastAccessedRowPairType;
 typedef map<unsigned int, int> bankLastAccessedRowMapType;
 
 // Data structure to hold rank-bank last accessed row
 typedef pair<unsigned int, bankLastAccessedRowMapType> rankBankLastAccessedRowPairType;
 typedef map<unsigned int, bankLastAccessedRowMapType> rankBankLastAccessedRowMapType;

 // Data structure to hold last "actual" precharge to the bank
 typedef pair<unsigned int, uint64_t> bankLastPrechargeTimeStampPairType;
 typedef map<unsigned int, uint64_t> bankLastPrechargeTimeStampMapType;
 
 // Data structure to hold rank-bank last "actual" precharge
 typedef pair<unsigned int, bankLastPrechargeTimeStampMapType> rankBankLastPrechargeTimeStampPairType;
 typedef map<unsigned int, bankLastPrechargeTimeStampMapType> rankBankLastPrechargeTimeStampMapType;

 rankBankTimeStampMapType rankBankTimeStampMap;
 rankBankLastAccessedRowMapType rankBankLastAccessedRowMap;
 rankBankLastPrechargeTimeStampMapType rankBankLastPrechargeTimeStampMap; 
	//functions
	CommandQueue(vector< vector<BankState> > &states, ostream &dramsim_log);
	virtual ~CommandQueue(); 
 
 void adjustTimeOut();
	void enqueue(BusPacket *newBusPacket);
	bool pop(BusPacket **busPacket);
	bool hasRoomFor(unsigned numberToEnqueue, unsigned rank, unsigned bank);	 
 bool isIssuable(BusPacket *busPacket);
	bool isEmpty(unsigned rank);
	void needRefresh(unsigned rank);
	void print();
	void update(); //SimulatorObject requirement
	void rearrangeFRFCS();
	void dumpActiveRowPerBank();
 void updateBankRWTimeStamp(unsigned rank, unsigned bank, unsigned currentClockCycle); 
 void updateBankLastAccessedRow(unsigned rank, unsigned bank, int row);	
 void updateBankLastPrecharge(unsigned rank, unsigned bank, unsigned currentClockCycle); 
 vector<BusPacket *> &getCommandQueue(unsigned rank, unsigned bank);

	//fields
	BusPacket1D fifo_queue;	
	BusPacket1D fifo_read_queue;
	BusPacket1D fifo_write_queue;
	BusPacket1D frfcs_queue;
	BusPacket3D queues; // 3D array of BusPacket pointers

	vector< vector<BankState> > &bankStates;
private:
	void nextRankAndBank(unsigned &rank, unsigned &bank);
	void nextRankAndBankPRE(unsigned &rank, unsigned &bank);
	int setMinTimeStamp(uint64_t &timeStamp, unsigned &rank, unsigned &bank);
 //fields
 
	unsigned nextBank;
	unsigned nextRank;

	unsigned nextBankPRE;
	unsigned nextRankPRE;

	unsigned refreshRank;
	bool refreshWaiting;

	vector< vector<unsigned> > tFAWCountdown;
	vector< vector<unsigned> > rowAccessCounters;

	bool sendAct;
	unsigned mistakeCounter;
 
 // For RW FIFO arbitration scheme
	bool isCurrentQueueRead;
	bool rwDependencyFound;
	uint64_t indexAddress;
	bool isWriteDrained; 
};
}

#endif

