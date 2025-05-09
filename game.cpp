//============================================================================
// Name        : RushHour.cpp
// Author      : Dr. Sibt Ul Hussain
// Version     :
// Copyright   : (c) Reserved
// Description : Basic 2D game with boundary and road constraints, resized to 680x720 with 40x40 cells
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

// Helper function to check if a cell is a road cell
bool isRoadCell(int i, int j) {
    return (i == 0 || i == 4 || i == 8 || i == 12 || i == 16 ||
            j == 0 || j == 4 || j == 8 || j == 12 || j == 16);
}

// Function to generate a random position on a road
Position getRandomRoadPosition() {
    while (true) {
        int i = rand() % 17; // 0 to 16
        int j = rand() % 17;
        if (isRoadCell(i, j)) {
            int x = i * 40;
            int y = j * 40;
            return {x, y};
        }
    }
}

// Function to get a random adjacent building cell position
Position getRandomAdjacentBuildingPosition(const Position* occupied, int count) {
    while (true) {
        int i = rand() % 17;
        int j = rand() % 17;
        if (!isRoadCell(i, j)) {
            // Check if adjacent to a road
            bool adjacent = false;
            if (i > 0 && isRoadCell(i - 1, j)) adjacent = true;
            else if (i < 16 && isRoadCell(i + 1, j)) adjacent = true;
            else if (j > 0 && isRoadCell(i, j - 1)) adjacent = true;
            else if (j < 16 && isRoadCell(i, j + 1)) adjacent = true;
            if (adjacent) {
                int x = i * 40;
                int y = j * 40;
                bool available = true;
                for (int k = 0; k < count; k++) {
                    if (occupied[k].x == x && occupied[k].y == y) {
                        available = false;
                        break;
                    }
                }
                if (available) {
                    return {x, y};
                }
            }
        }
    }
}

class Roads {
public:
    void drawRoads() {
        for (int i = 0; i < 17; ++i) { // 17x17 grid
            for (int j = 0; j < 17; ++j) {
                if (isRoadCell(i, j)) {
                    int x = i * 40;
                    int y = j * 40;
                    DrawSquare(x, y, 40, colors[WHITE]);
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
        DrawSquare(x, y, 40, colors[ORANGE]);
    }

    int getX() const { return x; }
    int getY() const { return y; }
};

class Passenger {
private:
    int x, y;
    bool active;
public:
    Passenger(int startX, int startY) : x(startX), y(startY), active(true) {}

    void draw() const {
        if (active) {
            DrawCircle(x + 20, y + 30, 5, colors[RED]);
            DrawLine(x + 20, y + 25, x + 20, y + 10, 2, colors[BLUE]);
            DrawLine(x + 20, y + 20, x + 15, y + 15, 2, colors[BLUE]);
            DrawLine(x + 20, y + 20, x + 25, y + 15, 2, colors[BLUE]);
            DrawLine(x + 20, y + 10, x + 15, y + 5, 2, colors[BLUE]);
            DrawLine(x + 20, y + 10, x + 25, y + 5, 2, colors[BLUE]);
        }
    }

    int getX() const { return x; }
    int getY() const { return y; }
    bool isActive() const { return active; }
    void setActive(bool status) { active = status; }
    void setPosition(int newX, int newY) { x = newX; y = newY; }
};

class Destination {
private:
    int x, y;
    bool active;
public:
    Destination(int startX = 0, int startY = 0) : x(startX), y(startY), active(false) {}

    void draw() const {
        if (active) {
            DrawSquare(x, y, 40, colors[GREEN]);
        }
    }

    void setPosition(int newX, int newY) { x = newX; y = newY; active = true; }
    int getX() const { return x; }
    int getY() const { return y; }
    bool isActive() const { return active; }
    void setActive(bool status) { active = status; }
};

class GameState {
private:
    Passenger* passengers[4];
    int activePassengers;
    FuelStation* fuelStations[3];

public:
    GameState() : activePassengers(0) {
        for (int i = 0; i < 4; i++) passengers[i] = nullptr;
        for (int i = 0; i < 3; i++) fuelStations[i] = nullptr;
    }

    ~GameState() {
        for (int i = 0; i < 4; i++) delete passengers[i];
        for (int i = 0; i < 3; i++) delete fuelStations[i];
    }

    void setPassenger(int index, Passenger* passenger) {
        if (index >= 0 && index < 4) passengers[index] = passenger;
    }

    Passenger* getPassenger(int index) const {
        if (index >= 0 && index < 4) return passengers[index];
        return nullptr;
    }

    void setActivePassengers(int count) {
        if (count >= 0 && count <= 4) activePassengers = count;
    }

    int getActivePassengers() const { return activePassengers; }

    void setFuelStation(int index, FuelStation* station) {
        if (index >= 0 && index < 3) fuelStations[index] = station;
    }

    FuelStation* getFuelStation(int index) const {
        if (index >= 0 && index < 3) return fuelStations[index];
        return nullptr;
    }
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
    int score;

public:
    PlayerCar(int startX = 0, int startY = 0, float* startColor = colors[BLACK], float startFuel = 100.0, float startMoney = 500.0)
        : Vehicle(startX, startY, startColor), fuel(startFuel), money(startMoney), score(0) {}

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
    void setMoney(float newMoney) { money = newMoney > 0 ? newMoney : 0; }
    void addMoney(float amount) { money += amount; if (money < 0) money = 0; }
    int getScore() const { return score; }
    void addScore(int points) { score += points; }
};

class Taxi : public PlayerCar {
private:
    bool hasPassenger;
    Destination* destination;
    GameState& gameState;

public:
    Taxi(int startX, int startY, float* startColor, float startFuel, float startMoney, GameState& gs)
        : PlayerCar(startX, startY, startColor, startFuel, startMoney), hasPassenger(false), destination(new Destination()), gameState(gs) {}

    ~Taxi() { delete destination; }

    void refuel() {
        if (money >= 1) {
            float fuelToAdd = 5; // 1 money = 5 fuel
            setFuel(getFuel() + fuelToAdd);
            addMoney(-1);
            cout << "Refueled! +5 fuel, -1 money." << endl;
        } else {
            cout << "Not enough money to refuel!" << endl;
        }
    }

    void pickUp() override {
        if (fuel > 0 && !hasPassenger) {
            for (int i = 0; i < gameState.getActivePassengers(); i++) {
                Passenger* p = gameState.getPassenger(i);
                if (p && p->isActive() && 
                    abs(x - p->getX()) <= 40 && 
                    abs(y - p->getY()) <= 40) {
                    p->setActive(false);
                    hasPassenger = true;
                    Position destPos = getRandomAdjacentBuildingPosition(nullptr, 0);
                    destination->setPosition(destPos.x, destPos.y);
                    cout << "Passenger picked up!" << endl;
                    fuel -= 1;
                    break;
                }
            }
        }
    }

    void dropOff() override {
        if (fuel > 0 && hasPassenger && destination->isActive() &&
            abs(x - destination->getX()) <= 40 && 
            abs(y - destination->getY()) <= 40) {
            hasPassenger = false;
            destination->setActive(false);
            addScore(10);
            addMoney(10);
            cout << "Passenger dropped off! +10 score, +10 money." << endl;
            fuel -= 1;
            // Spawn new passenger
            for (int i = 0; i < gameState.getActivePassengers(); i++) {
                Passenger* p = gameState.getPassenger(i);
                if (p && !p->isActive()) {
                    Position newPos = getRandomAdjacentBuildingPosition(nullptr, 0);
                    p->setPosition(newPos.x, newPos.y);
                    p->setActive(true);
                    break;
                }
            }
        }
    }

    bool hasPassengerStatus() const { return hasPassenger; }
    Destination* getDestination() const { return destination; }
    GameState& getGameState() const { return gameState; }
};

class DeliveryCar : public PlayerCar {
public:
    DeliveryCar(int startX = 0, int startY = 0, float* startColor = colors[BLUE], float startFuel = 100.0, float startMoney = 500.0)
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
            case 0: new_y += MOVE_SPEED; break;
            case 1: new_y -= MOVE_SPEED; break;
            case 2: new_x -= MOVE_SPEED; break;
            case 3: new_x += MOVE_SPEED; break;
        }
        if (new_x >= 0 && new_x <= 660 && new_y >= 0 && new_y <= 640) {
            int cell_i = new_x / 40;
            int cell_j = new_y / 40;
            if (cell_i >= 0 && cell_i < 17 && cell_j >= 0 && cell_j < 17 && isRoadCell(cell_i, cell_j)) {
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
GameState gameState;
Taxi playerTaxi(0, 640, colors[YELLOW], 100.0, 500.0, gameState);
OtherCar otherCar, otherCar2, otherCar3, otherCar4;

void SetCanvasSize(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int xI = 340, yI = 360; // Center of 680x720

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
    for (int i = 0; i < 3; i++) {
        FuelStation* fs = gameState.getFuelStation(i);
        if (fs) fs->draw();
    }
    for (int i = 0; i < gameState.getActivePassengers(); i++) {
        Passenger* p = gameState.getPassenger(i);
        if (p) p->draw();
    }
    playerTaxi.getDestination()->draw();
    DrawString(50, 700, "Score=" + to_string(playerTaxi.getScore()), colors[RED]);
    DrawString(510, 700, "Fuel=" + to_string(static_cast<int>(playerTaxi.getFuel())), colors[BLUE]);
    DrawString(290, 700, "Money=" + to_string(static_cast<int>(playerTaxi.getMoney())), colors[GREEN]);
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
    if (new_x >= 0 && new_x <= 660 && new_y >= 0 && new_y <= 640) {
        int cell_i = new_x / 40;
        int cell_j = new_y / 40;
        if (cell_i >= 0 && cell_i < 17 && cell_j >= 0 && cell_j < 17 && isRoadCell(cell_i, cell_j)) {
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
    if (key == ' ') { // Spacebar for refueling
        for (int i = 0; i < 3; i++) {
            FuelStation* fs = gameState.getFuelStation(i);
            if (fs && abs(playerTaxi.x - fs->getX()) <= 40 && 
                abs(playerTaxi.y - fs->getY()) <= 40) {
                playerTaxi.refuel();
                break;
            }
        }
    }
    if (key == 13) { // Enter key for pickup/drop-off
        if (!playerTaxi.hasPassengerStatus()) {
            playerTaxi.pickUp();
        } else {
            playerTaxi.dropOff();
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
    int width = 680, height = 720;
    InitRandomizer();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(50, 50);
    glutInitWindowSize(width, height);
    glutCreateWindow("OOP Project");
    SetCanvasSize(width, height);

    Position pos;
    Position occupied[20];
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
        do {
            pos = getRandomAdjacentBuildingPosition(occupied, occupiedCount);
            bool valid = true;
            for (int j = 0; j < occupiedCount; j++) {
                if (pos.x == occupied[j].x && pos.y == occupied[j].y) {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                gameState.setFuelStation(i, new FuelStation(pos.x, pos.y));
                occupied[occupiedCount++] = pos;
                break;
            }
        } while (true);
    }

    // Passengers
    int activePassengers = 2 + rand() % 3; // 2, 3, or 4 passengers
    gameState.setActivePassengers(activePassengers);
    for (int i = 0; i < activePassengers; i++) {
        do {
            pos = getRandomAdjacentBuildingPosition(occupied, occupiedCount);
            bool valid = true;
            for (int j = 0; j < occupiedCount; j++) {
                if (pos.x == occupied[j].x && pos.y == occupied[j].y) {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                gameState.setPassenger(i, new Passenger(pos.x, pos.y));
                occupied[occupiedCount++] = pos;
                break;
            }
        } while (true);
    }

    glutDisplayFunc(GameDisplay);
    glutSpecialFunc(NonPrintableKeys);
    glutKeyboardFunc(PrintableKeys);
    glutTimerFunc(1000.0, Timer, 0);
    glutMouseFunc(MouseClicked);
    glutPassiveMotionFunc(MouseMoved);
    glutMotionFunc(MousePressedAndMoved);

    glutMainLoop();

    return 1;
}
#endif /* RushHour_CPP_ */