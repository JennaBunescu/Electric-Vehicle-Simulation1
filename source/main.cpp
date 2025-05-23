// Jenna Bunescu
// Jesiah Le
// CS172 Final project
// Simulation of an Electrical vehicle
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "../headers/driver_input.h"
#include "../headers/vehicle.h"
#include "../headers/components.h"
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>  //For sf::Clock
#include <SFML/Window.hpp>
#include <fstream>

// https://en.cppreference.com/w/cpp/thread/sleep_for
// https://cplusplus.com/reference/string/string/find/
// https://cplusplus.com/reference/string/string/substr/
// We learned all the SFML functionality from this code on this website: https://www.sfml-dev.org/tutorials/2.6/

using namespace std;



//@brief function to ensure valid, non-negative, real number input. Only -1 is allowed as a default choice,
//@param prompt - the message asking for input
//@return the valid value
float getValidatedInput(string prompt){
    float value;
    //Loop until valid input is received
    while (true){
        cout << prompt;
        cin >> value;
        //Use cin.fail() as the input variable is a float, so if user enters anything but a float (like a character), then the cin will fail to assign to the variable
        if (cin.fail() || value <= 0){ //Also make sure the input is non-negative
            if(value == -1){ //Allow user to input -1 as a choice of default settings
                cin.ignore(1000000, '\n'); //Clear out junk
                return value; 
            }
            //Clear the error state and ignore invalid input
            cin.clear();
            //Ignore a million characters in the input buffer until '\n'
            cin.ignore(1000000, '\n');
            cout << "Invalid input. Please enter a non-negative number.\n";
        } else {
            cin.ignore(1000000, '\n'); //Clear out junk
            return value;
        }
    }
}

//@brief gets the average speed over sampled time intervals
//@param sampleCount - number of data points to use, sampleInterval - number of seconds between each sample
void averageSpeed(int sampleCount, float sampleInterval){
    //dynamically allocate memory for storing the speed samples
    float* speeds = new float[sampleCount];

    ifstream file("output.csv"); //get the speed samples from the output file
    if (!file.is_open()){
        std::cout << "Cannot open file\n";
        return;
    }

    string line;
    //skip the header line
    getline(file, line);

    float lastSampleTime = -sampleInterval;  //initialize and sample the first entry
    int i = 0;  //Counter for how many samples we've read

    //Loop through each line of the CSV until we have enough samples
    while (i < sampleCount && getline(file, line)){
        //Find the first comma to extract the timestamp
        int firstComma = line.find(','); //we need to know where the first comma ends to accurately parse the for the time stamp
        if (firstComma == -1){
            break; //line.find() returns - 1 when assigned to an int and if the comma isn't found
        }

        //extract the time and convert from string to float using stof()
        float time = stof(line.substr(0, firstComma));

        //find the second comma to get the speed
        int secondComma = line.find(',', firstComma + 1);
        if (secondComma == -1){
            break;
        }

        //extract the speed and convert to float
        float speed = stof(line.substr(firstComma + 1, secondComma - firstComma - 1));

        //Only take this speed if enough time has passed since the last sample
        if (time - lastSampleTime >= sampleInterval){
            speeds[i] = speed; //Store the sampled speed
            lastSampleTime = time; //update the last sampled time
            i++; //Move to the next sample
        }
    }

    //if no samples were collected, notify the user
    if (i == 0){
       cout << "No samples collected.\n";
    } else{
        //Here is where we calculate the average of all collected speed samples
        float sum = 0;
        for (int j = 0; j < i; j++){
            sum += speeds[j];
        }
        float averageSpeed = sum / i;

        cout << "Average speed over " << i << " samples (every " << sampleInterval << "s): " << averageSpeed << " m/s\n";
    }

    delete[] speeds; //free memory
}



//@brief helper function to create a button on the display
void setupButton(sf::RectangleShape &button, sf::Vector2f size, sf::Vector2f position, sf::Color color){
    button.setSize(size);
    button.setPosition(position);
    button.setFillColor(color);
}

//@brief function to initialize assets (fonts, textures, and sprites)
//@param assets
bool loadAssets(sf::Font &font, sf::Texture &carTexture, sf::Texture &roadTexture1,
    sf::Texture &roadTexture2, sf::Texture &roadTexture3, sf::Texture &uiBoxTexture){
    if (!font.loadFromFile("./assets/Roboto.ttf")){ //check for success
        cout << "Error loading font" << endl;
        return false;
    }
    
    if (!carTexture.loadFromFile("./assets/car1.png") || 
        !roadTexture1.loadFromFile("./assets/road1.png") || 
        !roadTexture2.loadFromFile("./assets/road2.png") || 
        !roadTexture3.loadFromFile("./assets/road3.png") ||
        !uiBoxTexture.loadFromFile("./assets/display.png")){ //check for success
        cout << "Error loading textures" << endl;
        return false;
    }
    return true;
}

int main(){
    //Initialize clock (for tracking time)
    sf::Clock deltaClock;
    //Initialize window in 1280x720 mode
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Electric Vehicle Simulation");

    sf::Font font;
    sf::Texture carTexture, roadTexture1, roadTexture2, roadTexture3, uiBoxTexture;
    loadAssets(font, carTexture, roadTexture1, roadTexture2, roadTexture3, uiBoxTexture);

    //Create sprites - this could be further functionalized, but this is more convenient for now due to the textures that were loaded above
    sf::Sprite carSprite(carTexture);
    carSprite.setScale(0.3f, 0.3f);
    carSprite.setPosition(900, 200);

    sf::Sprite roadSprite1(roadTexture1), roadSprite2(roadTexture2), roadSprite3(roadTexture3);
    roadSprite1.setScale(float(window.getSize().x) / roadTexture1.getSize().x, float(window.getSize().y) / roadTexture1.getSize().y); //using the window size to fit the road 
    roadSprite2.setScale(float(window.getSize().x) / roadTexture2.getSize().x, float(window.getSize().y) / roadTexture2.getSize().y); //I need three road sprites to simulate GIF (movement)
    roadSprite3.setScale(float(window.getSize().x) / roadTexture3.getSize().x, float(window.getSize().y) / roadTexture3.getSize().y);

    //The blue box for the display information
    sf::Sprite uiBoxSprite;
    uiBoxSprite.setTexture(uiBoxTexture);
    uiBoxSprite.setPosition(15, 200);
    uiBoxSprite.setScale(0.5f, 0.5f);

    //EV ON/OFF button initialization
    sf::RectangleShape button(sf::Vector2f(150, 60));
    button.setPosition(50.f, 50.f);
    button.setFillColor(sf::Color(100, 100, 250));

    sf::Text buttonText;
    buttonText.setFont(font);
    buttonText.setString("EV ON");
    buttonText.setCharacterSize(30);
    buttonText.setFillColor(sf::Color::White);
    sf::FloatRect textRect = buttonText.getLocalBounds();
    buttonText.setOrigin(textRect.left + textRect.width / 2.0, textRect.top + textRect.height / 2.0);
    buttonText.setPosition(button.getPosition().x + button.getSize().x / 2.0, button.getPosition().y + button.getSize().y / 2.0);

    //track if the EV is on or off. Initialize at true as the first run is the default
    bool evOn = true;

    //Track mouse button previous state (this is to make sure the EV ON/OFF button does not react to mouse holds)
    bool mouseWasPressed = false;

    //Initialize classes + variables
    DriverInput input;
    Motor motor;
    Battery battery;
    EV myEV;
    Charger charger;
    float vehicleSpeed = 0, ambientTemp = 25, batteryTemp = 0;

    float roadYPosition = 0.0; //Default Y position of the road

    //Load csv file for info output
    ofstream logFile("output.csv");
    logFile << "Time,Speed,SOC,BatteryTemp,Throttle,Brake\n";

    float totalTime = 0.0;  //To track time for the loop of updates

    //Run window loop (open screen)
    while (window.isOpen()){
        sf::Event event; //create event
        float deltaTime = deltaClock.restart().asSeconds(); //use delta time as the interval between each frame

        //For logging battery state to csv
        totalTime += deltaTime;
        logFile << totalTime << "," 
        << vehicleSpeed << "," 
        << battery.get_SOC() << "," 
        << battery.get_temp() << "," 
        << input.get_throttle() << "," 
        << input.get_brake() << "\n";

        //handle window events
        while (window.pollEvent(event)){
            if (event.type == sf::Event::Closed)
                window.close();
        }

        //Get current mouse left button state
        bool mousePressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);
        sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
        sf::Vector2f mousePosF(static_cast<float>(mousePosI.x), static_cast<float>(mousePosI.y));

        //Check for mouse press (pressed now, but was not pressed before)
        if (mousePressed && !mouseWasPressed){

            //Only toggle if click is inside button bounds
            if (button.getGlobalBounds().contains(mousePosF)){
                evOn = !evOn; //change evOn state
    
                if (evOn){ //if the EV is on, just set the text to on
                    buttonText.setString("EV ON"); //change to ON
                    button.setFillColor(sf::Color(100, 100, 250));
                } else{ 
                    buttonText.setString("EV OFF"); //change to off
                    button.setFillColor(sf::Color(200, 50, 50));

                    //Ask for input into the terminal if the EV is off, to start new "session"
                    cout << "Enter '-1' if you want to set any elements to their default value." << endl;
                    float newBatteryCapacity = getValidatedInput("Enter new battery capacity (Ah): ");
                    float newBatteryVMAX = getValidatedInput("Enter new battery max voltage (V): ");
                    float newBatteryRinternal = getValidatedInput("Enter new battery internal resistance (Ohm): ");
                    float newBatteryHeatCap = getValidatedInput("Enter new battery heat capacity (J/K): ");
                    float newMaxTorque = getValidatedInput("Enter new motor max torque (Nm): ");
                    float newMaxSpeed = getValidatedInput("Enter new motor max speed (rad/s): ");
                    float newWheelRadius = getValidatedInput("Enter new wheel radius (m): ");

                    //Create new objects based on new inputs
                    Motor newMotor(newMaxTorque, newMaxSpeed);
                    Battery newBattery(newBatteryCapacity, newBatteryVMAX, newBatteryRinternal, newBatteryHeatCap);
                    EV newEV(newWheelRadius);

                    motor = newMotor;
                    battery = newBattery; //Replace old components with new ones
                    myEV = newEV;

                    cout << "EV components updated!\n\n";
                }

                //Re-center the EV ON/OFF text after string update (as EV OFF has one extra character)
                sf::FloatRect textRect = buttonText.getLocalBounds();
                buttonText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
                buttonText.setPosition(button.getPosition().x + button.getSize().x / 2.0f, button.getPosition().y + button.getSize().y / 2.0f);
            }
        }
        if (evOn){
            window.clear(sf::Color(200, 200, 200));
        }
        else{
            window.clear(sf::Color::Black);
        }

        //Handle driver inputs 
        //Check if W is pressed
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)){
            input.set_throttle(1.0); //W represents full throttle
        } else{
            input.set_throttle(0.0); //If W is not pressed, throttle is 0
        }
        //Check if S is pressed - braking
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)){
            input.set_brake(1.0); //S - full brake
        } else {
            input.set_brake(0.0);
        }
        //Check if "C" is pressed - this is to charge the battery. This can only be done when vehicle is stationary
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::C)){
            charger.startCharging(battery, deltaTime);
        }

        //Update vehicle speed and battery temperature
        vehicleSpeed = motor.updateSpeed(input, myEV ,battery, deltaTime);
        batteryTemp = battery.updateTemperature(deltaTime, ambientTemp);

        //Move the road upwards based on the speed(to simulate driving)
        roadYPosition += vehicleSpeed * deltaTime * 5;
        if (roadYPosition >= window.getSize().y){
            roadYPosition = 0;
        }

        //Set positions of road textures based on changed Y position
        roadSprite1.setPosition(0, roadYPosition);
        roadSprite2.setPosition(0, roadYPosition - window.getSize().y);
        roadSprite3.setPosition(0, roadYPosition - 2 * window.getSize().y);

        //Battery status alert 
        sf::Text alertText;
        alertText.setFont(font);
        alertText.setCharacterSize(50);
        alertText.setFillColor(sf::Color::Red);
        alertText.setStyle(sf::Text::Bold);
        alertText.setPosition(80, 300);

        //SOC text
        sf::Text socText;
        socText.setFont(font);
        socText.setString("Battery SOC: " + to_string(static_cast<int>(battery.get_SOC())) + "%");
        socText.setCharacterSize(30);
        socText.setFillColor(sf::Color::Black); 
        socText.setPosition(80, 400); 

        //Speed
        sf::Text speedText;
        speedText.setFont(font);
        speedText.setString("Speed: " + to_string(static_cast<int>(vehicleSpeed)) + " m/s");
        speedText.setCharacterSize(30);
        speedText.setFillColor(sf::Color::Black);
        speedText.setPosition(80, 450);

        //Battery Temperature
        sf::Text tempText;
        tempText.setFont(font);
        //State the battery temperature, and convert it to an int from float, using static cast
        tempText.setString("Battery Temperature: " + to_string(static_cast<int>(battery.get_temp())) + " C");
        tempText.setCharacterSize(30);
        tempText.setFillColor(sf::Color::Black);
        tempText.setPosition(80, 500);

        //Clear window and redraw
        window.clear(sf::Color(0, 0, 0)); //Black
        float soc = battery.get_SOC(); //Check State of charge to display alert
        if (soc >= 100){
            alertText.setString("Battery Full!");
            
        } else if(soc <= 20){
            alertText.setString("Battery Low!");
            
        } else{
            alertText.setString("");
        }

        if (evOn){ //Only draw sprites when EV is on
        window.draw(roadSprite1);
        window.draw(roadSprite2);
        window.draw(roadSprite3);
        window.draw(carSprite);
        window.draw(socText);
        window.draw(speedText);
        window.draw(tempText);
        window.draw(uiBoxSprite);
        window.draw(alertText);
        }
        
        window.draw(button);
        window.draw(buttonText);
        
        mouseWasPressed = mousePressed; //Use this to make sure that the button doesnt react to mouse being held, by updating the state at the end to indicate previous activity for the next run
        window.display(); //Display everything on the window

        //Delay for smoother movement
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); //~60 FPS

    }

    logFile.close();
    averageSpeed(100, 1); //100 every samples for every two seconds. If there isn't enought data to satisfy this, the function will use the data that does exist, so less samples than asked for essentially
    return 0;
}