// Multitouch.h
#pragma once

#include <SDL2/SDL.h>
#include "Disk.h"
#include "RenderWindow.h"
#include "EigenTypes.h"

class Multitouch
{
public:
	void setup();
	void process_event(SDL_Event event);

	void tick(int screen_width, int screen_height, int width, int height);
	void draw(RenderWindow* window, int screen_width, int screen_height, int width, int height);

	void set_decays(ArrayT* decays, int n);
	void set_ringing(ArrayT* ringing, int n);

private:
	const double radius = 30;
	const double dot = radius * 0.25;
	const double bigd = radius * 0.5;

	static const int fingers = 12;
	const double density = 1.25;
	static const int disks = 12; // density * width * height / (PI * radius * radius);
	const double inflation = radius * 2;

	const double rigidity = 0.1;
	const double wallrigidity = 0.05;
	const double bindanim = 0.95;
	const double floatanim = 0.8;
	const double grabanim = 0.9;

	const double restingval = 1.0 / 6;
	const double excitedval = 1.0 / 3;

	const double restingsat = 0.0 / 3; // 2.0 / 3;
	const double excitedsat = 2.0 / 3; // 0.0 / 3;

	const double alpha = 1.0 / 3;

	const double friction = 0.95;
	const int oversample = 1;

	Disk grabbers[fingers];
	bool active[fingers];

	SDL_FingerID ids[fingers];

	Disk floaters[disks];
	int grabbed[disks]; // floater #i is grabbed by finger grabbed[i]
	int grabbing[fingers]; // finger #i is grabbing floater grabbing[i]
	double bind[fingers];
	double targbind[fingers];

	double basehues[disks];
};