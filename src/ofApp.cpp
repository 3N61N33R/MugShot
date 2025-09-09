#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetWindowTitle("Emoji MugShot");
	ofSetBackgroundColor(20, 20, 30);
	ofSetFrameRate(60);

	// --- Load Assets ---
	titleFont.load(OF_TTF_SANS, 32);
	bodyFont.load(OF_TTF_SANS, 18);

	// Make sure these files are in your bin/data folder
	bgMusic.load("bg_music.mp3");
	hitSound.load("hit.mp3");

	bgMusic.setLoop(true);
	bgMusic.setVolume(0.4);

	// --- Setup Camera and OpenCV ---
	cam.setup(1280, 720);
	colorImg.allocate(cam.getWidth(), cam.getHeight());
	grayImg.allocate(cam.getWidth(), cam.getHeight());
	haarFinder.setup("haarcascade_frontalface_default.xml");
}

//--------------------------------------------------------------
void ofApp::update() {
	cam.update();
	if (!cam.isFrameNew()) return;

	ofSoundUpdate(); // Keep the sound engine running

	colorImg.setFromPixels(cam.getPixels());
	colorImg.mirror(false, true); // Flip the camera image horizontally
	grayImg = colorImg;
	haarFinder.findHaarObjects(grayImg);

	if (currentState == GAMEPLAY) {
		// --- Match detected faces to the active player ---
		if (!players.empty() && haarFinder.blobs.size() > 0) {
			Player & player = players[activePlayer];

			float closestDist = -1;
			int bestMatchIndex = -1;

			for (int i = 0; i < haarFinder.blobs.size(); i++) {
				float dist = player.smoothedPos.distance(haarFinder.blobs[i].boundingRect.getCenter());
				if (bestMatchIndex == -1 || dist < closestDist) {
					closestDist = dist;
					bestMatchIndex = i;
				}
			}

			if (bestMatchIndex != -1) {
				ofRectangle newRect = haarFinder.blobs[bestMatchIndex].boundingRect;
				player.smoothedPos = player.smoothedPos.getInterpolated(newRect.getCenter(), 0.1);
			}
		}

		// --- Target Logic (Spawning and Collision) ---
		targetSpawnTimer -= ofGetLastFrameTime();
		if (targetSpawnTimer <= 0) {
			Target t;
			t.pos = ofVec2f(ofRandom(100, ofGetWidth() - 100), ofRandom(100, ofGetHeight() - 100));
			targets.push_back(t);
			targetSpawnTimer = 0.8f;
		}

		// --- Collision Detection ---
		for (auto & target : targets) {
			if (!target.isHit) {
				Player & player = players[activePlayer];
				if (player.smoothedPos.distance(target.pos) < target.radius + 40) {
					target.isHit = true;
					player.score += 10;
					hitSound.play();
				}
			}
		}

		targets.erase(remove_if(targets.begin(), targets.end(), [](const Target & t) {
			return t.isHit;
		}),
			targets.end());
	}
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofSetColor(255);
	cam.draw(0, 0);

	string msg;
	ofRectangle boundingBox(ofGetWidth() / 2 - 350, ofGetHeight() / 2 - 100, 700, 200);

	switch (currentState) {
	case ENROLL_P1_AIM:
		msg = "Player 1: Look at the camera.\nPress [SPACE] to take your mugshot!";
		break;
	case ENROLL_P2_AIM:
		msg = "Player 2: Look at the camera.\nPress [SPACE] to take your mugshot!";
		break;
	case GAMEPLAY:
		ofSetColor(255, 255, 0);
		titleFont.drawString("P1: " + ofToString(players[0].score), 30, 50);
		titleFont.drawString("P2: " + ofToString(players[1].score), ofGetWidth() - 200, 50);

		for (auto const & target : targets) {
			ofSetColor(255, 0, 150);
			ofDrawCircle(target.pos, target.radius);
		}

		if (haarFinder.blobs.size() > 0) {
			ofNoFill();
			ofSetLineWidth(4);

			if (haarFinder.blobs.size() > 1) {
				ofSetColor(255, 100, 0);
				string turnMsg = "Too many faces! Player " + ofToString(activePlayer + 1) + "'s turn.";
				bodyFont.drawString(turnMsg, ofGetWidth() / 2 - bodyFont.stringWidth(turnMsg) / 2, 30);
			}

			Player & player = players[activePlayer];
			ofRectangle faceRect(player.smoothedPos - ofVec2f(75, 75), 150, 150);
			ofSetColor(0, 255, 0);
			ofDrawRectangle(faceRect);

			ofSetColor(255);
			// --- FIXED LINE ---
			player.emoji.draw((ofVec2f)faceRect.getTopRight() + ofVec2f(10, 0), 80, 80);
		}
		break;
	case GAME_OVER:
		string winnerMsg;
		if (players[0].score > players[1].score)
			winnerMsg = "Player 1 Wins!";
		else if (players[1].score > players[0].score)
			winnerMsg = "Player 2 Wins!";
		else
			winnerMsg = "It's a Tie!";

		msg = winnerMsg + "\nPress [R] to restart.";
		break;
	}

	if (currentState != GAMEPLAY) {
		ofSetColor(0, 0, 0, 150);
		ofDrawRectangle(boundingBox);
		ofSetColor(255, 255, 0);
		bodyFont.drawString(msg, boundingBox.x + 50, boundingBox.y + 80);
	}

	if (!players.empty()) {
		ofSetColor(255);
		players[0].mugshot.draw(20, ofGetHeight() - 120, 100, 100);
	}
	if (players.size() > 1) {
		ofSetColor(255);
		players[1].mugshot.draw(ofGetWidth() - 120, ofGetHeight() - 120, 100, 100);
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	if (key == ' ') {
		if (currentState == ENROLL_P1_AIM || currentState == ENROLL_P2_AIM) {
			if (haarFinder.blobs.size() > 0) {
				Player p;
				p.id = (currentState == ENROLL_P1_AIM) ? 1 : 2;

				ofRectangle faceRect = haarFinder.blobs[0].boundingRect;
				p.mugshot.grabScreen(faceRect.x, faceRect.y, faceRect.width, faceRect.height);

				p.smoothedPos = faceRect.getCenter();

				p.emoji.load("p" + ofToString(p.id) + "_emoji.png");

				players.push_back(p);

				if (currentState == ENROLL_P1_AIM) {
					currentState = ENROLL_P2_AIM;
				} else {
					currentState = GAMEPLAY;
					activePlayer = 0;
					bgMusic.play();
				}
			}
		}
	} else if (key == 's') {
		if (currentState == GAMEPLAY) {
			activePlayer = (activePlayer + 1) % players.size();
		}
	} else if (key == 'r' || key == 'R') {
		players.clear();
		targets.clear();
		currentState = ENROLL_P1_AIM;
		bgMusic.stop();
	}
}
