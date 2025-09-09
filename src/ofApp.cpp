#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetWindowTitle("Emoji MugShot");
	ofSetBackgroundColor(20, 20, 30);
	ofSetFrameRate(60);

	titleFont.load(OF_TTF_SANS, 32);
	bodyFont.load(OF_TTF_SANS, 18);
	bgMusic.load("bg_music.mp3");
	hitSound.load("hit.mp3");
	bgMusic.setLoop(true);
	bgMusic.setVolume(0.4);

	cam.setup(1280, 720);
	colorImg.allocate(cam.getWidth(), cam.getHeight());
	grayImg.allocate(cam.getWidth(), cam.getHeight());

	// Use absolute path for the Haar Cascade model
	haarFinder.setup(ofToDataPath("haarcascade_frontalface_default.xml", true));

	facemark = cv::face::FacemarkLBF::create();
	facemark->loadModel(ofToDataPath("lbfmodel.yaml", true));
}

//--------------------------------------------------------------
void ofApp::update() {
	cam.update();
	if (!cam.isFrameNew()) return;

	ofSoundUpdate();

	colorImg.setFromPixels(cam.getPixels());
	colorImg.mirror(true, false);

	grayImg = colorImg;

	// --- FIX: REMOVED equalizeHist() as it's not supported ---
	// grayImg.equalizeHist();

	// --- FIX: Reverted to the simple, supported version of findHaarObjects ---
	haarFinder.findHaarObjects(grayImg);

	if (haarFinder.blobs.size() > 0) {
		vector<cv::Rect> faces;
		for (int i = 0; i < haarFinder.blobs.size(); i++) {
			ofRectangle r = haarFinder.blobs[i].boundingRect;
			faces.push_back(cv::Rect(r.x, r.y, r.width, r.height));
		}

		cv::Mat colorMat(colorImg.getHeight(), colorImg.getWidth(), CV_8UC3, colorImg.getPixels().getData());
		vector<vector<cv::Point2f>> landmarks;
		facemark->fit(colorMat, faces, landmarks);

		if (currentState == GAMEPLAY && !players.empty() && landmarks.size() > 0) {
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

			if (bestMatchIndex != -1 && bestMatchIndex < landmarks.size()) {
				ofRectangle newRect = haarFinder.blobs[bestMatchIndex].boundingRect;
				player.smoothedPos = player.smoothedPos.getInterpolated(newRect.getCenter(), 0.1);

				player.landmarks.clear();
				for (const auto & p : landmarks[bestMatchIndex]) {
					player.landmarks.push_back(ofVec2f(p.x, p.y));
				}
			}
		}
	}

	if (currentState == GAMEPLAY) {
		targetSpawnTimer -= ofGetLastFrameTime();
		if (targetSpawnTimer <= 0) {
			Target t;
			t.pos = ofVec2f(ofRandom(100, ofGetWidth() - 100), ofRandom(100, ofGetHeight() - 100));
			targets.push_back(t);
			targetSpawnTimer = 0.8f;
		}

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

		targets.erase(remove_if(targets.begin(), targets.end(), [](const Target & t) { return t.isHit; }), targets.end());
	}
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofSetColor(255);

	float camX = (ofGetWidth() - cam.getWidth()) / 2.0f;
	float camY = (ofGetHeight() - cam.getHeight()) / 2.0f;
	cam.draw(camX, camY);

	string msg;

	ofRectangle guideBox(ofGetWidth() / 2 - 125, ofGetHeight() / 2 - 150, 250, 300);

	switch (currentState) {
	case ENROLL_P1_AIM:
	case ENROLL_P2_AIM: {
		ofNoFill();
		ofSetLineWidth(4);

		bool faceAligned = false;
		if (haarFinder.blobs.size() > 0) {
			if (guideBox.inside(haarFinder.blobs[0].boundingRect.getCenter())) {
				faceAligned = true;
			}
		}

		if (faceAligned) {
			ofSetColor(0, 255, 0);
			msg = "Perfect! Press [SPACE] to capture!";
		} else {
			ofSetColor(255, 0, 0);
			msg = "Position your face inside the box.";
		}
		ofDrawRectangle(guideBox);

		ofSetColor(255, 255, 0);
		float msgWidth = bodyFont.stringWidth(msg);
		bodyFont.drawString(msg, ofGetWidth() / 2 - msgWidth / 2, guideBox.getBottom() + 30);
		break;
	}

	case GAMEPLAY: {
		ofSetColor(255, 255, 0);
		titleFont.drawString("P1: " + ofToString(players[0].score), 30, 50);
		titleFont.drawString("P2: " + ofToString(players[1].score), ofGetWidth() - 200, 50);

		for (auto const & target : targets) {
			ofSetColor(255, 0, 150);
			ofDrawCircle(target.pos, target.radius);
		}

		if (!players.empty()) {
			Player & player = players[activePlayer];

			ofSetColor(0, 255, 255);
			for (const auto & p : player.landmarks) {
				ofDrawCircle(p, 2);
			}

			ofNoFill();
			ofSetLineWidth(4);
			ofRectangle faceRect(player.smoothedPos - ofVec2f(75, 75), 150, 150);
			ofSetColor(0, 255, 0);
			ofDrawRectangle(faceRect);

			ofSetColor(255);
			player.emoji.draw((ofVec2f)faceRect.getTopRight() + ofVec2f(10, 0), 80, 80);
		}

		if (haarFinder.blobs.size() > 1) {
			ofSetColor(255, 100, 0);
			string turnMsg = "Too many faces! Player " + ofToString(activePlayer + 1) + "'s turn.";
			bodyFont.drawString(turnMsg, ofGetWidth() / 2 - bodyFont.stringWidth(turnMsg) / 2, 30);
		}
		break;
	}
	case GAME_OVER:
		break;
	}

	if (!players.empty()) {
		players[0].mugshot.draw(20, ofGetHeight() - 120, 100, 100);
	}
	if (players.size() > 1) {
		players[1].mugshot.draw(ofGetWidth() - 120, ofGetHeight() - 120, 100, 100);
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	if (key == ' ') {
		if (currentState == ENROLL_P1_AIM || currentState == ENROLL_P2_AIM) {

			ofRectangle guideBox(ofGetWidth() / 2 - 125, ofGetHeight() / 2 - 150, 250, 300);
			if (haarFinder.blobs.size() > 0 && guideBox.inside(haarFinder.blobs[0].boundingRect.getCenter())) {

				Player p;
				p.id = (currentState == ENROLL_P1_AIM) ? 1 : 2;

				ofRectangle faceRect = haarFinder.blobs[0].boundingRect;

				float camX = (ofGetWidth() - cam.getWidth()) / 2.0f;
				float camY = (ofGetHeight() - cam.getHeight()) / 2.0f;
				p.mugshot.grabScreen(faceRect.x + camX, faceRect.y + camY, faceRect.width, faceRect.height);

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
