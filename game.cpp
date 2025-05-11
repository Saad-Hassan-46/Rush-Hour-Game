#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <GL/glut.h>
#include "util.h"
#include <iostream>
#include <string>
#include <cmath>
#include <fstream>
using namespace std;

// Audio variables
Mix_Music* gMenuMusic = NULL;
Mix_Music* gGameMusic = NULL;
Mix_Chunk* gCollisionSound = NULL;
Mix_Chunk* gDestinationSound = NULL;
Mix_Chunk* gRefuellingSound = NULL;

// Struct definitions
struct HighScore {
    char name[20];
    int score;
};

struct Position {
    int x, y;
};

// Class definitions
class Vehicle {
public:
    int x, y;
    float* color;
    Vehicle(int startX = 0, int startY = 0, float* startColor = colors[BLACK])
        : x(startX), y(startY), color(startColor) {}
    virtual void move() = 0;
    virtual void draw() const = 0;
};

// Collision detection function
bool collides(const Vehicle& v1, const Vehicle& v2) {
    return v1.x < v2.x + 20 && v1.x + 20 > v2.x &&
           v1.y < v2.y + 40 && v1.y + 40 > v2.y;
}

// Helper functions
bool isRoadCell(int i, int j) {
    return (i == 0 || i == 4 || i == 8 || i == 12 || i == 16 ||
            j == 0 || j == 4 || j == 8 || j == 12 || j == 16);
}

Position getRandomRoadPosition() {
    while (true) {
        int i = rand() % 17;
        int j = rand() % 17;
        if (isRoadCell(i, j)) {
            return {i * 40, j * 40};
        }
    }
}

Position getRandomAdjacentBuildingPosition(const Position* occupied, int count) {
    while (true) {
        int i = rand() % 17;
        int j = rand() % 17;
        if (!isRoadCell(i, j)) {
            bool adjacent = (i > 0 && isRoadCell(i - 1, j)) ||
                           (i < 16 && isRoadCell(i + 1, j)) ||
                           (j > 0 && isRoadCell(i, j - 1)) ||
                           (j < 16 && isRoadCell(i, j + 1));
            if (adjacent) {
                int x = i * 40, y = j * 40;
                bool available = true;
                for (int k = 0; k < count; k++) {
                    if (occupied[k].x == x && occupied[k].y == y) {
                        available = false;
                        break;
                    }
                }
                if (available) return {x, y};
            }
        }
    }
}

class Roads {
public:
    void drawRoads() {
        for (int i = 0; i < 17; ++i) {
            for (int j = 0; j < 17; ++j) {
                if (isRoadCell(i, j)) {
                    DrawSquare(i * 40, j * 40, 40, colors[WHITE]);
                }
            }
        }
    }
};

class FuelStation {
private:
    int x, y;
public:
    FuelStation(int startX, int startY) : x(startX), y(startY) {}
    void draw() const { DrawSquare(x, y, 40, colors[ORANGE]); }
    int getX() const { return x; }
    int getY() const { return y; }
};

class PickupItem {
public:
    virtual ~PickupItem() {}
    virtual void draw() const = 0;
    virtual int getX() const = 0;
    virtual int getY() const = 0;
    virtual bool isActive() const = 0;
    virtual void setActive(bool status) = 0;
    virtual void setPosition(int newX, int newY) = 0;
};

class Passenger : public PickupItem {
private:
    int x, y;
    bool active;
public:
    Passenger(int startX, int startY) : x(startX), y(startY), active(true) {}
    void draw() const override {
        if (active) {
            DrawCircle(x + 20, y + 30, 5, colors[RED]);
            DrawLine(x + 20, y + 25, x + 20, y + 10, 2, colors[BLUE]);
            DrawLine(x + 20, y + 20, x + 15, y + 15, 2, colors[BLUE]);
            DrawLine(x + 20, y + 20, x + 25, y + 15, 2, colors[BLUE]);
            DrawLine(x + 20, y + 10, x + 15, y + 5, 2, colors[BLUE]);
            DrawLine(x + 20, y + 10, x + 25, y + 5, 2, colors[BLUE]);
        }
    }
    int getX() const override { return x; }
    int getY() const override { return y; }
    bool isActive() const override { return active; }
    void setActive(bool status) override { active = status; }
    void setPosition(int newX, int newY) override { x = newX; y = newY; }
};

class Box : public PickupItem {
private:
    int x, y;
    bool active;
public:
    Box(int startX, int startY) : x(startX), y(startY), active(true) {}
    void draw() const override {
        if (active) {
            DrawSquare(x + 10, y + 10, 20, colors[BROWN]);
        }
    }
    int getX() const override { return x; }
    int getY() const override { return y; }
    bool isActive() const override { return active; }
    void setActive(bool status) override { active = status; }
    void setPosition(int newX, int newY) override { x = newX; y = newY; }
};

class Destination {
private:
    int x, y;
    bool active;
public:
    Destination(int startX = 0, int startY = 0) : x(startX), y(startY), active(false) {}
    void draw() const { if (active) DrawSquare(x, y, 40, colors[GREEN]); }
    void setPosition(int newX, int newY) { x = newX; y = newY; active = true; }
    int getX() const { return x; }
    int getY() const { return y; }
    bool isActive() const { return active; }
    void setActive(bool status) { active = status; }
};

class GameState {
private:
    PickupItem* pickupItems[4];
    int activePickupItems;
    FuelStation* fuelStations[3];
public:
    GameState() : activePickupItems(0) {
        for (int i = 0; i < 4; i++) pickupItems[i] = nullptr;
        for (int i = 0; i < 3; i++) fuelStations[i] = nullptr;
    }
    ~GameState() {
        for (int i = 0; i < 4; i++) delete pickupItems[i];
        for (int i = 0; i < 3; i++) delete fuelStations[i];
    }
    void setPickupItem(int index, PickupItem* item) { if (index >= 0 && index < 4) pickupItems[index] = item; }
    PickupItem* getPickupItem(int index) const { if (index >= 0 && index < 4) return pickupItems[index]; return nullptr; }
    void setActivePickupItems(int count) { if (count >= 0 && count <= 4) activePickupItems = count; }
    int getActivePickupItems() const { return activePickupItems; }
    void setFuelStation(int index, FuelStation* station) { if (index >= 0 && index < 3) fuelStations[index] = station; }
    FuelStation* getFuelStation(int index) const { if (index >= 0 && index < 3) return fuelStations[index]; return nullptr; }
};

class PlayerCar : public Vehicle {
protected:
    float fuel;
    float money;
    int score;
public:
    PlayerCar(int startX = 0, int startY = 0, float* startColor = colors[BLACK], float startFuel = 100.0, float startMoney = 0.0)
        : Vehicle(startX, startY, startColor), fuel(startFuel), money(startMoney), score(0) {}
    virtual void pickUp() = 0;
    virtual bool dropOff() = 0;
    virtual void drawDestination() const {}
    bool refuel() {
        if (money >= 1) {
            setFuel(getFuel() + 2);
            addMoney(-1);
            cout << "Refueled! +2 fuel, -1 money." << endl;
            return true;
        } else {
            cout << "Not enough money to refuel!" << endl;
            return false;
        }
    }
    void move() override {}
    void draw() const override {
        DrawRoundRect(x, y, 20, 40, color, 10);
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
    void pickUp() override {
        if (fuel > 0 && !hasPassenger) {
            for (int i = 0; i < gameState.getActivePickupItems(); i++) {
                PickupItem* p = gameState.getPickupItem(i);
                if (p && p->isActive() && abs(x - p->getX()) <= 40 && abs(y - p->getY()) <= 40) {
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
    bool dropOff() override {
        if (fuel > 0 && hasPassenger && destination->isActive() &&
            abs(x - destination->getX()) <= 40 && abs(y - destination->getY()) <= 40) {
            hasPassenger = false;
            destination->setActive(false);
            addScore(20);
            addMoney(20);
            cout << "Passenger dropped off! +20 score, +20 money." << endl;
            fuel -= 1;
            for (int i = 0; i < gameState.getActivePickupItems(); i++) {
                PickupItem* p = gameState.getPickupItem(i);
                if (p && !p->isActive()) {
                    Position newPos = getRandomAdjacentBuildingPosition(nullptr, 0);
                    p->setPosition(newPos.x, newPos.y);
                    p->setActive(true);
                    break;
                }
            }
            return true;
        }
        return false;
    }
    void drawDestination() const override {
        if (hasPassenger && destination->isActive()) {
            destination->draw();
        }
    }
    bool hasPassengerStatus() const { return hasPassenger; }
};

class DeliveryCar : public PlayerCar {
private:
    bool hasPackage;
    Destination* destination;
    GameState& gameState;
public:
    DeliveryCar(int startX, int startY, float* startColor, float startFuel, float startMoney, GameState& gs)
        : PlayerCar(startX, startY, startColor, startFuel, startMoney), hasPackage(false), destination(new Destination()), gameState(gs) {}
    ~DeliveryCar() { delete destination; }
    void pickUp() override {
        if (fuel > 0 && !hasPackage) {
            for (int i = 0; i < gameState.getActivePickupItems(); i++) {
                PickupItem* p = gameState.getPickupItem(i);
                if (p && p->isActive() && abs(x - p->getX()) <= 40 && abs(y - p->getY()) <= 40) {
                    p->setActive(false);
                    hasPackage = true;
                    Position destPos = getRandomAdjacentBuildingPosition(nullptr, 0);
                    destination->setPosition(destPos.x, destPos.y);
                    cout << "Package picked up!" << endl;
                    fuel -= 1;
                    break;
                }
            }
        }
    }
    bool dropOff() override {
        if (fuel > 0 && hasPackage && destination->isActive() &&
            abs(x - destination->getX()) <= 40 && abs(y - destination->getY()) <= 40) {
            hasPackage = false;
            destination->setActive(false);
            addScore(20);
            addMoney(20);
            cout << "Package delivered! +20 score, +20 money." << endl;
            fuel -= 1;
            for (int i = 0; i < gameState.getActivePickupItems(); i++) {
                PickupItem* p = gameState.getPickupItem(i);
                if (p && !p->isActive()) {
                    Position newPos = getRandomAdjacentBuildingPosition(nullptr, 0);
                    p->setPosition(newPos.x, newPos.y);
                    p->setActive(true);
                    break;
                }
            }
            return true;
        }
        return false;
    }
    void drawDestination() const override {
        if (hasPackage && destination->isActive()) {
            destination->draw();
        }
    }
    bool hasPackageStatus() const { return hasPackage; }
};

class OtherCar : public Vehicle {
private:
    int direction;
    static const int MOVE_SPEED = 2;
public:
    OtherCar(int startX = 42, int startY = 42, float* startColor = colors[GREEN])
        : Vehicle(startX, startY, startColor), direction(rand() % 4) {}
    void move() override {
        int current_i = x / 40;
        int current_j = y / 40;
        int new_x = x, new_y = y;
        switch (direction) {
            case 0: new_y += MOVE_SPEED; break;
            case 1: new_y -= MOVE_SPEED; break;
            case 2: new_x -= MOVE_SPEED; break;
            case 3: new_x += MOVE_SPEED; break;
        }
        if (new_x >= 0 && new_x <= 660 && new_y >= 0 && new_y <= 640) {
            int new_i = new_x / 40;
            int new_j = new_y / 40;
            if (new_i >= 0 && new_i < 17 && new_j >= 0 && new_j < 17 && isRoadCell(new_i, new_j)) {
                x = new_x;
                y = new_y;
                if (new_i != current_i || new_j != current_j) {
                    if (new_i % 4 == 0 && new_j % 4 == 0) {
                        int opposite = (direction + 2) % 4;
                        int new_dir;
                        do {
                            new_dir = rand() % 4;
                        } while (new_dir == opposite);
                        direction = new_dir;
                    }
                }
            } else {
                direction = rand() % 4;
            }
        } else {
            direction = rand() % 4;
        }
    }
    void resetPosition(const Vehicle* player, const OtherCar* const* others, int numOthers) {
        Position pos;
        do {
            pos = getRandomRoadPosition();
            bool valid = true;
            if (pos.x == player->x && pos.y == player->y) {
                valid = false;
            }
            for (int i = 0; i < numOthers; i++) {
                if (others[i] != this && pos.x == others[i]->x && pos.y == others[i]->y) {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                x = pos.x;
                y = pos.y;
                direction = rand() % 4;
                break;
            }
        } while (true);
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
HighScore highScores[10];
int numHighScores = 0;
string playerName;
PlayerCar* player = nullptr;
bool gameOver = false;
OtherCar otherCar, otherCar2, otherCar3, otherCar4;
Roads roads;
int startTime;
bool isWin = false;

// Function prototypes
void GameDisplay();
void NonPrintableKeys(int key, int x, int y);
void PrintableKeys(unsigned char key, int x, int y);
void Timer(int m);
void MousePressedAndMoved(int x, int y);
void MouseMoved(int x, int y);
void MouseClicked(int button, int state, int x, int y);
void sortHighScores(HighScore hs[], int n);

// High score functions
void loadHighScores() {
    ifstream file("highscores.txt", ios::binary);
    if (file.is_open()) {
        numHighScores = 0;
        while (numHighScores < 10 && file.read((char*)&highScores[numHighScores], sizeof(HighScore))) {
            highScores[numHighScores].name[19] = '\0';
            numHighScores++;
        }
        file.close();
        sortHighScores(highScores, numHighScores);
        cout << "Loaded " << numHighScores << " high scores from highscores.txt" << endl;
    } else {
        cout << "No highscores.txt found, starting with empty leaderboard" << endl;
        numHighScores = 0;
    }
}

void saveHighScores() {
    ofstream file("highscores.txt", ios::binary | ios::trunc);
    if (!file.is_open()) {
        cerr << "Error: Could not open highscores.txt for writing" << endl;
        return;
    }
    for (int i = 0; i < numHighScores; i++) {
        file.write((char*)&highScores[i], sizeof(HighScore));
    }
    file.close();
    cout << "Saved " << numHighScores << " high scores to highscores.txt" << endl;
}

void displayLeaderboard() {
    cout << "Leaderboard:\n";
    if (numHighScores == 0) {
        cout << "No high scores yet!" << endl;
    } else {
        for (int i = 0; i < numHighScores; i++) {
            cout << i + 1 << ". " << highScores[i].name << " - " << highScores[i].score << endl;
        }
    }
}

void sortHighScores(HighScore hs[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (hs[j].score < hs[j + 1].score) {
                HighScore temp = hs[j];
                hs[j] = hs[j + 1];
                hs[j + 1] = temp;
            }
        }
    }
}

void SetCanvasSize(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void drawCar() {
    player->draw();
    otherCar.draw();
    otherCar2.draw();
    otherCar3.draw();
    otherCar4.draw();
}

void moveCar() {
    otherCar.move();
    otherCar2.move();
    otherCar3.move();
    otherCar4.move();
}

void GameDisplay() {
    glClearColor(0.2, 0.2, 0.2, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    if (gameOver) {
        if (isWin) {
            DrawString(200, 360, "You Win! Your score: " + to_string(player->getScore()), colors[GREEN]);
        } else {
            DrawString(200, 360, "Game Over! Your score: " + to_string(player->getScore()), colors[RED]);
        }
        DrawString(200, 340, "Press any key to exit.", colors[RED]);
    } else {
        roads.drawRoads();
        for (int i = 0; i < 3; i++) {
            FuelStation* fs = gameState.getFuelStation(i);
            if (fs) fs->draw();
        }
        for (int i = 0; i < gameState.getActivePickupItems(); i++) {
            PickupItem* p = gameState.getPickupItem(i);
            if (p) p->draw();
        }
        player->drawDestination();
        int currentTime = glutGet(GLUT_ELAPSED_TIME);
        int elapsedTime = (currentTime - startTime) / 1000;
        int remainingTime = 180 - elapsedTime;
        if (remainingTime < 0) remainingTime = 0;
        int minutes = remainingTime / 60;
        int seconds = remainingTime % 60;
        string timeStr = "Time=" + to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + to_string(seconds);
        DrawString(50, 700, "Score=" + to_string(player->getScore()), colors[RED]);
        DrawString(510, 700, "Fuel=" + to_string(static_cast<int>(player->getFuel())), colors[BLUE]);
        DrawString(290, 700, "Money=" + to_string(static_cast<int>(player->getMoney())), colors[GREEN]);
        DrawString(170, 700, timeStr, colors[YELLOW]);
        drawCar();
    }
    glutSwapBuffers();
}

void NonPrintableKeys(int key, int x, int y) {
    if (gameOver) return;
    int new_x = player->x, new_y = player->y;
    if (key == GLUT_KEY_LEFT) { new_x -= 10; player->setFuel(player->getFuel() - 0.25); }
    else if (key == GLUT_KEY_RIGHT) { new_x += 10; player->setFuel(player->getFuel() - 0.25); }
    else if (key == GLUT_KEY_UP) { new_y += 10; player->setFuel(player->getFuel() - 0.25); }
    else if (key == GLUT_KEY_DOWN) { new_y -= 10; player->setFuel(player->getFuel() - 0.25); }
    if (new_x >= 0 && new_x <= 660 && new_y >= 0 && new_y <= 640) {
        int cell_i = new_x / 40;
        int cell_j = new_y / 40;
        if (cell_i >= 0 && cell_i < 17 && cell_j >= 0 && cell_j < 17) {
            if (isRoadCell(cell_i, cell_j)) {
                player->x = new_x;
                player->y = new_y;
                bool hasCollision = false;
                OtherCar* others[] = {&otherCar, &otherCar2, &otherCar3, &otherCar4};
                if (collides(*player, otherCar)) {
                    otherCar.resetPosition(player, others, 4);
                    hasCollision = true;
                    player->addScore(-5);
                }
                if (collides(*player, otherCar2)) {
                    otherCar2.resetPosition(player, others, 4);
                    hasCollision = true;
                    player->addScore(-5);
                }
                if (collides(*player, otherCar3)) {
                    otherCar3.resetPosition(player, others, 4);
                    hasCollision = true;
                    player->addScore(-5);
                }
                if (collides(*player, otherCar4)) {
                    otherCar4.resetPosition(player, others, 4);
                    hasCollision = true;
                    player->addScore(-5);
                }
                if (hasCollision) {
                    Mix_PlayChannel(-1, gCollisionSound, 0);
                }
            } else {
                Mix_PlayChannel(-1, gCollisionSound, 0);
                player->addScore(-4);
            }
        } else {
            Mix_PlayChannel(-1, gCollisionSound, 0);
            player->addScore(-4);
        }
    }
    glutPostRedisplay();
}

void PrintableKeys(unsigned char key, int x, int y) {
    if (gameOver) { exit(0); }
    if (key == 27) { exit(1); }
    if (key == 'b' || key == 'B') { cout << "b pressed" << endl; }
    if (key == ' ') {
        for (int i = 0; i < 3; i++) {
            FuelStation* fs = gameState.getFuelStation(i);
            if (fs && abs(player->x - fs->getX()) <= 40 && abs(player->y - fs->getY()) <= 40) {
                if (player->refuel()) {
                    Mix_PlayChannel(-1, gRefuellingSound, 0);
                }
                break;
            }
        }
    }
    if (key == 13) {
        if (dynamic_cast<Taxi*>(player) && !dynamic_cast<Taxi*>(player)->hasPassengerStatus()) {
            player->pickUp();
        } else if (dynamic_cast<DeliveryCar*>(player) && !dynamic_cast<DeliveryCar*>(player)->hasPackageStatus()) {
            player->pickUp();
        } else {
            if (player->dropOff()) {
                Mix_PlayChannel(-1, gDestinationSound, 0);
            }
        }
    }
    glutPostRedisplay();
}

void Timer(int m) {
    if (!gameOver) {
        moveCar();
        bool hasCollision = false;
        OtherCar* others[] = {&otherCar, &otherCar2, &otherCar3, &otherCar4};
        if (collides(*player, otherCar)) {
            otherCar.resetPosition(player, others, 4);
            hasCollision = true;
            player->addScore(-5);
        }
        if (collides(*player, otherCar2)) {
            otherCar2.resetPosition(player, others, 4);
            hasCollision = true;
            player->addScore(-5);
        }
        if (collides(*player, otherCar3)) {
            otherCar3.resetPosition(player, others, 4);
            hasCollision = true;
            player->addScore(-5);
        }
        if (collides(*player, otherCar4)) {
            otherCar4.resetPosition(player, others, 4);
            hasCollision = true;
            player->addScore(-5);
        }
        if (hasCollision) {
            Mix_PlayChannel(-1, gCollisionSound, 0);
        }
        int currentTime = glutGet(GLUT_ELAPSED_TIME);
        if (currentTime - startTime >= 3 * 60 * 1000 || player->getFuel() <= 0 || player->getScore() < 0 || player->getScore() >= 100) {
            gameOver = true;
            if (player->getScore() >= 100) {
                isWin = true;
            }
            if (player->getScore() > 0 || numHighScores == 0) {
                cout << "Attempting to save score: " << player->getScore() << " for " << playerName << endl;
                if (numHighScores < 10) {
                    strncpy(highScores[numHighScores].name, playerName.c_str(), 19);
                    highScores[numHighScores].name[19] = '\0';
                    highScores[numHighScores].score = player->getScore();
                    numHighScores++;
                    cout << "Added new high score at index " << numHighScores - 1 << endl;
                } else {
                    int minIndex = 0;
                    for (int i = 1; i < numHighScores; i++) {
                        if (highScores[i].score < highScores[minIndex].score) {
                            minIndex = i;
                        }
                    }
                    if (player->getScore() > highScores[minIndex].score) {
                        strncpy(highScores[minIndex].name, playerName.c_str(), 19);
                        highScores[minIndex].name[19] = '\0';
                        highScores[minIndex].score = player->getScore();
                        cout << "Replaced high score at index " << minIndex << endl;
                    }
                }
                sortHighScores(highScores, numHighScores);
                saveHighScores();
            }
        } else {
            glutTimerFunc(100, Timer, 0);
        }
    }
}

void MousePressedAndMoved(int x, int y) {
    cout << x << " " << y << endl;
    glutPostRedisplay();
}

void MouseMoved(int x, int y) { glutPostRedisplay(); }

void MouseClicked(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) cout << "Left Button Down" << endl;
        else if (state == GLUT_UP) cout << "Left Button Up" << endl;
    }
    else if (button == GLUT_RIGHT_BUTTON) {
        cout << "Right Button Pressed" << endl;
    }
    glutPostRedisplay();
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << endl;
        return 1;
    }
    if (!(Mix_Init(MIX_INIT_MP3) & MIX_INIT_MP3)) {
        cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << endl;
        SDL_Quit();
        return 1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        cerr << "SDL_mixer could not open audio device! SDL_mixer Error: " << Mix_GetError() << endl;
        Mix_Quit();
        SDL_Quit();
        return 1;
    }
    gMenuMusic = Mix_LoadMUS("menu.mp3");
    if (gMenuMusic == NULL) {
        cerr << "Failed to load menu music! SDL_mixer Error: " << Mix_GetError() << endl;
    }
    gGameMusic = Mix_LoadMUS("gametime.mp3");
    if (gGameMusic == NULL) {
        cerr << "Failed to load game music! SDL_mixer Error: " << Mix_GetError() << endl;
    }
    gCollisionSound = Mix_LoadWAV("collision.mp3");
    if (gCollisionSound == NULL) {
        cerr << "Failed to load collision sound! SDL_mixer Error: " << Mix_GetError() << endl;
    }
    gDestinationSound = Mix_LoadWAV("destination.mp3");
    if (gDestinationSound == NULL) {
        cerr << "Failed to load destination sound! SDL_mixer Error: " << Mix_GetError() << endl;
    }
    gRefuellingSound = Mix_LoadWAV("refueling.mp3");
    if (gRefuellingSound == NULL) {
        cerr << "Failed to load refuelling sound! SDL_mixer Error: " << Mix_GetError() << endl;
    }
    Mix_PlayMusic(gMenuMusic, -1);
    InitRandomizer();
    loadHighScores();
    bool startGame = false;
    while (!startGame) {
        cout << "1. View Leaderboard\n2. Start Game\n";
        int choice;
        cin >> choice;
        if (choice == 1) {
            displayLeaderboard();
        } else if (choice == 2) {
            startGame = true;
        } else {
            cout << "Invalid choice.\n";
        }
    }
    cout << "Choose role: 1. Taxi Driver, 2. Delivery Driver, 3. Random\n";
    int roleChoice;
    cin >> roleChoice;
    string role;
    if (roleChoice == 1) {
        role = "taxi";
    } else if (roleChoice == 2) {
        role = "delivery";
    } else {
        role = (rand() % 2 == 0 ? "taxi" : "delivery");
    }
    cout << "Enter your name: ";
    cin >> ws;
    getline(cin, playerName);
    if (playerName.empty()) playerName = "Anonymous";
    if (playerName.length() > 19) playerName = playerName.substr(0, 19);
    float* carColor = (role == "taxi" ? colors[YELLOW] : colors[RED]);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(50, 50);
    int width = 680, height = 720;
    glutInitWindowSize(width, height);
    glutCreateWindow("OOP Project");
    SetCanvasSize(width, height);
    if (role == "taxi") {
        player = new Taxi(0, 640, carColor, 100.0, 0.0, gameState);
    } else {
        player = new DeliveryCar(0, 640, carColor, 100.0, 0.0, gameState);
    }
    Position pos;
    Position occupied[20];
    int occupiedCount = 0;
    do {
        pos = getRandomRoadPosition();
    } while (pos.x == player->x && pos.y == player->y);
    otherCar = OtherCar(pos.x, pos.y);
    do {
        pos = getRandomRoadPosition();
    } while ((pos.x == player->x && pos.y == player->y) || 
             (pos.x == otherCar.x && pos.y == otherCar.y));
    otherCar2 = OtherCar(pos.x, pos.y);
    do {
        pos = getRandomRoadPosition();
    } while ((pos.x == player->x && pos.y == player->y) || 
             (pos.x == otherCar.x && pos.y == otherCar.y) || 
             (pos.x == otherCar2.x && pos.y == otherCar2.y));
    otherCar3 = OtherCar(pos.x, pos.y);
    do {
        pos = getRandomRoadPosition();
    } while ((pos.x == player->x && pos.y == player->y) || 
             (pos.x == otherCar.x && pos.y == otherCar.y) || 
             (pos.x == otherCar2.x && pos.y == otherCar2.y) || 
             (pos.x == otherCar3.x && pos.y == otherCar3.y));
    otherCar4 = OtherCar(pos.x, pos.y);
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
    int activePickupItems = 2 + rand() % 3;
    gameState.setActivePickupItems(activePickupItems);
    for (int i = 0; i < activePickupItems; i++) {
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
                if (role == "taxi") {
                    gameState.setPickupItem(i, new Passenger(pos.x, pos.y));
                } else {
                    gameState.setPickupItem(i, new Box(pos.x, pos.y));
                }
                occupied[occupiedCount++] = pos;
                break;
            }
        } while (true);
    }
    startTime = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(100, Timer, 0);
    Mix_HaltMusic();
    Mix_PlayMusic(gGameMusic, -1);
    glutDisplayFunc(GameDisplay);
    glutSpecialFunc(NonPrintableKeys);
    glutKeyboardFunc(PrintableKeys);
    glutMouseFunc(MouseClicked);
    glutPassiveMotionFunc(MouseMoved);
    glutMotionFunc(MousePressedAndMoved);
    glutMainLoop();
    Mix_HaltMusic();
    Mix_FreeMusic(gMenuMusic);
    Mix_FreeMusic(gGameMusic);
    Mix_FreeChunk(gCollisionSound);
    Mix_FreeChunk(gDestinationSound);
    Mix_FreeChunk(gRefuellingSound);
    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();
    delete player;
    return 0;
}