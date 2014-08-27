#######################################################################################
#Python file to simulate traffic gen across different arbitration schemes 
#######################################################################################
#!/usr/bin/python
import re
import os

#Real deal
#addressStyle = ['scheme1']
#pagePolicy = ['open_page', 'close_page']
#arbScheme = ['FIFO', 'bank_then_rank_round_robin', 'FRFCFS']

#Test cases
addressStyle = ['scheme1']
pagePolicy = ['open_page']
arbScheme = ['bank_then_rank_round_robin']

needQueue = False
iniLine1 = ''
iniLine2 = ''
for i in pagePolicy:
	for j in arbScheme:
		if str(j) == "FIFO" or str(j) == "FRFCFS":
			needQueue = True
		else:
			needQueue = False
		buffDirCmd = 'mkdir folder_arb_buff_' + str(j) + '_' + str(i)
		simoutDirCmd = 'mkdir folder_arb_sim_out'+str(j)+'_'+str(i)
		os.system(buffDirCmd)
		os.system(simoutDirCmd)
		f = open('system.ini', 'r')
		g = open('system.ini.example', 'w')				
		iniLine1 = "SCHEDULING_POLICY="+str(j)
		iniLine2 = "ROW_BUFFER_POLICY="+str(i)
		iniLine3 = "QUEUING_STRUCTURE=queue"
		for lines in f:
			modifiedString1 = re.sub(r'SCHEDULING_POLICY=.*', iniLine1, lines)
			modifiedString2 = re.sub(r'ROW_BUFFER_POLICY=.*', iniLine2, lines)
			modifiedString3 = re.sub(r'QUEUING_STRUCTURE=.*', iniLine3, lines)
			if(modifiedString1 != lines):
				g.write(modifiedString1)				
			elif (modifiedString2 != lines):
				g.write(modifiedString2)
			elif (modifiedString3 != lines and needQueue == True):
				g.write(modifiedString3)	
			else:
				g.write(lines)
		f.seek(0)
		g.seek(0)
		f.close()
		g.close()
        
		accessSequence1 = ['0x00800000', '0x0200F000'] 
		accessSequence2 = ['0x00800000', '0x0E000080']
	
		# k loop is for time-stamps for second request
		for k in range(1, 60):
		# l loop is for time-stamps for third request
			for l in range(k, 60):
				#############################################################
				traceInputFile = 'traces/mase_art_trace_arb1_'+str(j)+'_'+str(i)+'_first'+str(k)+'_second'+str(l)+'.trc'
				fileCreateCmd = 'touch '+str(traceInputFile)
				#create trace file inputs
				os.system(fileCreateCmd)
				h = open(traceInputFile, 'w')
				line1 = '0x02000000 READ 0\n'
				h.write(line1)
				#for m in accessSequence1:

				line1 = str(accessSequence1[0]) + ' READ '+str(k)+'\n'
				line2 = str(accessSequence1[1]) + ' READ ' +str(l) + '\n'
				h.write(line1)
				h.write(line2)

				h.close()
				outputBuff = 'buff_arb1_'+str(j)+'_'+str(i)+'_first'+str(k)+'_second'+str(l)
				dramCmd = './DRAMSim -s system.ini.example -d ini/DDR3_micron_16M_8B_x8_sg15.ini -t '+str(traceInputFile) + ' -c1000 &> '+str(outputBuff)
				os.system(dramCmd)
				renameTraceCmd = 'mv sim_out_DDR3_micron_16M_8B_x8_sg15.ini.tmp sim_out_arb1_'+str(j)+'_'+str(i)+'_first'+str(k)+'_second'+str(l)
				os.system(renameTraceCmd)
				
				##############################################################

				traceInputFile = 'traces/mase_art_trace_arb2_'+str(j)+'_'+str(i)+'_first'+str(k)+'_second'+str(l)+'.trc'
				fileCreateCmd = 'touch '+str(traceInputFile)
				#create trace file inputs
				os.system(fileCreateCmd)
				h = open(traceInputFile, 'w')
				line1 = '0x02000000 READ 0\n'
				h.write(line1)
				#for m in accessSequence2:
				line1 = str(accessSequence2[0]) + ' READ '+str(k)+'\n'
				line2 = str(accessSequence2[1]) + ' READ '+str(l) + '\n'
				h.write(line1)
				h.write(line2)
					
				h.close()
				outputBuff = 'buff_arb2_'+str(j)+'_'+str(i)+'_first'+str(k)+'_second'+str(l)
				dramCmd = './DRAMSim -s system.ini.example -d ini/DDR3_micron_16M_8B_x8_sg15.ini -t '+str(traceInputFile) + ' -c100 &> '+str(outputBuff)
				os.system(dramCmd)
				renameTraceCmd = 'mv sim_out_DDR3_micron_16M_8B_x8_sg15.ini.tmp sim_out_arb2_'+str(j)+'_'+str(i)+'_first'+str(k)+'_second'+str(l)
				os.system(renameTraceCmd)
		
		
		collectCmd = "mv buff_* folder_arb_buff_"+str(j)+'_'+str(i)+'/; mv sim_out* folder_arb_sim_out'+str(j)+'_'+str(i)+'/'
		print collectCmd
		os.system(collectCmd)	
