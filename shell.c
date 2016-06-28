void ps();
void setColor(char*);
void print(char* string);
void println(char* string);
void readConsole(char[]);
void readFile(char* filename, char buffer[13312]);
void readSector(char[], int sector);
void ls();
void clearBuffers();
void executeProgram(char* name);
void echo();
int compareString(char* x, char* y);
int mod(int x, int y);
int div(int x, int y);
void printInt(int x);
int getFileSize(char* filename);
void copy();
void clearScreen();
void parseInput();
void kill(char* id);

int i,j,k;
char line[80];
char buffer[512];
char file[13312];

char* command;
char* parameter;

void toString(int number, char buffer[32]);
int getNumberOfSectors(char* filename);

#define COMMAND_TYPE     "type"
#define COMMAND_EXECUTE  "execute"
#define COMMAND_EXECUTEW "executew"
#define COMMAND_CLEAR    "cls"
#define COMMAND_COLOR    "color"
#define COMMAND_DELETE   "delete"
#define COMMAND_LS       "ls"
#define COMMAND_ECHO     "echo"
#define COMMAND_COPY     "copy"
#define COMMAND_KILL     "kill"
#define COMMAND_PS       "ps"

struct PCB {
	unsigned int status;
	unsigned int sp;
	unsigned int segment;
	struct PCB* waiter;
	struct PCB* waitee;
 	int waiter_id;
	int waitee_id;
};

int main() {

	for (i = 0; i < 2; ++i)
		println(" ");

	println("*** Napky Shell ***");
	while(1){
		clearBuffers();
		print(">: ");
		readConsole(line);
		parseInput();
	}
	return 0;
}


void print(char* string){syscall_printString(string);}
void readConsole(char buffer[80]){ clearBuffers(); interrupt(0x21, 1, buffer, 0, 0);}
void readSector(char buffer[512], int sector){syscall_readSector(buffer,sector);}
void readFile(char* filename, char buffer[13312]){syscall_readFile(filename, buffer);}
void deleteFile(char* filename){syscall_deleteFile(filename);}
void executeProgram(char* name, int blocking){
	syscall_executeProgram(name, blocking);
}
void clearScreen(){syscall_clearScreen();}
void setColor(char* number){syscall_setColor(number);}

void parseInput(){
	int a;
	char test[10];
	for (i = 0; i < 10; ++i){
		command[i] = 0;
		parameter[i] = 0;
	}


	for (i = 0; line[i] != ' ' && line[i] != 0; ++i)
		command[i] = *(line + i);
	++i;
	parameter = line + i;

	if(compareString(command,COMMAND_TYPE)){
		readFile(parameter,file);
		println(file);
	}
	else if(compareString(command,COMMAND_EXECUTE))
		executeProgram(parameter,0);
	else if(compareString(command,COMMAND_EXECUTEW))
		executeProgram(parameter,1);
	else if(compareString(command,COMMAND_CLEAR))
		clearScreen();
	else if(compareString(command,COMMAND_COLOR))
		setColor(parameter);
	else if(compareString(command,COMMAND_DELETE))
		deleteFile(parameter);
	else if(compareString(command,COMMAND_LS))
		ls();
	else if(compareString(command,COMMAND_ECHO))
		echo();
	else if(compareString(command,COMMAND_COPY))
		copy();
	else if(compareString(command,COMMAND_KILL))
		kill(parameter);
	else if(compareString(command,COMMAND_PS))
		ps();
}

void println(char* string){
	print(string);
	print("\n\r");
}

void clearBuffers(){
	for (i = 0; i < 80; ++i)line[i] = 0;
	for (i = 0; i < 13312; ++i)file[i] = 0;
	for (i = 0; i < 512; ++i)buffer[i] = 0;
	command = 0;
	parameter = 0;
}

int compareString(char* x, char* y){
	for ( i = 0; x[i] != 0 || y[i] != 0;  ++i)
		if(x[i] != y[i])
			return 0;

	return 1;
}

void ls(){
	char directory[512];
	char filename[7];
	int x,y;
	int byte_offset;
	int counter = 0;
	readSector(directory,2);
	println(" ");
	for (x = 0; x < 16; ++x){
		byte_offset = 32*x;
		if(directory[byte_offset] != 0){
			for ( y = 0; y < 6; ++y)
				filename[y] = directory[byte_offset + y];

			filename[6]  = 0;

			print(" - ");
			print(filename);
			print(" : ");
			printInt(getFileSize(filename));
			print(" bytes");
			println(" ");
		}
	}
	println(" ");
}

void echo(){
	char filename[6];
	char file[13312];
	int  numberOfsectors = 0;
	int cursor = 0;
	print(" Filename : ");
	readConsole(line);

	for (i = 0; i < 6; ++i)
		filename[i] = line[i];
	i = 0;
	line[0] = 1;
	while(line[0] != 0){
		readConsole(line);
		for ( j = 0; j < 80; ++j){

			if(line[0] == 0)
				break;

			if(line[j] == 0){
				file[cursor] = ' ';
				cursor++;
				break;
			}
			else{
				file[cursor] = line[j];
				cursor++;
			}
		}
		++i;
		if(line[0] == 0){
			println(" Save file? [y] to confirm");
			readConsole(line);
			if(line[0] == 'y'){
				print("Saved Text : ");
				file[cursor-1] = 0;
				print(file);
				println("*");
				line[0] = 0;

				numberOfsectors = div(cursor,512);
				if(mod(cursor,512) != 0)
					numberOfsectors++;
				syscall_writeFile(filename,file,numberOfsectors);
			}
		}
	}
}

int mod(int x, int y){
	while(x >= y)
		x -= y;
	return x;
}

int div(int x, int y){
	int counter = 0;
	while(x >= y){
		x -= y;
		counter++;
	}
	return counter;
}

void toString(int x, char buffer[32]){
	int length = 1;
	int index = 0;
	char character = 0;
	int temp = 0;
	for (i = 0; i < 32; ++i)
		buffer[i] = 0;

	i = x;
	while(i >= 10){
		i = div(i,10);
		length++;
	}

	i = x;
	while(length > 0){
		temp = mod(i,10);
		i = div(i,10);
		switch(temp){
			case 0: character = '0'; break;
			case 1: character = '1'; break;
			case 2: character = '2'; break;
			case 3: character = '3'; break;
			case 4: character = '4'; break;
			case 5: character = '5'; break;
			case 6: character = '6'; break;
			case 7: character = '7'; break;
			case 8: character = '8'; break;
			case 9: character = '9'; break;
		}
		length--;
		buffer[length] = character;
	}
}

void printInt(int x){
	char number[512];
	toString(x,number);
	print(number);
}

int getFileSize(char* filename){
	char file[13312];
	int cursor = 0;
	readFile(filename,file);
	while(file[cursor] != '\0' || file[cursor + 1] != '\0' ||
		file[cursor + 2] != '\0' || file[cursor + 3] != '\0' )
		++cursor;
	return cursor;
}

void copy(){
	char fl1[80];
	char fl2[80];
	char file[13312];
	char directory[512];

	readSector(directory,2);
	print("src : ");
	readConsole(fl1);

	print("dst : ");
	readConsole(fl2);

	readFile(fl1,file);

	syscall_writeFile(fl2,file,getNumberOfSectors(fl1));


}


int getNumberOfSectors(char* filename){
	char directory[512];
	int entry_offset = 0;
	int byte_offset  = 0;
	int found = 0;
	int k;
	int i;
	char current;
	int sectors[26];
	int sector_num;
	int number_of_sectors = 0;

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
					number_of_sectors++;
			}
			return number_of_sectors;
		}
	}
	print("File not found :");
	println(filename);
	return -1;
}

void kill(char* id){
	syscall_kill(id);
}

void ps(){
	int index = 0;
	int z;
	int count;
	struct PCB processes[8];
	int segment;
	count = syscall_getProcesses(processes);
	println(" ");
	print(" - ");
	printInt(count);
	println(" processes running");
	for (z = 0; z < 8; ++z)
	{
		if(processes[z].status != 4){

			print(" *** Process ");
			printInt(z);
			println(" ******** ");
			print(" *  Status  : ");

			if(processes[z].status == 2){
				print("Waiting");
				print(" *");
			}
			else if(processes[z].status == 3){
				print("Running");
				print(" *");
			}

			println("");

			print(" *  Segment : ");
			segment = (processes[z].segment/0x1000) * 1000;
			print("0x");
			printInt(segment);
			print("  *");
			println(" ");

			print(" *  Waiter  : ");
			if(!processes[z].waiter){
				print("None    *");
			}else{
				printInt(processes[z].waiter_id);
				print("       *");
			}
			println(" ");

			print(" *  Waitee  : ");
			if(!processes[z].waitee){
				print("None    *");
			}else{
				printInt(processes[z].waitee_id);
				print("       *");
			}
			println("");
			index++;
		}
	}
	println(" **********************");
}
