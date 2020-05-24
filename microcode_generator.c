#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define EEPROM_SIZE 65536 //number of byte in 16bit byte addressable EEPROM

// any address is made up of opcode(bit 15-12), step(bit 11-8) and condition code (bit 7-0)
// any data are 1 byte in size,  output 8 control bits.

/*opcode:
shift left 8 bit because opcode is from bit 12 to 15
shift left 4 bit because opcode is because they are the upper 4 bit. Lower 4 bit are reserved for config bit
*/
//arithmetic
#define NOP 0x0000
#define STP 0x0800
#define RSF 0x1000
#define ADD 0x2000 
#define ADDI 0x2800
#define SUB 0x3000 
#define SUBI 0x3800
#define NOT 0x4000
#define XOR 0x5000
#define XORI 0x5800
#define ORR 0x6000
#define ORRI 0x6800
#define AND 0x7000
#define ANDI 0x7800

//flow control
#define JMP 0x8000 //1000
#define JMPB 0x8800
#define JMP_C 0x0100
#define JMP_N 0x0200
#define JMP_Z 0x0400
#define JZ JMP+JMP_Z
#define JN JMP+JMP_N
#define JC JMP+JMP_C
#define JZN JMP + JMP_Z + JMP_N
#define JZC JMP + JMP_Z + JMP_C
#define JNC JMP + JMP_N + JMP_C
#define JZNC JMP + JMP_Z + JMP_N + JMP_C

//data movement (load store)
#define LDA 0xA000 //1010
#define MOVA 0xA400
#define LDAB 0xA800 //not done
#define LDB 0xB000 //1011
#define MOVB 0xB400
#define LDBB 0xB800 //not done
#define LDC 0xC000 //1100
#define MOVC 0xC400
#define LDCB 0xC800 //not done
#define STC 0xD000 //1101
#define STCB 0xD800 //not done


#define MMIO 0x0100 // memory mapped IO
#define BYTE_ADDRESSING_MODE 0x0800


//condition code & interrupt
#define I 0x1
#define C 0x2
#define N 0x4
#define Z 0x8
#define MEM_READY 0x4
#define FAST_FETCH 0x80	//mechanism not yet implemented, but using this, 
						//the fetching will automatively detect if the fetch stage requires loading the higher address of pc;
						//for example, if current pc is 0x1000, there is no need to load the 0x10 part into the MAR1 again when PC increment


//step
#define S0 0x00
#define S1 0x10
#define S2 0x20
#define S3 0x30
#define S4 0x40
#define S5 0x50
#define S6 0x60
#define S7 0x70
#define S8 0x80
#define S9 0x90
#define S10 0xA0
#define S11 0xB0
#define S12 0xC0
#define S13 0xD0
#define S14 0xE0
#define S15 0xF0

//next step
#define NS0 0x000000
#define NS1 0x100000
#define NS2 0x200000
#define NS3 0x300000
#define NS4 0x400000
#define NS5 0x500000
#define NS6 0x600000
#define NS7 0x700000
#define NS8 0x800000
#define NS9 0x900000
#define NS10 0xA00000
#define NS11 0xB00000
#define NS12 0xC00000
#define NS13 0xD00000
#define NS14 0xE00000
#define NS15 0xF00000
#define NSJUMP 0xF00000
//control signal for left chip(so shift to the left 16)
#define STEP3 0x800000
#define STEP2 0x400000
#define STEP1 0x200000
#define STEP0 0x100000

#define LOAD_MAR0 0x080000//EEPROM
#define LOAD_MAR1 0x040000//EEPROM
#define GATE_MEM 0x020000//EEPROM
#define WRITE	  0x010000//EEPROM


//control signal for middle chip
#define LOAD_IR 0x8000//Control Unit

#define LOAD_A 0x4000//ALU
#define LOAD_B 0x2000//ALU
#define LOAD_C 0x1000//ALU
#define GATE_C 0x0800//ALU
#define ALU2 0x0400//ALU 
#define ALU1 0x0200//ALU
#define ALU0 0x0100//ALU

#define ALU_NOP 0x000
#define ALU_SHF 0x100
#define ALU_ADD 0x200
#define ALU_SUB 0x300
#define ALU_NOT 0x400
#define ALU_XOR 0x500
#define ALU_ORR 0x600
#define ALU_AND 0x700
#define ADD_INC 0x04 //control signal overload with load_io, used for increment add


//control signal for right chip 
#define LOAD_PC0 0x80//Program Counter
#define LOAD_PC1 0x40//Program Counter
#define GATE_PC0 0x20//Program Counter
#define GATE_PC1 0x10//Program Counter
#define INCR_PC  0x08//Program Counter

#define LOAD_IO 0x04//IO
#define GATE_IO 0x02//IO
#define WRITE_IO 0x01//IO

struct micro_code{
	int32_t input;  //16 bit should be enough, but i want negative to indicate if a array reach the end
	int32_t output; //because 32 bits are enough to hold 3*8=24 control signal
};

struct micro_code micro_code_list[]= {
    
    //NOP
	{NOP+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{NOP+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{NOP+S2, NS3 + GATE_MEM + LOAD_IR},
	{NOP+S3, NS0},

	{STP+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{STP+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{STP+S2, NS3 + GATE_MEM + LOAD_IR},
	{STP+S3, NS3},

    //SHF
	{RSF+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{RSF+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{RSF+S2, NS3 + GATE_MEM + LOAD_IR},
	{RSF+S3, NS0 + ALU_SHF + LOAD_C},

    //ADD
    {ADD+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{ADD+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{ADD+S2, NS3 + GATE_MEM + LOAD_IR},
	{ADD+S3, NS0 + ALU_ADD + LOAD_C},
    
	{ADDI+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{ADDI+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{ADDI+S2, NS3 + GATE_MEM + LOAD_IR},
	{ADDI+S3, NS4 + LOAD_MAR0 + GATE_PC0 },
	{ADDI+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{ADDI+S5, NS6 + GATE_MEM + LOAD_B},
	{ADDI+S6, NS0 + ALU_ADD + LOAD_C},


    //SUB
	{SUB+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{SUB+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{SUB+S2, NS3 + GATE_MEM + LOAD_IR},
	{SUB+S3, NS0 + ALU_SUB + LOAD_C + ADD_INC}, //increment 1

	{SUBI+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{SUBI+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{SUBI+S2, NS3 + GATE_MEM + LOAD_IR},
	{SUBI+S3, NS4 + LOAD_MAR0 + GATE_PC0 },
	{SUBI+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{SUBI+S5, NS6 + GATE_MEM + LOAD_B},
	{SUBI+S6, NS0 + ALU_SUB + LOAD_C + ADD_INC}, //increment 1


    //NOT
	{NOT+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{NOT+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{NOT+S2, NS3 + GATE_MEM + LOAD_IR},
	{NOT+S3, NS0 + ALU_NOT + LOAD_C},

    //XOR
	{XOR+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{XOR+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{XOR+S2, NS3 + GATE_MEM + LOAD_IR},
	{XOR+S3, NS0 + ALU_XOR + LOAD_C},

	{XORI+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{XORI+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{XORI+S2, NS3 + GATE_MEM + LOAD_IR},
	{XORI+S3, NS4 + LOAD_MAR0 + GATE_PC0 },
	{XORI+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{XORI+S5, NS6 + GATE_MEM + LOAD_B},
	{XORI+S6, NS0 + ALU_XOR + LOAD_C},	
	
    //OR
	{ORR+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{ORR+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{ORR+S2, NS3 + GATE_MEM + LOAD_IR},
	{ORR+S3, NS0 + ALU_ORR + LOAD_C},

	{ORRI+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{ORRI+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{ORRI+S2, NS3 + GATE_MEM + LOAD_IR},
	{ORRI+S3, NS4 + LOAD_MAR0 + GATE_PC0 },
	{ORRI+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{ORRI+S5, NS6 + GATE_MEM + LOAD_B},
	{ORRI+S6, NS0 + ALU_ORR + LOAD_C},			

	//AND
	{AND+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{AND+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{AND+S2, NS3 + GATE_MEM + LOAD_IR},
	{AND+S3, NS0 + ALU_AND + LOAD_C},

	{ANDI+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{ANDI+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{ANDI+S2, NS3 + GATE_MEM + LOAD_IR},
	{ANDI+S3, NS4 + LOAD_MAR0 + GATE_PC0 },
	{ANDI+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC},
	{ANDI+S5, NS6 + GATE_MEM + LOAD_B},
	{ANDI+S6, NS0 + ALU_ORR + LOAD_C},		

	//JMP
	{JMP+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{JMP+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{JMP+S2, NS3 + GATE_MEM + LOAD_IR},
	{JMP+S3, NS4 + LOAD_MAR0 + GATE_PC0},
	{JMP+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{JMP+S5, NS6 + LOAD_C + GATE_MEM },
	{JMP+S6, NS7 + LOAD_MAR0 + GATE_PC0 },
	{JMP+S7, NS8 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{JMP+S8, NS9 + LOAD_PC0 + GATE_MEM },
	{JMP+S9, NS0 + LOAD_PC1 + GATE_C},

	//DEC decrement
	{DEC+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{DEC+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{DEC+S2, NS3 + GATE_MEM + LOAD_IR}, 
	//step1, get value from 0xFFFF, add 0xFF, store it back
	{DEC+S3, NS4 + LOAD_A},
	{DEC+S4, NS5 + LOAD_B + ALU_NOT + GATE_ALU + LOAD_MAR1+ LOAD_MAR0}, //put 0xFF into B put 0xFFFF into the MAR
	{DEC+S5, NS6 + GATE_MEM + LOAD_A},//load content into A
	{DEC+S6, NS7 + LOAD_C + GATE_ALU + ALU_ADD + GATE_MEM + WRITE},//add A with B(0xFF, which is -1), write back the result to 0xFFFF, save carry to reg C
	//step2, get value from 0xFFFE, add 0xFF, add carry, store it back 
	{DEC+S7, NS8 + LOAD_A},
	{DEC+S8, NS9 + LOAD_A + LOAD_B + ALU_NOT + GATE_ALU}, //both A and B now have 0xFF
	{DEC+S9, NS10 + LOAD_MAR0 + ALU_ADD + GATE_ALU}, // now the result should be 0xFE(0xFF+0xFF), 0xFFFE has loaded into MAR
	{DEC+S10, NS11 + LOAD_A + GATE_MEM},
	{DEC+S11, NS0 + GATE_ALU + ALU_ADD + GATE_MEM + WRITE},//save second byte
	{DEC+S11+C, NS0 + GATE_ALU + ALU_ADD + ADD_INC + GATE_MEM + WRITE},//increment 1 as well if there is carry from step 1


	//INC increment
	{INC+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{INC+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{INC+S2, NS3 + GATE_MEM + LOAD_IR}, 
	//step1, get value from 0xFFFF, add 0xFF, store it back
	{INC+S3, NS4 + LOAD_A + LOAD_B},//load 0 into A and B
	{INC+S4, NS5 + ALU_NOT + GATE_ALU + LOAD_MAR1+ LOAD_MAR0}, //put 0xFFFF into the MAR
	{INC+S5, NS6 + GATE_MEM + LOAD_A},//load content of 0xFFFF into A
	{INC+S6, NS7 + LOAD_C + GATE_ALU + ALU_ADD + ADD_INC + GATE_MEM + WRITE},//add A by 1, write back the result to 0xFFFF, save carry to reg C
	//step2, get value from 0xFFFE, add 0xFF, add carry, store it back 
	{INC+S7, NS8 + LOAD_A},
	{INC+S8, NS9 + LOAD_A + LOAD_B + ALU_NOT + GATE_ALU}, //both A and B now have 0xFF
	{INC+S9, NS10 + LOAD_MAR0 + ALU_ADD + GATE_ALU}, // now the result should be 0xFE(0xFF+0xFF), 0xFFFE has loaded into MAR
	{INC+S10, NS11 + LOAD_A + GATE_MEM},
	{INC+S11, NS12 + LOAD_B} //load 0 into B
	{INC+S12, NS0 + GATE_ALU + ALU_ADD + GATE_MEM + WRITE},//save second byte
	{INC+S12+C, NS0 + GATE_ALU + ALU_ADD + ADD_INC + GATE_MEM + WRITE},//increment 1 as well if there is carry from step 1


	//PUSH
	{PSH+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{PSH+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{PSH+S2, NS3 + GATE_MEM + LOAD_IR}, 
	//step1, get value from 0xFFFF, decrement it, store it back
	{PSH+S3, NS4 + LOAD_A},
	{PSH+S4, NS5 + LOAD_B + ALU_NOT + GATE_ALU + LOAD_MAR1+ LOAD_MAR0}, //put 0xFFFF into the MAR
	{PSH+S5, NS6 + GATE_MEM + LOAD_A},//A now points to the stack
	{PSH+S6, NS7 + GATE_ALU + ALU_ADD + GATE_MEM + WRITE}, //A-1
	//step2,
	{PSH+S7, NS8 + LOAD_B},
	{PSH+S8, NS9 + LOAD_MAR0 + ALU_ADD + GATE_ALU}, 
	{PSH+S9, NS10 + GATE_C + GATE_MEM + WRITE},
	{PSH+S10, NS11 + LOAD_A + GATE_MEM},
	{PSH+S11, NS0 + GATE_ALU + ALU_ADD + GATE_MEM + WRITE},//save second byte
	
	//POP
	{POP+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{POP+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{POP+S2, NS3 + GATE_MEM + LOAD_IR}, 
	//step1, get value from 0xFFFF, read from c, then increment the value
	{POP+S3, NS4 + LOAD_A, LOAD_B}, //A=0, B=0
	{POP+S4, NS5 + ALU_NOT + GATE_ALU + LOAD_MAR1+ LOAD_MAR0}, //put 0xFFFF into the MAR
	{POP+S5, NS6 + GATE_MEM + LOAD_C + LOAD_A},//C and A now points to the stack
	{PSH+S6, NS7 + GATE_ALU + ALU_ADD + ADD_INC + GATE_MEM + WRITE}, //A+1, store it back to 0xFFFF
	//step2,
	{POP+S8, NS9 + LOAD_MAR0 + GATE_C}, 
	{POP+S9, NS10 + GATE_C + GATE_MEM + WRITE},
	{POP+S10, NS11 + LOAD_A + GATE_MEM},
	{POP+S11, NS0 + GATE_ALU + ALU_ADD + GATE_MEM + WRITE},//save second byte

	/*
	//RET
	{RET+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{RET+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{RET+S2, NS3 + GATE_MEM + LOAD_IR}, 
	{RET+S3, NS4 + LOAD_MAR0 + GATE_PC0 },
	{RET+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{RET+S5, NS6 + LOAD_C + GATE_MEM },
	{RET+S6, NS7 + LOAD_MAR0 + GATE_PC0 },
	{RET+S7, NS8 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{RET+S8, NS9 + LOAD_MAR0 + GATE_MEM + LOAD_A},
	{RET+S9, NS10 + LOAD_MAR1 + GATE_C},
	{RET+S10, NS11 + LOAD_PC1 + GATE_MEM},
	{RET+S11, NS12 + LOAD_B}
	{RET+S12, NS13 + LOAD_MAR0 + GATE_ALU + ALU_ADD}
	{RET+S13, NS0 + LOAD_PC0, GATE_MEM}
	*/

	//LDA
	{LDA+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{LDA+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDA+S2, NS3 + GATE_MEM + LOAD_IR}, 
	{LDA+S3, NS4 + LOAD_MAR0 + GATE_PC0 },
	{LDA+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDA+S5, NS6 + LOAD_C + GATE_MEM },
	{LDA+S6, NS7 + LOAD_MAR0 + GATE_PC0 },
	{LDA+S7, NS8 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDA+S8, NS9 + LOAD_MAR0 + GATE_MEM },
	{LDA+S9, NS10 + LOAD_MAR1 + GATE_C},
	{LDA+S10, NS0 + LOAD_A + GATE_MEM},

	{LDA+S0+MMIO, NS1 + LOAD_MAR0 + GATE_PC0 },
	{LDA+S1+MMIO, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDA+S2+MMIO, NS3 + GATE_MEM + LOAD_IR},
	{LDA+S3+MMIO, NS4 + INCR_PC }, 
	{LDA+S4+MMIO, NS4 + LOAD_MAR0 + GATE_PC0 },
	{LDA+S5+MMIO, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDA+S6+MMIO, NS6 + LOAD_IO + GATE_MEM },
	{LDA+S7+MMIO, NS0 + LOAD_A + GATE_IO },

	{MOVA+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{MOVA+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{MOVA+S2, NS3 + GATE_MEM + LOAD_IR}, 
	{MOVA+S3, NS4 + LOAD_MAR0 + GATE_PC0 },
	{MOVA+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{MOVA+S5, NS0 + LOAD_A + GATE_MEM },

	{LDA+S0+BYTE_ADDRESSING_MODE, NS1 + LOAD_MAR0 + GATE_PC0 },
	{LDA+S1+BYTE_ADDRESSING_MODE, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDA+S2+BYTE_ADDRESSING_MODE, NS3 + GATE_MEM + LOAD_IR}, 
	{LDA+S3+BYTE_ADDRESSING_MODE, NS4 + LOAD_MAR0 + GATE_PC0 },
	{LDA+S4+BYTE_ADDRESSING_MODE, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDA+S5+BYTE_ADDRESSING_MODE, NS6 + LOAD_MAR0 + GATE_MEM },
	{LDA+S10+BYTE_ADDRESSING_MODE, NS0 + LOAD_A + GATE_MEM},

	/* if fast fetch is enable
	{LDA+S0+FAST_FETCH, NS1 + LOAD_MAR0 + GATE_PC0 + INCR_PC}, 
	{LDA+S1+FAST_FETCH, NS2 + GATE_MEM + LOAD_IR}, 
	{LDA+S2+FAST_FETCH, NS3 + LOAD_MAR0 + GATE_PC0 + INCR_PC},
	{LDA+S3+FAST_FETCH, NS5 + LOAD_C + GATE_MEM },
	{LDA+S4+FAST_FETCH, NS7 + LOAD_MAR0 + GATE_PC0 + INCR_PC},
	{LDA+S5+FAST_FETCH, NS8 + LOAD_MAR0 + GATE_MEM },
	{LDA+S6+FAST_FETCH, NS9 + LOAD_MAR1 + GATE_C},
	{LDA+S7+FAST_FETCH, NS0 + LOAD_A + GATE_MEM},


	{LDA+S0+FAST_FETCH, NS1 + LOAD_MAR0 + GATE_PC0 + INCR_PC}, 
	{LDA+S1+FAST_FETCH, NS2 + GATE_MEM + LOAD_IR}, 
	{LDA+S2+FAST_FETCH, NS3 + LOAD_MAR0 + GATE_PC0 + INCR_PC},
	{LDA+S4+FAST_FETCH, NS5 + LOAD_C + GATE_MEM },
	{LDA+S6+FAST_FETCH, NS7 + LOAD_MAR0 + GATE_PC0 + INCR_PC},
	{LDA+S7+FAST_FETCH, NS8 + LOAD_MAR0 + GATE_MEM },
	{LDA+S8+FAST_FETCH, NS9 + LOAD_MAR1 + GATE_C},
	{LDA+S9+FAST_FETCH, NS0 + LOAD_A + GATE_MEM},
	*/

	//LDB
	{LDB+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{LDB+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDB+S2, NS3 + GATE_MEM + LOAD_IR}, 
	{LDB+S3, NS4 + LOAD_MAR0 + GATE_PC0 },
	{LDB+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDB+S5, NS6 + LOAD_C + GATE_MEM },
	{LDB+S6, NS7 + LOAD_MAR0 + GATE_PC0 },
	{LDB+S7, NS8 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDB+S8, NS9 + LOAD_MAR0 + GATE_MEM },
	{LDB+S9, NS10 + LOAD_MAR1 + GATE_C},
	{LDB+S10, NS0 + LOAD_B + GATE_MEM},

	{LDB+S0+MMIO, NS1 + LOAD_MAR0 + GATE_PC0 },
	{LDB+S1+MMIO, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDB+S2+MMIO, NS3 + GATE_MEM + LOAD_IR},
	{LDB+S3+MMIO, NS4 + INCR_PC }, 
	{LDB+S4+MMIO, NS4 + LOAD_MAR0 + GATE_PC0 },
	{LDB+S5+MMIO, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDB+S6+MMIO, NS6 + LOAD_IO + GATE_MEM },
	{LDB+S7+MMIO, NS0 + LOAD_B + GATE_IO },

	{MOVB+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{MOVB+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{MOVB+S2, NS3 + GATE_MEM + LOAD_IR}, 
	{MOVB+S3, NS4 + LOAD_MAR0 + GATE_PC0 },
	{MOVB+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{MOVB+S5, NS0 + LOAD_B + GATE_MEM },

	{LDB+S0+BYTE_ADDRESSING_MODE, NS1 + LOAD_MAR0 + GATE_PC0 },
	{LDB+S1+BYTE_ADDRESSING_MODE, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDB+S2+BYTE_ADDRESSING_MODE, NS3 + GATE_MEM + LOAD_IR}, 
	{LDB+S3+BYTE_ADDRESSING_MODE, NS4 + LOAD_MAR0 + GATE_PC0 },
	{LDB+S4+BYTE_ADDRESSING_MODE, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDB+S5+BYTE_ADDRESSING_MODE, NS6 + LOAD_MAR0 + GATE_MEM },
	{LDB+S10+BYTE_ADDRESSING_MODE, NS0 + LOAD_B + GATE_MEM},

	//LDC
	{LDC+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{LDC+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDC+S2, NS3 + GATE_MEM + LOAD_IR}, 
	{LDC+S3, NS4 + LOAD_MAR0 + GATE_PC0 },
	{LDC+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDC+S5, NS6 + LOAD_C + GATE_MEM },
	{LDC+S6, NS7 + LOAD_MAR0 + GATE_PC0 },
	{LDC+S7, NS8 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDC+S8, NS9 + LOAD_MAR0 + GATE_MEM },
	{LDC+S9, NS10 + LOAD_MAR1 + GATE_C},
	{LDC+S10, NS0 + LOAD_C + GATE_MEM},

	{LDC+S0+MMIO, NS1 + LOAD_MAR0 + GATE_PC0 },
	{LDC+S1+MMIO, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDC+S2+MMIO, NS3 + GATE_MEM + LOAD_IR},
	{LDC+S3+MMIO, NS4 + INCR_PC }, 
	{LDC+S4+MMIO, NS4 + LOAD_MAR0 + GATE_PC0 },
	{LDC+S5+MMIO, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDC+S6+MMIO, NS6 + LOAD_IO + GATE_MEM },
	{LDC+S7+MMIO, NS0 + LOAD_C + GATE_IO },

	{MOVC+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{MOVC+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{MOVC+S2, NS3 + GATE_MEM + LOAD_IR}, 
	{MOVC+S3, NS4 + LOAD_MAR0 + GATE_PC0 },
	{MOVC+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{MOVC+S5, NS0 + LOAD_C + GATE_MEM },

	{LDC+S0+BYTE_ADDRESSING_MODE, NS1 + LOAD_MAR0 + GATE_PC0 },
	{LDC+S1+BYTE_ADDRESSING_MODE, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDC+S2+BYTE_ADDRESSING_MODE, NS3 + GATE_MEM + LOAD_IR}, 
	{LDC+S3+BYTE_ADDRESSING_MODE, NS4 + LOAD_MAR0 + GATE_PC0 },
	{LDC+S4+BYTE_ADDRESSING_MODE, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{LDC+S5+BYTE_ADDRESSING_MODE, NS6 + LOAD_MAR0 + GATE_MEM },
	{LDC+S10+BYTE_ADDRESSING_MODE, NS0 + LOAD_C + GATE_MEM},

	//STC
	{STC+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{STC+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{STC+S2, NS3 + GATE_MEM + LOAD_IR}, 
	{STC+S3, NS4 + LOAD_MAR0 + GATE_PC0 },
	{STC+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{STC+S5, NS6 + LOAD_A + GATE_MEM },
	{STC+S6, NS7 + LOAD_MAR0 + GATE_PC0 },
	{STC+S7, NS8 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{STC+S8, NS9 + LOAD_MAR0 + GATE_MEM },
	{STC+S9, NS10 + LOAD_B },
	{STC+S10, NS11 + LOAD_MAR1 + ALU_XOR},
	{STC+S11, NS0 + GATE_C + GATE_MEM + WRITE},

	{STC+S0+MMIO, NS1 + LOAD_MAR0 + GATE_PC0 },
	{STC+S1+MMIO, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{STC+S2+MMIO, NS3 + GATE_MEM + LOAD_IR},
	{STC+S3+MMIO, NS4 + INCR_PC }, 
	{STC+S4+MMIO, NS5 + LOAD_MAR0 + GATE_PC0 },
	{STC+S5+MMIO, NS6 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{STC+S6+MMIO, NS7 + LOAD_IO + GATE_MEM },
	{STC+S7+MMIO, NS0 + GATE_C + WRITE_IO },

	{STC+S0+BYTE_ADDRESSING_MODE, NS1 + LOAD_MAR0 + GATE_PC0 },
	{STC+S1+BYTE_ADDRESSING_MODE, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{STC+S2+BYTE_ADDRESSING_MODE, NS3 + GATE_MEM + LOAD_IR}, 
	{STC+S3+BYTE_ADDRESSING_MODE, NS4 + LOAD_MAR0 + GATE_PC0 },
	{STC+S4+BYTE_ADDRESSING_MODE, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{STC+S5+BYTE_ADDRESSING_MODE, NS6 + LOAD_MAR0 + GATE_MEM },
	{STC+S10+BYTE_ADDRESSING_MODE, NS0 + LOAD_B + GATE_MEM},	


	//
	{-1,0}
};

struct micro_code jump_template[] = {
	{JMP+S0, NS1 + LOAD_MAR0 + GATE_PC0 },
	{JMP+S1, NS2 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{JMP+S2, NS3 + GATE_MEM + LOAD_IR},
	{JMP+S4, NS5 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{JMP+S5, NS6 + LOAD_C + GATE_MEM },
	{JMP+S6, NS7 + LOAD_MAR0 + GATE_PC0 },
	{JMP+S7, NS8 + LOAD_MAR1 + GATE_PC1 + INCR_PC}, 
	{JMP+S8, NS9 + LOAD_PC0 + GATE_MEM },
	{JMP+S9, NS0 + LOAD_PC1 + GATE_C},
	{-1,0}
};


int main(){
	/* Variable to store user content */
    char buffer[EEPROM_SIZE];

    /* File pointer to hold reference to our file */
    FILE * fPtr;

    for(int CHIP_SELECTED =0 ; CHIP_SELECTED < 3 ; CHIP_SELECTED++)
    {
    	char file_name[20]="chip .bin";
    	file_name[4]=(char)CHIP_SELECTED+0x30;
	    fPtr = fopen(file_name, "w");
	    if(fPtr == NULL)
	    {
	        /* File not created hence exit */
	        printf("Unable to create file.\n");
	        exit(EXIT_FAILURE);
	    }

	    printf("generating binary file for chip %d\n\n", CHIP_SELECTED );

	    int32_t mask_8bit=0xFF;
	    

	    int i = 0;
	    
	    //unconditional
	    while(micro_code_list[i].input != -1)
	    {
	    	for(int j = 0; j < 16; j++) //j <16 because there are 4 bit of conditional code/interrupt bit
	    	{
	    		fseek(fPtr, micro_code_list[i].input+j, SEEK_SET);
	    		int status = fputc(  (micro_code_list[i].output>>(CHIP_SELECTED*8)) &  mask_8bit  , fPtr );
	    		if(status==EOF)
	    			printf("oh no, the write failed!!!\n");
	    		printf("input: %x    output: %x\n",micro_code_list[i].input+j,  (uint8_t) (micro_code_list[i].output>>(CHIP_SELECTED*8)) &  mask_8bit );
	    	}
	    	i++;
	    }

	    //conditional
	    printf("generating micro code for conditional jump\n");
	    i=0;
		while(jump_template[i].input != -1)
		{
			for(int k = 1 ; k < 8; k++)
			{
				for(int j = 0; j < 16; j++) //j <16 because there are 4 bit of conditional code/interrupt bit
	    		{
	    			fseek(fPtr, jump_template[i].input+j+ (k<<8), SEEK_SET);
	    			int status = fputc(  (jump_template[i].output>>(CHIP_SELECTED*8)) &  mask_8bit  , fPtr );
	    			if(status==EOF)
	    				printf("oh no, the write failed!!!\n");
	    			printf("input: %x    output: %x\n",jump_template[i].input+j,  (uint8_t) (jump_template[i].output>>(CHIP_SELECTED*8)) &  mask_8bit );
	    		}	
			}
	    	i++;
		}

		// for step 3 which is the branching step
		for(int i = 1; i < 8 ; i++)
		{
			for(int j = 0; j < 8; j++)
			{
				fseek(fPtr, JMP + S3 + (i<<8) + (j<<1), SEEK_SET);
				int status;
				if( (i&j) != 0 ){
					status = fputc(  ((NS4 + LOAD_MAR0 + GATE_PC0)>>(CHIP_SELECTED*8)) &  mask_8bit  , fPtr );
				}
				else{
					status = fputc(  (NS0>>(CHIP_SELECTED*8)) &  mask_8bit  , fPtr );
				}
				if(status==EOF)
	    			printf("oh no, the write failed!!!\n");
			}
		}
		fclose(fPtr);

    }




	printf("micro code finished generating\n");
	return 0;
}