#######################################################################################
#Python file to simulate traffic gen across different address schemes and page-policies
#######################################################################################
#!/usr/bin/python
import re
import os

#Real deal
#addressStyle = ['scheme1', 'scheme2', 'scheme3', 'scheme4', 'scheme5', 'scheme6', 'scheme7', 'wang']
#pagePolicy = ['open_page', 'close_page']

# Test cases
addressStyle = ['Wang']
pagePolicy = ['open_page']

iniLine1 = ''
iniLine2 = ''
for i in pagePolicy:
	for j in addressStyle:
		buffDirCmd = 'mkdir folder_buff_' + str(j) + '_' + str(i)
		simoutDirCmd = 'mkdir folder_sim_out'+str(j)+'_'+str(i)
		os.system(buffDirCmd)
		os.system(simoutDirCmd)
		f = open('system.ini', 'r')
		g = open('system.ini.example', 'w')				
		iniLine1 = "ADDRESS_MAPPING_SCHEME="+str(j)
		iniLine2 = "ROW_BUFFER_POLICY="+str(i)
		for lines in f:
			modifiedString1 = re.sub(r'ADDRESS_MAPPING_SCHEME=.*', iniLine1, lines)
			modifiedString2 = re.sub(r'ROW_BUFFER_POLICY=.*', iniLine2, lines)
			if(modifiedString1 != lines):
				g.write(modifiedString1)
			elif (modifiedString2 != lines):
				g.write(modifiedString2)
			else:
				g.write(lines)
		f.seek(0)
		g.seek(0)
		f.close()
		g.close()
        
		referenceAddrStr = "0x00000000"
		addrStr = ["0x00000001", "0x00000002", "0x00000004","0x00000008","0x00000010","0x00000020", "0x00000040", "0x00000080","0x00000100","0x00000200","0x00000400", "0x00000800", "0x00001000","0x00002000", "0x00004000", "0x00008000", "0x00010000","0x00020000","0x00040000", "0x00080000", "0x00100000","0x00200000", "0x00400000", "0x00800000", "0x01000000","0x02000000", "0x04000000", "0x08000000", "0x10000000","0x20000000", "0x40000000", "0x80000000"]
        # k loop for arrival time-stamp for second request.
		for k in range(0,50):
			bitPosition = 0
			for l in addrStr: 
				traceInputFile = ''
				# m loop for different access patterns : 
        # read-read, read-write, write-read, write-write
				for m in range(0,4):
					if m == 0:
						traceInputFile = 'traces/mase_art_trace_read_read_bit_' + str(bitPosition)+'_timing_'+str(k)+'.trc'
						fileCreateCmd = 'touch ' + str(traceInputFile)
						# create trace file inputs
						os.system(fileCreateCmd)
						h = open(traceInputFile, 'w')
						line1 = str(referenceAddrStr) + ' READ 0\n'
						line2 = str(l) + ' READ ' + str(k)
						h.write(line1)
						h.write(line2)
						h.close()
						outputBuff = 'buff_read_read' + str(bitPosition) + '_' + str(i) + '_' + str(j)+'_t'+str(k) 						
						dramCmd = './DRAMSim -s system.ini.example -d ini/DDR3_micron_16M_8B_x8_sg15.ini -t '+str(traceInputFile) + ' -c1000 &> '+str(outputBuff)

						os.system(dramCmd)
						renameTraceCmd = 'mv sim_out_DDR3_micron_16M_8B_x8_sg15.ini.tmp  sim_out' + str(i) + '_' + str(j) + '_' + str(bitPosition)+'_read_read_timing'+str(k)
						os.system(renameTraceCmd)
						#####################################################################
					if m == 1:
						traceInputFile = 'traces/mase_art_trace_read_write_bit_' + str(bitPosition)+'_timing_'+str(k)+'.trc'
						fileCreateCmd = 'touch ' + str(traceInputFile)
                  # create trace file input 
						os.system(fileCreateCmd)
						h = open(traceInputFile, 'w')
						line1 = str(referenceAddrStr) + ' READ 0\n'
						line2 = str(l) + ' WRITE ' + str(k)
						h.write(line1)
						h.write(line2)
						h.close()
						outputBuff = 'buff_read_write' + str(bitPosition) + '_' + str(i) + '_' + str(j) +'_t'+str(k)
						dramCmd = './DRAMSim -s system.ini.example -d ini/DDR3_micron_16M_8B_x8_sg15.ini -t '+str(traceInputFile) + ' -c100 &> '+str(outputBuff)
						os.system(dramCmd)
						renameTraceCmd = 'mv sim_out_DDR3_micron_16M_8B_x8_sg15.ini.tmp  sim_out' + str(i) + '_' + str(j) + '_' + str(bitPosition)+'_read_write_timing'+str(k)
						os.system(renameTraceCmd)

						#####################################################################
					if m == 2:
						traceInputFile = 'traces/mase_art_trace_write_read_bit_' + str(bitPosition)+'_timing_'+str(k)+'.trc'
						fileCreateCmd = 'touch ' + str(traceInputFile)
                   # create trace file input 
						os.system(fileCreateCmd)
						h = open(traceInputFile, 'w')
						line1 = str(referenceAddrStr) + ' WRITE 0\n'
						line2 = str(l) + ' READ ' + str(k)
						h.write(line1)
						h.write(line2)
						h.close()
						outputBuff = 'buff_write_read' + str(bitPosition) + '_' + str(i) + '_' + str(j)+'_t'+str(k) 
						dramCmd = './DRAMSim -s system.ini.example -d ini/DDR3_micron_16M_8B_x8_sg15.ini -t '+str(traceInputFile) + ' -c100 &> '+str(outputBuff)
						os.system(dramCmd)
						renameTraceCmd = 'mv sim_out_DDR3_micron_16M_8B_x8_sg15.ini.tmp  sim_out' + str(i) + '_' + str(j) + '_' + str(bitPosition)+'_write_read_timing'+str(k)
						os.system(renameTraceCmd)
					
						#####################################################################
					if m == 3:
						traceInputFile = 'traces/mase_art_trace_write_write_bit_' + str(bitPosition)+'_timing_'+str(k)+'.trc'
						fileCreateCmd = 'touch ' + str(traceInputFile)
                  # create trace file input 
						os.system(fileCreateCmd)
						h = open(traceInputFile, 'w')
						line1 = str(referenceAddrStr) + ' WRITE 0\n'
						line2 = str(l) + ' WRITE ' + str(k)
						h.write(line1)
						h.write(line2)
						h.close()
						outputBuff = 'buff_write_write' + str(bitPosition) + '_' + str(i) + '_' + str(j) +'_t'+str(k)
						dramCmd = './DRAMSim -s system.ini.example -d ini/DDR3_micron_16M_8B_x8_sg15.ini -t '+str(traceInputFile) + ' -c100 &> '+str(outputBuff)
						os.system(dramCmd)
						renameTraceCmd = 'mv sim_out_DDR3_micron_16M_8B_x8_sg15.ini.tmp  sim_out' + str(i) + '_' + str(j) + '_' + str(bitPosition)+'_write_write_timing'+str(k)                
						os.system(renameTraceCmd)
				
						#####################################################################
				bitPosition = bitPosition + 1     
	
		collectCmd = "mv buff_* folder_buff_"+str(j)+'_'+str(i)+'/; mv sim_out* folder_sim_out'+str(j)+'_'+str(i)+'/'
		print collectCmd
		os.system(collectCmd)	
