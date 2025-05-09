//============================================================================
// Name        : RushHour.cpp
// Author      : Dr. Sibt Ul Hussain
// Version     :
// Copyright   : (c) Reserved
// Description : Basic 2D game with boundary and road constraints
//============================================================================

#ifndef RushHour_CPP_
#define RushHour_CPP_
#include "util.h"
#include <iostream>
#include <string>
#include <cmath>
using namespace std;

// Struct to hold position coordinates
struct Position {
    int x, y;
};

// Function to generate a random position on a road
Position getRandomRoadPosition() {
    while (true) {
        int x = rand() % 821; // 0 to 820
        int y = rand() % 801; // 0 to 800
        if ((x / 50 % 4 == 0) || (y / 50 % 4 == 0)) {
            return {x, y};
        }
    }
}

// Function to get a random building square position (top-left corner, adjacent to road)
Position getRandomBuildingPosition() {
    // Building squares start at (50, 50), (250, 50), ..., (650, 50), etc.
    int buildingX[] = {50, 250, 450, 650};
    int buildingY[] = {50, 250, 450, 650};
    int i = rand() % 4; // Random row
    int j = rand() % 4; // Random column
    return {buildingX[i], buildingY[j]};
}

class Roads {
public:
    void drawRoads() {
        // 17x17 grid, each cell 50x50 pixels
        for (int i = 0; i < 17; ++i) {
            for (int j = 0; j < 17; ++j) {
                // Roads on rows and columns where index % 4 == 0
                if (i % 4 == 0 || j % 4 == 0) {
                    int x = i * 50;
                    int y = j * 50;
                    DrawSquare(x, y, 50, colors[WHITE]);
                }
            }
        }
    }
};

Roads roads;

class FuelStation {
private:
    int x, y;
public:
    FuelStation(int startX, int startY) : x(startX), y(startY) {}

    void draw() const {
        DrawSquare(x, y, 50, colors[ORANGE]); // 50x50 orange square
    }

    int getX() const { return x; }
    int getY() const { return y; }
};

class Vehicle {   
public:
    int x, y;
    float* color;
    Vehicle(int startX = 0, int startY = 0, float* startColor = colors[BLACK])
        : x(startX), y(startY), color(startColor) {}

    virtual void move() = 0;
    virtual void draw() const = 0;
};

class PlayerCar : public Vehicle {
protected:
    float fuel;
    float money;

public:
    PlayerCar(int startX = 0, int startY = 0, float* startColor = colors[BLACK], float startFuel = 100.0, float startMoney = 0.0)
        : Vehicle(startX, startY, startColor), fuel(startFuel), money(startMoney) {}

    virtual void pickUp() = 0;
    virtual void dropOff() = 0;

    void move() override {
        // Move based on arrow keys handled in NonPrintableKeys
    }

    void draw() const override {
        DrawRoundRect(x, y, 20, 40, colors[DARK_SEA_GREEN], 10);
        DrawCircle(x + 2, y + 4, 3, colors[BLACK]);
        DrawCircle(x + 2, y + 32, 3, colors[BLACK]);
        DrawCircle(x + 18, y + 4, 3, colors[BLACK]);
        DrawCircle(x + 18, y + 32, 3, colors[BLACK]);
    }

    float getFuel() const { return fuel; }
    void setFuel(float newFuel) { fuel = newFuel > 0 ? newFuel : 0; }
    float getMoney() const { return money; }
    void addMoney(float amount) { money += amount; }
};

class Taxi : public PlayerCar {
public:
    Taxi(int startX = 0, int startY = 0, float* startColor = colors[YELLOW], float startFuel = 100.0, float startMoney = 0.0)
        : PlayerCar(startX, startY, startColor, startFuel, startMoney) {}

    void pickUp() override {
        if (fuel > 0) {
            cout << "Passenger picked up!" << endl;
            fuel -= 1;
        }
    }

    void dropOff() override {
        if (fuel > 0) {
            cout << "Passenger dropped off! Earned 10 units." << endl;
            addMoney(10);
            fuel -= 1;
        }
    }
};

class DeliveryCar : public PlayerCar {
public:
    DeliveryCar(int startX = 0, int startY = 0, float* startColor = colors[BLUE], float startFuel = 100.0, float startMoney = 0.0)
        : PlayerCar(startX, startY, startColor, startFuel, startMoney) {}

    void pickUp() override {
        if (fuel > 0) {
            cout << "Package picked up!" << endl;
            fuel -= 5;
        }
    }

    void dropOff() override {
        if (fuel > 0) {
            cout << "Package delivered! Earned 20 units." << endl;
            addMoney(20);
            fuel -= 5;
        }
    }
};

class OtherCar : public Vehicle {
private:
    int direction; // 0: up, 1: down, 2: left, 3: right
    static const int MOVE_SPEED = 2;

public:
    OtherCar(int startX = 42, int startY = 42, float* startColor = colors[GREEN])
        : Vehicle(startX, startY, startColor), direction(rand() % 4) {}

    void move() override {
        int new_x = x;
        int new_y = y;
        switch (direction) {
            case 0: new_y += MOVE_SPEED; break; // up
            case 1: new_y -= MOVE_SPEED; break; // down
            case 2: new_x -= MOVE_SPEED; break; // left
            case 3: new_x += MOVE_SPEED; break; // right
        }
        // Boundary checks
        if (new_x >= 0 && new_x <= 820 && new_y >= 0 && new_y <= 800) {
            // Road check using integer division
            if ((new_x / 50 % 4 == 0) || (new_y / 50 % 4 == 0)) {
                x = new_x;
                y = new_y;
            }
        }
    }

    void draw() const override {
        DrawRoundRect(x, y, 20, 40, colors[VIOLET], 10);
        DrawCircle(x + 2, y + 4, 3, colors[BLACK]);
        DrawCircle(x + 2, y + 32, 3, colors[BLACK]);
        DrawCircle(x + 18, y + 4, 3, colors[BLACK]);
        DrawCircle(x + 18, y + 32, 3, colors[BLACK]);
    }
};

// Global instances
Taxi playerTaxi(0, 800);
OtherCar otherCar, otherCar2, otherCar3, otherCar4;
FuelStation* fuelStations[3]; // Array of pointers to avoid default constructor issue

void SetCanvasSize(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int xI = 400, yI = 400;

void drawCar() {
    playerTaxi.draw();
    otherCar.draw();
    otherCar2.draw();
    otherCar3.draw();
    otherCar4.draw();
}

bool flag = true;
void moveCar() {
    otherCar.move();
    otherCar2.move();
    otherCar3.move();
    otherCar4.move();
}

void GameDisplay() {
    glClearColor(0.2, 0.2, 0.2, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    roads.drawRoads();
    // Draw fuel stations
    for (int i = 0; i < 3; i++) {
        fuelStations[i]->draw();
    }
    DrawString(50, 800, "Score=0", colors[RED]);
    DrawString(700, 800, "Fuel=" + to_string(static_cast<int>(playerTaxi.getFuel())), colors[BLUE]);
    DrawString(350, 800, "Money=0", colors[GREEN]);
    drawCar();
    glutSwapBuffers();
}

void NonPrintableKeys(int key, int x, int y) {
    int new_x = playerTaxi.x;
    int new_y = playerTaxi.y;
    if (key == GLUT_KEY_LEFT) {
        new_x -= 10;
        playerTaxi.setFuel(playerTaxi.getFuel() - 0.25);
    } else if (key == GLUT_KEY_RIGHT) {
        new_x += 10;
        playerTaxi.setFuel(playerTaxi.getFuel() - 0.25);
    } else if (key == GLUT_KEY_UP) {
        new_y += 10;
        playerTaxi.setFuel(playerTaxi.getFuel() - 0.25);
    } else if (key == GLUT_KEY_DOWN) {
        new_y -= 10;
        playerTaxi.setFuel(playerTaxi.getFuel() - 0.25);
    }
    // Boundary checks
    if (new_x >= 0 && new_x <= 820 && new_y >= 0 && new_y <= 800) {
        // Road check using integer division
        if ((new_x / 50 % 4 == 0) || (new_y / 50 % 4 == 0)) {
            playerTaxi.x = new_x;
            playerTaxi.y = new_y;
        }
    }
    glutPostRedisplay();
}

void PrintableKeys(unsigned char key, int x, int y) {
    if (key == 27) {
        exit(1);
    }
    if (key == 'b' || key == 'B') {
        cout << "b pressed" << endl;
    }
    if (key == 'p' || key == 'P') {
        playerTaxi.pickUp();
    }
    if (key == 'd' || key == 'D') {
        playerTaxi.dropOff();
    }
    if (key == 'f' || key == 'F') {
        // Check if near a fuel station
        for (int i = 0; i < 3; i++) {
            if (abs(playerTaxi.x - fuelStations[i]->getX()) <= 50 && 
                abs(playerTaxi.y - fuelStations[i]->getY()) <= 50) {
                if (playerTaxi.getMoney() >= 10) {
                    playerTaxi.setFuel(100);
                    playerTaxi.addMoney(-10);
                    cout << "Refueled! Spent 10 units." << endl;
                } else {
                    cout << "Not enough money to refuel!" << endl;
                }
                break;
            }
        }
    }
    glutPostRedisplay();
}

void Timer(int m) {
    moveCar();
    glutTimerFunc(100, Timer, 0);
}

void MousePressedAndMoved(int x, int y) {
    cout << x << " " << y << endl;
    glutPostRedisplay();
}

void MouseMoved(int x, int y) {
    glutPostRedisplay();
}

void MouseClicked(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        cout << GLUT_DOWN << " " << GLUT_UP << endl;
    } else if (button == GLUT_RIGHT_BUTTON) {
        cout << "Right Button Pressed" << endl;
    }
    glutPostRedisplay();
}

int main(int argc, char* argv[]) {
    int width = 840, height = 840;
    InitRandomizer();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(50, 50);
    glutInitWindowSize(width, height);
    glutCreateWindow("OOP Project");
    SetCanvasSize(width, height);

    // Initialize positions for other cars and fuel stations
    Position pos;
    Position occupied[8]; // Store occupied building positions
    int occupiedCount = 0;

    // OtherCar
    do {
        pos = getRandomRoadPosition();
    } while (pos.x == playerTaxi.x && pos.y == playerTaxi.y);
    otherCar.x = pos.x;
    otherCar.y = pos.y;

    // OtherCar2
    do {
        pos = getRandomRoadPosition();
    } while ((pos.x == playerTaxi.x && pos.y == playerTaxi.y) || 
             (pos.x == otherCar.x && pos.y == otherCar.y));
    otherCar2.x = pos.x;
    otherCar2.y = pos.y;

    // OtherCar3
    do {
        pos = getRandomRoadPosition();
    } while ((pos.x == playerTaxi.x && pos.y == playerTaxi.y) || 
             (pos.x == otherCar.x && pos.y == otherCar.y) || 
             (pos.x == otherCar2.x && pos.y == otherCar2.y));
    otherCar3.x = pos.x;
    otherCar3.y = pos.y;

    // OtherCar4
    do {
        pos = getRandomRoadPosition();
    } while ((pos.x == playerTaxi.x && pos.y == playerTaxi.y) || 
             (pos.x == otherCar.x && pos.y == otherCar.y) || 
             (pos.x == otherCar2.x && pos.y == otherCar2.y) || 
             (pos.x == otherCar3.x && pos.y == otherCar3.y));
    otherCar4.x = pos.x;
    otherCar4.y = pos.y;

    // Fuel stations
    for (int i = 0; i < 3; i++) {
        bool valid = false;
        do {
            pos = getRandomBuildingPosition();
            valid = true;
            // Check for overlap with other fuel stations
            for (int j = 0; j < occupiedCount; j++) {
                if (pos.x == occupied[j].x && pos.y == occupied[j].y) {
                    valid = false;
                    break;
                }
            }
            // Check for overlap with cars (within 50x50)
            if (valid) {
                if ((abs(pos.x - playerTaxi.x) < 50 && abs(pos.y - playerTaxi.y) < 50) ||
                    (abs(pos.x - otherCar.x) < 50 && abs(pos.y - otherCar.y) < 50) ||
                    (abs(pos.x - otherCar2.x) < 50 && abs(pos.y - otherCar2.y) < 50) ||
                    (abs(pos.x - otherCar3.x) < 50 && abs(pos.y - otherCar3.y) < 50) ||
                    (abs(pos.x - otherCar4.x) < 50 && abs(pos.y - otherCar4.y) < 50)) {
                    valid = false;
                }
            }
        } while (!valid);
        fuelStations[i] = new FuelStation(pos.x, pos.y);
        occupied[occupiedCount++] = pos;
    }

    glutDisplayFunc(GameDisplay);
    glutSpecialFunc(NonPrintableKeys);
    glutKeyboardFunc(PrintableKeys);
    glutTimerFunc(1000.0, Timer, 0);
    glutMouseFunc(MouseClicked);
    glutPassiveMotionFunc(MouseMoved);
    glutMotionFunc(MousePressedAndMoved);

    glutMainLoop();

    // Clean up fuel stations
    for (int i = 0; i < 3; i++) {
        delete fuelStations[i];
    }

    return 1;
}
#endif /* RushHour_CPP_ */