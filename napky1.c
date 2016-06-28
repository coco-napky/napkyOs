int main(){

	char line[80];
	syscall_printString("\n\r");
	syscall_printString("Ingrese color: ");
	syscall_readString(line);
	syscall_setColor(line);
	syscall_terminate();
	return 0;
}