#pragma config(I2C_Usage, I2C1, i2cSensors)
#pragma config(Sensor, in5,    ballDetector,   sensorLineFollower)
#pragma config(Sensor, in6,    gyro,           sensorGyro)
#pragma config(Sensor, dgtl3,  sonarIn,        sensorSONAR_inch)
#pragma config(Sensor, I2C_1,  flywheelEncoder, sensorQuadEncoderOnI2CPort,    , AutoAssign )
#pragma config(Sensor, I2C_2,  rightEncoder,   sensorQuadEncoderOnI2CPort,    , AutoAssign )
#pragma config(Motor,  port2,           Sprockets,     tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port3,           leftBack,      tmotorVex393_MC29, openLoop, driveLeft)
#pragma config(Motor,  port4,           leftFront,     tmotorVex393_MC29, openLoop, driveLeft)
#pragma config(Motor,  port5,           rightFront,    tmotorVex393_MC29, openLoop, reversed, driveRight, encoderPort, I2C_2)
#pragma config(Motor,  port6,           rightBack,     tmotorVex393_MC29, openLoop, reversed, driveRight)
#pragma config(Motor,  port7,           Intake,        tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port9,           Flywheel,      tmotorVex393_MC29, openLoop, reversed, encoderPort, I2C_1)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*        Description: programming skills program for VEX Team 4300U         */
/*        Created by Yasadu De Silva, Century High School                    */
/*        Naming Conventions:                                                */
/*          functions and non-const variables: camelCase                     */
/*          const variables: ALLCAPS                                         */
/*---------------------------------------------------------------------------*/

// This code is for the VEX cortex platform
#pragma platform(VEX2)

// Select Download method as "competition"
#pragma competitionControl(Competition)

//Main competition background code...do not modify!
#include "Vex_Competition_Includes.c"

/*---------------------------------------------------------------------------*/
/*                          Global Variables                                 */
/*                                                                           */
/*  These global variables are used throughout the program, and include      */
/*  variables for slew rate, flywheel velocity control, and filtering        */
/*  sensor data out. I used shorts instead of ints for many of these         */
/*  variables because global assignments to many ints could cause trouble    */
/*  with the Cortex.                                                         */
/*  See https://www.vexforum.com/t/cortex-slave-cpu-error/39166/17           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                          Slew Rate Variables                              */
/*  These variables are used in the slew rate code, which prevents motor     */
/*  burnout by gradually increasing the power being sent to the motor,       */
/*  rather than suddenly sending a high value to the motor.                  */
/*---------------------------------------------------------------------------*/

/*
The motorRequests array holds the "requested power" for each motor.
When a part of the program wants to ramp a motor up to a certain power,
it changes the corresponding value in motorRequests, rather than directly
assigning the power to the motor.
*/
short motorRequests[]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
/*
The currentMotorValues array holds the current power being
sent to the motor, so that the slew rate code can know what power to
send next.
*/
short currentMotorValues[]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
/*
The motorSlewRates array holds the slew rate (how much the power
increases/decreases by each "cycle" of the slew rate task) for
each motor.
*/
short motorSlewRates[]={0, 40, 10, 10, 10, 10, 10, 20, 40, 10};
/*
This constant just stores the length of the arrays for the loops,
so I could easily change the length if I wanted to.
*/
const short MOTORREQUESTSLEN=10;

/*
These constants hold the index of each motor in the motorRequests
array, so that it's easier to write code to request a power value
for a specific motor.
*/
const short FRONTR=4;
const short BACKR=5;
const short BACKL=2;
const short FRONTL=3;
const short INTK=6;
const short FLYWHL=8;
const short TWR=1;

/*---------------------------------------------------------------------------*/
/*                     Flywheel Velocity Control Variables                   */
/*  These variables are used by the flywheel velocity control task.          */
/*---------------------------------------------------------------------------*/

/*
When a part of the program requests that the flywheel be set to some
velocity, this variable stores that velocity.
*/
long flywheelVelocityRequest=0;
/*
This variable stores the amount of power predicted to be necessary
for the requested velocity.
*/
float flywheelPredictPower=0;
/*
These variables are used for the take-back-half velocity control algorithm,
and their uses are explained in the updateFlywheelVelocity task.
*/
float error=0.0;
float previousError=0.0;
float takeBackHalfValue=0.0;
float gain=0.1;
float cap=100;
const short MAXPOWER=127;
const short LAUNCHDETECTACTIVATIONTIME=3000;
const short LAUNCHDETECTIONSPIKE=10;
const float STOPSPEEDUPATPERCENT=0.80;
const float SPEEDUPMULTIPLIER=1.8;
//This variable indicates that a new flywheel velocity request has been made.
bool newRequest=false;
/*
This variable indicates whether the flywheel power should be ramped up
after a ball is launched.
*/
bool rampUpPower=true;
/*
This variable is false at the start of the match to indicate that
a flywheel velocity has not been requested yet and thus the
task shouldn't bother with controlling the flywheel velocity yet.
*/
bool flywheelRequested=false;
/*
These variables are used to detect a ball launch, and
their uses are explained in the updateFlywheelVelocity task.
*/
bool launchDetectorArmed=false;
bool launched=false;

/*---------------------------------------------------------------------------*/
/*                     Filter List Variables                                 */
/*  These variables are used by to filter out faulty sensor data.            */
/*---------------------------------------------------------------------------*/
/*
This variable stores the max length of each filter list, so that a hard-coded number
is not used.
*/
const int MAXLISTLENGTH=5;
/*
Each array in this 2d array stores the last 5 sensor values for their respective
sensors, to be used for filtering out faulty sensor data (sensor spikes, etc.).
Their uses are explained more in the rollingAddToList and medianFilter functions
*/
float filterLists[3][MAXLISTLENGTH]={
	{0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0},
};
/*
At the start of the match, no sensor data has been recorded yet, but the lists
already are filled with 0s. Thus, if the median filter used the whole list right
at the start of the match, the presence of the default 0s could affect the
filter. This issue is avoided by storing how much actual sensor data has already
been recorded for each list in the listLengths array, and using this to only
consider actual data when filtering.
*/
int listLengths[3]={0, 0, 0};
//These constants store the index of each list in the 2d array.
const int FLYWHEELLIST=0;
const int RIGHTENCODERLIST=1;
const int SONARLIST=2;

/*
This constant is used to scale the gyro to degrees, since the raw sensor values
are somewhat off. It was determined experimentally.
*/
const float GYROSCALE=0.922;
//This constant is the value at which the gyroscope "rolls over" to 0 degrees.
const float GYROROLL=3845;

//These values are used in the function used to lift a ball until it goes under the flywheel
const short BALLDETECTORDROP=12;
const int BALLDETECTORTIMEFAILSAFE=850;
/*---------------------------------------------------------------------------*/
/*                     Filter List Functions                                 */
/*  These functions are used to improve the effectiveness of our sensors     */
/*  by filtering out sensor value spikes, which is especially useful for     */
/*  the ultrasonic and flywheel IME, which tend to spike a lot.              */
/*  The Renegade Robotics article on filtering                               */
/*  (https://renegaderobotics.org/filtering-sensor-data/) was a great        */
/*  resource that I used while working on this code, and it provided a      */
/*  a "base" that I could use and improve upon.                             */
/*---------------------------------------------------------------------------*/

/*
This function adds a new sensor value to the appropriate filter list, removes
the first value in the list, and bumps up each value in the list
one place. (i.e. it adds values to the list like a queue)
*/
void rollingAddToList(int list, float newValue)
{
	//If there is already MAXLISTLENGTH actual sensor values in the list...
	if(listLengths[list]==MAXLISTLENGTH){
		//Go through the list, replace each value with the next newest one
		for(int i=MAXLISTLENGTH-1; i>0; i--)
		{
			filterLists[list][i]=filterLists[list][i-1];
		}
		//Set the first value of the list to the new value
		filterLists[list][0]=newValue;
	}else
	{
		/*
		If it's near the start of the match, and the list still has
		some default 0s in there...
		*/
		for(int i=listLengths[list]; i>0; i--)
		{
			//Go through the list, replace each value with the next newest one
			filterLists[list][i]=filterLists[list][i-1];
		}
		//Add the new value to the start of the list
		filterLists[list][0]=newValue;
		//Increase the length of the list
		listLengths[list]+=1;
	}
}
/*
This function uses a median filter to compare a given value with the
recent recorded values for that sensor. If it's outside of a given threshhold,
it instead returns and records the median of the existing values
*/
float medianFilter(int list, float newValue, float threshhold)
{
	//If we have enough values to filter and the new value is outside of the threshhold
	int listLength=listLengths[list];
	if(listLength==MAXLISTLENGTH && (newValue>filterLists[list][0]+threshhold || newValue<filterLists[list][0]-threshhold)){
		//The new value is not fine
		//Create a duplicate list for calculating the median
		float duplicate[MAXLISTLENGTH];
		//Copy the values over from the actual list
		for(int i=0; i<listLength; i++)
		{
			duplicate[i]=filterLists[list][i];
		}
		//bubblesort the duplicate list
		for (int i=0; i<MAXLISTLENGTH; i++){
			//For as many cycles as number of elements +1
      for (int j = 0; j < MAXLISTLENGTH-1-i; j++)
      {
       /*
       for each cycle, go through the list except for the last i elements
       (which are already in place) and order each adjacent pair
       */
       if (duplicate[j] > duplicate[j+1])
       {
        int temp=duplicate[j];
        duplicate[j]=duplicate[j+1];
        duplicate[j+1]=temp;
       }
      }
    }
    //Get the median and return it
    float medianValue=duplicate[2];
    rollingAddToList(list, medianValue);
    return medianValue;
		}else
		{
		//The new value is fine, return it and add it to the list
		rollingAddToList(list, newValue);
		return newValue;
		}
}
//Reset a list to all 0s.
void resetFilterList(int list)
{
	for(int i=0; i<MAXLISTLENGTH; i++)
	{
		filterLists[list][i]=0;
	}
	listLengths[list]=0;
}


/*---------------------------------------------------------------------------*/
/*                     Flywheel Velocity Functions                           */
/*  These functions are used in conjuction with the updateFlywheelVelocity   */
/*  task to control the flywheel velocity. It is used with sensor data       */
/*  from the IME attached to the flywheel.                                   */
/*---------------------------------------------------------------------------*/

//This function calculates the velocity of the flywheel
float calculateFlywheelVelocity(){
	//Get the raw velocity, using the IME.
	float velocity=getMotorVelocity(Flywheel);
	//Filter out any outliers.
	float filteredVelocity=medianFilter(FLYWHEELLIST,velocity, 20);
	//Return the filtered value.
	return filteredVelocity;
}
/*
This function is used to establish a new velocity request for the flywheel. The caller of the function
specifies the desired speed, in RPM, of the flywheel, and other parameters that the flywheel control
function uses, the specific details of which are outlined in the comments of this function and
the comments in the updateFlywheelVelocity task.
*/
void requestFlywheelVelocity(float request, float power, float g, float c, bool ramp){
	//Set the new desired velocity
	flywheelVelocityRequest=request;
	//Set a power that will get close to that velocity
	flywheelPredictPower=power;
	/*
	The takBackHalfVariable is set to this particular value so that the power drops to the predicted
	power needed for the requested velocity as soon as the velocity exceeds the requested velocity.
	See the updateFlywheelVelocity task for more details
	*/
	takeBackHalfValue=2*power-MAXPOWER;
	//Set the current and previous error values
	error=request-calculateFlywheelVelocity();
	previousError=error;
	/*
	Update the gain and cap (more details in updateFlywheelVelocity task) based on the
	new request
	*/
	gain=g;
	cap=c;
	//This is a new request, so indicate that it is so
	newRequest=true;
	flywheelRequested=true;
	//Indicate that the ball has not been launched yet
	launched=false;
	launchDetectorArmed=false;
	//Indicate whether the power should be ramped up after a ball is launched
	rampUpPower=ramp;
	//Clear the timer used to detect launches (look at updateFlywheelVelocity for more details)
	clearTimer(T1);
}

/*---------------------------------------------------------------------------*/
/*                     Sensor Value Functions                                */
/*  These functions are used to automatically filter and scale the values    */
/*  from the sensors. When a part of the program needs a value from a        */
/*  sensor, it calls these functions instead of directly accessing the       */
/*  sensor value.
/*---------------------------------------------------------------------------*/

//Return the right encoder's sensor value, it doesn't spike much so we don't need a small threshhold
float rightEncoderValue()
{
	return medianFilter(RIGHTENCODERLIST, getMotorEncoder(rightFront), 200);
}
//Return the sonar's value, it tends to spike about ~7 inches or so sometimes, so use a 6 in threshhold
float sonarValue()
{
	return medianFilter(SONARLIST, SensorValue[sonarIn], 6);
}
//Return the value from the gyroscope.
float getGyro()
{
	/*
	The gyroscope is supposed to return a value in tenths of degrees, so divide it by 10
	to get degrees. However, the values it returns are slightly off, so multiply it
	by the scaling factor to make them correct.
	*/
	return SensorValue[gyro]/10 * GYROSCALE;
}
//This function resets the right encoder and the corresponding filter list.
void resetRightEncoderVars()
{
	//reset encoder to 0
	resetMotorEncoder(rightFront);
	//reset the encoder list to 0.
	resetFilterList(RIGHTENCODERLIST);
}

/*---------------------------------------------------------------------------*/
/*                     Robot Movement Functions                              */
/*  These functions are used to physically move the robot, according to      */
/*  sensor data.                                                             */
/*---------------------------------------------------------------------------*/

/*
This function is used to correct the robot's angle based on the gyroscope sensor.
It is usually used after the robot turns to correct for momentum. This function takes
the target degree measurement as input. The turn function and this function
illustrate an effective use of sensors because they use the gyroscope
sensor to accurately turn to a specified degree value, making our programming skills routine more
consistent.
*/
void correctHeading(int degrees)
{
	//Establish whether the turn is counterclockwise (positive sign) or clockwise (negative sign)
	int sign=1;
	if(degrees<0)
	{
		sign=-1;
	}
	//Use a really small speed, so momentum won't impact the correction much
	int speed=35;
	if(sign==-1)
	{
		//If turning clockwise (negative)...
		if(getGyro()<degrees)
		{
			//If the robot has turned too much...
			while(getGyro()<degrees)
			{
				//Correct by moving the right side forwards until the robot meets the correct angle
				motorRequests[FRONTR]=motorRequests[BACKR]=speed;
			}
		}else if(getGyro()>degrees)
		{
			//If the robot has turned too little...
			while(getGyro()>degrees)
			{
				//Correct by moving the right side backwards until the robot meets the correct angle
				motorRequests[FRONTR]=motorRequests[BACKR]=-speed;
			}
		}
	}else
	{
		//If turning counterclockwise (positive)...
		if(getGyro()<degrees)
		{
			//If the robot has turned too little...
			while(getGyro()<degrees)
			{
				//Correct by moving the left side backwards until the robot meets the correct angle
				motorRequests[FRONTL]=motorRequests[BACKL]=-speed;
			}
		}else if(getGyro()>degrees)
		{
			//If the robot has turned too much...
			while(getGyro()>degrees)
			{
				//Correct by moving the left side forwards until the robot meets the correct angle
				motorRequests[FRONTL]=motorRequests[BACKL]=speed;
			}
		}
	}
	//Stop all the motors
	motorRequests[FRONTR]=motorRequests[BACKR]=0;
	motorRequests[FRONTL]=motorRequests[BACKL]=0;
	//Wait a bit for the slew rate task to update.
	wait1Msec(100);
}
/*
This function is used to drive a specific distance based on the value of the ultrasonic sensor. It
takes as input the target distance to the wall, in inches. It also takes as input the amount of power to send
to the wall, a multiplier to reduce the power of the stronger left side drive, a boolean to indicate
whether the robot should go towards/away from the wall, and a boolean to indicate whether or not the
sonar's filter should be reset. This is an effective use of sensors because it makes
programming skills routine more accurate, since the ultrasonic is more accurate
than the motor encoders, which are affected by slippage, etc.
*/
void sonarDrive(short goal, short power, float leftMultiplier, bool towardsWall, bool resetSonarFilter)
{
	/*
	If the sonar filter needs to be reset (e.g. if the last recorded sonar values were radically different
	than the ones that will be recorded), do so.
	*/
	if(resetSonarFilter)
	{
		resetFilterList(SONARLIST);
	}
	//Store the current angle for correction at the end.
	float startHeading=getGyro();
	//If you're going towards the wall (sonar values get lower)...
	if(towardsWall)
	{
		while(sonarValue()>goal)
		{
			/*
			Go forwards until you reach the specified distance.
			The left side tends to be stronger than the right, so the power is multipled by
			some factor to make the robot go straighter.
			*/
			motorRequests[FRONTL]=motorRequests[BACKL]=power*leftMultiplier;
			motorRequests[FRONTR]=motorRequests[BACKR]=power;

		}
		//Stop all the motors, wait a bit for the slew rate code to update
		motorRequests[FRONTL]=motorRequests[FRONTR]=motorRequests[BACKL]=motorRequests[BACKR]=0;
		wait1Msec(100);
	}else //If you're going away from the wall (sonar values get higher)...
	{

		while(sonarValue()<goal)
		{
			/*
			Go backwards until you reach the specified distance.
			The left side tends to be stronger than the right, so the power is multipled by
			some factor to make the robot go straighter.
			*/
			motorRequests[FRONTL]=motorRequests[BACKL]=power*leftMultiplier;
			motorRequests[FRONTR]=motorRequests[BACKR]=power;
		}
		//Stop all the motors, wait a bit for the slew rate code to update.
		motorRequests[FRONTL]=motorRequests[FRONTR]=motorRequests[BACKL]=motorRequests[BACKR]=0;
		wait1Msec(100);
	}
	//Correct the angle based on the starting angle
	correctHeading(startHeading);
}
/*
This function is used to drive a specific distance based on the value of the encoder. It takes as input
a targer encoder value, an amount of power to send to the motors, and a multiplier to reduce the power
of the stronger left side drive. Since the encoder is affected by slippage, etc., only use
this for short hops where the sonarDrive function can't be used. Usually, this is when the sonar sensor
is pointing towards an uneven surface or object, like a goal.
*/
void encoderDrive(short goal, short power, float leftMultiplier)
{
	//Reset the encoder
	resetRightEncoderVars();
	//Store the current angle for correction at the end.
	float startHeading=getGyro();
	//If the goal is backwards...
	if(rightEncoderValue()>goal)
	{
		while(rightEncoderValue()>goal)
		{
			/*
			Go backwards until you reach the specified distance.
			The left side tends to be stronger than the right, so the power is multipled by
			some factor to make the robot go straighter.
			*/
			motorRequests[FRONTL]=motorRequests[BACKL]=power*leftMultiplier;
			motorRequests[FRONTR]=motorRequests[BACKR]=power;
		}
		//Stop all the motors, wait a bit for the slew rate code to update
		motorRequests[FRONTL]=motorRequests[FRONTR]=motorRequests[BACKL]=motorRequests[BACKR]=0;
		wait1Msec(100);
	}else //If the goal is forwards..
	{
		while(rightEncoderValue()<goal)
		{
			/*
			Go forwards until you reach the specified distance.
			The left side tends to be stronger than the right, so the power is multipled by
			some factor to make the robot go straighter.
			*/
			motorRequests[FRONTL]=motorRequests[BACKL]=power*leftMultiplier;
			motorRequests[FRONTR]=motorRequests[BACKR]=power;
		}
		//Stop all the motors, wait a bit for the slew rate code to update
		motorRequests[FRONTL]=motorRequests[FRONTR]=motorRequests[BACKL]=motorRequests[BACKR]=0;
		wait1Msec(100);
	}
	//Correct the angle based on the starting angle
	correctHeading(startHeading);
}
/*
This function turns the robot to a specified angle. It takes as input the degree measurement to turn to.
*/
void turn(int degrees)
{
	//Set the speed to something low to reduce the impact of momentum
	int speed=34;
	//Establish the direction of the turn: positive sign for counterclockwise (ccw) and negative sign for clockwise (cw)
	int sign=1;
	if(degrees<0)
	{
		sign=-1;
	}
	//While the turn isn't completed...
	while(abs(getGyro())<abs(degrees))
	{
		/*
		Move the left and right sides.
		If the turn is ccw, the left side will move back and the right side will move forwards.
		If the turn is cw, the right side will move forwards and the left side will move backwards.
		*/
		motorRequests[FRONTL]=motorRequests[BACKL]=-1*sign*speed;
		motorRequests[FRONTR]=motorRequests[BACKR]=sign*speed;

	}
	//Stop the motors, wait fot a bit for the slew rate code to update
	motorRequests[FRONTR]=motorRequests[BACKR]=0;
	motorRequests[FRONTL]=motorRequests[BACKL]=0;
	wait1Msec(100);
	//correct a bit for momentum
	correctHeading(degrees);
}
/*
This function lifts a ball until it is under the flywheel.
It detects a decrease in the values returned from the line-following sensor
as the presence of the ball. If we didn't use a sensor for this, we would
have to rely solely on a timer, which would be innacurate and might lead to accidental launches. We use a
line-following sensor because it works by shining an IR light and detecting how much light is reflected back.
Thus, unlike a light sensor, it doesn't depend on ambient light levels, which makes our program
more consistent.
*/
void liftBallToUnderFlywheel()
{
	//The initial value for the ballDetector sensor is stored.
	int ballDetectorBaseline=SensorValue[ballDetector];
	/*
	A timer is used as a failsafe. In case the sensor doesn't work properly,
	the tower stops moving the ball after BALLDETECTORTIMEFAILSAFE ms.
	*/
	clearTimer(T2);
	while(SensorValue[ballDetector]>ballDetectorBaseline-BALLDETECTORDROP && time1[T2]<BALLDETECTORTIMEFAILSAFE)
	{
		//Until the ballDetector drops by BALLDETECTORDROP or the failsafe is reached, move the ball upwards.
		motorRequests[INTK]=80;
		motorRequests[TWR]=80;
	}
	/*
	The tower and intake are stopped directly so that there is no delay.
	A delay might result in an accidental launch.
	*/
	motor[Sprockets]=motor[Intake]=0;
	//To keep the slew rate code updated, the motorRequests values are changed.
	motorRequests[TWR]=0;
	motorRequests[INTK]=0;
	//Wait for the slew rate task to update.
	wait1Msec(100);
}

/*
This function launches a ball. It can use either a timer, or the "launched" boolean variable, which detects
a launch based on a downwards spike in the flywheel velocity using the flywheel motor encoder (see the
updateFlywheelVelocity task for more information on this). The timer is a safer method (as the launch
detection code could report a false launch), but shouldn't be used when there are multiple balls in the
tower, as both balls could be launched instead of just one. The function takes as input a
boolean which specifies if the timer should be used, and the length of the timer, in milliseconds.
*/
void launchBall(bool timerBased, int timer)
{
	if(timerBased)
	{
		//If a timer is being used
		//spin the tower and the intake (to keep in any balls that might get out)
		motorRequests[TWR]=80;
		motorRequests[INTK]=80;
		wait1Msec(timer);
		//Directly stop the tower (to avoid a delay).
		motor[Sprockets]=0;
		//Update the motorRequests values, and wait for the slew rate code to update
		motorRequests[TWR]=0;
		motorRequests[INTK]=0;
		wait1Msec(100);
	}else
	{
		//If launch detection code is being used.
		//While the flywheel velocity control does not detect a launch
		while(!launched)
		{
			//spin the tower and the intake (to keep in any balls that might get out)
			motorRequests[TWR]=80;
			motorRequests[INTK]=80;
		}
		//Directly stop the tower (to avoid a delay, which could accidentally launch a second ball).
		motor[Sprockets]=0;
		//Update the motorRequests values, and wait for the slew rate code to update
		motorRequests[TWR]=0;
		motorRequests[INTK]=0;
		wait1Msec(100);
	}
}
void pre_auton()
{
  // Set bStopTasksBetweenModes to false if you want to keep user created tasks
  // running between Autonomous and Driver controlled modes. You will need to
  // manage all user created tasks if set to false.
  bStopTasksBetweenModes = true;

	// Set bDisplayCompetitionStatusOnLcd to false if you don't want the LCD
	// used by the competition include file, for example, you might want
	// to display your team name on the LCD in this function.
	// bDisplayCompetitionStatusOnLcd = false;

  //Reset the encoders
  resetMotorEncoder(Flywheel);
  resetMotorEncoder(rightFront);
  //Calibrate the gyroscope, as it says in the instructions for the gyro
  SensorType[gyro]=sensorNone;
  wait1Msec(1000);
  SensorType[gyro]=sensorGyro;
  wait1Msec(2000);
  SensorFullCount[gyro]=GYROROLL;
}

/*---------------------------------------------------------------------------*/
/*                     Flywheel Velocity Control                             */
/*  This task is used to control the velocity of the flywheel. It uses       */
/*  a take-back-half (TBH) algorithm. This algorithm increases the power     */
/*  sent flywheel based on the difference between the current and target     */
/*  velocities (the error). Once the error changes signs (from positive      */
/*  to negative or vice versa), the output is decreased by half.             */
/*  This was a great resource that I used when figuring out TBH:             */
/*  https://www.vexwiki.org/programming/controls_algorithms/tbh              */
/*  This task demonstrates the effective use of sensors because it uses      */
/*  an encoder sensor to maintain the speed of the flywheel at a set value   */
/*  and to quickly bring the flywheel speed back up after a ball is launched.*/
/*  This is crucial for our programming skills routine, as it means that     */
/*  our balls will launch at a predictable velocity, and we do not need to   */
/*  wait around for the flywheel to regain speed.
/*---------------------------------------------------------------------------*/

task updateFlywheelVelocity(){
	float output=0.0;
	while(true){
		/*
		At the start of the game, a speed has not been requested, so make sure that a speed has been requested
		before bothering to do anything.
		*/
		if(flywheelRequested){
			//If a new velocity is set, set the power to MAXPOWER
			if(newRequest){
				output=MAXPOWER;
				//This is not a new request anymore, so designate it as such
				newRequest=false;
			}else{
				//Get the current velocity
				float velocity=calculateFlywheelVelocity();
				/*
				The next bit of code is used to detect a launch, so that the flywheel can speed up after a launch
				Only try to detect launches if it has been LAUNCHDETECTACTIVATIONTIME since startup (since velocity
				fluctuates wildly at start) and the velocity has approached within 5% of the target.
				*/
				if(!launchDetectorArmed && time1(T1)>LAUNCHDETECTACTIVATIONTIME && velocity>=flywheelVelocityRequest*0.95 && velocity<=flywheelVelocityRequest*1.05)
				{
					//Indicate that the robot is ready to detect launches
					launchDetectorArmed=true;
				}
				/*
				If:
					a ball hasn't been launched
					velocity spikes down by LAUNCHDETECTIONSPIKE
					the robot is ready to detect launches
				Then say that a ball has been launched.
				*/
				if(!launched && launchDetectorArmed && filterLists[FLYWHEELLIST][0]<=filterLists[FLYWHEELLIST][4]-LAUNCHDETECTIONSPIKE)
				{
					launched=true;
				}
				//Calculate the difference between the target and current velocities
				error=flywheelVelocityRequest-velocity;
				//Scale the error by "gain" (so that the changes in output are smoother) and add it to output
				output=output+(gain*error);
				//Make sure that the output is not negative, and that it doesn't exceed the "cap"
				if (output<0){
					output=0.0;
				}
				if (output>cap){
					output=cap;
				}
				/*
				If a ball has been launched and velocity approaches within STOPSPEEDUPATPERCENT of
				the desired velocity, stop the speed up
				*/
				if(launched && velocity>=STOPSPEEDUPATPERCENT*flywheelVelocityRequest)
				{
					launched=false;
				}
				/*
				If a ball has been launched, and the request allows for the power to be ramped up, speed it up by
				SPEEDUPMULTIPLIER
				*/
				if(launched && rampUpPower)
				{
					output=flywheelPredictPower*SPEEDUPMULTIPLIER;
				}
				/*
				If the sign of the error is not equal to the sign of the predicted error...
				(i.e. if the current velocity crossed the target velocity
				*/
				if(sgn(error)!=sgn(previousError))
				{
					/*
					Set the output to 1/2 * output+takeBackHalfValue.
					When a new request is made, takeBackHalfValue is set to 2*power-MAXPOWER,
					(power=predictedPower)and output is set to MAXPOWER. So the flywheel will go
					at max speed until it passes the target velocity, at which point the output
					will be set to: 0.5*(MAXPOWER+2*predictedPower-MAXPOWER)=0.5*2*predictedPower=predictedPower.
					Since predictedPower is close to the correct power for the target velocity,
					the flywheel will quickly reach and then settle at the target velocity.
					*/
					output=0.5*(output+takeBackHalfValue);
					//Set the takeBackHalfValue to the new output
					takeBackHalfValue=output;
				}
				//Store the error in previousError
				previousError=error;
			}
			//Send the output to the flywheel
			motorRequests[FLYWHL]=output;
		}
		//Wait so that the task doesn't hog the CPU.
		wait1Msec(20);
	}
}
/*---------------------------------------------------------------------------*/
/*                     Slew Rate Control                                     */
/*  This task is used to gradually ramp up the power sent to each motor      */
/*  so that the motors don't burn out.                                       */
/*---------------------------------------------------------------------------*/
task slewRate(){
	while(true){
		//For every motor...
		for(int j=0; j<MOTORREQUESTSLEN; j++){
			//Check if the current assigned power is less than the requested power
			if(currentMotorValues[j]<motorRequests[j])
			{
				//If so, increase the power by the given rate for the motor
				currentMotorValues[j]+=motorSlewRates[j];
				if(currentMotorValues[j]>motorRequests[j])
				{
					//If it increased past the desired power, set the current value to the desired power
					currentMotorValues[j]=motorRequests[j];
				}
				//Set the motor to the current power
				motor[j]=currentMotorValues[j];
			}
			else if(currentMotorValues[j]>motorRequests[j])
			{
				//Check if the current assigned power is greater than the requested power
				//If so, decrease the power by the given rate for the motor
				currentMotorValues[j]-=motorSlewRates[j];
				if(currentMotorValues[j]<motorRequests[j])
				{
					//If it decreased past the desired power, set the current value to the desired power
					currentMotorValues[j]=motorRequests[j];
				}
				//Set the motor to the current power
				motor[j]=currentMotorValues[j];
			}
			else
			{
				//If the current power matches the desired power
				//Set the motor to the current power
				motor[j]=currentMotorValues[j];
			}
		}
		//Wait a bit so that the task doesn't hog the CPU
		wait1Msec(15);
	}
}
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              Autonomous Task                              */
/*                                                                           */
/*  This task is used to control your robot during the autonomous phase of   */
/*  a VEX Competition.                                                       */
/*                                                                           */
/*  You must modify the code to add your own robot specific commands here.   */
/*---------------------------------------------------------------------------*/

task autonomous()
{
	//Start the slew rate and flywheel control tasks
	startTask(slewRate);
	startTask(updateFlywheelVelocity);
	//At the start, the hood is hooked onto the tower. Spin the tower to unhook the hood
	motorRequests[TWR]=80;
	wait1Msec(500);
	//Stop spinning the tower
	motorRequests[TWR]=0;
	wait1Msec(1000);
	//Start spinning the flywheel
	requestFlywheelVelocity(40, 45, 0.015, 127, true);
	//Pull the preload in slightly
	motorRequests[INTK]=80;
	motorRequests[TWR]=80;
	wait1Msec(400);
	//Directly set the motors to 0 so there is no delay, which could pull in the ball more than necessary
	motor[Sprockets]=motor[Intake]=0;
	//Request that the motors stop
	motorRequests[TWR]=0;
	motorRequests[INTK]=0;
	wait1Msec(100);
	/*
	Drive a bit forward to get ball in front, use a low power (adjusted by battery level)
	and slightly reduce the power being sent to the left motor using a multiplier of 0.9
	*/
	encoderDrive(100, 35*7505/nAvgBatteryLevel, 0.9);
	//Pull in the second ball
	liftBallToUnderFlywheel();
	//Turn clockwise 90 degrees, towards the goal
	turn(-90);
	/*
	Move forward towards the first goal (until the robot is
	19 inches from the wall). Use a moderate power (40) and adjust it by the battery level
	Slightly reduce the power being sent to the left motor using a multiplier of 0.9
	*/
	sonarDrive(19, 40*7505/nAvgBatteryLevel, 0.9, true,true);
	//turn 33 degrees clockwise, to a total of 123 degrees clockwise
	turn(-123);
	/*
	Launch the ball, don't use a timer because we have multiple balls in the tower,
	Pass in -1 since we have to pass in a parameter for the length of time.
	*/
	launchBall(false, -1);
	/*
	Move backwards a bit, using a moderate power (40) and adjusting it by the battery level
	Slightly reduce the power being sent to the left motor using a multiplier of 0.9
	*/
	encoderDrive(-500, -40*7505/nAvgBatteryLevel, 0.9);
	//Turn to -90 degrees again (90 degrees clockwise from the original position)
	turn(-90);
	//Spin the flywheel slightly faster, wait a bit for it to update.
	requestFlywheelVelocity(46, 48, 0.015, 127, true);
	wait1Msec(100);
	/*
	Go backwards to near the second goal (until the robot is 57
	inches from the wall). Adjust the speed by the battery level and use a multiplier of 0.9 to
	reduce the power sent to the left side.
	*/
	sonarDrive(57, -35*7505/nAvgBatteryLevel, 0.9, false,true);
	wait1Msec(500);
	/*
	Turn 90 degrees towards the goal. Use the current gyro value instead of an "absolute angle" to
	reduce the effect of drift
	*/
	turn(getGyro()-90);
	/*
	Drive a bit forward at a moderate speed, adjusting for battery level and using a multiplier
	of 0.9 for the left side
	*/
	encoderDrive(75, 40*7505/nAvgBatteryLevel, 0.9);
	//Launch the ball, using a timer, since we only have 1 ball in the tower.
	launchBall(true, 2000);
	//Autonomous is over, stop the tasks
  stopTask(updateFlywheelVelocity);
  stopTask(slewRate);
}
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              User Control Task                            */
/*                                                                           */
/*  This task is used to control your robot during the user control phase of */
/*  a VEX Competition.                                                       */
/*                                                                           */
/*  You must modify the code to add your own robot specific commands here.   */
/*---------------------------------------------------------------------------*/

task usercontrol()
{

}
