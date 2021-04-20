# define TIMER_1_BASE 0xFF202000;
# define TIMER_2_BASE 0xFF202020;
# define GPIO_BASE 0xff200060;

typedef struct _GPIO
{
unsigned int data ;
unsigned int control;
} GPIO ;
volatile GPIO* const port_A = (GPIO*) GPIO_BASE;

typedef struct _interval_timer
{
int status ;
int control ;
int low_period ;
int high_period ;
int low_counter ;
int high_counter ;
} interval_timer ;
volatile interval_timer * const timer_1 = (interval_timer *) TIMER_1_BASE ;

volatile interval_timer * const timer_2 = (interval_timer *) TIMER_2_BASE ;


//global variable
		int stoveIsOn;


	//global variables for led
		volatile int* const led = (int *)(0xFF200000);
	
	//global variables for timer
		int weightTime = 0;
		int awayTime = 0;
		int timer1IsStarted = 0;
		int timer2IsStarted = 0;

	//global variable for switches
		volatile int* const switches = (int *)(0xFF200040);
	
	//global variables buttons	
		volatile int* buttons = (int*)0xff200050;

	//global variables for segment displays
		volatile int *display = (int *)0xff200020;
		volatile int *display2 = (int *)0xff200030;
		volatile int HexDisplayCode[14] = {0b0111111,0b01110001,0b1110111,0b0111000,0b111000,0b1011110, 0b111001,0b11110011,0b110,0b1101101,0b0110111,0b0111110};
		// o, f, a, l, d, t, c, p, s, n i lost track
		volatile int numberCode[11] = {0b0111111,6,91,0b1001111,102,109,125,0b0000111,0b1111111, 0b1100111,0b1110111,0b1111110};
	
//end of globals

//gpio methods
void setUpGPIO(){
	port_A->control = (1<<2); //second bit is relay out, first is weight value, 0th is motion sensor
}

void setStove(int value){
	port_A-> data = value<<2;
}

int getWeight(){
	return (port_A->data & 0b10)>>1;
}

int getMotionDetec(){
	return port_A->data & 0b1;
}
//end gpio

//timer control methods
void startTimer(int s){
	timer1IsStarted = 1;
	if(s){
		timer_1-> status = 1;
		timer_1->control = 0b0100;
	}
	if(!s){
		timer_2-> status = 1;
		timer_2->control = 0b0100;
	}
}

void stopTimer(int s){	
	timer1IsStarted = 0;
	if(s){
		timer_1 -> control = 0b1000;
	}
	if(!s){
		timer_2 -> control = 0b1000;
	}
}

void resetTimer(int s){
	stopTimer(s);
	setUpTimer(s);
}
		
void setUpTimer(int s){
	if(s){
		int time = weightTime * 100000000;
		timer_1 -> low_period = time;
		timer_1 -> high_period = time >> 16;
	}
	
	if(!s){
		int time = awayTime * 100000000;
		timer_2 -> low_period = time;
		timer_2 -> high_period = time >> 16;
	}
}

int getTime(int s){
	if(s){
		timer_1 -> low_counter = 1;
		return timer_1 -> low_counter + ( timer_1 -> high_counter << 16);
	}
	if(!s){
		timer_2 -> low_counter = 1;
		return timer_2 -> low_counter + ( timer_2 -> high_counter << 16);
	}
}

int timeOut(int s){
	if(s) {
		return timer_1->status & 0b01;
	}
	if (!s) {
		return timer_2->status & 0b01;
	}
}

int timeRunning(int s){
	if(s) {
		return (timer_1->status & 0b10) >> 1;
	}
	if (!s) {
		return (timer_2->status & 0b10) >> 1;
	}
}
//end timer methods

//switch methods
int onSwitch(){
	int onS = *switches & 1; 
	return onS;
}
int timeSwitch(){
	int timeS = *switches & 0b10; 
	return timeS;
}

int changeSelector(){
	int selector = *switches & 0b100; 
	return selector;
}
//end switches

//display methods
void setDisplayOff(){
	int code = HexDisplayCode[1] + (HexDisplayCode[1]<<8) + (HexDisplayCode[0]<<16);
	*display = code;
}

void setDisplayOn(){
	int code = HexDisplayCode[10] + (HexDisplayCode[0]<<8);
	*display = code;
}

void setDisplayWarning(){
	int code = (HexDisplayCode[10]<<24) + (HexDisplayCode[11]<<16) + (HexDisplayCode[3]<<8) + HexDisplayCode[3];
	*display = code;
}

void setDisplayTimeSelect(int s){
	if(s){
		*display2 = (HexDisplayCode[7]<<8);
	}
	if(!s){
		*display2 = (HexDisplayCode[5]<<8) + (HexDisplayCode[9]);
	}
}

void displayInt(int s){
	int value;
	
	if(s){
		value = weightTime;
	}
	if(!s){
		value = awayTime;	
	}
	
	*display = numberCode[value%10] + (numberCode[(value-(value%10))/10]<<8);
}

void setDisplayStoveOff(){
	int code = HexDisplayCode[1] + (HexDisplayCode[1]<<8) + (HexDisplayCode[0]<<16) +(HexDisplayCode[9]<<24);
	*display = code ;
}

void clearDisplay(){
	*display = 0;
	*display2 = 0;
}

//end of display methods

//help methods
void increaseInterval(int s){
	if(s){
		weightTime++;
	}
	if(!s){
		awayTime++;
	}
}

void decreaseInterval(int s){
	if(s){
		weightTime--;
	}
	if(!s){
		awayTime--;
	}
}

void holdSystem(){
	while(*buttons != 0){
	}
}

void stoveOff(){
	stoveIsOn = 0;
	
	setDisplayStoveOff();
	
	setStove(1);
	
	while (*buttons != 1) {}

	holdSystem();
	
	setStove(0);
	setDisplayOn();
}
	
void resetSys(){
	stoveIsOn = 0;
	resetTimer(0);
	resetTimer(1);
}

//


void main(){
	*led = 0;
	clearDisplay();
	setUpGPIO();
	stoveIsOn = 0;
	while(1){				
		if(timeSwitch() && !onSwitch()){
			while(timeSwitch()){
				int selector = changeSelector();
				setDisplayTimeSelect(selector);
				displayInt(selector);
				
				if(*buttons == 1){
					increaseInterval(selector);
					displayInt(selector);
					holdSystem();
				}

				if(*buttons == 2){
					decreaseInterval(selector);	
					displayInt(selector);
					holdSystem();
				}
			}
			setUpTimer(0);
			setUpTimer(1);
			clearDisplay();
		}
		
		if(!onSwitch()){
			setDisplayOff();
			resetTimer(0);	
			resetTimer(1);
			setStove(0);
		}
		
		if(onSwitch()){			
			if((weightTime==0) || (awayTime == 0)){
				setDisplayWarning();	
			}
			   
			if(weightTime>0 && awayTime>0){ // have to set weight and away time, 	
				setDisplayOn();
				
				if(!getWeight() && timeRunning(1)){	//if stove has pot taken off and burner left on for certain time, state 2 of system and is returned if pot is placed back on within timer		
					*led = 0b11;
					
					stopTimer(1);
					timer1IsStarted = 0;
					startTimer(0);
							
					while(!getWeight() && stoveIsOn){
						if(getMotionDetec()){  //case 1, person shut stove off, return to normal and do nothing, if motion was detected but not enough to warrant an assumption that the user disabled the stove, reset the timer and continue
							resetTimer(0);
							int motionCount = 0;		
							
							while(getMotionDetec() && stoveIsOn){
								*led = motionCount;
								if(motionCount > 1000000){
										resetSys();
								}
								else{
									motionCount++;
								}
							}
							startTimer(0);
						}
					
						if(timeOut(0)){ //case 2, stove shut down 
							stoveOff();
						}	
					}
					resetTimer(0);
				}
				
				if(!getWeight()){ //if nothing on stove top and no one around dont do anything until
					while(!getWeight()){
						*led = 0b1;
						resetTimer(0);
						resetTimer(1);
						
					}
					*led = 0;
					stoveIsOn = 1; //system detected weight or motion 
				}
				
				if(!timer1IsStarted){
					timer1IsStarted = 1;
					startTimer(1);
				}
				
				if(getWeight() && !getMotionDetec()){
					if(timeOut(1)){
						stoveOff();
						timer1IsStarted = 0;
					}
				}
				
				if(getMotionDetec() && getWeight()){
					resetTimer(1);
					startTimer(1);
				}
				
			}
		}
	}
}



	