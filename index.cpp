#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <bitset>
using namespace std;

/*
	Twos Complement Conversion
*/
#define TWOCOMP(x) (((x)>7)?x-16:x)
#define TWOCOMP2(x) (((x)>127)?x-256:x)

/*
	Size Limits
*/
#define MAX_IMEM_SIZE 256
#define MAX_DMEM_SIZE 512
#define MAX_REGS 16

/*
	Instruction Reference
*/
#define ADD  0
#define ADDI 1
#define SUB  2
#define SUBI 3
#define MUL  6
#define MULI 7

#define LD  8
#define SD 10

#define JMP  12
#define BEQZ 13
#define HLT  14

/*
	Structure to hold instructions
*/
typedef struct Instruction{
	unsigned short opcode : 4;
	unsigned short op1 : 4;
	unsigned short op2 : 4;
	unsigned short op3 : 4;

	void reset(){
		opcode = 0;
		op1 = op2 = op3 = 0;
	}

	Instruction(){
		opcode = 0;
		op1 = op2 = op3 = 0;
	}

	Instruction(int op, int o1, int o2, int o3){
		opcode = op;
		op1 = o1;
		op2 = o2;
		op3 = o3;
	}

} Instruction;

/*
	Structure to hold 10-bit memory addresses
*/
typedef struct MemoryAddress{
	unsigned short isData : 1;
	unsigned short address : 9;

	void reset(){
		isData = 0;
		address = 0;
	}

	MemoryAddress(){
		isData  = 0;
		address = 0;
	}

	MemoryAddress(int iD, int add){
		isData = iD;
		address = add;
	} 

} MemoryAddress;

// Typedef-ed
typedef unsigned char  DataElement;
typedef signed short Data;

/*
	Simulation Processor Class
*/
class Processor{
	Instruction *IR;
	MemoryAddress PC;

	/*
		512 byte data memory
	*/
	DataElement DataMemory[MAX_DMEM_SIZE];

	/*
		512 instruction memory => 256 instructions
	*/
	Instruction InstrMemory[MAX_IMEM_SIZE];

	/*
		Register file of 16-bit regs
	*/
	Data RegisterFile[MAX_REGS];

public:

	/* Initialize data members */
	Processor(){
		IR = InstrMemory;
		PC.reset();

		memset(DataMemory, 0, sizeof(char) * MAX_DMEM_SIZE);
		for(int i=0; i<MAX_IMEM_SIZE; i++)
			InstrMemory[i].reset();
		
		for(int i=0; i<MAX_REGS; i++)
			RegisterFile[i] = 0;
	}

	/* Loads Instruction to InstrMemory */
	int iload(const MemoryAddress &addr, const Instruction &i){
		// Maligned instruction address
		if(addr.address % 2)
			return -1;
		
		InstrMemory[addr.address >> 1] = i;  
		return 0;
	}

	/* Loads Data to InstrMemory */
	void dload(const MemoryAddress &addr, const Data &val){
		DataMemory[addr.address] = (val >> 8) & 0xff;
		DataMemory[addr.address + 1] = val & 0xff;
	}

	/* Loads initial values of registers */
	void reg_init(const DataElement regAddr, const Data val){
		if(regAddr < 0 || regAddr > 15)		return;
		RegisterFile[regAddr] = val;
	}

	void start(){
		do{
			// Fetch & Increment PC
			IR = &InstrMemory[PC.address/2];
			PC.address += 2; 

			// Decode & then Execute & then Store Results
			switch(IR->opcode){
				case ADD: 	RegisterFile[IR->op1] = RegisterFile[IR->op2] + RegisterFile[IR->op3];
						  	break;
				
				case ADDI:	RegisterFile[IR->op1] = RegisterFile[IR->op2] + TWOCOMP(IR->op3);
							break;
				
				case SUB:	RegisterFile[IR->op1] = RegisterFile[IR->op2] - RegisterFile[IR->op3];
						  	break;
				
				case SUBI:	RegisterFile[IR->op1] = RegisterFile[IR->op2] - TWOCOMP(IR->op3);
						  	break;
				
				case MUL:	RegisterFile[IR->op1] = RegisterFile[IR->op2] * RegisterFile[IR->op3];
						  	break;
				
				case MULI:	RegisterFile[IR->op1] = RegisterFile[IR->op2] * TWOCOMP(IR->op3);
						  	break;

				case LD:	{
								unsigned short loc = RegisterFile[IR->op2] + RegisterFile[IR->op3] - 512;
								RegisterFile[IR->op1] = ((unsigned short) DataMemory[loc] << 8) | DataMemory[loc+1];
							}
						  	break;

				case SD:	{
								unsigned short loc = RegisterFile[IR->op1] + RegisterFile[IR->op2] - 512;
								DataMemory[loc] = ( ((unsigned short)RegisterFile[IR->op3]) >> 8 );
								DataMemory[loc+1] = (RegisterFile[IR->op3]);
						  	}
						  	break;

				case JMP:	PC.address += TWOCOMP2((((unsigned short) (IR->op2)) << 4) | IR->op3) - 2;
							break;

				case BEQZ:	if(RegisterFile[IR->op1] == 0)
								PC.address += TWOCOMP2((((unsigned short) (IR->op2)) << 4) | IR->op3) - 2;
							break;
			
				case HLT: break; // do nothing, loop will terminate

				default: break; // silently skip instruction
			}
		} while(IR->opcode != HLT);
	}

	/* Save Memory Dump to outfile */
	void generateDump(char *outfile){
		fstream fil(outfile, ios::out);

		for(int i=0; i<MAX_IMEM_SIZE; i++){
			fil << ((i<5)?"000":((i<50)?"00":"0"))  << (i << 1) << " : " 
				<< bitset<4>(InstrMemory[i].opcode) << " "
				<< bitset<4>(InstrMemory[i].op1) << endl;
			fil << ((i<5)?"000":((i<50)?"00":"0"))  << (i << 1) + 1 << " : "
				<< bitset<4>(InstrMemory[i].op2) << " "
				<< bitset<4>(InstrMemory[i].op3) << endl;
		}
	
		for(int i=0; i<MAX_DMEM_SIZE; i+=1){
			fil << ((512+i < 1000)?"0":"") << 512+i << " : "
				<< bitset<4>(DataMemory[i] >> 4) << " " 
				<< bitset<4>(DataMemory[i]) << endl;
		}
	}

} SimulationProcessor;

/*
	Compile Binary from Assembly 
	and Load to SimulationProcessor.
*/
void InitializeProcessor(char *infile){
	/* Compile Binary and load to Simulation Processor */
	FILE *fp = fopen(infile, "r");
	int val1, val2;
	
	// Register Initialization
	for(int i=0; i<16; i++){
		fscanf(fp, "%d", &val1);
		SimulationProcessor.reg_init(i, val1);
	}

	// Memory Initialization
	fscanf(fp, "%d %d", &val1, &val2);
	while(val1 != -1){
		SimulationProcessor.dload(MemoryAddress(1, val1-512), val2);
		fscanf(fp, "%d %d", &val1, &val2);
	}

	// Instructions Load
	int opcode, op1, op2, op3;
	int address = 0;

	fscanf(fp, "%d %d %d %d", &opcode, &op1, &op2, &op3);
	while(opcode != -1){
		SimulationProcessor.iload(MemoryAddress(0, address), Instruction(opcode, op1, op2, op3));
		address += 2;
		fscanf(fp, "%d %d %d %d", &opcode, &op1, &op2, &op3);
	}
}

int main(int argc, char *argv[]){
	if(argc != 3){
		printf("Usage: ./ProcessorSimulator.o <infile> <outfile>\n");
		return 1;
	}

	// Compile Assembly to binary and load
	InitializeProcessor(argv[1]);

	// Simulate Processor on binary
	SimulationProcessor.start();
	
	// Dump memory to file
	SimulationProcessor.generateDump(argv[2]);	

	return 0;
}