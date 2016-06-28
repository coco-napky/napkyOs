int getProcesses(int buffer);
void kill(char* id);
void scheduleProcess();
void terminate();
void initializeProgram(int segment);
void initProcessTable();
int getSegment(int blocking);
void handleInterrupt21(int ax, char* bx, int cx, int dx);
void print(char*);
void println(char*);
void readConsole(char[]);
int readFile(char* filename, char buffer[13312]);
int deleteFile(char* filename);
void readSector(char[], int sector);
void executeProgram(char* name, int blocking);
void writeFile(char* filename, char buffer[13312], int numberOfSectors);
void setCharColor(int row, int column);
void setColorLn(int row);
void setColor(char*);
int atoi(char* number);
void clearScreen();
void getFreeSectors(int sectors[512]);
/*FONT COLORS*/
#define FT_BLUE     0X01
#define FT_GREEN    0X02
#define FT_TEAL     0X03
#define FT_RED      0X04
#define FT_PURPLE   0X05
#define FT_ORANGE   0X06
#define FT_LT_GRAY  0x07
#define FT_GRAY     0x08
#define FT_LT_BLUE  0X08
#define FT_LT_GREEN 0X0A
#define FT_LT_TEAL  0X0B
#define FT_LT_RED   0X0C
#define FT_PINK     0X0D
#define FT_YELLOW   0X0E
#define FT_WHITE    0x0F

#define VIDEO_MEM_COLOR_OFFSET 0x8001
#define COLUMNS 80 /*Number of columns per row*/
#define ROWS 25    /*Number of rows on the screen 17*/
#define BYTES_PER_ROW 160 /*Bytes on each row on the screen*/
#define BYTES_PER_CHAR 2  /*1 Byte for character, 1 Byte for color*/
int color_background = 0x00;
int color_font       = 0x07;
int color;

struct PCB {
	unsigned int status;
	unsigned int sp;
	unsigned int segment;
	struct PCB* waiter;
	struct PCB* waitee;
	int waiter_id;
	int waitee_id;
};

struct Regs {
	unsigned int es;
	unsigned int ds;
	unsigned int ax;
	unsigned int bp;
	unsigned int di;
	unsigned int si;
	unsigned int dx;
	unsigned int cx;
	unsigned int bx;
	unsigned int ip;
	unsigned int cs;
	unsigned int flags;
};
struct PCB process_queu[8];
struct PCB* currentProcess;
int counter  = 0;
int main(){
	makeInterrupt21();
	initProcessTable();
	executeProgram("shell", 0);
	irqInstallHandler();
	setTimerPhase(100);
	while(1);
	return 0;
}

void kill(char* id){
	int nid;
	struct PCB* temp;
	struct PCB* temp2;
	nid = atoi(id);
	if(nid > -1 && nid < 8 ){
		setKernelDataSegment();

		if(process_queu[nid].waitee != 0){
			process_queu[nid].waiter->waitee_id  =  process_queu[nid].waitee_id;
			process_queu[nid].waitee->waiter_id  =  process_queu[nid].waiter_id;

			process_queu[nid].waiter->waitee  = process_queu[nid].waitee;
			process_queu[nid].waitee->waiter  = process_queu[nid].waiter;



		}else{
			process_queu[nid].waiter->status = 1;
			if(process_queu[nid].waiter != 0){
				process_queu[nid].waiter->waitee    = 0;
				process_queu[nid].waiter->waitee_id = -1;
			}
		}

		process_queu[nid].waiter  = 0;
		process_queu[nid].waitee  = 0;
		process_queu[nid].waiter_id  = -1;
		process_queu[nid].waitee_id  = -1;
		process_queu[nid].status  = 4;
		process_queu[nid].sp      = 0xff00;
		#asm
		sti
		#endasm
		restoreDataSegment();
	}
}

void handleInterrupt21(int ax, char* bx, int cx, int dx){
	int i;
	if(ax == 0)
		if(cx == 1)
			println(bx);
		else
			print(bx);
	else if(ax == 1)
		readConsole(bx);
	else if(ax == 2)
		readSector(bx,cx);
	else if(ax == 3)
		readFile(bx,cx);
	else if(ax == 4)
		executeProgram(bx, cx);
	else if(ax == 5)
		terminate();
	else if(ax == 6)
		clearScreen();
	else if(ax == 7)
		writeSector(bx,cx);
	else if(ax == 8)
		deleteFile(bx);
	else if(ax == 9)
		writeFile(bx,cx,dx);
	else if(ax == 10)
		setColor(bx);
	else if(ax == 11)
		kill(bx);
	else if(ax == 12)
		ax = getProcesses(bx);
	else
		println("Invalid ax");
}

void executeProgram(char* name, int blocking){
	int byte_offset = 0;
	char buffer[13312];
	int i;
	int found;
	int segment;
	found = readFile(name,buffer);
	if(found){

		if(segment == -1){
			println("No available process");
			return;
		}
		setKernelDataSegment();
		segment = getSegment(blocking);
		restoreDataSegment();

		copyToSeg(segment,0,buffer,13312);
		initializeProgram(segment);
	}
}

int readFile(char* filename, char buffer[13312]){
	char directory[512];
	int entry_offset = 0;
	int byte_offset  = 0;
	int found = 0;
	int k;
	int i;
	char current;
	int sectors[26];
	int sector_num;

	for (i = 0; i < 26; ++i)
		sectors[i] = 0;

	readSector(directory,2);
	for(entry_offset = 0; entry_offset < 16; ++entry_offset){
		found = 0;
		if(directory[entry_offset*32] != 0){
			found = 1;
			for(k = 0; k<6; ++k){
				byte_offset = entry_offset*32 + k;
				current = directory[byte_offset];
				if(current != filename[k])
					found = 0;
			}
		}
		if(found){
			byte_offset = entry_offset*32;
			for (k = k; k < 32; ++k){
				if(directory[byte_offset + k] != 0)
					sectors[k-6] = directory[byte_offset + k];
			}

			for (i = 0; i < 26; ++i){
				sector_num = sectors[i];
				byte_offset = i*512;
				if(sector_num != 0)
					readSector(buffer + byte_offset,sector_num);
			}
			return 1;
		}
	}
	print("File not found :");
	println(filename);
	return 0;
}

void print(char* string){
	while(*string != 0){
		printChar(*string);
		string++;
	}
}

void println(char* string){
	print(string);
	print("\r");
	print("\n");
}

void readConsole(char buffer[80]){
    int i = 0;
    char read = 0;
    while((int)read != 0xd && i < 78){
        read = readChar();
        if(read == 0x8 && i > 0){
            printChar(read);
            printChar(' ');
            printChar(read);
            i--;
        }else if((int)read != 0x8){
            buffer[i] = read;
            printChar(buffer[i++]);
        }
    }
    buffer[i++] = 0xa;
    buffer[i] = 0;

    if(i > 2){
    	buffer[i - 1 ] = 0;
    	buffer[i - 2 ] = 0;
    }
    else{
    	buffer[0] = 0;
    	buffer[1] = 0;
    }
    printChar('\n');
}

void setCharColor(int row, int column){
    int row_offset    = row    * BYTES_PER_ROW;
    int column_offset = column * BYTES_PER_CHAR;
    int byte_offset   = column_offset + row_offset;

    int target_addr   = VIDEO_MEM_COLOR_OFFSET + byte_offset;
    color = color_background | color_font;
    putInMemory(0xB000, target_addr, color);
}

void setColorLn(int row){
    int c;
    for(c = 0; c < COLUMNS; c++)
        setCharColor(row,c);
}

void setColor(char* number){
    int c,i;
    i = atoi(number);
    color_background = 0x00;
	switch(i) {
		case 0  : color_font = FT_WHITE; break;
   		case 1  : color_font = FT_BLUE; break;
		case 2  : color_font = FT_GREEN; break;
   		case 3  : color_font = FT_TEAL; break;
		case 4  : color_font = FT_RED; break;
   		case 5  : color_font = FT_PURPLE; break;
		case 6  : color_font = FT_ORANGE; break;
   		case 7  : color_font = FT_LT_GRAY; break;
		case 8  : color_font = FT_GRAY; break;
   		case 9  : color_font = FT_LT_BLUE; break;
		case 10 : color_font = FT_LT_GREEN; break;
   		case 11 : color_font = FT_LT_TEAL; break;
		case 12 : color_font = FT_RED; break;
   		case 13 : color_font = FT_PINK; break;
		case 14 : color_font = FT_YELLOW; break;
	   	default : color_font = FT_LT_GRAY;break;
	}
    for(c = 0; c < ROWS; c++)
        setColorLn(c);
}

int atoi(char* number){
	int init = 0;
	int i,j,k = 0,l,base = 0;
	int r = 0;
	if(number[0] == 45)
		init = 1;

	for (i = init; number[i] != 0; ++i){
		if(!(number[i] > 47 && number[i] < 58 ))
			return -1;
	}
	k = i;
	for (j = init; j < k; ++j){
		base = number[j] - 48;
		for (l = 0; l < i - 1 - init; ++l)
			base *= 10;
		--i;
		r = r + base;
	}
	if(init) return r*-1;
	else return r;
}
void clearScreen(){int i;for ( i = 0; i < ROWS; ++i) printChar(0xa);}
int deleteFile(char* filename){
	char directory[512];
	char map[512];
	int entry_offset = 0;
	int byte_offset  = 0;
	int found = 0;
	int k;
	int i;
	char current;
	int sectors[26];
	int sector_num;

	for (i = 0; i < 26; ++i)
		sectors[i] = 0;

	readSector(directory,2);
	for(entry_offset = 0; entry_offset < 16; ++entry_offset){
		found = 0;
		if(directory[entry_offset*32] != 0){
			found = 1;
			for(k = 0; k<6; ++k){
				byte_offset = entry_offset*32 + k;
				current = directory[byte_offset];
				if(current != filename[k])
					found = 0;
			}
		}
		if(found){
			println("file found");
			byte_offset = entry_offset*32;

			//delete file name
			for (k = 0; k < 6; ++k)
				directory[byte_offset + k] = 0;

			//get sector numbers
			for (k = k; k < 32; ++k){
				if(directory[byte_offset + k] != 0)
					sectors[k-6] = directory[byte_offset + k];
			}

			readSector(map,1);
			//reset sectors from map
			for (i = 0; i < 26; ++i){
				sector_num = sectors[i];
				byte_offset = i*512;
				if(sector_num != 0)
					map[sector_num] = 0;
			}
			writeSector(map,1);
			writeSector(directory,2);
			return 1;
		}
	}
	print("File not found :");
	println(filename);
	return 0;
}

void getFreeSectors(int sectors[512]){
	char map[512];
	int i,j,k;
	int counter = 0;

	readSector(map,1);

	for (i = 0; i < 512; ++i){
		sectors[i] = 0;
		if(map[i] == 0){
			sectors[counter] = i;
			counter++;
		}
	}
}


void writeFile(char* filename, char buffer[13312], int numberOfSectors){
	char map[512];
	char directory[512];
	char batch[512];

	int i,j,k;
	int byte_offset;
	int found = 0;
	int sectors[512];
	int sector;
	if(numberOfSectors > 26){
		println("Too many sectors to write");
		return;
	}
	getFreeSectors(sectors);
	readSector(map,1);
	readSector(directory,2);


	for (i = 0; i < 16 && !found; ++i){
		byte_offset = i*32;
		found = directory[byte_offset] == 0;
	}

	if(found){

		for (i = 0; i < 6; ++i)
			directory[byte_offset + i] = filename[i];

		for (i = 6; (i-6) < numberOfSectors; ++i){
			j = i - 6;
			sector = sectors[j];
			map[sector] = 0xFF;
			directory[byte_offset + i] = sector;

			for (k = 0; k < 512; ++k)
				batch[k] = buffer[j*512+k];

			writeSector(batch, sector);
		}
		writeSector(map,1);
		writeSector(directory,2);
	}else
		println("disk is full");
}

int getSegment(int blocking){
	int i = -1;
	for (i = 0; i < 8; ++i){
		if(process_queu[i].status == 4){
			process_queu[i].status = 1;
			if(currentProcess != 0 && blocking){
				process_queu[i].waiter = currentProcess;
				process_queu[i].waiter_id = process_queu[i].waiter->segment/0x1000 - 2;
				currentProcess->status = 2;
				currentProcess->waitee = &process_queu[i];
				currentProcess->waitee_id = process_queu[i].segment/0x1000 - 2;
			}
			return process_queu[i].segment;
		}
	}
	return -1;
}


void initProcessTable(){
	int i;
	currentProcess = 0;
	for (i = 0; i < 8; ++i)
	{
		process_queu[i].waiter_id  = -1;
	 	process_queu[i].waitee_id  = -1;
	 	process_queu[i].waiter     = 0;
	 	process_queu[i].waitee     = 0;
		process_queu[i].status     = 4;
		process_queu[i].sp         = 0xff00;
	 	process_queu[i].segment    = (i+2) * 0x1000;
	}
}

void initializeProgram(int segment){
	struct Regs regs;
	regs.ds = segment;
	regs.es = segment;
	regs.ax = 0;
	regs.bp = 0;
	regs.di = 0;
	regs.si = 0;
	regs.dx = 0;
	regs.cx = 0;
	regs.bx = 0;
	regs.ip = 0;
	regs.cs = segment;
	regs.flags = 0x0200;
	copyToSeg(segment,0xff00,&regs,24);
}

void terminate(){
	setKernelDataSegment();
	currentProcess->status = 4;

	if(currentProcess->waitee != 0){
		currentProcess->waiter->waitee =  currentProcess->waitee;
		currentProcess->waitee->waiter =  currentProcess->waiter;
	}else{
		if(currentProcess->waiter != 0)
			currentProcess->waiter->status = 1;
	}
	currentProcess->waiter = 0;
	currentProcess->waitee = 0;
	currentProcess->sp     = 0xff00;
	restoreDataSegment();
	#asm
	sti
	#endasm
	while(1);
}

void scheduleProcess(int cpSP){
	int index=-1;
	setKernelDataSegment();

	if( !(currentProcess == 0 || currentProcess->status==4) ){
			currentProcess->sp = cpSP;
			index = (currentProcess->segment/0x1000)-2;

			if(currentProcess->status==3)
				currentProcess->status = 1;
	}
	while(1){
		index = (index == 7) ? 0 : index + 1;
		if(process_queu[index].status==1){
			currentProcess = &(process_queu[index]);
			currentProcess->status = 3;
			break;
		}
	}
	restoreDataSegment();
}

int getProcesses(int buffer[]){
	int i;
	int j = 0;
	setKernelDataSegment();
	for (i = 0; i < 8; ++i)
		if(process_queu[i].status != 4)
			j++;
	copyToSeg(currentProcess->segment,buffer,&process_queu,120);
	restoreDataSegment();
	return j;
}