#######################################################################################
#Python file to simulate traffic gen across different address schemes and page-policies
#######################################################################################
#!/usr/bin/python
import re
import os

#Real deal
addressStyle = ['Wang', 'scheme1', 'scheme2', 'scheme3', 'scheme4', 'scheme5', 'scheme6']
pagePolicy = ['open_page', 'close_page']
arbSchemes = ['FRFCFS', 'FIFO', 'bank_then_rank_round_robin']

# Test cases
#addressStyle = ['Wang']
#pagePolicy = ['open_page']
#arbSchemes = ['bank_then_rank_round_robin']

iniLine1 = ''
iniLine2 = ''
iniLine3 = ''
needQueue = False
for k in arbSchemes:
	for i in pagePolicy:
		for j in addressStyle:		
			buffDirCmd = 'mkdir folder_tg_buff_'+str(i)+'_'+str(j)+'_'+str(k)
			simoutDirCmd = 'mkdir folder_tg_sim_out'+str(i)+'_'+str(j)+'_'+str(k)
			os.system(buffDirCmd)
			os.system(simoutDirCmd)
			if str(k) == "FIFO" or str(k) == "FRFCFS":
				needQueue = True
			else:
				needQueue = False
			f = open('system.ini', 'r')
			g = open('system.ini.example', 'w')
			iniLine1 = "ADDRESS_MAPPING_SCHEME="+str(j)
			iniLine2 = "ROW_BUFFER_POLICY="+str(i)
			iniLine3 = "SCHEDULING_POLICY="+str(k)
			iniLine4 = "QUEUING_STRUCTURE=queue"
			for lines in f:
				modifiedString1 = re.sub(r'ADDRESS_MAPPING_SCHEME=.*', iniLine1, lines)
				modifiedString2 = re.sub(r'ROW_BUFFER_POLICY=.*', iniLine2, lines)
				modifiedString3 = re.sub(r'SCHEDULING_POLICY=.*', iniLine3, lines)
				modifiedString4 = re.sub(r'QUEUING_STRUCTURE=.*', iniLine4, lines)
				if(modifiedString1 != lines):
					g.write(modifiedString1)
				elif(modifiedString2 != lines):
					g.write(modifiedString2)
				elif(modifiedString3 != lines):
					g.write(modifiedString3)
				elif(modifiedString4 != lines and needQueue == True):
					g.write(modifiedString4)
				else:
					g.write(lines)
			f.seek(0)
			g.seek(0)
			f.close()
			g.close()
    
			referenceAddrStr = "0x00000000"
			addrStr = ["0x00000001", "0x00000002", "0x00000004","0x00000008","0x00000010","0x00000020", "0x00000040", "0x00000080","0x00000100","0x00000200","0x00000400", "0x00000800", "0x00001000","0x00002000", "0x00004000", "0x00008000", "0x00010000","0x00020000","0x00040000", "0x00080000", "0x00100000","0x00200000", "0x00400000", "0x00800000", "0x01000000","0x02000000", "0x04000000", "0x08000000", "0x10000000","0x20000000", "0x40000000", "0x80000000"]
		
		# Step 1 : demystify open-page and close-page
		# Step 2 : Based on page-policy, demystify address mapping
		# Step 3 : Demystify arb scheme



			bitPosition = 0
			for l in addrStr:
		  	#####################################################################################
				## TEST FOR OPEN PAGE			
				# Demystify open-page, give second request at t >= 15 cycles for read-read case
				traceInputFile = 'traces/mase_art_trace_tg_op_col_read_read_bit_'+str(bitPosition)+'_timing_15_'+str(i)+'_'+str(j)+'_'+str(k)+'.trc'
				fileCreateCmd = 'touch '+str(traceInputFile)
				os.system(fileCreateCmd)
				h = open(traceInputFile, 'w')
				line1 = str(referenceAddrStr) + ' READ 0\n'
				line2 = str(l) + ' READ 15'
				h.write(line1)
				h.write(line2)
				h.close()
				outputBuff = 'buff_tg_op_col_read_read_arrival15_bit'+str(bitPosition)+'_'+str(i)+'_'+str(j)+'_'+str(k)
				dramCmd = './DRAMSim -s system.ini.example -d ini/DDR3_micron_16M_8B_x8_sg15.ini -t '+str(traceInputFile) + ' -c1000 *. &>'+str(outputBuff)
				os.system(dramCmd)
				renameTraceCmd = 'mv sim_out_DDR3_micron_16M_8B_x8_sg15.ini.tmp sim_out_tg_op_col_read_read_arrival15_bit_'+str(bitPosition)+'_'+str(i)+'_'+str(j)+'_'+str(k)
				os.system(renameTraceCmd)
				###################################################################################

				###################################################################################
				## TEST FOR CLOSE PAGE
				# Demystify close-page, give second request at t = [24-29] for read-read case
				traceInputFile = 'traces/mase_art_trace_tg_cp_samebank_read_read_bit_'+str(bitPosition)+'_timing_26_'+str(i)+'_'+str(j)+'_'+str(k)+'.trc'
				fileCreateCmd = 'touch '+str(traceInputFile)
				os.system(fileCreateCmd)
				h = open(traceInputFile, 'w')
				line1 = str(referenceAddrStr) + ' READ 0\n'
				line2 = str(l) + ' READ 26'
				h.write(line1)
				h.write(line2)
				h.close()
				outputBuff = 'buff_tg_cp_samebank_read_read_arrival15_bit'+str(bitPosition)+'_'+str(i)+'_'+str(j)+'_'+str(k)
				dramCmd = './DRAMSim -s system.ini.example -d ini/DDR3_micron_16M_8B_x8_sg15.ini -t '+str(traceInputFile) + ' -c1000 *. &>'+str(outputBuff)
				os.system(dramCmd)
				renameTraceCmd = 'mv sim_out_DDR3_micron_16M_8B_x8_sg15.ini.tmp sim_out_tg_cp_samebank_read_read_arrival26_bit_'+str(bitPosition)+'_'+str(i)+'_'+str(j)+'_'+str(k)
				os.system(renameTraceCmd)	
				###################################################################################
	
				###################################################################################
				# Demystify open-page row-bits, give second request at t = 1 for read-read case
				traceInputFile = 'traces/mase_art_trace_tg_row_op_read_read_bit_'+str(bitPosition)+'_timing_1_'+str(i)+'_'+str(j)+'_'+str(k)+'.trc'
				fileCreateCmd = 'touch '+str(traceInputFile)
				os.system(fileCreateCmd)
				h = open(traceInputFile, 'w')
				line1 = str(referenceAddrStr) + ' READ 0\n'
				line2 = str(l) + ' READ 1'
				h.write(line1)
				h.write(line2)
				h.close()
				outputBuff = 'buff_tg_row_op_read_read_arrival1_bit'+str(bitPosition)+'_'+str(i)+'_'+str(j)+'_'+str(k)
				dramCmd = './DRAMSim -s system.ini.example -d ini/DDR3_micron_16M_8B_x8_sg15.ini -t '+str(traceInputFile) + ' -c1000 *. &>'+str(outputBuff)
				os.system(dramCmd)
				renameTraceCmd = 'mv sim_out_DDR3_micron_16M_8B_x8_sg15.ini.tmp sim_out_tg_row_op_read_read_arrival1_bit_'+str(bitPosition)+'_'+str(i)+'_'+str(j)+'_'+str(k)
				os.system(renameTraceCmd)
				###################################################################################
	
				###################################################################################
				# Demystify open-page bank bits, give second request at t = 1 for write-read case
				traceInputFile = 'traces/mase_art_trace_tg_op_bank_write_read_bit_'+str(bitPosition)+'_timing_1_'+str(i)+'_'+str(j)+'_'+str(k)+'.trc'
				fileCreateCmd = 'touch '+str(traceInputFile)
				os.system(fileCreateCmd)
				h = open(traceInputFile, 'w')
				line1 = str(referenceAddrStr) + ' WRITE 0\n'
				line2 = str(l) + ' READ 1'
				h.write(line1)
				h.write(line2)
				h.close()
				outputBuff = 'buff_tg_op_bank_write_read_arrival1_bit'+str(bitPosition)+'_'+str(i)+'_'+str(j)+'_'+str(k)
				dramCmd = './DRAMSim -s system.ini.example -d ini/DDR3_micron_16M_8B_x8_sg15.ini -t '+str(traceInputFile) + ' -c1000 *. &>'+str(outputBuff)
				os.system(dramCmd)
				renameTraceCmd = 'mv sim_out_DDR3_micron_16M_8B_x8_sg15.ini.tmp sim_out_tg_op_bank_write_read_arrival1_bit_'+str(bitPosition)+'_'+str(i)+'_'+str(j)+'_'+str(k)
				os.system(renameTraceCmd)
				###################################################################################
							
				###################################################################################
				#Demystify open-page rank bits, give second request at t = 1 for write-read case
				traceInputFile = 'traces/mase_art_trace_tg_op_rank_write_read_bit_'+str(bitPosition)+'_timing_1_'+str(i)+'_'+str(j)+'_'+str(k)+'.trc'
				fileCreateCmd = 'touch '+str(traceInputFile)
				os.system(fileCreateCmd)
				h = open(traceInputFile, 'w')
				line1 = str(referenceAddrStr) + ' WRITE 0\n'
				line2 = str(l) + ' READ 1'
				h.write(line1)
				h.write(line2)
				h.close()
				outputBuff = 'buff_tg_op_rank_write_read_arrival1_bit'+str(bitPosition)+'_'+str(i)+'_'+str(j)+'_'+str(k)
				dramCmd = './DRAMSim -s system.ini.example -d ini/DDR3_micron_16M_8B_x8_sg15.ini -t '+str(traceInputFile) + ' -c1000 *. &>'+str(outputBuff)
				os.system(dramCmd)
				renameTraceCmd = 'mv sim_out_DDR3_micron_16M_8B_x8_sg15.ini.tmp sim_out_tg_op_rank_write_read_arrival1_bit_'+str(bitPosition)+'_'+str(i)+'_'+str(j)+'_'+str(k)
				os.system(renameTraceCmd)	

				bitPosition = bitPosition + 1
			###################################################################################
			#Demystify arb scheme FRFCFS
			accessSequence = []
			if str(j) == "scheme1" :
				accessSequence = ['0x00800000', '0x0000F000']
			elif str(j) == "scheme2" :
				accessSequence = ['0x08000000', '0x0000F000']
			elif str(j) == "scheme3" :
				accessSequence = ['0x0000F000', '0x00F00000']
			elif str(j) == "scheme4":
				accessSequence = ['0x00F00000', '0x00000F00']
			elif str(j) == "scheme5":
				accessSequence = ['0x0F000000', '0x0000F000']
			elif str(j) == "scheme6":
				accessSequence = ['0x0F000000', '0x000000F0']
			elif str(j) == "Wang":
				accessSequence = ['0x0F000000', '0x00000080']
			traceInputFile = 'traces/mase_art_trace_FRFCFScheck'+str(i)+'_'+str(j)+'.trc'
			fileCreateCmd = 'touch '+str(traceInputFile)
			os.system(fileCreateCmd)
			h = open(traceInputFile, 'w')
			line0 = '0x00000000 READ 0\n'
			line1 = str(accessSequence[0]) + ' READ 1\n'
			line2 = str(accessSequence[1]) + ' READ 2\n'
			h.write(line0)
			h.write(line1)
			h.write(line2)
			h.close()
			outputBuff = 'buff_tg_FRFCFScheck'+str(i)+'_'+str(j)
			dramCmd = './DRAMSim -s system.ini.example -d ini/DDR3_micron_16M_8B_x8_sg15.ini -t '+str(traceInputFile) + ' -c1000 *. &>'+str(outputBuff)
			os.system(dramCmd)
			renameTraceCmd = 'mv sim_out_DDR3_micron_16M_8B_x8_sg15.ini.tmp sim_out_tg_FRFCFScheck'+str(i)+'_'+str(j)
			os.system(renameTraceCmd)					
			###################################################################################
				
			###################################################################################
			#Demystify arb scheme RR
			accessSequence = []
			if str(j) == "scheme1" :
				accessSequence = ['0x00880000', '0x000F0080']
			elif str(j) == "scheme2" :
				accessSequence = ['0x0F000000', '0x0F000100']
			elif str(j) == "scheme3" :
				accessSequence = ['0x0000F000', '0x100F0000']
			elif str(j) == "scheme4":
				accessSequence = ['0x00F00000', '0x100F0000']
			elif str(j) == "scheme5":
				accessSequence = ['0x0F000000', '0x0F000080']
			elif str(j) == "scheme6":
				accessSequence = ['0x0F000000', '0x0F001000']
			elif str(j) == "Wang":
				accessSequence = ['0x10000000', '0x10008000']
			traceInputFile = 'traces/mase_art_trace_RRcheck'+str(i)+'_'+str(j)+'.trc'
			fileCreateCmd = 'touch '+str(traceInputFile)
			os.system(fileCreateCmd)
			h = open(traceInputFile, 'w')
			line0 = '0x00000000 READ 0\n'
			line1 = str(accessSequence[0]) + ' READ 1\n'
			line2 = str(accessSequence[1]) + ' READ 2\n'
			h.write(line0)
			h.write(line1)
			h.write(line2)
			h.close()
			outputBuff = 'buff_tg_RRcheck'+str(i)+'_'+str(j)
			dramCmd = './DRAMSim -s system.ini.example -d ini/DDR3_micron_16M_8B_x8_sg15.ini -t '+str(traceInputFile) + ' -c1000 *. &>'+str(outputBuff)
			os.system(dramCmd)
			renameTraceCmd = 'mv sim_out_DDR3_micron_16M_8B_x8_sg15.ini.tmp sim_out_tg_RRcheck'+str(i)+'_'+str(j)
			os.system(renameTraceCmd)						
							
			collectCmd ='mv buff_tg_* folder_tg_buff_'+str(i)+'_'+str(j)+'_'+str(k)+'/; mv sim_out_tg* folder_tg_sim_out'+str(i)+'_'+str(j)+'_'+str(k)
			print collectCmd
			os.system(collectCmd)
