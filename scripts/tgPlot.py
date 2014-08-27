#!/usr/bin/python

import os
import re

#Real deal
pagePolicies = ['open_page', 'close_page']
arbSchemes = ['FIFO', 'FRFCFS', 'bank_then_rank_round_robin']
addressStyles = ['Wang', 'scheme1', 'scheme2', 'scheme3', 'scheme4', 'scheme5', 'scheme6']

# Test cases
#addressStyles = ['Wang']
#pagePolicies = ['open_page']
#arbSchemes = ['bank_then_rank_round_robin']

filePrefix = 'folder_tg_sim_out'
for i in addressStyles:
	for j in pagePolicies:
		for l in arbSchemes:
			dirName = str(filePrefix) + str(j)+'_'+str(i)+'_'+str(l)
			os.chdir(str(dirName))
			dirList = os.listdir(os.getcwd())
			cpSameBankDir = []
			opBankWriteReadDir = []
			opColReadReadDir = []
			opRankWriteReadDir = []
			opRowReadReadDir = []
			for k in dirList:
					cpSameBankDirMatch = re.search(r'cp_samebank', str(k), re.M|re.I)
					opBanksWriteReadMatch = re.search(r'op_bank_write_read', str(k), re.M|re.I)
					opColReadReadMatch = re.search(r'op_col_read_read', str(k), re.M|re.I)
					opRankWriteReadMatch = re.search(r'op_rank_write_read', str(k), re.M|re.I)
					opRowReadReadMatch = re.search(r'row_op_read_read', str(k), re.M|re.I)
				
					if(cpSameBankDirMatch):
						cpSameBankDir.append(str(k))
					if(opBanksWriteReadMatch):
						opBankWriteReadDir.append(str(k))
					if(opColReadReadMatch):
						opColReadReadDir.append(str(k))
					if(opRankWriteReadMatch):
						opRankWriteReadDir.append(str(k))
					if(opRowReadReadMatch):						
						opRowReadReadDir.append(str(k))
		
			##################################################################################
			
			gnuplotDatFile = 'gnuplot_cpsamebank'+str(i)+'_'+str(j)+'.dat'
			os.system('touch '+str(gnuplotDatFile))
			line1 = '.*\s(\d+): activate .*'
			line2 = '.*\s(\d+): read .*'	
			g = open(str(gnuplotDatFile), 'w')
			for k in cpSameBankDir:
				print k
				accessTime = 0
				activateTime = 0
				totalTime = 0
				entryTime = 0
				accessTypeMatch = re.match(r'.*bit_(\d+)', str(k))
				f = open(str(k),'r')							
				lineCount = 0
				activateFound = False

				matchResult1 = []
				matchResult2 = []
				entryTime = 26
				for lines in f:	
					if lineCount > 0:
						if(activateFound == False):
							matchResult1 = re.match(line1, lines, re.M|re.I)
							if(matchResult1):
								activateFound = True
						matchResult2 = re.match(line2, lines, re.M|re.I)
					lineCount = lineCount + 1
	
				if matchResult2 :
					accessTime = str(matchResult2.group(1))
				if matchResult1 :
					activateTime = str(matchResult1.group(1))
				
				###########################################
				print 'Debugging info'
				print 'Access Time : ' + str(accessTime)
				print 'Activate Time : ' + str(activateTime)
				print 'entryTime : '+str(entryTime)
				print 'File Name : ' + str(k)
				print '@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@'
				###########################################
				totalTime = (int(accessTime) - int(activateTime)) + 10 +(int(activateTime) - int(entryTime) - 2)
				g.write(str(accessTypeMatch.group(1)) + '\t'+str(totalTime) + '\n')
						
			g.close()	
			
			##################################################################################
			
			gnuplotDatFile = 'gnuplot_opbank_write_read'+str(i)+'_'+str(j)+'.dat'
			os.system('touch '+str(gnuplotDatFile))
			line1 = '.*\s(\d+): activate .*'
			line2 = '.*\s(\d+): read .*'	
			g = open(str(gnuplotDatFile), 'w')
			for k in opBankWriteReadDir:
				print k
				accessTime = 0
				activateTime = 0
				totalTime = 0
				entryTime = 0
				accessTypeMatch = re.match(r'.*bit_(\d+)', str(k))
				f = open(str(k),'r')							
				lineCount = 0
				activateFound = False

				matchResult1 = []
				matchResult2 = []
				entryTime = 1
				for lines in f:	
					if lineCount > 0:
						if(activateFound == False):
							matchResult1 = re.match(line1, lines, re.M|re.I)
							if(matchResult1):
								activateFound = True
						matchResult2 = re.match(line2, lines, re.M|re.I)
					lineCount = lineCount + 1
	
				if matchResult2 :
					accessTime = str(matchResult2.group(1))
				if matchResult1 :
					activateTime = str(matchResult1.group(1))
				
				###########################################
				print 'Debugging info'
				print 'Access Time : ' + str(accessTime)
				print 'Activate Time : ' + str(activateTime)
				print 'entryTime : '+str(entryTime)
				print 'File Name : ' + str(k)
				print '@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@'
				###########################################
				totalTime = (int(accessTime) - int(activateTime)) + 10 +(int(activateTime) - int(entryTime) - 2)
				g.write(str(accessTypeMatch.group(1)) + '\t'+str(totalTime) + '\n')
						
			g.close()	

			##################################################################################
			
			gnuplotDatFile = 'gnuplot_opcol_read_read'+str(i)+'_'+str(j)+'.dat'
			os.system('touch '+str(gnuplotDatFile))
			line1 = '.*\s(\d+): activate .*'
			line2 = '.*\s(\d+): read .*'	
			g = open(str(gnuplotDatFile), 'w')
			for k in opColReadReadDir:
				print k
				accessTime = 0
				activateTime = 0
				totalTime = 0
				entryTime = 0
				accessTypeMatch = re.match(r'.*bit_(\d+)', str(k))
				f = open(str(k),'r')							
				lineCount = 0
				activateFound = False

				matchResult1 = []
				matchResult2 = []
				entryTime = 15
				for lines in f:	
					if lineCount > 0:
						if(activateFound == False):
							matchResult1 = re.match(line1, lines, re.M|re.I)
							if(matchResult1):
								activateFound = True
						matchResult2 = re.match(line2, lines, re.M|re.I)
					lineCount = lineCount + 1
	
				if matchResult2 :
					accessTime = str(matchResult2.group(1))
				if matchResult1 :
					activateTime = str(matchResult1.group(1))
				
				totalTime = (int(accessTime) - int(activateTime)) + 10 +(int(activateTime) - int(entryTime) - 2)
				g.write(str(accessTypeMatch.group(1)) + '\t'+str(totalTime) + '\n')
				###########################################
				print 'Debugging info'
				print 'Access Time : ' + str(accessTime)
				print 'Activate Time : ' + str(activateTime)
				print 'entryTime : '+str(entryTime)
				print 'Total Time : '+str(totalTime)
				print 'File Name : ' + str(k)
				print '@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@'
				###########################################

						
			g.close()	

			##################################################################################

			gnuplotDatFile = 'gnuplot_oprank_write_read'+str(i)+'_'+str(j)+'.dat'
			os.system('touch '+str(gnuplotDatFile))
			line1 = '.*\s(\d+): activate .*'
			line2 = '.*\s(\d+): read .*'	
			g = open(str(gnuplotDatFile), 'w')
			for k in opRankWriteReadDir:
				print k
				accessTime = 0
				activateTime = 0
				totalTime = 0
				entryTime = 0
				accessTypeMatch = re.match(r'.*bit_(\d+)', str(k))
				f = open(str(k),'r')							
				lineCount = 0
				activateFound = False

				matchResult1 = []
				matchResult2 = []
				entryTime = 1
				for lines in f:	
					if lineCount > 0:
						if(activateFound == False):
							matchResult1 = re.match(line1, lines, re.M|re.I)
							if(matchResult1):
								activateFound = True
						matchResult2 = re.match(line2, lines, re.M|re.I)
					lineCount = lineCount + 1
	
				if matchResult2 :
					accessTime = str(matchResult2.group(1))
				if matchResult1 :
					activateTime = str(matchResult1.group(1))
				
				totalTime = (int(accessTime) - int(activateTime)) + 10 +(int(activateTime) - int(entryTime) - 2)
				g.write(str(accessTypeMatch.group(1)) + '\t'+str(totalTime) + '\n')
				###########################################
				print 'Debugging info'
				print 'Access Time : ' + str(accessTime)
				print 'Activate Time : ' + str(activateTime)
				print 'entryTime : '+str(entryTime)
				print 'Total time : '+str(totalTime)
				print 'File Name : ' + str(k)
				print '@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@'
				###########################################

						
			g.close()	

			##################################################################################

			gnuplotDatFile = 'gnuplot_rowop_read_read'+str(i)+'_'+str(j)+'.dat'
			os.system('touch '+str(gnuplotDatFile))
			line1 = '.*\s(\d+): activate .*'
			line2 = '.*\s(\d+): read .*'	
			g = open(str(gnuplotDatFile), 'w')
			for k in opRowReadReadDir:
				print k
				accessTime = 0
				activateTime = 0
				totalTime = 0
				entryTime = 0
				accessTypeMatch = re.match(r'.*bit_(\d+)', str(k))
				f = open(str(k),'r')							
				lineCount = 0
				activateFound = False

				matchResult1 = []
				matchResult2 = []
				entryTime = 1
				for lines in f:	
					if lineCount > 0:
						if(activateFound == False):
							matchResult1 = re.match(line1, lines, re.M|re.I)
							if(matchResult1):
								activateFound = True
						matchResult2 = re.match(line2, lines, re.M|re.I)
					lineCount = lineCount + 1
	
				if matchResult2 :
					accessTime = str(matchResult2.group(1))
				if matchResult1 :
					activateTime = str(matchResult1.group(1))
				
				###########################################
				print 'Debugging info'
				print 'Access Time : ' + str(accessTime)
				print 'Activate Time : ' + str(activateTime)
				print 'entryTime : '+str(entryTime)
				print 'File Name : ' + str(k)
				print '@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@'
				###########################################
				totalTime = (int(accessTime) - int(activateTime)) + 10 +(int(activateTime) - int(entryTime) - 2)
				g.write(str(accessTypeMatch.group(1)) + '\t'+str(totalTime) + '\n')
						
			g.close()	
			os.chdir('../')
