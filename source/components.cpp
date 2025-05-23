#include <iostream>
#include "../headers/driver_input.h"
#include "../headers/vehicle.h"
#include "../headers/components.h"
using namespace std;


//constructor that lets the user choose the values of the battery
Battery::Battery(float Q_max, float V_max, float R_internal, float heatCapacity){
    if(Q_max <= -1){
        this->Q_max = 150; //default
        this->Q_now = 150;
    } else{
        this->Q_max = Q_max;
        this->Q_now = Q_max;
    }
    if (V_max <= -1){
        this->V_max = 420;
        this->voltage = 0.9 * 420; //nominal voltage is about 90% of the max voltage 
    } else {
        this->V_max = V_max;
        this->voltage = 0.9 * V_max; //nominal voltage is about 90% of the max voltage 
    } 
    if (R_internal <= -1){
        R_internal = 0.02;
    } else {
        this->R_internal = R_internal;
    }
    if (heatCapacity <= -1){
        this->heatCapacity = 1000;
    } else {
        this->heatCapacity = heatCapacity;
    }
    this->stateOfHealth = 1; //(100% == 1)
    this->current = 0; 
    this->heatTransferCoeff = 0.6; 
    this->temperature = 25; 
    this->totalTimeSeconds = 0;     
    this->totalDistanceKm = 0; 


};

//default constructor
Battery::Battery(){
    Q_max = 150; //Realistic Max Charge for an EV
    Q_now = 150; //Starting fully charged so Q_now = Q_max
    V_max = 420; //Fully charged state
    R_internal = 0.02; //Internal resistance in ohms typical for EVs
    stateOfHealth = 1; //New battery starts at full health (100% == 1)
    voltage = 400; //Nominal voltage 
    current = 0; //No current flow initially
    heatCapacity = 1000; //Realistic thermal mass of battery
    heatTransferCoeff = 0.6; //Realistic heat transfer coefficient
    temperature = 25; //Room temperature in Celsius at the start
    totalTimeSeconds = 0; //by default starts at 0
    totalDistanceKm = 0; //by default starts at 0

};

//@brief function that returns the state of charge of the battery
float Battery::get_SOC(){
    //the current state of charge in percent is the ratio of charge remaining and max charge capacity
    return (Q_now / Q_max) * 100.0;
}


void Battery::setCurrent(float I) {
    current = I;
}

//@brief function that discharges the battery by modifyin the current amount of charge in the battery
//based on speed and time. Discharge rate is affected by temperature. This function is called every fraction of a second in main. 
//@param speed - the current speed of the car, delta_t - the change in time (which would be the interval between each frame)
void Battery::discharge(float speed, float delta_t){
    //The discharge rate coefficient
    float baseDischargeRate = 10; //0.01

    //Temperature factor adjustment
    float tempFactor = 1.0; //Base factor at reasonable temperatures 
    if (temperature < 0){
        tempFactor = 0.7;  //When temperature is below 0 C, discharge is 30% less effective
    } else if (temperature > 40.0){
        tempFactor = 1.2;  //There is 20% more discharge at high temperatures
    }

    //Calculate deltaQ (change in charge) based on the Base discharge rate, temperature factor
    //Assume a linear discharge rate proportional to speed and delta_t
    float deltaQ = baseDischargeRate * speed * delta_t * tempFactor /3600; //Divide by 3600 because delta_t is in seconds and we need Q_now in ampere - hours
    Q_now -= deltaQ; //Decrease the current charge level 

    //Current is negative when discharging (because the battery is supplying current to the motor)
    current += -deltaQ / delta_t;

    //Prevent battery from going below 0
    if (Q_now < 0){
        current = 0; //Battery cannot continue supplying current at 0
        Q_now = 0;
    }
}

//@brief function that charges the battery. This function is called when the EV is going through a charging station.
//@param V_applied - Charging station applies a voltage, delta_t - how long was this voltage applied for, isFull - indicates if the battery is full
bool Battery::charge(float V_applied, float delta_t, bool &isFull){
    //The change in charge is the current applied (V_applied/R_internal) times the change in time (which will be every frame)
    float deltaQ = delta_t*V_applied/(1000*R_internal); 

    //Ensure that Q_now is capped at Q_max
    if(Q_now < Q_max){
        Q_now += deltaQ;
        if (Q_now >= Q_max){
            Q_now = Q_max;
        }
    } else{ //If battery is fully charged, don't change anything and just set isFull to true
        isFull = true;
        current = 0; //Stop providing current to battery
        return true;
    }

    return false;

}

//@brief function that changes the temperature of the battery
//@param delta_t - time elapsed, ambientTemp - temperature of the environment
//@return temperature
float Battery::updateTemperature(float delta_t, float ambientTemp){
    //Formulas listed below:
    //Q = I^2 * R * t
    float heatGenerated = 0.00001 * current * current * R_internal * delta_t;

    //Q = h * (T_batt - T_ambient) * t
    float cooling = heatTransferCoeff * (temperature - ambientTemp) * delta_t;

    //The net heat change 
    float netHeat = heatGenerated - cooling;

    //Temperature change = Q / C
    float deltaTemp = netHeat / heatCapacity;

    //Update temperature attribute value
    this->temperature += deltaTemp;

    return temperature;
}

//@brief update the battery's state of health based on usage
//@param delta_t - time elapsed (to update function every call)
void Battery::degradeSOH(float delta_t){
    //Check if the battery temperature is above the threshold (40°C)
    if (temperature > 40){
        //Decrease the state of health proportionally to by how much the temperature
        //exceeds 40°C and how long this condition lasts (delta_t).
        stateOfHealth -= 0.001 * delta_t * (temperature - 40);
        //Cap at zero
        if (stateOfHealth < 0) {
            stateOfHealth = 0;
        }
    }
}

//@brief function that adds to current charge level after regenerative braking was applied
//@param deltaQ - the amount of charge gained from regenerative braking
void Battery::rechargeFromRegen(float deltaQ){
    Q_now += deltaQ;
    //cap Q_now at Q_max
    if (Q_now > Q_max){ 
        Q_now = Q_max; 
    }
}

//@brief function that degrades the battery's state of health based on charge used
//@param deltaQ - change in charge
void Battery::degradeWithCycle(float deltaQ){
    //Static variable to accumulate charge
    static float cycleCharge = 0;

    //Add the value of deltaQ (charge used or replenished) to the accumulator
    cycleCharge += deltaQ;

    //When the accumulated charge reaches or exceeds the battery's max capacity,
    //we consider one full charge-discharge cycle completed
    if (cycleCharge >= Q_max) {
        //Decrease the state of health by 10% per full cycle (10% is not a realistic parameter, but it was chosen for simplicity and showcase)
        stateOfHealth -= 0.1;

        //Prevent SOH from dropping below zero.
        if (stateOfHealth < 0) 
            stateOfHealth = 0;

        //Reset the cycle charge counter for the next cycle
        cycleCharge = 0;
    }
}

//setters
void Battery::set_Q_max(float Q){
    Q_max = Q;
}

void Battery::set_Q_current(float Q){
    Q_now = Q;
}

void Battery::set_V_max(float V){
    V_max = V;
}

void Battery::set_R_internal(float R){
    R_internal = R;
}

void Battery::set_SOH(float SOH){
    stateOfHealth = SOH;
}

//getters
float Battery::get_Q_max(){
    return Q_max;
}

float Battery::get_Q_current(){
    return Q_now;
}

float Battery::get_V_max(){
    return V_max;
}

float Battery::get_R_internal(){
    return R_internal;
}

float Battery::get_SOH(){
    return stateOfHealth;
}

float Battery::get_temp(){
    return temperature;
}


/////////////////////////////////////////////////////////////////////////////////////////
//default constructor
Motor::Motor(){
    speed = 0; //Speed of the vehicle in km/h
    R_internal = 0; //Internal resistance of the motor - we haven't actually used this attribute anywhere yet as we only modelled the electrical activity of the battery, but we plan to in the future
    efficiency = 1; //Efficiency of the motor - also can be implemented in the future
    maxSpeed = 100; //Maximum motor speed
    maxTorque = 200; //Maximum torque the motor can deliver in Newton-meters
    maxBrakeTorque = 300; //Maximum torque generated by braking in Newton-meters
    inertia = 10; //Rotational inertia of the motor (kg * m^2)
    regenEfficiency = 0.5; // Efficiency factor for regenerative braking (0 to 1)
    maxRegenPower = 100; // Maximum power that can be recovered through regen braking (Watts)
    heatTransferCoeff = 1.2; // Heat transfer coefficient for motor cooling (W/C)
    temperature = 25; // Current temperature of the motor (C)
    heatCapacity = 12; // Thermal capacity of motor, heat needed to raise temp by 1°C (J/C)
}

//constructor for user chosen parameters
Motor::Motor(float maxTorque, float maxSpeed){
        if(maxTorque == -1){
            this->maxTorque = 200;
        }
        this->maxTorque = maxTorque;
        this->maxSpeed = maxSpeed;
        speed = 0; //Speed of the vehicle in km/h
        R_internal = 0; //Internal resistance of the motor - we haven't actually used this attribute anywhere yet as we only modelled the electrical activity of the battery, but we plan to in the future
        efficiency = 1; //Efficiency of the motor - also can be implemented in the future
        maxBrakeTorque = 300; //Maximum torque generated by braking in Newton-meters
        inertia = 10; //Rotational inertia of the motor (kg * m^2)
        regenEfficiency = 0.5; // Efficiency factor for regenerative braking (0 to 1)
        maxRegenPower = 100; // Maximum power that can be recovered through regen braking (Watts)
        heatTransferCoeff = 1.2; // Heat transfer coefficient for motor cooling (W/C)
        temperature = 25; // Current temperature of the motor (C)
        heatCapacity = 12;
    }

//copy constructor
Motor::Motor(const Motor& other) : maxTorque(other.maxTorque), maxSpeed(other.maxSpeed){
        this->speed = other.speed; //Speed of the vehicle in m/s
        this->R_internal = other.R_internal; //Internal resistance of the motor - we haven't actually used this attribute anywhere yet as we only modelled the electrical activity of the battery, but we plan to in the future
        this->efficiency = other.efficiency; //Efficiency of the motor - also can be implemented in the future
        this->maxBrakeTorque = other.maxBrakeTorque; //Maximum torque generated by braking in Newton-meters
        this->inertia = other.inertia; //Rotational inertia of the motor (kg * m^2)
        this->regenEfficiency = other.regenEfficiency; // Efficiency factor for regenerative braking (0 to 1)
        this->maxRegenPower = other.maxRegenPower; // Maximum power that can be recovered through regen braking (Watts)
        this->heatTransferCoeff = other.heatTransferCoeff; // Heat transfer coefficient for motor cooling (W/C)
        this->temperature = other.temperature; // Current temperature of the motor (C)
        this->heatCapacity = other.heatCapacity;
}

//assignment operator overload
Motor& Motor::operator=(const Motor& other) {
    if (this != &other) {
        maxTorque = other.maxTorque;
        maxSpeed = other.maxSpeed;
        speed = other.speed;
        R_internal = other.R_internal;
        efficiency = other.efficiency;
        maxBrakeTorque = other.maxBrakeTorque;
        inertia = other.inertia;
        regenEfficiency = other.regenEfficiency;
        maxRegenPower = other.maxRegenPower;
        heatTransferCoeff = other.heatTransferCoeff;
        temperature = other.temperature;
        heatCapacity = other.heatCapacity;
    }
    return *this;
}

//@brief function that checks if regenerative braking is being applied
//@param input - driver's input to check brake conditions
//@return true if it's in regenerative state
bool Motor::isRegenerating(DriverInput& input){
    if (speed > 0 && input.get_brake() > 0){ //if the car is moving and braking
        return true;
    }    
    //else return false
    return false;
}

//@brief function that calculates the power of regeneration
//@param input - driver input
//@return the power of regeneration
float Motor::calculateRegenPower(DriverInput &input) {
    if (!isRegenerating(input)){ return 0; } //double check if regeneration is being applied 

    //Regen torque proportional to braking (simplified model)
    float regenTorque = input.get_brake() * regenEfficiency * maxTorque;

    //Convert torque to power using Power = torque * angular velocity
    float angularVelocity = speed;
    float power = regenTorque * angularVelocity; 
    //cap at max regen power
    if (power > maxRegenPower){
        return maxRegenPower;
    } else {
        return power;
    }
}

//@brief function that handles regenerative braking (chargers the battery as the vehicle brakes)
void Motor::applyRegenerativeBraking(DriverInput &input, EV &vehicle, Battery& battery, float deltaTime) {
    if (!isRegenerating(input)){ 
        return;} //check if it's regenerating again, just in case
    //get the power of regeneration
    float regenPower = calculateRegenPower(input);
    //check if it's above 0
    if (regenPower > 0){

        float regenVoltage = battery.get_V_max();
        float regenCurrent = regenPower / regenVoltage;
        //change in current Q = Current * time
        float deltaQ = regenCurrent * deltaTime;

        //Call battery recharge function
        battery.rechargeFromRegen(deltaQ);
        battery.setCurrent(regenCurrent);
    }
}

//setter of speed
void Motor::set_speed(float num){
    if (num >= 0){
        this->speed = num;
    }
}

//@brief function that updates speed based on driver input
//@param input - driver input (throttle/brake), battery - the battery being used by the car, vehicle - the vehicle being used (for its wheelRadius), deltaTime - time elapsed
//@return speed
float Motor::updateSpeed(DriverInput &input, EV &vehicle, Battery &battery, float deltaTime){
    if (isRegenerating(input)){ //check if regenerative braking is at play
        applyRegenerativeBraking(input, vehicle, battery, deltaTime); //apply regenerative braking
    }
    //set angular Speed to 0 initially
    static float angularSpeed = 0;
    float netTorque = 0;
    //get inputs
    float throttle = input.get_throttle();
    float brake = input.get_brake();

    //Apply throttle torque
    if (throttle > 0) {
        float throttleTorque = throttle * maxTorque;
        netTorque += throttleTorque;
    }

    //Apply braking torque
    if (brake > 0) {
        float brakingTorque = brake * maxBrakeTorque;
        netTorque -= brakingTorque;
    }

    //Passive drag when neither throttle nor brake is pressed
    if (throttle == 0 && brake == 0) {
        float dragTorque = 0.8 * maxTorque; //0.8 is the drag coefficient and it can be tuned
        netTorque -= dragTorque;
    }

    //Calculate angular acceleration using torque/inertia
    float angularAcceleration = netTorque / inertia;
    angularSpeed += angularAcceleration * deltaTime;

    //No negative angular speed is allowed as our car does not go in reverse yet
    if (angularSpeed < 0.0){
        angularSpeed = 0.0;
    }

    //Convert angular speed to linear speed using radius * angular speed
    this->speed = vehicle.get_wheelRadius() * angularSpeed;

    //Cap to max speed
    if (this->speed > maxSpeed) {
        this->speed = maxSpeed;
    }

    //Discharge battery based on current speed
    battery.discharge(speed, deltaTime);

    return speed;
}


void Motor:: setMaxRegenPower(float power) {
    maxRegenPower = power;
}

float Motor:: getMaxRegenPower() const {
    return maxRegenPower;
}

///////////////////////////////////////////////////////////////////////////////////////////

//default constructor
Charger::Charger(){
    isCharging = false;
    maxPowerOutput = 10000; // watts, example max power output
    efficiency = 0.9; // 90% efficiency
}

//@brief start delivering current to battery
//@param battery - the battery object, delta_t - time elapsed
void Charger::startCharging(Battery &battery, float delta_t){
    isCharging = true;
    //simple charging logic
    float chargingVoltage = 0.2 *battery.get_V_max(); //set charging voltage based on max voltage of battery
    float chargingCurrent = maxPowerOutput / chargingVoltage * efficiency; //current = power / voltage * efficiency
    if(battery.charge(chargingVoltage, delta_t, isCharging) == true){
        //if true then stop charging
        stopCharging();
    } //start charing the battery and check if it's full
}

bool Charger::get_charging_state(){
    return isCharging;
}

//@brief stop charging the battery
void Charger::stopCharging(){
    isCharging = false;
}