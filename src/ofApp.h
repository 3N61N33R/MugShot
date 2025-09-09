#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxOpenCv.h"

// Include the OpenCV face module
#include "opencv2/face.hpp"

// A simple struct for our game targets
struct Target {
	ofVec2f pos;
	float radius = 25;
	bool isHit = false;
};

// A struct to hold data for each enrolled player
struct Player {
	int id;
	ofImage mugshot;
	ofImage emoji;
	ofVec2f smoothedPos;
	int score = 0;
	vector<ofVec2f> landmarks; // To store the player's facial landmarks
};

// Enum to manage the game's flow
enum GameState {
	ENROLL_P1_AIM,
	ENROLL_P2_AIM,
	GAMEPLAY,
	GAME_OVER
};

class ofApp : public ofBaseApp {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void keyPressed(int key) override;

	// Core objects
	ofVideoGrabber cam;
	ofxCvColorImage colorImg;
	ofxCvGrayscaleImage grayImg;
	ofxCvHaarFinder haarFinder;

	// --- NEW: Facemark object ---
	cv::Ptr<cv::face::Facemark> facemark;

	// Game objects
	vector<Player> players;
	vector<Target> targets;
	float targetSpawnTimer = 0;
	int activePlayer = 0;

	// UI and State Management
	GameState currentState = ENROLL_P1_AIM;
	ofTrueTypeFont titleFont;
	ofTrueTypeFont bodyFont;
	ofSoundPlayer bgMusic;
	ofSoundPlayer hitSound;
};
