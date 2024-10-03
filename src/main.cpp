/*
Vector fields is a program which visualizes a vector field and 
animates particles going through the field. At first, the particles
are distributed evenly among the field. The particles will move throughout
the filed in hopefully meaningful or pretty ways. Then, eventually,
each particle will end up either at an edge of the window or a "sink". 
A sink is a programmer-placed entity which absorbs a particle if the
particle touches it. Then, the particle is transported to a "source" 
and the source emits the particle, where it can begin its journey again.
*/

#include <SFML/Graphics.hpp>
#include <math.h>
#include <limits>
#include <random>

#define MAX_X 1200.f
#define MAX_Y 700.f
#define NUM_ARROWS_IN_ROW 40
#define NUM_ARROWS_IN_COLUMN 30
#define ARROW_OPACITY 100 // 0-255

#define NUM_PARTICLES 8000
#define PARTICLE_SIZE 1.f
#define PARTICLE_SPEED 0.1

float toDegrees(float radian) {
    return (radian * (180.0f / M_PI));
}

float toRadians(float degrees) {
    return (degrees * (M_PI / 180.0f));
}

float getMagnitudeFromVector(sf::Vector2f inputVector){
    return sqrt(pow(inputVector.x, 2) + pow(inputVector.y, 2));
}

float getAngleFromVector(sf::Vector2f inputVector) {
    // Returns angle theta from 0 (straight to the right) to the vector, in degrees, clockwise
    return toDegrees(atan2f(inputVector.y, inputVector.x));
}

sf::Vector2f normalizeVector(sf::Vector2f inputVector) {
    sf::Vector2f retVector(0.f, 0.f);
    float normalizationFactor = sqrt(pow(inputVector.x, 2) + pow(inputVector.y, 2));
    retVector.x = inputVector.x / normalizationFactor;
    retVector.y = inputVector.y / normalizationFactor;
    return retVector;
}

sf::Vector2f fieldFunctionF1(sf::Vector2f inputCoordinates) {
    // F(x,y) = -y*i + x*j Counter-clockwise concentric  circles (maybe it'll be clockwise because of y=0 being at top)
    
    sf::Vector2f origin(MAX_X/2.0, MAX_Y/2.0);
    inputCoordinates.x -= origin.x;
    inputCoordinates.y -= origin.y;

    sf::Vector2f outputVector(0.f,0.f);
    //outputVector.x = 0-inputCoordinates.y;
    //outputVector.y = inputCoordinates.x;
    outputVector.x = sin((inputCoordinates.x + inputCoordinates.y)/70.f);
    outputVector.y = cos((inputCoordinates.x - inputCoordinates.y)/70.f);
    // Shift origin to center
    
    return outputVector;
}

sf::Vector2f fieldFunctionF2(sf::Vector2f inputCoordinates) {
    sf::Vector2f outputVector(0.f, 0.f);
    outputVector.x = inputCoordinates.x;
    outputVector.y = inputCoordinates.y;
    return outputVector;
}

sf::Vector2f fieldFunctionF3(sf::Vector2f inputCoordinates) {
    sf::Vector2f origin(MAX_X/2.0, MAX_Y/2.0);
    inputCoordinates.x -= origin.x;
    inputCoordinates.y -= origin.y;

    sf::Vector2f outputVector(0.f,0.f);
    outputVector.x = inputCoordinates.y/100.0;
    outputVector.y = sin((inputCoordinates.x)/10.f)/10.0;

    return outputVector;
}

sf::Vector2f fieldFunction(sf::Vector2f inputCoordinates) {
    return fieldFunctionF1(inputCoordinates);
}

sf::Color findColorFromMagnitude(float magnitude, float max, float min) {
    /*
    Returns a color based on the magnitude given. A magnitude of 0 is blue, a magnitude of max is red.
    This shit is like matllaaabbb mannnn
    */
    sf::Color retColor(0, 0, 0, 0);

    if (magnitude > max) {
        retColor = sf::Color(255, 0, 0, 255);
        printf("Magnitude given %f greater than max given %f\n", magnitude, max);
    } else if (magnitude < min) {
        retColor = sf::Color(0, 0, 255, 255);
        printf("Magnitude given %f less than min given %f\n", magnitude, min);
    }
    // Normalize that bih
    magnitude -= min;
    max -= min;
    magnitude = magnitude / max; // Normalize that bih
    if (magnitude >= 0.0 && magnitude <= 0.25) {
        retColor = sf::Color(0, 255*(magnitude/0.25), 255, ARROW_OPACITY);
    } else if (magnitude > 0.25 && magnitude <= 0.5) {
        retColor = sf::Color(0, 255, 255*(1 - (magnitude - 0.25)/0.25), ARROW_OPACITY);
    } else if (magnitude > 0.5 && magnitude <= 0.75) {
        retColor = sf::Color(255*(magnitude - 0.5)/0.25, 255, 0, ARROW_OPACITY);
    } else if (magnitude > 0.75 && magnitude <= 1) {
        retColor = sf::Color(255, 255*(1 - (magnitude - 0.75)/0.25), 0, ARROW_OPACITY);
    }
    return retColor;
}

struct Particle {
    sf::CircleShape shape;
    sf::Vector2f velocity;
};

int main()
{
    // Set up the screen
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(MAX_X, MAX_Y), "Vector Field", sf::Style::Default, settings);    
    window.setFramerateLimit(144);

    // Set up the arrows
    sf::ConvexShape arrows[NUM_ARROWS_IN_ROW][NUM_ARROWS_IN_COLUMN];
    for (int j = 0; j < NUM_ARROWS_IN_COLUMN; j++) {
        for(int i=0; i < NUM_ARROWS_IN_ROW; i++) {
            arrows[i][j].setPointCount(7);
            arrows[i][j].setPoint(0, sf::Vector2f(10,10));
            arrows[i][j].setPoint(1, sf::Vector2f(40, 10));
            arrows[i][j].setPoint(2, sf::Vector2f(40, 0));
            arrows[i][j].setPoint(3, sf::Vector2f(60, 12));
            arrows[i][j].setPoint(4, sf::Vector2f(40, 24));
            arrows[i][j].setPoint(5, sf::Vector2f(40, 14));
            arrows[i][j].setPoint(6, sf::Vector2f(10, 14));
            arrows[i][j].setOrigin(sf::Vector2f(40, 14));
            arrows[i][j].setScale(sf::Vector2f(0.3, 0.3));
            arrows[i][j].setPosition(sf::Vector2f((i+0.5) * MAX_X / (float) NUM_ARROWS_IN_ROW, (j+0.5) * MAX_Y / (float) NUM_ARROWS_IN_COLUMN)); // Offset by 1/2 to "center" in field
        }
    }

    // Find minimum and maximum magnitudes
    float magnitudeMin = __FLT_MAX__;
    float magnitudeMax = __FLT_MAX__ * -1.0;

    // Do the vector math on the arrows using the function
    for (int j = 0; j < NUM_ARROWS_IN_COLUMN; j++) {
        for(int i=0; i < NUM_ARROWS_IN_ROW; i++) { 
            //sf::Vector2f targetVector = fieldFunction(arrows[i][j].getPosition());
            sf::Vector2f targetVector = fieldFunction(arrows[i][j].getPosition());
            float magnitude = getMagnitudeFromVector(targetVector);
            if (magnitude < magnitudeMin) {
                magnitudeMin = magnitude;
            }
            if (magnitude > magnitudeMax) {
                magnitudeMax = magnitude;
            }
            targetVector = normalizeVector(targetVector);
            float targetAngle = getAngleFromVector(targetVector);
            arrows[i][j].setRotation(targetAngle);
        }
    }

    // Give colors to the arrows
    for (int j = 0; j < NUM_ARROWS_IN_COLUMN; j++) {
        for(int i=0; i < NUM_ARROWS_IN_ROW; i++) { 
            // TODO we're doing this twice. The optimization is probably to make arrows a 2d array of structs, with the shape, and also magnitude and other useful shit
            sf::Vector2f targetVector = fieldFunctionF1(arrows[i][j].getPosition());
            float magnitude = getMagnitudeFromVector(targetVector);
            arrows[i][j].setFillColor(findColorFromMagnitude(magnitude, magnitudeMax, magnitudeMin));
        }
    }

    // Initialize all the particles
    Particle particles[NUM_PARTICLES];
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].shape = sf::CircleShape(PARTICLE_SIZE);
        //particle.shape.setFillColor(sf::Color(100, 100, 100, 100));
        particles[i].velocity = sf::Vector2f(0.f, 0.f);
        //particles[i].shape.setPosition(sf::Vector2f(rand() % (int) MAX_X, rand() % (int) MAX_Y));
        particles[i].shape.setPosition(sf::Vector2f((rand() % (int) MAX_X*2) - MAX_X/2, (rand() % (int) MAX_Y*2) - MAX_Y/2));
    }



    while (window.isOpen())
    {
        for (auto event = sf::Event{}; window.pollEvent(event);)
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
        }

        // Do particle calculations 
        for (int i = 0; i < NUM_PARTICLES; i++) {
            particles[i].velocity = fieldFunction(particles[i].shape.getPosition());
            particles[i].velocity.x *= PARTICLE_SPEED;
            particles[i].velocity.y *= PARTICLE_SPEED;
            particles[i].shape.move(particles[i].velocity);
        }

        // Show dem bitches
        window.clear();
        for (int j = 0; j < NUM_ARROWS_IN_COLUMN; j++) {
            for (int i=0; i < NUM_ARROWS_IN_ROW; i++) {
                window.draw(arrows[i][j]);
            }
        }
        for (Particle particle: particles) {
            window.draw(particle.shape);
        }
        window.display();
    }
}